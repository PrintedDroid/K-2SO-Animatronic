# K-2SO Animatronic Sketch v1.2.3 -- Previous Stable (kept as reference)

The previous stable production version. Superseded by **v1.2.4** (same EEPROM layout,
six stability fixes on top). Kept here as the canonical reference for the v1 EEPROM
layout that v1.3.0's auto-migration reads from.

**For new builds use v1.2.4 (Stable) or v1.3.0 (Promotion Candidate).**

## What this version has

- 4-servo motion control (Eye Pan/Tilt, Head Pan/Tilt) with calibratable centers and limits
- Dual NeoPixel eyes (7-LED or 13-LED, 12 animation modes)
- WS2812 detail LED strip on GP10 (replaces the old GP13 single LED)
- Single WS2812 status LED on GP21
- DFPlayer Mini audio + PAM8406 amplifier (40+ K-2SO sound clips)
- IR learning remote (21 buttons, NEC/Sony codecs)
- WiFi STA + AP fallback (`K2SO-XXXXXX`), HTTP Basic Auth, mDNS `k2so.local`
- Voice assistant trigger endpoints (Google Home, Alexa, Siri)
- 5 EEPROM profiles
- Personality modes (Scanning / Alert / Idle)

## What it does NOT have (added in v1.2.4 / v1.2.5 / v1.3.0)

- v1.2.4: 6 stability fixes (status-LED non-blocking, restore checksum, profile-slot guard, servo-limit min<max, blocking-CLI timeout, detail-LED loop guard)
- v1.2.5 / v1.3.0: sequence recording, playlist, IR-to-sequence mapping, EEPROM v2 + auto-migration

## Documentation

Full user manual: `generate docs/K-2SO_Animatronic_Controller_System_Documentation_v1.3.0.pdf`

The original v1.2.3 README is preserved as `README.md.bak` next to this file.

## Source and Support

- Repository: https://github.com/PrintedDroid/K-2SO-Animatronic
- Printed Droid builders (controller / Droid Logic Motion board): https://www.facebook.com/groups/printeddroid/
- Droid Division builders (the K-2SO figure): https://www.facebook.com/groups/2505708886350784/
- Droid Division shop: https://www.droiddivision.com/

## Hardware

- Board: Waveshare ESP32-S3-Zero on Droid Logic Motion v1.3
- 4x SG90-class servos
- 2x NeoPixel eye boards (7 or 13 LEDs)
- WS2812 detail strip (1-8 LEDs)
- DFPlayer Mini + HW-301 PAM8406
- IR receiver
- 5V / 3A minimum

---

May the Force be with your build.
