#include "NetworkManager.h"

#include <ESPmDNS.h>
#include <Preferences.h>
#include <WiFi.h>

namespace {

constexpr char kPreferencesNamespace[] = "crt-network";
constexpr char kAccessPointSsid[] = "CRT-Control";
constexpr char kAccessPointPassword[] = "12345678";
constexpr char kMdnsHostname[] = "crt-control";
constexpr uint32_t kConnectionTimeoutMs = 10000;
constexpr uint32_t kRestartDelayMs = 800;
constexpr wifi_power_t kTransmitPower = WIFI_POWER_13dBm;

String savedSsid;
String savedPassword;
bool accessPointActive = false;
bool connectionAttempting = false;
bool mdnsActive = false;
bool reconnectRequested = false;
bool restartRequested = false;
uint32_t connectionStartedAt = 0;
uint32_t restartAt = 0;

bool startMdnsService() {
  if (mdnsActive) return true;
  mdnsActive = MDNS.begin(kMdnsHostname);
  if (mdnsActive) {
    MDNS.addService("http", "tcp", 80);
  }
  return mdnsActive;
}

void stopMdnsService() {
  if (!mdnsActive) return;
  MDNS.end();
  mdnsActive = false;
}

bool startAccessPoint(bool includeStation) {
  const bool wasAlreadyActive = accessPointActive;
  WiFi.mode(includeStation ? WIFI_AP_STA : WIFI_AP);
  WiFi.setTxPower(kTransmitPower);
  WiFi.setSleep(false);
  const bool started = WiFi.softAP(kAccessPointSsid, kAccessPointPassword);
  return started || wasAlreadyActive;
}

bool waitForStationConnection(uint32_t timeoutMs) {
  const uint32_t startedAt = millis();
  while (WiFi.status() != WL_CONNECTED &&
         static_cast<uint32_t>(millis() - startedAt) < timeoutMs) {
    delay(250);
  }
  return WiFi.status() == WL_CONNECTED;
}

void loadCredentials() {
  Preferences preferences;
  preferences.begin(kPreferencesNamespace, true);
  savedSsid = preferences.getString("ssid", "");
  savedPassword = preferences.getString("password", "");
  preferences.end();
}

}  // namespace

void NetworkControl::begin() {
  loadCredentials();
  stopMdnsService();
  WiFi.setTxPower(kTransmitPower);
  WiFi.setSleep(false);

  if (savedSsid.length() == 0) {
    accessPointActive = startAccessPoint(false);
    startMdnsService();
    return;
  }

  WiFi.mode(WIFI_STA);
  WiFi.begin(savedSsid.c_str(), savedPassword.c_str());
  if (waitForStationConnection(kConnectionTimeoutMs)) {
    accessPointActive = false;
    startMdnsService();
    return;
  }

  WiFi.disconnect(false, false);
  accessPointActive = startAccessPoint(false);
  startMdnsService();
}

String NetworkControl::configuredSsid() {
  return savedSsid;
}

bool NetworkControl::saveCredentials(
    String ssid, String password, bool openNetwork, String& errorMessage) {
  ssid.trim();
  if (ssid.length() == 0) {
    errorMessage = "Enter a Wi-Fi network name.";
    return false;
  }
  if (ssid.length() > 32) {
    errorMessage = "The Wi-Fi network name is too long.";
    return false;
  }
  if (password.length() > 63) {
    errorMessage = "The Wi-Fi password is too long.";
    return false;
  }

  if (openNetwork) {
    password = "";
  } else if (password.length() == 0) {
    if (ssid == savedSsid) {
      password = savedPassword;
    } else {
      errorMessage = "Enter the new network password or mark the network as open.";
      return false;
    }
  }

  savedSsid = ssid;
  savedPassword = password;

  Preferences preferences;
  preferences.begin(kPreferencesNamespace, false);
  preferences.putString("ssid", savedSsid);
  preferences.putString("password", savedPassword);
  preferences.end();
  return true;
}

void NetworkControl::requestReconnect() {
  reconnectRequested = true;
}

void NetworkControl::requestRestart() {
  restartRequested = true;
  restartAt = millis() + kRestartDelayMs;
}

void NetworkControl::processPendingActions() {
  if (restartRequested && static_cast<int32_t>(millis() - restartAt) >= 0) {
    ESP.restart();
    return;
  }

  if (reconnectRequested) {
    reconnectRequested = false;
    stopMdnsService();
    accessPointActive = startAccessPoint(true);
    WiFi.disconnect(false, false);
    WiFi.begin(savedSsid.c_str(), savedPassword.c_str());
    connectionStartedAt = millis();
    connectionAttempting = true;
  }

  if (!connectionAttempting) return;

  if (WiFi.status() == WL_CONNECTED) {
    connectionAttempting = false;
    startMdnsService();
  } else if (static_cast<uint32_t>(millis() - connectionStartedAt) >= kConnectionTimeoutMs) {
    connectionAttempting = false;
    WiFi.disconnect(false, false);
  }
}

NetworkStatus NetworkControl::status() {
  NetworkStatus current;
  current.savedSsid = savedSsid;
  current.stationConnected = WiFi.status() == WL_CONNECTED;
  current.accessPointActive = accessPointActive;
  current.connectionAttempting = connectionAttempting;
  current.mdnsActive = mdnsActive;

  if (current.stationConnected) {
    current.ipAddress = WiFi.localIP().toString();
  } else if (current.accessPointActive) {
    current.ipAddress = WiFi.softAPIP().toString();
  } else {
    current.ipAddress = "";
  }
  return current;
}