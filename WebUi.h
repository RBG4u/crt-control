#pragma once

#include <Arduino.h>

#include "ControllerState.h"
#include "NetworkManager.h"

namespace WebUi {

String render(
    const AppState& state,
    const String presetNames[],
    const NetworkStatus& networkStatus);

}  // namespace WebUi
