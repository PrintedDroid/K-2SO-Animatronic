# K-2SO Development Notes

> Internal development documentation for maintainers and contributors.
> For user-facing documentation, see [README.md](README.md).

---

## Architecture Decisions

### webpage.h Cleanup (v1.2.5 Post-Review)

During the v1.2.5 code review, `webpage.h` was found to contain **49 function declarations**, of which only **15 were actually implemented** in `webpage.cpp`. The 34 unimplemented declarations were removed to prevent potential linker issues and reduce confusion.

This section documents the rationale for each removal and identifies candidates worth reconsidering for future versions.

---

### Removed: Correctly Eliminated (30 functions)

These declarations represented web-development patterns that are inappropriate for an embedded ESP32 system or were already handled inline.

#### Widget Sub-Functions (5)
`getServoGamepad()`, `getColorPicker()`, `getBrightnessSlider()`, `getVolumeSlider()`, `getModeSelector()`

**Why removed:** Already contained inline within the section generators (e.g., `getServoControlSection()` generates the gamepad HTML directly). Separate functions would create extra stack frames, additional temporary `String` allocations, and function-call overhead for zero reusability benefit. On ESP32, every temporary String contributes to heap fragmentation.

#### Error Pages (4)
`get404Page()`, `get500Page()`, `getOfflinePage()`, `getMaintenancePage()`

**Why removed:** This is a single-page droid controller on a local WiFi AP, not a production web application. There is one page at `/`. A simple text 404 response in `handleNotFound()` is sufficient and costs near-zero flash. A styled maintenance page is nonsensical for an embedded system -- if the ESP32 is running, it is serving. An offline page would require service worker infrastructure.

#### Theme/Styling Functions (4)
`getK2SOTheme()`, `getDarkMode()`, `getLightMode()`, `getAnimatedElements()`

**Why removed:** The page IS the K-2SO theme (dark gradient background, blue accents). Light/dark mode switching adds ~5-8 KB of flash for two CSS variants plus toggle logic, for a droid controller typically used in dim workshop or convention environments. CSS animations (pulse, spin, slide) are already embedded where needed.

#### PWA/ServiceWorker/Install (3)
`getProgressiveWebApp()`, `getServiceWorker()`, `getInstallPrompt()`

**Why removed:** The ESP32 IS the server. If the ESP32 is off, there is nothing to cache. A PWA manifest for a local WiFi AP with no internet access provides no benefit. Service workers add ~5-10 KB of flash for functionality that has no use case. "Install as App" -- just bookmark the IP.

#### Mobile Optimization (3)
`getMobileCSS()`, `getTouchControls()`, `getResponsiveLayout()`

**Why removed:** Already handled inline in `getPageCSS()` via `@media (max-width: 768px)` breakpoint, and touch/swipe handling is embedded in `getPageJavaScript()`. Separate functions for mobile CSS would mean either serving duplicate CSS blocks or conditional server-side rendering -- both wasteful.

#### AJAX Endpoint Helpers (4)
`handleStatusRequest()`, `handleServoRequest()`, `handleLEDRequest()`, `handleAudioRequest()`

**Why removed:** These belong in `handlers.cpp`, not `webpage.cpp`. Request handling is an HTTP server concern, not a page-generation concern. The v1.2.5 architecture correctly separates: `webpage.cpp` generates HTML/CSS/JS, `handlers.cpp` processes HTTP endpoints.

#### Extra JSON Generators (2)
`getServoPositionsJSON()`, `getSystemInfoJSON()`

**Why removed:** `getCurrentStatusJSON()` already returns servo positions and system info in a single JSON payload polled every 2 seconds. Splitting into multiple endpoints means more HTTP round-trips, more WiFi overhead, and more handler registrations.

#### Misc (5)
`getPageFooter()`, `getIndexPageWithStatus()`, `formatColorCSS()`, `generateButtonHTML()`, `getConnectionStatusScript()`

**Why removed:** `getPageFooter()` generates ~20 bytes of closing HTML -- not worth a function call. `getIndexPageWithStatus()` duplicates the main page. The utility generators were either unused or trivially inlined.

---

### Removed But Worth Reconsidering (2-4 functions)

These features have genuine merit and could be implemented in a future version if flash budget and development time allow.

#### Servo Calibration Page
`getServoCalibrationPage()`

**Potential value: HIGH.** Calibrating servo min/max/center positions through a web interface with real-time sliders while watching the physical servos move is extremely practical for droid builders. Currently, calibration requires Serial CLI commands (`servo eye center`, `servo eye limits`), which is less intuitive.

**Implementation cost:** ~5-8 KB flash for HTML/CSS/JS + 2-3 handler functions. Would need EEPROM save integration to persist calibrated values.

