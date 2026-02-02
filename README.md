# K-2SO Animatronic Controller

**Advanced ESP32-S3 based animatronics controller for Star Wars K-2SO droid builds**

Developed by [Printed-Droid.com](https://www.printed-droid.com)

---

## Repository Structure

This repository contains multiple firmware versions for the K-2SO Droid Logic Motion Board:

```
K-2SO-Animatronic/
├── K_2SO_DroidLogicMotion_v1.1.0/   ← Original First Sketch
├── K_2SO_DroidLogicMotion_v1.2.3/   ← Stable Production Version
├── K_2SO_DroidLogicMotion_v1.2.5/   ← Beta Version (Testing)
├── K-2SO Audio Files/               ← Audio Files for DFPlayer
└── README.md                         ← This file
```

---

## Version Overview

### v1.1.0 - Original First Sketch (Archive)

**Status:** Archived - One of the first sketches for the K-2SO animatronic system

**Note:** This version is preserved as a historical reference showing the original foundation of the project. For active use, please refer to v1.2.3 or newer.

---

### v1.2.3 - Stable Production Version ✅

**Recommended for:** Production use, exhibitions, daily operation

**Status:** Fully tested and stable

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

### v1.2.5 - Beta Version (Testing) ⚠️

**Recommended for:** Testing, development, advanced users

**Status:** Beta - comprehensive bug fixes applied, testing in progress

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

> **Note:** For stable production use, v1.2.3 is recommended until v1.2.5 testing is complete.

---

## Quick Start

### 1. Choose Your Version

| Use Case | Recommended Version |
|----------|---------------------|
| Exhibition / Daily Use | v1.2.3 (Stable) |
| Testing New Features | v1.2.5 (Beta) |
| Sequence Recording | v1.2.5 (Beta) |
| Historical Reference | v1.1.0 (Archive) |

### 2. Hardware Requirements

- **Board:** Waveshare ESP32-S3-Zero (or compatible ESP32-S3)
- **Servos:** 4x Standard servos (SG90 or similar)
- **Eyes:** 2x NeoPixel strips (7 or 13 LEDs each)
- **Audio:** DFPlayer Mini + PAM8406 amplifier
- **Power:** 5V/3A minimum

### 3. Installation

**PlatformIO (Recommended):**
```bash
cd K_2SO_DroidLogicMotion_v1.2.3   # or v1.2.5
pio run -t upload
pio device monitor
```

**Arduino IDE:**
1. Open the `.ino` file from your chosen version folder
2. Select Board: "ESP32S3 Dev Module"
3. Enable "USB CDC On Boot"
4. Upload

### 4. WiFi Configuration

Connect via Serial Monitor (115200 baud):
```
wifi set "YourSSID" "YourPassword"
wifi show
```

### 5. Web Interface

After WiFi connection:
- Access: `http://[IP-ADDRESS]` or `http://k2so.local`
- Login: `admin` / `k2so2025` (change in config.h!)

---

## Documentation

Each version folder contains detailed documentation:

- `README.md` - Complete feature documentation and changelog
- `SEQUENCE_RECORDING_GUIDE.md` - Recording system tutorial (v1.2.5)
- `config.h` - Hardware configuration and pin assignments

---

## Key Serial Commands

```
help          # Show all commands
status        # System status
wifi show     # WiFi configuration
mode scanning # Change personality mode
led mode pulse # Change eye animation
servo test all # Test all servos
seq new "Test" # Start recording (v1.2.5)
```

---

## Support

- **Website:** [www.printed-droid.com](https://www.printed-droid.com)
- **Hardware:** Droid Logic Motion Board v1.3

---

## License & Disclaimer

⚠️ **BUILD AT YOUR OWN RISK**

This project involves electrical components and servo motors. Users are responsible for proper electrical safety, adequate power supply, and safe mechanical assembly.

---

**May the Force be with your build!** 🌟
