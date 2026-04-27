# K-2SO Animatronic Sketch v1.2.5 -- Beta (superseded by v1.3.0)

The Beta release that introduced **frame-based sequence recording**, **playlist chaining**,
and **IR-to-sequence mapping** to the K-2SO Animatronic Controller.

**Superseded by v1.3.0** (Promotion Candidate). v1.3.0 carries the same features plus:
the playlist chaining bug fix, automatic v1 -> v2 EEPROM migration, LittleFS auto-format
removed, truncated-JSON safety, path-traversal sanitiser, and a long list of follow-up
improvements (crash-safe save, dual-format restore, new CLI commands, JSON envelope on
HTTP endpoints).

**Use v1.3.0 unless you have a specific reason to stay on v1.2.5.** This folder is
kept as the source from which v1.3.0 was branched.

## What this version added (over v1.2.3)

- Frame-based sequence recording (up to 200 frames per sequence)
- LittleFS sequence storage (~1.5-2 MB available)
- Playlist chaining (up to 10 sequences -- but transitions break after item 1 in this version, fixed in v1.3.0)
- IR-button to sequence mapping (21 buttons)
- Web UI for sequence management
- 20+ stability and security fixes (safe integer parsing, non-blocking restore, dynamic recording buffer, improved error messages, 7-LED boot sequence support, web handler input validation)

## Known issues (fixed in v1.3.0)

- Playlist stops after first transition (`stopPlayback` clears `playlist.active` unconditionally)
- LittleFS auto-formats on mount failure (silent data loss risk)
- Truncated sequence JSON can drive servos with uninitialised memory
- Sequence names are not sanitised (path traversal possible via web/CLI)
- DynamicJsonDocument allocation is not OOM-checked

## Documentation

Full user manual: `generate docs/K-2SO_Animatronic_Controller_System_Documentation_v1.3.0.pdf`
(the manual targets v1.3.0; v1.2.5-specific differences are flagged where relevant).

The full v1.2.5 README in markdown, the SEQUENCE_RECORDING_GUIDE, and DEVELOPMENT_NOTES
are preserved as `*.md.bak` next to their live counterparts.

## Source and Support

- Repository: https://github.com/PrintedDroid/K-2SO-Animatronic
- Printed Droid builders (controller / Droid Logic Motion board): https://www.facebook.com/groups/printeddroid/
- Droid Division builders (the K-2SO figure): https://www.facebook.com/groups/2505708886350784/
- Droid Division shop: https://www.droiddivision.com/

## Hardware

Same as v1.2.3 / v1.2.4 / v1.3.0 -- Waveshare ESP32-S3-Zero on Droid Logic Motion v1.3.

---

May the Force be with your build.