**Recommendation:** Strong candidate for v1.3. Would significantly improve builder experience during initial setup and servo replacement.

#### WebSocket Support
`getWebSocketScript()`

**Potential value: MEDIUM-HIGH.** Would replace the 2-second polling interval with real-time push updates and enable true real-time servo control (dragging a virtual joystick with immediate physical response). Current fetch-based approach has inherent latency.

**Implementation cost:** ~8-10 KB flash for `AsyncWebSocket` library + JS code. ~2-4 KB RAM per connected client. Requires switching from `WebServer` to `AsyncWebServer` library (significant refactor).

**Recommendation:** Consider for v1.3 or v2.0 if real-time servo control is a priority. The `AsyncWebServer` migration would also improve overall web interface responsiveness.

#### Battery Status
`getBatteryStatus()`

**Potential value: CONDITIONAL.** Only useful if the droid has battery monitoring hardware (voltage divider on an ADC pin). The ESP32-S3-Zero does not have built-in battery monitoring.

**Implementation cost:** Minimal (~1 KB flash) if hardware exists.

**Recommendation:** Add only when battery monitoring circuitry is part of the hardware design. Not worth pre-implementing as dead code.

#### Diagnostics Page
`getDiagnosticsPage()`

**Potential value: LOW-MEDIUM.** Seeing free heap, PSRAM usage, WiFi RSSI, GPIO states, and I2C scan results in a web page is helpful for debugging. However, the current status section already shows mode, uptime, and free RAM, and deeper diagnostics are better served by the Serial CLI (`status`, `monitor`, `test` commands).

**Implementation cost:** ~3-5 KB flash.

**Recommendation:** Low priority. Serial diagnostics are more appropriate for the target audience (builders comfortable with Arduino IDE).

---

### Current webpage.h Functions (15 retained)

| # | Function | Purpose |
|---|----------|---------|
| 1 | `getIndexPage()` | Main page assembler |
| 2 | `getPageHeader()` | HTML `<head>` section |
| 3 | `getPageCSS()` | All CSS styles (~400 lines) |
| 4 | `getPageJavaScript()` | All JavaScript (~700 lines) |
| 5 | `getServoControlSection()` | Servo gamepad HTML |
| 6 | `getLEDControlSection()` | Eye LED control HTML |
| 7 | `getDetailLEDControlSection()` | Detail LED control HTML |
| 8 | `getAudioControlSection()` | Audio control HTML |
| 9 | `getModeControlSection()` | Mode selection HTML |
| 10 | `getStatusSection()` | Status display HTML |
| 11 | `getSequenceControlSection()` | Sequence management HTML |
| 12 | `getCurrentStatusJSON()` | Status as JSON for AJAX |
| 13 | `formatUptime()` | Format seconds to HH:MM:SS |
| 14 | `formatMemory()` | Format bytes to KB/MB |
| 15 | `escapeHTML()` | HTML entity escaping |

---

### Memory Budget Context

| Component | Source Size | Est. Flash |
|-----------|-----------|------------|
| `webpage.cpp` | ~53 KB | ~42-50 KB (mostly string literals) |
| `handlers.cpp` | ~164 KB | ~80-100 KB |
| `.ino` main | ~31 KB | ~15-20 KB |
| Other modules | ~60 KB | ~30-40 KB |
| **Total application** | **~310 KB** | **~170-210 KB** |

ESP32-S3-Zero: 8 MB flash, ~1.3 MB available for application (after bootloader, partition table, OTA).
Current usage: ~13-16% of available flash. Headroom exists for future features, but each web page added costs 5-10 KB.

---

## Post-Review Bugfixes (v1.2.5)

A comprehensive code review identified and fixed 26 issues. The full changelog is in [README.md](README.md) under "Version 1.2.5".

Key categories:
- **Security**: Buffer overflow prevention, SSID injection fix
- **Stability**: Null pointer guards, JSON/JS field mismatches, state synchronization
- **Performance**: `updatePlayback()` optimization (removed per-frame `listSequences` call)
- **Web UI**: Added Pause/Resume/Stop playback controls, per-sequence Loop button, sequence list truncation warning
- **Serial CLI**: Added delete confirmation for `seq delete` command
- **Build**: ArduinoJson v6/v7 version conflict resolved

---

## Code Style & Conventions

- **Arduino IDE** is the primary build environment (PlatformIO also supported)
- **ArduinoJson v6** API (`DynamicJsonDocument`, `createNestedArray`) -- NOT v7
- Raw literal strings (`R"rawliteral(...)rawliteral"`) for HTML/CSS/JS in `webpage.cpp`
- `F()` macro for Serial strings to save RAM
- Handler functions: `handle[Feature]()` for web, `handle[Feature]Command()` for Serial CLI
- EEPROM structs use `__attribute__((packed))` to prevent padding-related checksum issues
