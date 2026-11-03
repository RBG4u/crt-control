#include <WiFi.h>
#include <esp32-hal-bt.h>

#include "ControllerState.h"
#include "I2cController.h"
#include "NetworkManager.h"
#include "SettingsStore.h"
#include "WebServerController.h"

namespace {
AppState appState;
WebServerController webController(appState);
}  // namespace

void setup() {
  // Give the connected device time to boot up before isolation.
  delay(3000);

  // Reduce power consumption before starting the access point.
  btStop();
  setCpuFrequencyMhz(160);

  appState.isIsolated = SettingsStore::loadIsolationState();
  I2cController::initializeIsolationPin(appState.isIsolated);

  appState.settings = SettingsStore::defaultSettings();
  appState.activePresetSlot = SettingsStore::loadActivePresetSlot();

  if (appState.activePresetSlot >= 0 && appState.activePresetSlot < kPresetCount) {
    appState.settings = SettingsStore::loadPreset(appState.activePresetSlot);
  } else {
    appState.activePresetSlot = -1;
  }

  appState.i2cSettings = SettingsStore::loadGlobalI2cSettings();

  I2cController::beginBus();
  
  // On first boot, data is not sent while the device is connected.
  if (appState.isIsolated) {
    I2cController::applySettings(appState.settings, appState.i2cSettings, appState.isIsolated);
  }

  NetworkControl::begin();
  webController.begin();
}

void loop() {
  webController.applyPendingSettings();
  NetworkControl::processPendingActions();
  // Small delay to yield to the Wi-Fi stack and prevent watchdog resets
  delay(10);
}