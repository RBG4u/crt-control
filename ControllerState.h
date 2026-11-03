#pragma once

#include <Arduino.h>
#include <stdint.h>

constexpr int kPresetCount = 3;
constexpr uint8_t kParameterCount = 12;

enum ParameterIndex : uint8_t {
  kSubContrast = 0,
  kRGain,
  kGGain,
  kBGain,
  kSubBrightness,
  kRBias,
  kGBias,
  kBBias,
  kAbl,
  kG2,
  kDa1,
  kDa2,
};

namespace ControllerConfig {
constexpr const char* kFirmwareVersion = "1.1.0";
constexpr uint8_t kI2cSdaPin = 21;
constexpr uint8_t kI2cSclPin = 22;
constexpr uint8_t kIsolationControlPin = 4;
constexpr uint32_t kI2cFrequencyHz = 40000;

constexpr int kDefaultI2cAddress = 0x6E;
constexpr int kDefaultParameterValue = 128;
constexpr int kUnconfiguredRegister = -1;
}  // namespace ControllerConfig

struct Settings {
  int values[kParameterCount];
};

struct I2cSettings {
  int address;
  int registerAddresses[kParameterCount];
  bool parameterEnabled[kParameterCount];
};

struct AppState {
  Settings settings;
  I2cSettings i2cSettings;
  bool isIsolated = false;
  int activePresetSlot = -1;
  String scanResultLog = "Scanner idle";
};
