#pragma once

#include <ESPAsyncWebServer.h>
#include "ControllerState.h"

class WebServerController {
 public:
  explicit WebServerController(AppState& state);
  void begin();
  void applyPendingSettings();

 private:
  void handleRoot(AsyncWebServerRequest *request);
  void handleIsolation(AsyncWebServerRequest *request);
  void handleSetRegisterAddresses(AsyncWebServerRequest *request);
  void handlePreset(AsyncWebServerRequest *request);
  void handleScan(AsyncWebServerRequest *request);
  void handleSetParameter(AsyncWebServerRequest *request);
  void handleSaveNetwork(AsyncWebServerRequest *request);
  void handleNetworkStatus(AsyncWebServerRequest *request);
  void handleNetworkRestart(AsyncWebServerRequest *request);

  void sendAndClose(AsyncWebServerRequest *request, int code, const String& contentType, const String& body);

  AsyncWebServer server_;
  AppState& state_;
  bool pendingSettingsApply_ = false;
  bool pendingAddressSave_ = false;
  bool pendingParameterApply_[kParameterCount] = {};
};