#include "WebServerController.h"
#include <stdlib.h>
#include "I2cController.h"
#include "NetworkManager.h"
#include "SettingsStore.h"
#include "WebUi.h"

namespace {
bool parseParameterValue(const String& rawValue, int& parsedValue) {
  String value = rawValue;
  value.trim();
  if (value.length() == 0) return false;
  char* end = nullptr;
  const char* begin = value.c_str();
  const long parsed = strtol(begin, &end, 10);
  if (end == begin || *end != '\0' || parsed < 0 || parsed > 255) {
    return false;
  }
  parsedValue = static_cast<int>(parsed);
  return true;
}
}  // namespace

WebServerController::WebServerController(AppState& state)
    : server_(80), state_(state) {}

// Helper to force connection close and disable caching
void WebServerController::sendAndClose(AsyncWebServerRequest *request, int code, const String& contentType, const String& body) {
  AsyncWebServerResponse *response = request->beginResponse(code, contentType, body);
  response->addHeader("Connection", "close");
  response->addHeader("Cache-Control", "no-store, no-cache, must-revalidate");
  request->send(response);
}

void WebServerController::begin() {
  server_.on("/", HTTP_GET, [this](AsyncWebServerRequest *request) { handleRoot(request); });
  server_.on("/set", HTTP_GET, [this](AsyncWebServerRequest *request) { handleSetParameter(request); });
  server_.on("/setRegs", HTTP_GET, [this](AsyncWebServerRequest *request) { handleSetRegisterAddresses(request); });
  server_.on("/scan", HTTP_GET, [this](AsyncWebServerRequest *request) { handleScan(request); });
  server_.on("/preset", HTTP_GET, [this](AsyncWebServerRequest *request) { handlePreset(request); });
  server_.on("/iso", HTTP_GET, [this](AsyncWebServerRequest *request) { handleIsolation(request); });
  server_.on("/wifi/save", HTTP_POST, [this](AsyncWebServerRequest *request) { handleSaveNetwork(request); });
  server_.on("/wifi/status", HTTP_GET, [this](AsyncWebServerRequest *request) { handleNetworkStatus(request); });
  server_.on("/wifi/restart", HTTP_POST, [this](AsyncWebServerRequest *request) { handleNetworkRestart(request); });

  server_.on("/favicon.ico", HTTP_GET, [](AsyncWebServerRequest *request) {
    request->send(204);
  });

  server_.onNotFound([](AsyncWebServerRequest *request) {
    request->send(404, "text/plain", "Not found");
  });

  server_.begin();
}

void WebServerController::applyPendingSettings() {
  if (pendingAddressSave_) {
    pendingAddressSave_ = false;
    SettingsStore::saveGlobalI2cSettings(state_.i2cSettings);
  }
  if (pendingSettingsApply_) {
    pendingSettingsApply_ = false;
    for (uint8_t i = 0; i < kParameterCount; ++i) {
      pendingParameterApply_[i] = false;
    }
    I2cController::applySettings(state_.settings, state_.i2cSettings, state_.isIsolated);
    return;
  }
  if (state_.isIsolated) {
    for (uint8_t i = 0; i < kParameterCount; ++i) {
      if (pendingParameterApply_[i]) {
        pendingParameterApply_[i] = false;
        I2cController::writeParameter(state_.i2cSettings, state_.isIsolated, i, state_.settings.values[i]);
      }
    }
  }
}

void WebServerController::handleRoot(AsyncWebServerRequest *request) {
  String presetNames[kPresetCount];
  SettingsStore::getAllPresetNames(presetNames);
  String html = WebUi::render(state_, presetNames, NetworkControl::status());
  sendAndClose(request, 200, "text/html", html);
}

void WebServerController::handleIsolation(AsyncWebServerRequest *request) {
  if (!request->hasArg("state")) {
    sendAndClose(request, 400, "text/plain", "Missing isolation state");
    return;
  }
  state_.isIsolated = request->arg("state").toInt() == 1;
  I2cController::setIsolation(state_.isIsolated);
  SettingsStore::saveIsolationState(state_.isIsolated);
  pendingSettingsApply_ = true;
  sendAndClose(request, 200, "text/plain", "OK");
}

void WebServerController::handleSetRegisterAddresses(AsyncWebServerRequest *request) {
  static const char* registerArguments[kParameterCount] = {
      "r_sc", "r_rg", "r_gg", "r_bg", "r_sb", "r_rb",
      "r_gb", "r_bb", "r_abl", "r_g2", "r_d1", "r_d2"};
  static const char* enabledArguments[kParameterCount] = {
      "e_sc", "e_rg", "e_gg", "e_bg", "e_sb", "e_rb",
      "e_gb", "e_bb", "e_abl", "e_g2", "e_d1", "e_d2"};
  int registerAddresses[kParameterCount];
  bool enabled[kParameterCount];

  if (!request->hasArg("addr")) {
    sendAndClose(request, 400, "text/plain", "Missing I2C chip address");
    return;
  }
  const int address = request->arg("addr").toInt();
  if (address < 1 || address > 127) {
    sendAndClose(request, 400, "text/plain", "I2C address must be between 01 and 7F");
    return;
  }

  for (uint8_t i = 0; i < kParameterCount; ++i) {
    if (!request->hasArg(registerArguments[i]) || !request->hasArg(enabledArguments[i])) {
      sendAndClose(request, 400, "text/plain", "Missing register setting");
      return;
    }
    enabled[i] = request->arg(enabledArguments[i]) == "1";
    const String rawRegister = request->arg(registerArguments[i]);
    if (rawRegister == "-1" && !enabled[i]) {
      registerAddresses[i] = ControllerConfig::kUnconfiguredRegister;
    } else if (!parseParameterValue(rawRegister, registerAddresses[i])) {
      sendAndClose(request, 400, "text/plain", "Register addresses must be between 0 and 255");
      return;
    }
    if (enabled[i] && registerAddresses[i] < 0) {
      sendAndClose(request, 400, "text/plain", "An enabled parameter must have a register address");
      return;
    }
  }

  state_.i2cSettings.address = address;
  for (uint8_t i = 0; i < kParameterCount; ++i) {
    state_.i2cSettings.registerAddresses[i] = registerAddresses[i];
    state_.i2cSettings.parameterEnabled[i] = enabled[i];
  }
  pendingAddressSave_ = true;
  pendingSettingsApply_ = true;
  sendAndClose(request, 200, "text/plain", "I2C addresses saved");
}

