## v1.1.0
- Migrated from synchronous `WebServer` to `ESPAsyncWebServer` for non-blocking performance.
- Fixed an issue where the web interface would freeze or drop connections during frequent page reloads.
- Optimized I2C writes: moved heavy operations out of HTTP request handlers using asynchronous pending flags.
- Restored mDNS (`crt-control.local`) support, now running stably in the background.
- Enforced `Connection: close` headers to prevent TCP socket exhaustion when opening multiple tabs.

## v1.0.0 — Initial release
- I2C control of CRT preamp with 12 parameters
- 3 saved presets in NVS
- Parameter register map configurable via web UI
- Wi-Fi station mode with mDNS, fallback to access point
- Isolation control to disconnect the monitor MCU
- Bus scanner