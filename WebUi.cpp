#include "WebUi.h"

#include <stdio.h>

namespace {

String escapeHtml(String value) {
  value.replace("&", "&amp;");
  value.replace("<", "&lt;");
  value.replace(">", "&gt;");
  value.replace("\"", "&quot;");
  value.replace("'", "&#39;");
  return value;
}

String hexByte(uint8_t value) {
  char buffer[3];
  snprintf(buffer, sizeof(buffer), "%02X", static_cast<unsigned int>(value));
  return String(buffer);
}

String initialNetworkMessage(const NetworkStatus& status) {
  if (status.connectionAttempting) {
    return "Connecting to the saved Wi-Fi network...";
  }
  
  const String localName = status.mdnsActive
      ? " Hostname: <a href='http://crt-control.local'>http://crt-control.local</a>."
      : "";
      
  const String ipLink = "<a href='http://" + status.ipAddress + "'>http://" + status.ipAddress + "</a>";

  if (status.stationConnected && status.accessPointActive) {
    return "Connected to the local network. IP: " + ipLink +
           "." + localName +
           " The setup access point is still active; restart to turn it off.";
  }
  if (status.stationConnected) {
    return "Connected to the local network. Open " + ipLink + "." + localName;
  }
  if (status.accessPointActive) {
    return "Setup access point is active. Address: " + ipLink + ".";
  }
  return "No network connection is active.";
}

String parameterRegisterField(
    const char* id,
    const char* label,
    int registerAddress,
    bool enabled) {
  String field;
  field.reserve(260);
  field += "<div class='param-address-item'><label for='reg_";
  field += id;
  field += "'>";
  field += label;
  field += "</label><input class='reg-input' type='text' id='reg_";
  field += id;
  field += "' maxlength='2' placeholder='HEX' value='";
  if (registerAddress >= 0) {
    field += hexByte(static_cast<uint8_t>(registerAddress));
  }
  field += "'><label class='checkbox-row'><input type='checkbox' id='enable_";
  field += id;
  field += "'";
  if (enabled) field += " checked";
  field += "> Send parameter</label></div>";
  return field;
}

String parameterCard(
    const char* label,
    const char* id,
    uint8_t parameterIndex,
    int currentValue,
    bool enabled,
    bool isIsolated) {
  const bool controlsAvailable = enabled && isIsolated;
  String card;
  card.reserve(700);
  card += "<div class='card parameter-card' data-param-enabled='";
  card += enabled ? "true" : "false";
  card += "' data-param-index='";
  card += String(parameterIndex);
  card += "'><h3>";
  card += label;
  card += "</h3><div class='control-row'>";
  card += "<button class='step-btn' type='button' aria-label='Decrease value' ";
  if (!controlsAvailable) card += "disabled ";
  card += "onclick='stepParam(\"";
  card += id;
  card += "\", -1)'>&#9664;</button>";
  card += "<input type='range' min='0' max='255' value='";
  card += String(currentValue);
  card += "' id='sld_";
  card += id;
  card += "'";
  if (!controlsAvailable) card += " disabled";
  card += " oninput='onSlider(\"";
  card += id;
  card += "\", this.value)'>";
  card += "<button class='step-btn' type='button' aria-label='Increase value' ";
  if (!controlsAvailable) card += "disabled ";
  card += "onclick='stepParam(\"";
  card += id;
  card += "\", 1)'>&#9654;</button>";
  card += "<input type='number' min='0' max='255' step='1' value='";
  card += String(currentValue);
  card += "' id='num_";
  card += id;
  card += "'";
  if (!controlsAvailable) card += " disabled";
  card += " oninput='onNumInput(\"";
  card += id;
  card += "\", this.value)'>";
  card += "<button class='btn' ";
  if (!controlsAvailable) card += "disabled ";
  card += "onclick='sendDirect(\"";
  card += id;
  card += "\")'>SET</button>";
  card += "</div></div>";
  return card;
}

const char kStyles[] = R"CSS(
body { font-family: system-ui, sans-serif; background: #121214; color: #e1e1e6; text-align: center; padding: 15px; margin: 0; }
h2 { color: #00adb5; margin: 15px 0; font-weight: 600; font-size: 1.4rem; }
.topbar { max-width: 420px; margin: 0 auto 14px; display: flex; align-items: center; justify-content: space-between; gap: 12px; }
.brand-title { text-align: left; margin: 0; font-size: 1.3rem; white-space: nowrap; }
.section-title { font-size: 0.9rem; text-transform: uppercase; color: #7c7c8a; letter-spacing: 1px; text-align: left; max-width: 420px; margin: 15px auto 5px auto; padding-left: 5px; }
.card { background: #202024; padding: 15px; border-radius: 10px; margin: 8px auto; max-width: 420px; box-shadow: 0 4px 10px rgba(0,0,0,0.4); text-align: left; box-sizing: border-box; }
h3 { margin: 0 0 5px 0; font-size: 1rem; color: #a8a8b3; }
.control-row { display: flex; align-items: center; gap: 10px; margin: 10px 0; }
input[type=range] { flex-grow: 1; height: 6px; border-radius: 3px; background: #29292e; outline: none; -webkit-appearance: none; margin: 0; }
input[type=range]::-webkit-slider-thumb { -webkit-appearance: none; width: 20px; height: 20px; border-radius: 50%; background: #00adb5; cursor: pointer; }
input[type=number], input[type=text], input[type=password] { background: #29292e; color: #00adb5; border: 1px solid #4e4e5a; padding: 8px; border-radius: 6px; font-size: 0.9rem; box-sizing: border-box; }
input[type=number] { width: 58px; text-align: center; font-weight: bold; }
input[type=text], input[type=password] { width: 100%; }
.btn { background: #00adb5; color: #fff; border: none; padding: 8px 12px; font-size: 0.9rem; font-weight: bold; border-radius: 6px; cursor: pointer; box-shadow: 0 2px 5px rgba(0,173,181,0.2); }
.btn:disabled, .step-btn:disabled, .parameter-card input:disabled { opacity: 0.42; cursor: not-allowed; }
.preset-save-btn { width: 100%; }
.btn-big { padding: 14px; font-size: 1rem; width: 100%; max-width: 420px; margin: 15px auto; display: block; }
.btn-scan { background: #28a745; box-shadow: 0 4px 10px rgba(40,167,69,0.2); width: 100%; padding: 12px; margin-top: 10px; }
.step-btn { flex: 0 0 30px; width: 30px; height: 32px; padding: 0; background: #29292e; color: #00adb5; border: 1px solid #4e4e5a; border-radius: 6px; font-size: 0.85rem; font-weight: bold; cursor: pointer; }
.btn-iso { background: #ffc107; color: #121214; width: 100%; padding: 12px; font-size: 1.1rem; }
.btn-iso.active { background: #dc3545; color: #fff; }
select { background: #29292e; color: #00adb5; border: 1px solid #4e4e5a; padding: 8px; border-radius: 6px; font-size: 1rem; flex-grow: 1; font-weight: bold; }
.log-box { font-family: monospace; font-size: 0.85rem; background: #1a1a1e; padding: 10px; border-radius: 6px; color: #ffc107; margin-top: 5px; white-space: pre-line; }
.settings-trigger-wrap { margin: 0; text-align: right; flex: 0 0 auto; }
.settings-trigger { background: #29292e; color: #e1e1e6; border: 1px solid #4e4e5a; border-radius: 8px; padding: 8px 11px; font-size: 0.9rem; cursor: pointer; }
.drawer-backdrop { position: fixed; inset: 0; background: rgba(0,0,0,0.58); opacity: 0; visibility: hidden; transition: opacity 0.22s ease, visibility 0.22s ease; z-index: 20; }
.drawer-backdrop.open { opacity: 1; visibility: visible; }
.settings-drawer { 
  position: fixed; 
  top: 0; 
  bottom: 0; /* Use top/bottom instead of height: 100vh for mobile */
  right: 0; 
  width: min(400px, 92vw); 
  padding: 18px; 
  padding-bottom: calc(24px + env(safe-area-inset-bottom, 0px)); /* Safe area padding for iOS */
  box-sizing: border-box; 
  overflow-y: auto; 
  -webkit-overflow-scrolling: touch; /* Smooth inertial scrolling on iOS */
  background: #202024; 
  box-shadow: -8px 0 28px rgba(0,0,0,0.45); 
  transform: translateX(105%); 
  transition: transform 0.22s ease; 
  z-index: 21; 
  text-align: left; 
}
.settings-drawer.open { transform: translateX(0); }
.drawer-header { display: flex; align-items: center; justify-content: space-between; gap: 12px; margin-bottom: 14px; }
.drawer-close { background: transparent; color: #e1e1e6; border: 1px solid #4e4e5a; border-radius: 6px; padding: 5px 10px; font-size: 1.2rem; cursor: pointer; }
.menu-tabs { display: flex; gap: 8px; margin-bottom: 14px; }
.tab-btn { flex: 1; background: #29292e; color: #a8a8b3; border: 1px solid #4e4e5a; border-radius: 6px; padding: 9px; cursor: pointer; }
.tab-btn.active { color: #fff; border-color: #00adb5; background: #174044; }
.settings-panel { display: none; }
.settings-panel.active { display: block; }
.reg-grid { display: grid; grid-template-columns: 1fr 1fr; gap: 10px; margin: 8px 0; }
.param-address-item { background: #29292e; border: 1px solid #3b3b43; border-radius: 7px; padding: 8px; min-width: 0; }
.param-address-item > label:first-child { display: block; color: #a8a8b3; font-size: 0.82rem; margin-bottom: 4px; }
.param-address-item .checkbox-row { margin: 7px 0 0; font-size: 0.78rem; }
.reg-field label { display: block; color: #a8a8b3; font-size: 0.82rem; margin-bottom: 4px; }
.reg-input { width: 100% !important; text-align: center; text-transform: uppercase; }
.help-text { color: #a8a8b3; font-size: 0.82rem; line-height: 1.45; margin: 7px 0 12px; }
.status-box { background: #1a1a1e; border-radius: 6px; padding: 10px; color: #e1e1e6; font-size: 0.88rem; line-height: 1.45; overflow-wrap: anywhere; }
.status-box a { color: #00adb5; text-decoration: underline; font-weight: bold; }
.checkbox-row { display: flex; align-items: center; gap: 8px; font-size: 0.88rem; color: #c7c7cf; margin: 10px 0; }
.checkbox-row input { accent-color: #00adb5; }
.network-actions { display: grid; gap: 8px; }
)CSS";

const char kClientScript[] = R"JS(
var timers = {};
var isRequestPending = false;

window.addEventListener('DOMContentLoaded', function() {
  updatePresetName();
  setParameterControlsEnabled(document.body.dataset.isolated === 'true');
  setTimeout(refreshNetworkStatus, 1000); 
});

function toggleSettingsMenu(open) {
  document.getElementById('settingsDrawer').classList.toggle('open', open);
  document.getElementById('settingsBackdrop').classList.toggle('open', open);
  document.getElementById('settingsDrawer')
    .setAttribute('aria-hidden', open ? 'false' : 'true');
  document.body.style.overflow = open ? 'hidden' : '';
}

function showSettingsTab(tab) {
  var isI2c = tab === 'i2c';
  document.getElementById('i2cPanel').classList.toggle('active', isI2c);
  document.getElementById('wifiPanel').classList.toggle('active', !isI2c);
  document.getElementById('i2cTab').classList.toggle('active', isI2c);
  document.getElementById('wifiTab').classList.toggle('active', !isI2c);
}

function setParameterControlsEnabled(isolated) {
  document.querySelectorAll('.parameter-card').forEach(function(card) {
    var enabled = isolated && card.dataset.paramEnabled === 'true';
    card.querySelectorAll('input, button').forEach(function(control) {
      control.disabled = !enabled;
    });
  });
}

document.addEventListener('keydown', function(event) {
  if (event.key === 'Escape') toggleSettingsMenu(false);
});

function toggleIso(state) {
  var button = document.getElementById('isoButton');
  button.disabled = true;
  var controller = new AbortController();
  var timeout = setTimeout(function() { controller.abort(); }, 3000);
  fetch('/iso?state=' + state, { signal: controller.signal }).then(function(response) {
    if (!response.ok) throw new Error('Isolation could not be changed');
    clearTimeout(timeout);
    var isolated = state === 1;
    button.className = isolated ? 'btn btn-iso active' : 'btn btn-iso';
    button.textContent = isolated
      ? 'ISOLATED (Click to Reconnect)'
      : 'CONNECTED (Click to Isolate)';
    button.onclick = function() { toggleIso(isolated ? 0 : 1); };
    setParameterControlsEnabled(isolated);
  }).catch(function(error) {
    clearTimeout(timeout);
    if (error.name === 'AbortError') {
      alert('Network timeout. Please check connection.');
    } else {
      alert('Isolation change failed.');
    }
  }).finally(function() {
    button.disabled = false;
  });
}

function updatePresetName() {
  var select = document.getElementById('presetSelect');
  document.getElementById('presetName').value =
    select.options[select.selectedIndex].text;
}

function selectPreset() {
  var select = document.getElementById('presetSelect');
  var slot = select.value;
  updatePresetName();
  
  select.disabled = true;
  
  fetch('/preset?action=apply&slot=' + slot).then(function(response) {
    if (!response.ok) throw new Error('Preset could not be applied');

    setTimeout(function() { location.reload(); }, 150);
    
  }).catch(function() {
    alert('Preset could not be applied.');
    // Re-enable select on error
    select.disabled = false; 
  });
}

function savePreset() {
  if (isRequestPending) return;
  isRequestPending = true;

  var slot = document.getElementById('presetSelect').value;
  var name = document.getElementById('presetName').value;
  fetch('/preset?action=save&slot=' + slot + '&name=' + encodeURIComponent(name))
    .then(function(response) {
    if (!response.ok) throw new Error('Preset could not be saved');
    setTimeout(function() { location.reload(); }, 200);
  }).catch(function() {
    alert('Preset could not be saved.');
    isRequestPending = false;
  });
}

function stepParam(param, delta) {
  var slider = document.getElementById('sld_' + param);
  var next = Math.max(0, Math.min(255, Number(slider.value) + delta));
  slider.value = next;
  onSlider(param, next);
}

function onSlider(param, value) {
  document.getElementById('num_' + param).value = value;
  clearTimeout(timers[param]);
  timers[param] = setTimeout(function() {
    // Skip if a previous request is still pending
    if (isRequestPending) return; 
    isRequestPending = true;
    
    fetch('/set?' + param + '=' + value).finally(function() {
      isRequestPending = false; // Release the lock when ESP32 responds
    });
  }, 100); 
}

function onNumInput(param, value) {
  if (value === '') return;
  var numericValue = Number(value);
  if (!Number.isFinite(numericValue)) return;
  var boundedValue = Math.max(0, Math.min(255, Math.trunc(numericValue)));
  document.getElementById('num_' + param).value = boundedValue;
  document.getElementById('sld_' + param).value = boundedValue;
}

function sendDirect(param) {
  var input = document.getElementById('num_' + param);
  var slider = document.getElementById('sld_' + param);
  
  if (input.value === '') {
    input.value = slider.value;
    return;
  }
  
  var numericValue = Number(input.value);
  if (!Number.isFinite(numericValue)) {
    input.value = slider.value;
    return;
  }
  
  var value = Math.max(0, Math.min(255, Math.trunc(numericValue)));
  input.value = value;
  slider.value = value;

  if (isRequestPending) return; 
  
  isRequestPending = true;

  fetch('/set?' + param + '=' + value)
    .finally(function() {
      isRequestPending = false; 
    });
  // ------------------------
}

function changeAddress(address) {
  document.getElementById('hexAddrInput').value =
    Number(address).toString(16).toUpperCase().padStart(2, '0');
}

function saveAddressSettings() {
  var hexString = document.getElementById('hexAddrInput').value;
  var address = parseInt(hexString, 16);
  if (!/^[0-9a-f]{1,2}$/i.test(hexString.trim()) ||
      isNaN(address) || address < 1 || address > 127) {
    alert('Enter a hexadecimal I2C address from 01 to 7F.');
    return;
  }

  var btn = document.querySelector("button[onclick='saveAddressSettings()']");
  if (btn) {
    btn.disabled = true;
    btn.textContent = 'SAVING...';
  }

  var select = document.getElementById('addrSelect');
  var optionExists = Array.from(select.options).some(function(option) {
    return Number(option.value) === address;
  });
  if (!optionExists) {
    select.add(new Option(
      'Custom (0x' + address.toString(16).toUpperCase().padStart(2, '0') + ')',
      address
    ));
  }
  select.value = String(address);

  var fields = [
    ['sc', 'subContrast'], ['rg', 'rGain'], ['gg', 'gGain'], ['bg', 'bGain'],
    ['sb', 'subBrightness'], ['rb', 'rBias'], ['gb', 'gBias'], ['bb', 'bBias'],
    ['abl', 'abl'], ['g2', 'g2'], ['d1', 'da1'], ['d2', 'da2']
  ];
  var params = new URLSearchParams();
  params.set('addr', String(address));
  for (var i = 0; i < fields.length; i++) {
    var id = fields[i][1];
    var text = document.getElementById('reg_' + id).value.trim();
    var enabled = document.getElementById('enable_' + id).checked;
    var registerAddress = -1;
    if (text !== '') {
      if (!/^[0-9a-f]{1,2}$/i.test(text)) {
        alert('Register addresses must be hexadecimal values from 00 to FF.');
        return;
      }
      registerAddress = parseInt(text, 16);
    } else if (enabled) {
      alert('Enter a register address before enabling this parameter.');
      return;
    }
    params.set('r_' + fields[i][0], String(registerAddress));
    params.set('e_' + fields[i][0], enabled ? '1' : '0');
  }

  fetch('/setRegs?' + params.toString()).then(function(response) {
    if (!response.ok) throw new Error('I2C addresses could not be saved');
    setTimeout(function() { location.reload(); }, 200); // Delay to allow socket closure
  }).catch(function(error) {
    alert(error.message || 'I2C addresses could not be saved.');
    if (btn) {
      btn.disabled = false;
      btn.textContent = 'SAVE ADDRESSES';
    }
  });
}

function updateNetworkMessage(network) {
  var hostname = network.mdns
    ? " Hostname: <a href='http://crt-control.local'>http://crt-control.local</a>."
    : "";
  var ipLink = "<a href='http://" + network.ip + "'>http://" + network.ip + "</a>";

  if (network.attempting) {
    return 'Connecting to the saved Wi-Fi network...';
  }
  if (network.connected && network.accessPoint) {
    return 'Connected to the local network. IP: ' + ipLink + '.' +
      hostname + ' The setup access point is still active; restart to turn it off.';
  }
  if (network.connected) {
    return 'Connected to the local network. Open ' + ipLink + '.' + hostname;
  }
  if (network.accessPoint) {
    return 'Access point mode is active. Address: ' + ipLink + '.';
  }
  return 'No network connection is active.';
}

function runScanner() {
  document.getElementById('scanLog').innerText = 'Scanning...';
  fetch('/scan')
    .then(function(response) { return response.text(); })
    .then(function(text) {
      document.getElementById('scanLog').innerText = text;
    });
}

function saveNetworkSettings() {
  var button = document.getElementById('saveNetworkButton');
  var status = document.getElementById('networkStatus');
  var password = document.getElementById('wifiPassword');
  button.disabled = true;
  status.textContent = 'Saving settings and starting connection attempt...';

  var body = new URLSearchParams();
  body.set('ssid', document.getElementById('wifiSsid').value.trim());
  body.set('password', password.value);
  body.set(
    'open',
    document.getElementById('wifiOpenNetwork').checked ? '1' : '0'
  );

  fetch('/wifi/save', {
    method: 'POST',
    headers: { 'Content-Type': 'application/x-www-form-urlencoded' },
    body: body.toString()
  }).then(function(response) {
    return response.text().then(function(text) {
      if (!response.ok && response.status !== 202) throw new Error(text);
    });
  }).then(function() {
    password.value = '';
    refreshNetworkStatus();
  }).catch(function(error) {
    status.textContent = error.message || 'Could not save Wi-Fi settings.';
  }).finally(function() {
    button.disabled = false;
  });
}

function refreshNetworkStatus() {
  var controller = new AbortController();
  var timeout = setTimeout(function() { controller.abort(); }, 10000);
  
  fetch('/wifi/status', { signal: controller.signal })
    .then(function(response) { 
      if (!response.ok) throw new Error('HTTP error');
      return response.json(); 
    })
    .then(function(network) {
      clearTimeout(timeout);
      var status = document.getElementById('networkStatus');
      var restartButton = document.getElementById('restartNetworkButton');
      status.innerHTML = updateNetworkMessage(network);
      restartButton.style.display =
        network.connected && network.accessPoint ? 'block' : 'none';
      if (network.attempting) {
        setTimeout(refreshNetworkStatus, 5000); 
      }
    })
    .catch(function() {
      clearTimeout(timeout);
      var statusBox = document.getElementById('networkStatus');
      if (statusBox.textContent.indexOf('Could not') === -1) {
        statusBox.textContent = 'Could not refresh network status.';
      }
    });
}

function restartIntoLocalWifi() {
  var status = document.getElementById('networkStatus');
  status.textContent =
    'Restarting. Reconnect to the local Wi-Fi; try http://crt-control.local or the IP shown above.';
  fetch('/wifi/restart', { method: 'POST' }).catch(function() {});
}
)JS";

}  // namespace

namespace WebUi {

String render(
    const AppState& state,
    const String presetNames[],
    const NetworkStatus& networkStatus) {
  const int selectedPreset =
      (state.activePresetSlot >= 0 && state.activePresetSlot < kPresetCount)
          ? state.activePresetSlot
          : 0;

  String html;
  html.reserve(28000);
  html += "<!DOCTYPE html><html><head>";
  html += "<meta charset='UTF-8'>";
  html += "<meta name='viewport' content='width=device-width, initial-scale=1.0'>";
  html += "<title>CRT Control</title><style>";
  html += kStyles;
  html += "</style></head><body data-isolated='";
  html += state.isIsolated ? "true" : "false";
  html += "'>";

  // Main layout: settings menu, isolation, presets, parameter controls.
  html += "<header class='topbar'><h2 class='brand-title'>CRT Control</h2>";
  html += "<span style='font-size:0.75rem;color:#7c7c8a'>v";
  html += ControllerConfig::kFirmwareVersion;
  html += "</span>";
  html += "<div class='settings-trigger-wrap'>";
  html += "<button class='settings-trigger' onclick='toggleSettingsMenu(true)'>";
  html += "&#9776; I2C &amp; Wi-Fi</button></div></header>";

  html += "<div class='drawer-backdrop' id='settingsBackdrop' ";
  html += "onclick='toggleSettingsMenu(false)'></div>";
  html += "<aside class='settings-drawer' id='settingsDrawer' aria-hidden='true'>";
  html += "<div class='drawer-header'><h3>I2C Tools &amp; Network Settings</h3>";
  html += "<button class='drawer-close' aria-label='Close settings' ";
  html += "onclick='toggleSettingsMenu(false)'>&times;</button></div>";
  html += "<div class='menu-tabs'>";
  html += "<button id='i2cTab' class='tab-btn active' ";
  html += "onclick=\"showSettingsTab('i2c')\">I2C</button>";
  html += "<button id='wifiTab' class='tab-btn' ";
  html += "onclick=\"showSettingsTab('wifi')\">Wi-Fi</button></div>";

  html += "<section id='i2cPanel' class='settings-panel active'>";
  html += "<div class='section-title'>Active I2C Chip Address</div>";
  html += "<div class='control-row'><select id='addrSelect' ";
  html += "onchange='changeAddress(this.value)'>";

  const char* addressLabels[] = {"0x6E", "0x6F", "0x44", "0x45", "0x88"};
  const int addressValues[] = {0x6E, 0x6F, 0x44, 0x45, 0x88};
  bool addressIsListed = false;
  for (int i = 0; i < 5; ++i) {
    html += "<option value='";
    html += String(addressValues[i]);
    html += "'";
    if (state.i2cSettings.address == addressValues[i]) {
      html += " selected";
      addressIsListed = true;
    }
    html += ">";
    html += addressLabels[i];
    html += "</option>";
  }

  const String addressHex =
      hexByte(static_cast<uint8_t>(state.i2cSettings.address));
  if (!addressIsListed) {
    html += "<option value='";
    html += String(state.i2cSettings.address);
    html += "' selected>Custom (0x";
    html += addressHex;
    html += ")</option>";
  }

  html += "</select></div><div class='control-row'>";
  html += "<input type='text' id='hexAddrInput' maxlength='2' value='";
  html += addressHex;
  html += "' placeholder='HEX' style='flex-grow:1'>";
  html += "</div><div class='section-title'>Parameter Registers (HEX)</div>";
  html += "<p class='help-text'>Parameters without a known register address start disabled. ";
  html += "Enter their address before enabling them.</p>";
  html += "<div class='reg-grid'>";
  html += parameterRegisterField(
      "subContrast", "Sub Contrast",
      state.i2cSettings.registerAddresses[kSubContrast],
      state.i2cSettings.parameterEnabled[kSubContrast]);
  html += parameterRegisterField(
      "rGain", "R Gain", state.i2cSettings.registerAddresses[kRGain],
      state.i2cSettings.parameterEnabled[kRGain]);
  html += parameterRegisterField(
      "gGain", "G Gain", state.i2cSettings.registerAddresses[kGGain],
      state.i2cSettings.parameterEnabled[kGGain]);
  html += parameterRegisterField(
      "bGain", "B Gain", state.i2cSettings.registerAddresses[kBGain],
      state.i2cSettings.parameterEnabled[kBGain]);
  html += parameterRegisterField(
      "subBrightness", "Sub Brightness",
      state.i2cSettings.registerAddresses[kSubBrightness],
      state.i2cSettings.parameterEnabled[kSubBrightness]);
  html += parameterRegisterField(
      "rBias", "R Bias", state.i2cSettings.registerAddresses[kRBias],
      state.i2cSettings.parameterEnabled[kRBias]);
  html += parameterRegisterField(
      "gBias", "G Bias", state.i2cSettings.registerAddresses[kGBias],
      state.i2cSettings.parameterEnabled[kGBias]);
  html += parameterRegisterField(
      "bBias", "B Bias", state.i2cSettings.registerAddresses[kBBias],
      state.i2cSettings.parameterEnabled[kBBias]);
  html += parameterRegisterField(
      "abl", "ABL", state.i2cSettings.registerAddresses[kAbl],
      state.i2cSettings.parameterEnabled[kAbl]);
  html += parameterRegisterField(
      "g2", "G2", state.i2cSettings.registerAddresses[kG2],
      state.i2cSettings.parameterEnabled[kG2]);
  html += parameterRegisterField(
      "da1", "D/A 1", state.i2cSettings.registerAddresses[kDa1],
      state.i2cSettings.parameterEnabled[kDa1]);
  html += parameterRegisterField(
      "da2", "D/A 2", state.i2cSettings.registerAddresses[kDa2],
      state.i2cSettings.parameterEnabled[kDa2]);
  html += "</div><button class='btn btn-scan' ";
  html += "onclick='saveAddressSettings()'>SAVE ADDRESSES</button>";
  html += "<div class='section-title'>Bus Scanner</div>";
  html += "<button class='btn btn-scan' onclick='runScanner()'>";
  html += "RUN CLEAN BUS SCANNER</button><div class='log-box' id='scanLog'>";
  html += escapeHtml(state.scanResultLog);
  html += "</div></section>";

  html += "<section id='wifiPanel' class='settings-panel'>";
  html += "<div class='section-title'>Network Status</div>";
  html += "<div class='status-box' id='networkStatus'>";
  html += initialNetworkMessage(networkStatus);
  html += "</div>";
  html += "<div class='section-title'>Connect to Local Wi-Fi</div>";
  html += "<label for='wifiSsid'>Network name (SSID)</label>";
  html += "<input type='text' id='wifiSsid' maxlength='32' value='";
  html += escapeHtml(networkStatus.savedSsid);
  html += "' autocomplete='off' placeholder='Your Wi-Fi name'>";
  html += "<label for='wifiPassword' style='display:block;margin-top:10px'>";
  html += "Network password</label>";
  html += "<input type='password' id='wifiPassword' maxlength='63' ";
  html += "autocomplete='new-password' placeholder='Leave empty to keep the saved password'>";
  html += "<p class='help-text'>The password is not shown on this page. ";
  html += "For a new secured network, enter its password. For an open network, ";
  html += "select the option below.</p>";
  html += "<label class='checkbox-row'><input type='checkbox' ";
  html += "id='wifiOpenNetwork'> This network has no password / clear saved password";
  html += "</label><div class='network-actions'>";
  html += "<button id='saveNetworkButton' class='btn btn-scan' ";
  html += "onclick='saveNetworkSettings()'>SAVE AND CONNECT</button>";
  html += "<button id='restartNetworkButton' class='btn' style='display:none' ";
  html += "onclick='restartIntoLocalWifi()'>RESTART IN LOCAL WI-FI MODE</button>";
  html += "</div></section></aside>";

  html += "<div class='section-title'>Processor Isolation Mode</div>";
  html += "<div class='card'><h3>Cut off Monitor MCU Link</h3>";
  if (state.isIsolated) {
    html += "<button id='isoButton' class='btn btn-iso active' ";
    html += "onclick='toggleIso(0)'>ISOLATED (Click to Reconnect)</button>";
  } else {
    html += "<button id='isoButton' class='btn btn-iso' ";
    html += "onclick='toggleIso(1)'>CONNECTED (Click to Isolate)</button>";
  }
  html += "</div>";

  html += "<div class='section-title'>Saved Presets</div>";
  html += "<div class='card'><h3>Choose, rename, or save settings</h3>";
  html += "<div class='control-row'><select id='presetSelect' onchange='selectPreset()'>";
  for (int slot = 0; slot < kPresetCount; ++slot) {
    html += "<option value='";
    html += String(slot);
    html += "'";
    if (slot == selectedPreset) html += " selected";
    html += ">";
    html += escapeHtml(presetNames[slot]);
    html += "</option>";
  }
  html += "</select></div><div class='control-row'>";
  html += "<input type='text' id='presetName' maxlength='24' value='";
  html += escapeHtml(presetNames[selectedPreset]);
  html += "' style='width:auto;flex-grow:1;text-transform:none' ";
  html += "placeholder='Preset name'></div>";
  html += "<div class='control-row'><button class='btn preset-save-btn' onclick='savePreset()'>";
  html += "SAVE PRESET</button></div></div>";

  html += "<div class='section-title'>Main</div>";
  html += parameterCard(
      "Sub Contrast", "subContrast", kSubContrast,
      state.settings.values[kSubContrast],
      state.i2cSettings.parameterEnabled[kSubContrast] &&
          state.i2cSettings.registerAddresses[kSubContrast] >= 0,
      state.isIsolated);
  html += parameterCard(
      "Sub Brightness", "subBrightness", kSubBrightness,
      state.settings.values[kSubBrightness],
      state.i2cSettings.parameterEnabled[kSubBrightness] &&
          state.i2cSettings.registerAddresses[kSubBrightness] >= 0,
      state.isIsolated);

  html += "<div class='section-title'>White Balance</div>";
  html += parameterCard(
      "R Gain", "rGain", kRGain, state.settings.values[kRGain],
      state.i2cSettings.parameterEnabled[kRGain] &&
          state.i2cSettings.registerAddresses[kRGain] >= 0,
      state.isIsolated);
  html += parameterCard(
      "G Gain", "gGain", kGGain, state.settings.values[kGGain],
      state.i2cSettings.parameterEnabled[kGGain] &&
          state.i2cSettings.registerAddresses[kGGain] >= 0,
      state.isIsolated);
  html += parameterCard(
      "B Gain", "bGain", kBGain, state.settings.values[kBGain],
      state.i2cSettings.parameterEnabled[kBGain] &&
          state.i2cSettings.registerAddresses[kBGain] >= 0,
      state.isIsolated);
  html += parameterCard(
      "R Bias", "rBias", kRBias, state.settings.values[kRBias],
      state.i2cSettings.parameterEnabled[kRBias] &&
          state.i2cSettings.registerAddresses[kRBias] >= 0,
      state.isIsolated);
  html += parameterCard(
      "G Bias", "gBias", kGBias, state.settings.values[kGBias],
      state.i2cSettings.parameterEnabled[kGBias] &&
          state.i2cSettings.registerAddresses[kGBias] >= 0,
      state.isIsolated);
  html += parameterCard(
      "B Bias", "bBias", kBBias, state.settings.values[kBBias],
      state.i2cSettings.parameterEnabled[kBBias] &&
          state.i2cSettings.registerAddresses[kBBias] >= 0,
      state.isIsolated);

  html += "<div class='section-title'>Other</div>";
  html += parameterCard(
      "ABL", "abl", kAbl, state.settings.values[kAbl],
      state.i2cSettings.parameterEnabled[kAbl] &&
          state.i2cSettings.registerAddresses[kAbl] >= 0,
      state.isIsolated);
  html += parameterCard(
      "G2", "g2", kG2, state.settings.values[kG2],
      state.i2cSettings.parameterEnabled[kG2] &&
          state.i2cSettings.registerAddresses[kG2] >= 0,
      state.isIsolated);
  html += parameterCard(
      "D/A 1", "da1", kDa1, state.settings.values[kDa1],
      state.i2cSettings.parameterEnabled[kDa1] &&
          state.i2cSettings.registerAddresses[kDa1] >= 0,
      state.isIsolated);
  html += parameterCard(
      "D/A 2", "da2", kDa2, state.settings.values[kDa2],
      state.i2cSettings.parameterEnabled[kDa2] &&
          state.i2cSettings.registerAddresses[kDa2] >= 0,
      state.isIsolated);

  html += "<script>";
  html += kClientScript;
  html += "</script></body></html>";
  return html;
}

}  // namespace WebUi
