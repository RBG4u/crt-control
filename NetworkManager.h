#pragma once

#include <Arduino.h>

struct NetworkStatus {
  String savedSsid;
  String ipAddress;
  bool stationConnected;
  bool accessPointActive;
  bool connectionAttempting;
  bool mdnsActive;
};

namespace NetworkControl {

void begin();
String configuredSsid();
bool saveCredentials(
    String ssid, String password, bool openNetwork, String& errorMessage);
void requestReconnect();
void requestRestart();
void processPendingActions();
NetworkStatus status();

}  // namespace NetworkControl
