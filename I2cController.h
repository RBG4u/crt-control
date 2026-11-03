#pragma once

#include <Arduino.h>

#include "ControllerState.h"

namespace I2cController {

void initializeIsolationPin(bool isIsolated);
void beginBus();
void setIsolation(bool isIsolated);

void writeParameter(
    const I2cSettings& settings,
    bool isIsolated,
    uint8_t parameterIndex,
    int value);
void applySettings(
    const Settings& values, const I2cSettings& i2cSettings, bool isIsolated);
String scanBus(bool isIsolated);

}  // namespace I2cController
