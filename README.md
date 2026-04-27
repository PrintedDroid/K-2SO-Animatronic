# K-2SO Animatronic Controller README

**Advanced ESP32-S3 based animatronics controller for Star Wars K-2SO droid builds**

Developed by [Printed-Droid.com](https://www.printed-droid.com).
Designed for the K-2SO animatronic from
**[Droid Division](https://www.droiddivision.com/)** (sold there as the *Security Droid*),
running on the **Droid Logic Motion v1.3** carrier board.

---

## Repository Structure

```
K-2SO-Animatronic/
├── K_2SO_DroidLogicMotion_v1.1.0/   ← Original First Sketch (Archive)
├── K_2SO_DroidLogicMotion_v1.2.3/   ← Previous Stable (kept as v1 EEPROM reference)
├── K_2SO_DroidLogicMotion_v1.2.4/   ← Current Stable Production Version
├── K_2SO_DroidLogicMotion_v1.2.5/   ← Beta (superseded by v1.3.0)
├── K_2SO_DroidLogicMotion_v1.3.0/   ← Promotion Candidate (verification pending)
├── K-2SO_Animatronic_Controller_System_Documentation_v1.3.0.pdf
├── K-2SO_Animatronic_Controller_System_Documentation_v1.3.0_DE.pdf
└── README.md                         ← This file
```

---

## Version Overview

### v1.1.0 - Original First Sketch (Archive)

**Status:** Archived - one of the first sketches for the K-2SO animatronic system.

**Note:** Preserved as a historical reference showing the original foundation of the project.
For active use, please refer to v1.2.4 or newer.

---

### v1.2.3 - Previous Stable (Reference)

**Recommended for:** legacy reference only - kept as the canonical v1 EEPROM layout that
the v1.3.0 auto-migration reads from. Use **v1.2.4** for production.

**Status:** Stable, replaced by v1.2.4 (which adds six layout-neutral stability fixes).

**Features:**
- 4-Servo Motion Control (Eye + Head pan/tilt)
- Dual NeoPixel Eyes (7-LED or 13-LED configurations)
- 12 Eye Animation Modes
- WS2812 Detail LED Strip (1-8 LEDs)
- WiFi Web Interface with authentication
- IR Remote Learning (21 buttons)
- DFPlayer Audio System
- Voice Assistant Integration (Google Home, Alexa, Siri)
- EEPROM Configuration with profiles
- Status LED System

**Key Fixes in v1.2.3:**
- WiFi settings persistence fixed
- Boot sequence stability improvements
- PWM channel conflict resolution
- RMT/NeoPixel compatibility fixes

---

### v1.2.4 - Current Stable Production Version ✅

**Recommended for:** production use, exhibitions, daily operation.

**Status:** Stability patch on top of v1.2.3, EEPROM-compatible, no data loss when upgrading.

**Bug Fixes in v1.2.4:**
- Critical: 2-second main-loop freeze on WiFi reconnect removed (status LED is now non-blocking)
- Critical: detail-LED random-loop has an iteration cap
- High: `restoreFromSerial` verifies the configuration checksum before applying
- High: `profile save` aborts cleanly when all 5 slots are full instead of silently overwriting slot 0
- High: serial CLI confirmations no longer freeze the droid forever on USB disconnect (centralised 30 s timeout)
- Medium: servo-limit commands reject `min >= max`

---

### v1.2.5 - Beta (Superseded by v1.3.0) ⚠️

**Status:** Beta source for the v1.3.0 promotion. Use **v1.3.0** instead - it carries all
the same features plus the playlist fix, EEPROM auto-migration, LittleFS hardening, and
follow-up improvements.

**New Features (v1.2.4+):**
- Performance Recording System
- Frame-based sequence recording (up to 200 frames)
- LittleFS storage (~1.5-2 MB for sequences)
- Playlist chaining (up to 10 sequences)
- IR button to sequence mapping
- Web interface for sequence management

**Bug Fixes in v1.2.5:**
- 20+ stability and security fixes
- Safe integer parsing for all inputs
- Non-blocking configuration restore
- Dynamic recording buffer allocation
- Improved error messages
- 7-LED boot sequence support
- Web handler input validation

> **Known issues fixed in v1.3.0:** playlist stops after first transition; LittleFS auto-formats
> on mount failure; truncated JSON can drive servos with uninitialised memory; sequence names
> are not sanitised against path traversal.

---

### v1.3.0 - Promotion Candidate ⭐ (Hardware verification pending)

**Recommended for:** sequence recording, playlist chaining, IR-to-sequence mapping.
Use after your own hardware verification. Compile-clean (~60% flash with No-OTA partition, ~16% RAM).

**Status:** new stable candidate. Promotes the v1.2.5 Beta feature set to a stable release
plus an EEPROM auto-migration so upgrading from v1.2.3 / v1.2.4 keeps WiFi credentials, IR
mappings, servo calibration, and profiles.

**Core features promoted from v1.2.5 Beta:**
- Sequence Recording - frame-based, up to 200 frames per sequence, JSON in LittleFS
- Playlist Chaining - up to 10 sequences, optional loop, atomic transitions (no zombie state)
- IR-button to Sequence Mapping - assign any of the 21 learned IR buttons to a saved sequence

**v1.3.0-only additions:**
- **Automatic v1 -> v2 EEPROM migration** - upgrading from v1.2.3 / v1.2.4 keeps all settings
- **Crash-safe sequence save** - temp / backup / replace flow
- **Dual-format serial restore** - accepts both v1 (legacy) and v2 hex backups directly
- **Saved playlists** - `seq playlist save/load/list` (runtime-only in v1.2.5)
- **Sequence utilities** - `seq verify`, `seq verify all`, `seq export`, `seq import`, `seq duplicate`, `seq stats`
- **Playlist editing** - `seq playlist remove`, `seq playlist move`
- **Cleaner HTTP API** - consistent `{ok, message}` JSON envelope on sequence and IR endpoints
- **WebUI follow-up** - buttons for *Verify All*, *Stats*, per-sequence Copy / Export / Verify, playlist Save / Load / Move / Remove, IR-mapping re-assign

All v1.2.4 stability fixes carried over.

---

## Quick Start

### 1. Choose Your Version

| Use Case | Recommended Version |
|----------|---------------------|
| Exhibition / Daily Use | **v1.2.4** (Stable) ✅ |
| Sequence Recording, Playlist, IR-Mapping | **v1.3.0** (after own hardware test) ⭐ |
| Previous stable / EEPROM v1 reference | v1.2.3 |
| Beta source (superseded) | v1.2.5 ⚠️ |
| Historical reference | v1.1.0 (Archive) |

### 2. Hardware Requirements

- **Board:** Waveshare ESP32-S3-Zero on **Droid Logic Motion v1.3** carrier
- **Servos:** 4x SG90-class servos (Eye Pan/Tilt + Head Pan/Tilt)
- **Eyes:** 2x NeoPixel boards (7 or 13 LEDs each, runtime-switchable)
- **Detail LEDs:** WS2812 strip (1-8 LEDs)
- **Audio:** DFPlayer Mini + HW-301 PAM8406 amplifier
- **IR:** 21-button learning remote
- **Power:** 5V / 3A minimum, 5V / 5A recommended

### 3. Installation

**PlatformIO (recommended):**
```bash
cd K_2SO_DroidLogicMotion_v1.2.4    # current stable
# or
cd K_2SO_DroidLogicMotion_v1.3.0    # promotion candidate (sequence recording)
pio run -t upload
pio device monitor
```

**Arduino IDE:**
1. Open the `.ino` file from your chosen version folder
2. Select Board: *Waveshare ESP32-S3-Zero* (FQBN `esp32:esp32:waveshare_esp32_s3_zero`)
3. Enable *USB CDC On Boot*
4. Upload

### 4. WiFi Configuration

Edit `config.h` before flashing, or set credentials at runtime via the Serial Monitor (115200 baud):
```
wifi set "YourSSID" "YourPassword"
wifi show
```

If no credentials are set, the controller starts an AP fallback `K2SO-XXXXXX` (password
`k2so2025`) so you can configure WiFi from the browser.

### 5. Web Interface

After WiFi connection:
- Access: `http://[IP-ADDRESS]` or `http://k2so.local`
- Login: `admin` / `k2so2025` (**change in `config.h`!**)

---

## Documentation

The full user manual lives in the System Documentation PDF (also at the root of this
repository), available in English and German:

- [K-2SO_Animatronic_Controller_System_Documentation_v1.3.0.pdf](K-2SO_Animatronic_Controller_System_Documentation_v1.3.0.pdf) (EN)
- [K-2SO_Animatronic_Controller_System_Documentation_v1.3.0_DE.pdf](K-2SO_Animatronic_Controller_System_Documentation_v1.3.0_DE.pdf) (DE)

22 chapters across four parts: Getting Started, Hardware (with full pin map),
Core Features (servos, LEDs, audio, IR, personality, sequences, playlist, IR-mapping),
Reference & Appendices (CLI reference, voice assistants, configuration & backup,
migration v1.2.x to v1.3.0, troubleshooting, version history).

Each version folder additionally contains a slim per-version README with what is
specific to that release.

---

## Key Serial Commands

```
help                    Show all commands
status                  System status
wifi show               WiFi configuration
mode scanning           Change personality mode
led mode pulse          Change eye animation
servo test all          Test all servos
seq new "Test"          Start recording (v1.2.5 / v1.3.0)
seq stats               LittleFS / sequences / playlists overview (v1.3.0)
seq playlist save "X"   Save current playlist (v1.3.0)
```

---

## Upgrading from v1.2.3 / v1.2.4 to v1.3.0

1. On the running v1.2.4: `backup` in the serial monitor, save the hex output to a text file.
2. Flash v1.3.0 over USB.
3. Open Serial Monitor at 115200. Look for:
   `Config migrated from v1 (v1.2.3/v1.2.4) to v2 (v1.3.0). All settings preserved.`
4. Verify: `wifi show`, `show` (IR), `servo show`, `profile list`. Everything should match.

Downgrading back to v1.2.4 is supported via the hex backup. See the *Migration v1.2.x to v1.3.0*
chapter in the System Documentation PDF.

---

## Community and Support

- 💬 **Printed Droid builders** (controller / Droid Logic Motion board): https://www.facebook.com/groups/printeddroid/
- 💬 **Droid Division builders** (the K-2SO figure): https://www.facebook.com/groups/2505708886350784/
- 🛒 **Droid Division shop** (where the figure comes from): https://www.droiddivision.com/
- 🐛 **Issues** here on GitHub: https://github.com/PrintedDroid/K-2SO-Animatronic/issues
- 🌐 **Website:** https://www.printed-droid.com

---

## License & Disclaimer

⚠️ **BUILD AT YOUR OWN RISK**

This project involves servos under load, hot soldering, amplified audio, and mains-fed
5V supplies. Users are responsible for proper electrical safety, an adequate power supply,
and safe mechanical assembly. Full disclaimer on page 2 of the System Documentation PDF.

---

**May the Force be with your build!** 🌟
