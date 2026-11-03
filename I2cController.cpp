#include "I2cController.h"

#include <Wire.h>

void I2cController::initializeIsolationPin(bool isIsolated) {
  pinMode(ControllerConfig::kIsolationControlPin, OUTPUT);
  setIsolation(isIsolated);
}

void I2cController::beginBus() {
  Wire.begin(
      ControllerConfig::kI2cSdaPin,
      ControllerConfig::kI2cSclPin,
      ControllerConfig::kI2cFrequencyHz);
  Wire.setTimeOut(30);
}

void I2cController::setIsolation(bool isIsolated) {
  digitalWrite(
      ControllerConfig::kIsolationControlPin,
      isIsolated ? LOW : HIGH);
}

void I2cController::writeParameter(
    const I2cSettings& settings,
    bool isIsolated,
    uint8_t parameterIndex,
    int value) {
  // Safety check: do not write if not isolated, parameter is disabled,
  // or register address is not configured.
  if (!isIsolated || parameterIndex >= kParameterCount ||
      !settings.parameterEnabled[parameterIndex] ||
      settings.registerAddresses[parameterIndex] < 0) {
    return;
  }

  Wire.beginTransmission(settings.address);
  Wire.write(static_cast<uint8_t>(settings.registerAddresses[parameterIndex]));
  Wire.write(static_cast<uint8_t>(value));
  Wire.endTransmission();
}

void I2cController::applySettings(
    const Settings& values,
    const I2cSettings& i2cSettings,
    bool isIsolated) {
  if (!isIsolated) return;
  for (uint8_t i = 0; i < kParameterCount; ++i) {
    writeParameter(i2cSettings, isIsolated, i, values.values[i]);
  }
}

String I2cController::scanBus(bool isIsolated) {
  String results = "Scan results:\n";
  uint8_t devicesFound = 0;

  // Scanning is performed with the external bus temporarily isolated
  digitalWrite(ControllerConfig::kIsolationControlPin, LOW);
  delay(20);

  // Reset the I2C peripheral to clear any stuck states.
  Wire.end();
  delay(10);
  Wire.begin(
      ControllerConfig::kI2cSdaPin,
      ControllerConfig::kI2cSclPin,
      ControllerConfig::kI2cFrequencyHz);

  for (uint8_t address = 1; address < 127; ++address) {
    Wire.beginTransmission(address);
    const uint8_t error = Wire.endTransmission();
    if (error == 0) {
      results += "➔ Found device: 0x";
      results += String(address, HEX);
      results += "\n";
      ++devicesFound;
    }
    delay(1);
  }

  // Restore the isolation state if it was originally connected.
  if (!isIsolated) {
    digitalWrite(ControllerConfig::kIsolationControlPin, HIGH);
  }

  if (devicesFound == 0) {
    results += "❌ No devices found.";
  } else {
    results += "Total: ";
    results += String(devicesFound);
  }

  return results;
}
