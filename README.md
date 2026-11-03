# CRT Iso-Controller

[Русская версия](README.ru.md)

Firmware for the ESP32 that enables control of a CRT monitor's preamplifier via the I²C bus.

## Features
- **12 Configurable Parameters**: Control sub-contrast, RGB gains, biases, and more.
- **Universal Register Map**: Adjust chip addresses and register addresses directly from the UI to work with various chips.
- **Safety First**: Per-parameter enable flags prevent accidental writes to unknown registers.
- **3 Saved Presets**: Save and restore full parameter snapshots via non-volatile memory (NVS).
- **Hardware Isolation**: Controlling a switch to disconnect the monitor's MCU from the I²C bus.
- **Network Options**: Connect to your local Wi-Fi (with mDNS `crt-control.local`) or use the built-in fallback Access Point.
- **I²C Bus Scanner**: Built-in tool to scan and identify devices on the bus.

## Hardware Wiring
| Signal            | ESP32 GPIO |
| ----------------- | ---------- |
| I²C SDA           | 21         |
| I²C SCL           | 22         |
| Isolation relay   | 4          |

**Notes:**
- Power the preamp IC from the monitor's own power supply, **not** from the ESP32.
- The I²C lines require pull-up resistors; ensure they are present on the monitor side.

## Required Libraries 
Open Arduino IDE, go to **Sketch → Include Library → Manage Libraries...** and install:
1. **`ESPAsyncWebServer`** (by ESP32Async or me-no-dev)
2. **`AsyncTCP`** (by ESP32Async or me-no-dev)

## Installation
1. Install the [ESP32 Arduino Core](https://github.com/espressif/arduino-esp32) (v2.0 or newer).
2. Install the required libraries mentioned above.
3. Open `firmware/CRT_Iso_Controller/CRT_Iso_Controller.ino` in the Arduino IDE.
4. **Crucial Step:** Go to `Tools → Erase Flash → All Flash Contents`. This is required for the first installation to clear old configuration data.
5. Select your ESP32 board and click **Upload**.

## First Run & Network Setup
1. Power on the ESP32. After a 3-second delay, it will start in **Access Point (AP) mode**.
2. Connect your phone or PC to the Wi-Fi network **`CRT-Control`** (Password: **`12345678`**).
3. Open **`http://192.168.4.1`** in your browser.
4. By default, the device is in **Connected** mode. Go to **☰ Menu (top right) → I2C tab** to configure the addresses. Switch to **Isolated** to unlock the parameter controls.
5. Go to the **☰ Menu (Top Right) → Wi-Fi tab** to connect the ESP32 to your home network. Once connected, you can access the UI via `http://crt-control.local`.

## Web Interface Overview
### Main Screen
- **Isolation Control**: When the MCU is connected to the preamplifier, the controls are locked to prevent accidental recordings.
- **Saved Presets**: Select a slot from the dropdown to instantly apply it, or save your current settings.
- **Parameter Cards**: Use sliders, step buttons, or numeric inputs (0–255) to adjust values.

### Settings Menu (☰)
- **I²C Tab**: Enter the HEX addresses of the registers for the required parameters, check the *Send parameter* checkbox, and be sure to save. Use the **Bus Scanner** to check the preamplifier address.
- **Wi-Fi Tab**: Manage network credentials and view connection status.

## Safety and Troubleshooting
- **Do not guess registers**: Never write to a register unless you have verified its purpose in the chip's datasheet.
- **`crt-control.local` does not open:** If you can access the web interface by IP address but not by hostname, check if you have a **VPN enabled**. VPNs often intercept local DNS/mDNS queries and route them to the external server. Disable your VPN or use the direct IP address.
- **Web interface freezes on multiple tabs:** The ESP32 has a limited number of TCP sockets. The firmware enforces `Connection: close`, but avoid opening more than 3-4 tabs simultaneously.

## Wiring diagram

```text
┌─────────────────────────────────────────────────────────────────────────────┐
│                                  MONITOR                                    │
│                                                                             │
│   ┌───────────────────┐                                   ┌─────────────┐   │                         
│   │        MCU        │                                   │   PRE-AMP   │   │
│   │                   │                                   │             │   │
│   │ OUT:              │                 \ /               │ IN:         │   │
│   │ • [SDA_MCU]       │──────────────────x────────────────│ • [SDA_AMP] │   │
│   │ • [SCL_MCU]       │                 / \               │ • [SCL_AMP] │   │
│   └───────────────────┘                                   │             │   │
│     • [5V]                                                │             │   │
│     • [GND]                                               └─────────────┘   │
└─────────────────────────────────────────────────────────────────────────────┘
           │                                                       ▲
           │                                                       │ • Via 47–100 Ω resistors (usually already present on the board)
           ▼                                                       │
┌─────────────────────────────────────────────────────────────────────────────┐
│                                    CD4066                                   │
│                                                                             │
│   Power:                                                                    │
│   • VCC ◄────────────── [5V] (from the monitor)                             │
│   • GND ◄────────────── [GND] (from the monitor)                            │
│                                                                             │
│   Control:                                                                  │
│   • CTRL (A, B) ◄────── [GPIO_4] (from ESP32)                               │◄─┐
│                                                                             │  │
│   Switching:                                                                │  │
│   • IN A ◄───────────── [SDA_MCU]       OUT A ─────────────► [SDA_AMP]      │  │
│   • IN B ◄───────────── [SCL_MCU]       OUT B ─────────────► [SCL_AMP]      │  │
│                                                                             │  │
│                                                                             │  │
│                                         OUT A ◄───────────── [SDA_5V]       │  │
│                                         OUT B ◄───────────── [SCL_5V]       │  │
└─────────────────────────────────────────────────────────────────────────────┘  │
           │                                                       ▲             │
           │                                                       │             │
           ▼                                                       │             │
┌─────────────────────────────────────────────────────────────────────────────┐  │
│                            Logic Level Converter                            │  │
│                                                                             │  │
│   5V side (HV):                        3.3V side (LV):                      │  │
│   • HV_VCC ◄── [5V] (from the monitor) • LV_VCC ◄────── [3.3V] (от ESP32)   │  │
│   • HV_GND ◄── [GND] (from the monitor)• LV_GND ◄────── [GND] (от ESP32)    │  │
│   • HV_SDA ───► [SDA_5V] (to CD4066)   • LV_SDA ◄────── [SDA_ESP]           │  │
│   • HV_SCL ───► [SCL_5V] (to CD4066)   • LV_SCL ◄────── [SCL_ESP]           │  │
└─────────────────────────────────────────────────────────────────────────────┘  │ 
           │                                                       ▲             │
           │                                                       │             │
           ▼                                                       │             │
┌─────────────────────────────────────────────────────────────────────────────┐  │
│                                   ESP32                                     │  │
│                                                                             │  │
│   Power:                                                                    │  │
│   • VIN ◄────────────── [5V] (from the monitor or power supply unit)        │  │
│   • GND ◄────────────── [GND] (from the monitor or power supply unit)       │  │ 
│                                                                             │  │
│   Signal:                                                                   │  │
│   • GPIO 4 ───────────► [GPIO_4] (to the CD4066)                            │──┘
│   • SDA (Pin 21) ─────► [SDA_ESP] (to the LLC)                              │
│   • SCL (Pin 22) ─────► [SCL_ESP] (to the LLC)                              │
│   • 3.3V ─────────────► [3.3V] (to the LLC)                                 │
│   • GND ──────────────► [GND] (to the LLC)                                  │
└─────────────────────────────────────────────────────────────────────────────┘
```

## License
MIT License. See [LICENSE](LICENSE) for details.