void WebServerController::handlePreset(AsyncWebServerRequest *request) {
  if (!request->hasArg("slot") || !request->hasArg("action")) {
    sendAndClose(request, 400, "text/plain", "Missing preset slot or action");
    return;
  }
  const int slot = request->arg("slot").toInt();
  const String action = request->arg("action");
  if (slot < 0 || slot >= kPresetCount) {
    sendAndClose(request, 400, "text/plain", "Invalid preset slot");
    return;
  }

  if (action == "save") {
    const String name = SettingsStore::normalizePresetName(request->arg("name"), slot);
    SettingsStore::savePreset(slot, name, state_.settings);
    state_.activePresetSlot = slot;
    sendAndClose(request, 200, "text/plain", "Preset saved");
    return;
  }
  if (action == "apply") {
    SettingsStore::setActivePresetSlot(slot);
    state_.activePresetSlot = slot;
    state_.settings = SettingsStore::loadPreset(slot);
    for (uint8_t i = 0; i < kParameterCount; ++i) {
      pendingParameterApply_[i] = false;
    }
    pendingSettingsApply_ = true;
    sendAndClose(request, 200, "text/plain", "Preset applied");
    return;
  }
  sendAndClose(request, 400, "text/plain", "Invalid preset action");
}

void WebServerController::handleScan(AsyncWebServerRequest *request) {
  state_.scanResultLog = I2cController::scanBus(state_.isIsolated);
  sendAndClose(request, 200, "text/plain", state_.scanResultLog);
}

void WebServerController::handleSetParameter(AsyncWebServerRequest *request) {
  static const char* argumentNames[kParameterCount] = {
      "subContrast", "rGain", "gGain", "bGain", "subBrightness",
      "rBias", "gBias", "bBias", "abl", "g2", "da1", "da2"};
  int parsedValues[kParameterCount] = {};
  bool hasValue[kParameterCount] = {};
  bool foundAny = false;

  if (!state_.isIsolated) {
    sendAndClose(request, 409, "text/plain", "Parameters cannot be sent while connected");
    return;
  }
  for (uint8_t i = 0; i < kParameterCount; ++i) {
    if (!request->hasArg(argumentNames[i])) continue;
    if (!state_.i2cSettings.parameterEnabled[i] || state_.i2cSettings.registerAddresses[i] < 0) {
      sendAndClose(request, 409, "text/plain", "This parameter is disabled");
      return;
    }
    if (!parseParameterValue(request->arg(argumentNames[i]), parsedValues[i])) {
      sendAndClose(request, 400, "text/plain", "Parameter values must be integers from 0 to 255");
      return;
    }
    hasValue[i] = true;
    foundAny = true;
  }
  if (!foundAny) {
    sendAndClose(request, 400, "text/plain", "Missing parameter");
    return;
  }
  for (uint8_t i = 0; i < kParameterCount; ++i) {
    if (!hasValue[i]) continue;
    state_.settings.values[i] = parsedValues[i];
    pendingParameterApply_[i] = true;
  }
  sendAndClose(request, 200, "text/plain", "OK");
}

void WebServerController::handleSaveNetwork(AsyncWebServerRequest *request) {
  if (!request->hasArg("ssid")) {
    sendAndClose(request, 400, "text/plain", "Missing Wi-Fi network name");
    return;
  }
  String errorMessage;
  const bool openNetwork = request->arg("open") == "1";
  if (!NetworkControl::saveCredentials(request->arg("ssid"), request->arg("password"), openNetwork, errorMessage)) {
    sendAndClose(request, 400, "text/plain", errorMessage);
    return;
  }
  NetworkControl::requestReconnect();
  sendAndClose(request, 202, "text/plain", "Network settings saved");
}

void WebServerController::handleNetworkStatus(AsyncWebServerRequest *request) {
  const NetworkStatus status = NetworkControl::status();
  String response = "{\"connected\":";
  response += status.stationConnected ? "true" : "false";
  response += ",\"accessPoint\":";
  response += status.accessPointActive ? "true" : "false";
  response += ",\"attempting\":";
  response += status.connectionAttempting ? "true" : "false";
  response += ",\"mdns\":";
  response += status.mdnsActive ? "true" : "false";
  response += ",\"ip\":\"";
  response += status.ipAddress;
  response += "\",\"heap\":";
  response += String(ESP.getFreeHeap());
  response += "}";
  sendAndClose(request, 200, "application/json", response);
}

void WebServerController::handleNetworkRestart(AsyncWebServerRequest *request) {
  NetworkControl::requestRestart();
  sendAndClose(request, 200, "text/plain", "Restarting");
}