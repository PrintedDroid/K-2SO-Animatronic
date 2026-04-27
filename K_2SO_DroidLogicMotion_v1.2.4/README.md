# K-2SO Animatronic Sketch v1.2.4 -- Stable

Current stable, production-recommended version of the K-2SO Animatronic Controller.
Patch on top of v1.2.3 with six stability fixes from a Claude + Codex cross-review.
**EEPROM-compatible with v1.2.3** -- no data loss when upgrading.

For sequence recording, playlist chaining, and IR-to-sequence mapping, use **v1.3.0**
(Promotion Candidate). v1.2.4 stays as the safe production fallback.

## What this version has

Everything from v1.2.3 (4-servo motion, dual NeoPixel eyes, detail strip, status LED,
DFPlayer audio, IR remote, WiFi WebUI, voice triggers, 5 EEPROM profiles, personality
modes) **plus six stability fixes**:

- Critical: removed the 2-second main-loop freeze on WiFi reconnect (status LED is now non-blocking)
- Critical: detail-LED random-loop has an iteration cap (no theoretical worst-case hang)
- High: `restoreFromSerial` verifies the configuration checksum before applying
- High: `profile save` aborts cleanly when all 5 slots are full instead of silently overwriting slot 0
- High: serial CLI confirmations no longer freeze the droid forever on USB disconnect (centralised 30 s timeout)
- Medium: servo-limit commands reject `min >= max` instead of silently accepting inverted limits

## Documentation

Full user manual: `generate docs/K-2SO_Animatronic_Controller_System_Documentation_v1.3.0.pdf`
(the manual covers v1.3.0 as primary focus; v1.2.4 differences are called out where they matter).

The full v1.2.4 README in markdown is preserved as `README.md.bak` next to this file.

## Source and Support

- Repository: https://github.com/PrintedDroid/K-2SO-Animatronic
- Printed Droid builders (controller / Droid Logic Motion board): https://www.facebook.com/groups/printeddroid/
- Droid Division builders (the K-2SO figure): https://www.facebook.com/groups/2505708886350784/
- Droid Division shop: https://www.droiddivision.com/

## Hardware

- Board: Waveshare ESP32-S3-Zero on Droid Logic Motion v1.3
- 4x SG90-class servos (Eye Pan/Tilt, Head Pan/Tilt)
- 2x NeoPixel eye boards (7 or 13 LEDs each, runtime-switchable)
- WS2812 detail strip (1-8 LEDs)
- DFPlayer Mini + HW-301 PAM8406 amplifier
- IR receiver (21-button learning)
- 5V / 3A minimum, 5V / 5A recommended

## Quick Start

1. Open `config.h`, set `WIFI_SSID` / `WIFI_PASSWORD`, change `WEB_AUTH_PASS` from the default `k2so2025`.
2. Arduino IDE: Board = Waveshare ESP32-S3-Zero (FQBN `esp32:esp32:waveshare_esp32_s3_zero`), USB CDC On Boot = Enabled. Upload.
3. Serial Monitor at 115200 should print the boot banner. Status LED: blue blink -> yellow blink -> green solid 2 s -> mode color.
4. Find the WebUI: `http://k2so.local`, the IP from the serial log, or the `K2SO-XXXXXX` AP fallback at `http://192.168.4.1`.
5. First servo test: type `servo test all` in the serial monitor. **Run with the head off any mount until you trust the limits.**

Full step-by-step in the System Documentation PDF, "Quick Start" chapter.

## Build at your own risk

Servos under load, hot soldering, amplified audio, mains-fed 5V supplies. Electrical safety, an honest power
budget, and sane mechanical assembly are the builder's responsibility. Disclaimer in the System Documentation PDF.

---

May the Force be with your build.
