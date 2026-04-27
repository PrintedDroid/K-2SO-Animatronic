# K-2SO Animatronic Sketch v1.3.0 -- Promotion Candidate

Promotes the v1.2.5 Beta features to a stable release: frame-based sequence recording,
playlist chaining, IR-button to sequence mapping. Adds automatic EEPROM migration so
upgrading from v1.2.3 / v1.2.4 keeps all existing settings (WiFi, IR mappings,
servo calibration, profiles).

**Status:** compile-clean (~94% flash, ~16% RAM), hardware verification pending.
For pure stability use **v1.2.4**. Once this version is hardware-verified it will be
released as the new stable.

## What's new in v1.3.0 (over v1.2.5 Beta)

### Critical fixes
- Playlist chaining now actually chains (v1.2.5 dropped the playlist after the first transition)
- Truncated sequence JSON cannot drive servos with uninitialised memory anymore
- Sequence names are sanitised against path traversal (`^[A-Za-z0-9_-]{1,31}$`)
- LittleFS no longer auto-formats on mount failure (use the new explicit `seq format` CLI command instead)
- JSON document allocation is OOM-checked across save / load / info
- WiFi reconnect no longer blocks the main loop for 2 seconds

### Automatic EEPROM v1 -> v2 migration
On first boot after upgrading from v1.2.3 / v1.2.4, the firmware reads the old layout
into `ConfigDataV1`, validates the legacy checksum, copies all fields into the new
v2 layout, and writes the result back. Serial log shows
`Config migrated from v1 (v1.2.3/v1.2.4) to v2 (v1.3.0). All settings preserved.`

### Follow-up additions
- **Crash-safe sequence save:** `seq save` writes to `<name>.tmp`, protects the existing target as `<name>.bak`, promotes only on success.
- **Dual-format serial restore:** `restore` accepts both v1 (legacy) and v2 hex backups directly.
- **New CLI commands:** `seq verify`, `seq export`, `seq import`, `seq playlist remove <n>`, `seq playlist move <from> <to>`.
- **New HTTP endpoints:** `/seq/verify`, `/seq/export`, `/seq/import`, `/seq/playlist/remove`, `/seq/playlist/move`.
- **JSON `{ok, message}` envelope** on sequence/IR endpoints (replaces the old mixed text/plain `OK` / `ERROR` responses).
- **WebUI** consumes the new envelope and adds a sequence-import button.

## Documentation

- **Full user manual** (recommended): `generate docs/K-2SO_Animatronic_Controller_System_Documentation_v1.3.0.pdf`
- **Developer reference** (pin map, EEPROM layout, HTTP API, library constraints):
  `generate docs/K-2SO_Animatronic_Controller_Developer_Reference_v1.3.0.pdf`
- Both PDFs available in EN and DE (`_DE.pdf` suffix)

The original v1.2.5-style markdown README, SEQUENCE_RECORDING_GUIDE, and DEVELOPMENT_NOTES
are preserved as `*.md.bak` next to their live counterparts.

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
3. Serial Monitor at 115200 should print the boot banner. If you are upgrading from v1.2.3 / v1.2.4, look for the migration message.
4. Find the WebUI: `http://k2so.local`, the IP from the serial log, or the `K2SO-XXXXXX` AP fallback at `http://192.168.4.1`.
5. First servo test: `servo test all`. **Run with the head off any mount until you trust the limits.**
6. Record your first sequence: `seq new "greeting"`, drive the gamepad, `seq frame 1500`, repeat, `seq save`. See the System Documentation PDF for the full workflow.

## Migration from v1.2.3 / v1.2.4

Recommended procedure:

1. On the running v1.2.4: `backup` in serial, save the hex output as a text file.
2. Flash v1.3.0 over USB.
3. Open Serial Monitor at 115200 -- the boot log should show the migration message.
4. Verify with `wifi show`, `show` (IR buttons), `servo show`, `profile list` -- everything should match.

Downgrading from v1.3.0 back to v1.2.4 is not directly supported; restore the hex backup
after re-flashing v1.2.4 if needed.

## Build at your own risk

Servos under load, hot soldering, amplified audio, mains-fed 5V supplies. Electrical safety,
an honest power budget, and sane mechanical assembly are the builder's responsibility.
Full disclaimer in the System Documentation PDF.

---

May the Force be with your build.
