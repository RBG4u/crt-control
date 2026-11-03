#pragma once

#include <Arduino.h>

#include "ControllerState.h"

namespace SettingsStore {

Settings defaultSettings();
I2cSettings defaultI2cSettings();
String defaultPresetName(int slot);
String normalizePresetName(String name, int slot);
String getPresetName(int slot);

I2cSettings loadGlobalI2cSettings();
void saveGlobalI2cSettings(const I2cSettings& settings);
bool loadIsolationState();
void saveIsolationState(bool isIsolated);
int loadActivePresetSlot();
Settings loadPreset(int slot);

void setActivePresetSlot(int slot);
void savePreset(int slot, const String& name, const Settings& settings);
void getAllPresetNames(String names[]);

}  // namespace SettingsStore
