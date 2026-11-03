#include "SettingsStore.h"

#include <Preferences.h>

namespace {

constexpr char kPreferencesNamespace[] = "crt-matrix";
constexpr const char* kPresetParameterKeys[kParameterCount] = {
    "subContrast", "rGain", "gGain", "bGain", "subBrightness", "rBias",
    "gBias", "bBias", "abl", "g2", "da1", "da2"};
constexpr const char* kGlobalRegisterKeys[kParameterCount] = {
    "gRsc", "gRrg", "gRgg", "gRbg", "gRsb", "gRrb",
    "gRgb", "gRbb", "gRabl", "gRg2", "gRd1", "gRd2"};
constexpr const char* kGlobalEnabledKeys[kParameterCount] = {
    "eSc", "eRg", "eGg", "eBg", "eSb", "eRb",
    "eGb", "eBb", "eAbl", "eG2", "eD1", "eD2"};

String presetKey(int slot, const char* field) {
  return "p" + String(slot) + field;
}

}  // namespace

namespace SettingsStore {

Settings defaultSettings() {
  Settings settings = {};
  for (uint8_t i = 0; i < kParameterCount; ++i) {
    settings.values[i] = ControllerConfig::kDefaultParameterValue;
  }
  return settings;
}

I2cSettings defaultI2cSettings() {
  I2cSettings settings = {};
  settings.address = ControllerConfig::kDefaultI2cAddress;
  for (uint8_t i = 0; i < kParameterCount; ++i) {
    settings.registerAddresses[i] = ControllerConfig::kUnconfiguredRegister;
    settings.parameterEnabled[i] = false;
  }

  // Addresses are visible in the UI as hints, but only Sub Contrast is enabled.
  settings.registerAddresses[kSubContrast] = 0x00;
  settings.parameterEnabled[kSubContrast] = true;
  return settings;
}

String defaultPresetName(int slot) {
  return "Preset " + String(slot + 1);
}

String normalizePresetName(String name, int slot) {
  name.trim();
  if (name.length() == 0) name = defaultPresetName(slot);
  if (name.length() > 24) name = name.substring(0, 24);
  return name;
}

void getAllPresetNames(String names[]) {
  Preferences preferences;
  preferences.begin(kPreferencesNamespace, true);
  for (int slot = 0; slot < kPresetCount; ++slot) {
    names[slot] = preferences.getString(
        presetKey(slot, "name").c_str(), defaultPresetName(slot));
  }
  preferences.end();
}

String getPresetName(int slot) {
  Preferences preferences;
  preferences.begin(kPreferencesNamespace, true);
  String name = preferences.getString(
      presetKey(slot, "name").c_str(), defaultPresetName(slot));
  preferences.end();
  return name;
}

I2cSettings loadGlobalI2cSettings() {
  I2cSettings settings = defaultI2cSettings();
  Preferences preferences;
  preferences.begin(kPreferencesNamespace, true);
  settings.address = preferences.getInt("gAddr", settings.address);
  for (uint8_t i = 0; i < kParameterCount; ++i) {
    settings.registerAddresses[i] = preferences.getInt(
        kGlobalRegisterKeys[i], settings.registerAddresses[i]);
    settings.parameterEnabled[i] = preferences.getBool(
        kGlobalEnabledKeys[i], settings.parameterEnabled[i]);
  }
  preferences.end();
  return settings;
}

void saveGlobalI2cSettings(const I2cSettings& settings) {
  Preferences preferences;
  preferences.begin(kPreferencesNamespace, false);
  preferences.putInt("gAddr", settings.address);
  for (uint8_t i = 0; i < kParameterCount; ++i) {
    preferences.putInt(kGlobalRegisterKeys[i], settings.registerAddresses[i]);
    preferences.putBool(kGlobalEnabledKeys[i], settings.parameterEnabled[i]);
  }
  preferences.end();
}

bool loadIsolationState() {
  Preferences preferences;
  preferences.begin(kPreferencesNamespace, true);
  const bool isIsolated = preferences.getBool("isolated", false);
  preferences.end();
  return isIsolated;
}

void saveIsolationState(bool isIsolated) {
  Preferences preferences;
  preferences.begin(kPreferencesNamespace, false);
  preferences.putBool("isolated", isIsolated);
  preferences.end();
}

int loadActivePresetSlot() {
  Preferences preferences;
  preferences.begin(kPreferencesNamespace, true);
  const int slot = preferences.getInt("activePreset", -1);
  preferences.end();
  return slot;
}

Settings loadPreset(int slot) {
  Settings settings = defaultSettings();
  Preferences preferences;
  preferences.begin(kPreferencesNamespace, true);
  for (uint8_t i = 0; i < kParameterCount; ++i) {
    settings.values[i] = preferences.getInt(
        presetKey(slot, kPresetParameterKeys[i]).c_str(),
        settings.values[i]);
  }
  preferences.end();
  return settings;
}

void setActivePresetSlot(int slot) {
  Preferences preferences;
  preferences.begin(kPreferencesNamespace, false);
  preferences.putInt("activePreset", slot);
  preferences.end();
}

void savePreset(int slot, const String& name, const Settings& settings) {
  Preferences preferences;
  preferences.begin(kPreferencesNamespace, false);
  preferences.putString(presetKey(slot, "name").c_str(), name);
  for (uint8_t i = 0; i < kParameterCount; ++i) {
    preferences.putInt(
        presetKey(slot, kPresetParameterKeys[i]).c_str(),
        settings.values[i]);
  }
  preferences.putInt("activePreset", slot);
  preferences.end();
}

}  // namespace SettingsStore