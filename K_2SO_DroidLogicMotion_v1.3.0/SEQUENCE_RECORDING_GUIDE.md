# 🎬 K-2SO Performance Recording - Complete Guide

## 📖 Table of Contents

1. [Introduction](#introduction)
2. [Core Concept](#core-concept)
3. [Getting Started](#getting-started)
4. [Creating Recordings](#creating-recordings)
5. [Playing Sequences](#playing-sequences)
6. [Chaining Sequences (Playlists)](#chaining-sequences-playlists)
7. [IR Remote Mapping](#ir-remote-mapping)
8. [Tips & Best Practices](#tips--best-practices)
9. [Troubleshooting](#troubleshooting)
10. [Technical Details](#technical-details)
11. [Web API Reference](#web-api-reference-v125)

---

## 🎯 Introduction

The Performance Recording System allows you to record complex movement sequences, LED animations, and sounds as reusable sequences for playback.

### What Gets Recorded?

Per Frame:
- ✅ **4x Servo Positions** (Eye Pan/Tilt, Head Pan/Tilt)
- ✅ **Eye Animation** (Mode, Color, Brightness)
- ✅ **Detail LEDs** (Pattern, Color, Brightness)
- ✅ **Sound Trigger** (File, Folder, Volume)
- ✅ **Frame Duration** (how long the position is held)

### Storage Capacity

- **Storage Location:** LittleFS on ESP32-S3-Zero (4MB Flash)
- **Available:** ~1.5-2 MB for sequences
- **Sequences:** 20-50+ (depending on frame count)
- **Frames per Sequence:** up to 200
- **Sequence Name Length:** max 32 characters
- **Per Frame:** ~24 bytes (RAM) / ~80 bytes (JSON on flash)

---

## 💡 Core Concept

### Frame-Based Recording

**IMPORTANT:** The system does NOT record in real-time! You create individual "frames" (moments) that are later played back seamlessly without pauses.

**Workflow:**
```
1. Set position → seq frame 1500 → Frame saved (1.5s duration)
2. Change position → seq frame 2000 → Frame saved (2.0s duration)
3. Change position → seq frame 500  → Frame saved (0.5s duration)
```

**Playback:**
```
Frame 1 (1.5s) → Frame 2 (2.0s) → Frame 3 (0.5s) → Done!
```

→ No pauses between frames!
→ Movements are smoothly interpolated!

---

## 🚀 Getting Started

### System Check

```bash
# Check if LittleFS is running
seq status

# Expected output:
# ┌─ Sequence System Status ───────
# │ Storage: LittleFS OK
# │ State: Idle
# └────────────────────────────────
```

### All Available Commands

```bash
seq

# Shows complete command list with:
# - RECORDING
# - PLAYBACK
# - MANAGEMENT
# - PLAYLIST (CHAINING)
# - IR MAPPING
# - STATUS
```

---

## 🎬 Creating Recordings

### Example: "Greeting" Sequence

**Goal:** K-2SO looks up, blinks blue, looks right and plays sound.

#### Step 1: Start Recording

```bash
seq new "Greeting"

# Output:
# 🎬 Recording started: Greeting
# ✓ Recording started. Use 'seq frame <duration>' to add frames.
```

#### Step 2: Frame 1 - Center Head

**Via Web Interface:**
1. Open `http://k2so.local` (or IP address)
2. Use gamepad controller to move servos:
   - Eye Pan: 90° (center)
   - Eye Tilt: 90° (center)
   - Head Pan: 90° (center)
   - Head Tilt: 90° (center)

**Serial Command:**
```bash
seq frame 1000

# Output:
# ✓ Frame 1 added (1000ms)
#   Servos: EP=90 ET=90 HP=90 HT=90
```

#### Step 3: Frame 2 - Head Up

**Via Web Interface:**
- Head Tilt: 120° (upward)

**Serial Command:**
```bash
seq frame 1500

# Output:
# ✓ Frame 2 added (1500ms)
#   Servos: EP=90 ET=90 HP=90 HT=120
```

#### Step 4: Frame 3 - Set Eyes to Blue

**Via Web Interface:**
1. Color Picker: Ice Blue (#007FFF)
2. Animation: Select "Pulse"

**Serial Command:**
```bash
seq frame 2000

# Now LED color & animation are also saved!
```

#### Step 5: Frame 4 - Head Right + Sound

**Via Web Interface:**
1. Gamepad: Head Pan to 45° (left)
2. Sound: Play file 4 from folder 01

**Serial Command:**
```bash
seq frame 500

# Sound trigger is saved in the frame!
```

#### Step 6: Save

```bash
seq save

# Output:
# 💾 Sequence saved: Greeting (4 frames, 15.2s recording time)
```

---

## ▶️ Playing Sequences

### Play Single Sequence

```bash
# Play once
seq play "Greeting"

# Output:
# ▶️ Playing: Greeting

# Play in loop
seq loop "Greeting"
```

### Pause / Resume Playback

> **Note:** Pause and Resume are only available via the **Web Interface** (buttons in the Playback section). There are no serial commands for pause/resume.

When paused, the playback timer freezes at the current frame position and resumes exactly where it left off.

### Stop Playback

```bash
seq stop

# Output:
# ⏹️ Playback stopped
```

### List All Sequences

```bash
seq list

# Output:
# ┌─ Saved Sequences ──────────
# │ 1. Greeting
# │ 2. LookAround
# │ 3. Alert
# └────────────────────────────
# Total: 3 sequence(s)
```

### Show Sequence Details

```bash
seq info "Greeting"

# Output:
# ┌─ Sequence Info ────────────
# │ Name: Greeting
# │ Frames: 4
# │ Duration: 5.00 seconds
# │ Version: 1
# └────────────────────────────
```

---

## 🔗 Chaining Sequences (Playlists)

With playlists you can play multiple sequences one after another!

### Create Playlist

```bash
# Add sequences to playlist
seq playlist add "Greeting"
seq playlist add "LookAround"
seq playlist add "Idle"

# Output:
# ✓ Added to playlist: Greeting (1 items)
# ✓ Added to playlist: LookAround (2 items)
# ✓ Added to playlist: Idle (3 items)
```

### Show Playlist

```bash
seq playlist

# Output:
# ┌─ Playlist ─────────────────
# │ 1. Greeting
# │ 2. LookAround
# │ 3. Idle
# └────────────────────────────
# Total: 3 sequence(s)
```

### Play Playlist

```bash
# Play once through
seq playlist play

# Output:
# ▶️ Starting playlist (3 sequences)
# ▶️ Playing: Greeting
# ⏭️ Next in playlist: LookAround
# ⏭️ Next in playlist: Idle
# ✓ Playlist complete

# Play in loop
seq playlist loop
```

### Clear Playlist

```bash
seq playlist clear

# Output:
# ✓ Playlist cleared
```

---

## 🎮 IR Remote Mapping

You can map IR remote buttons directly to sequences!

### Program IR Remote

If not already done:

```bash
# Start IR remote learning
learn

# Follow instructions to program buttons
```

### Map Sequence to IR Button

```bash
# Map button "7" to sequence "Greeting"
seq map 7 "Greeting"

# Output:
# ✓ Mapped button '7' → sequence 'Greeting'
```

### Show All Mappings

```bash
seq map

# Output:
# ┌─ IR Button Mappings ───────
# │ 7 → Greeting
# │ 8 → Alert
# │ 9 → LookAround
# └────────────────────────────
```

### Clear Mapping

```bash
seq map 7 clear

# Output:
# ✓ Cleared mapping for button: 7
```

### Play Sequence via IR Remote

→ Simply press the mapped button on the remote!

```
# IR code detected → Sequence plays
▶️ IR triggering sequence: Greeting
```

---

## 💡 Tips & Best Practices

### 1. **Short, Reusable Sequences**

❌ **Bad:** One 100-frame sequence with everything
```
"CompleteShow" (100 frames)
```

✅ **Good:** Multiple short sequences combined
```
"Greeting" (5 frames)
"LookAround" (10 frames)
"Wave" (8 frames)
"Idle" (5 frames)

→ Combine via playlist!
```

### 2. **Meaningful Names**

❌ **Bad:**
```
seq1, seq2, test, aaa
```

✅ **Good:**
```
Greeting, AlertScan, CuriousLook, WaveHello
```

### 3. **Optimize Frame Durations**

- **Fast movements:** 300-800 ms
- **Normal movements:** 1000-2000 ms
- **Slow movements:** 2000-4000 ms
- **Pauses:** 500-1000 ms

### 4. **Use Sounds Sparingly**

→ Only trigger sounds at important moments
→ Sounds can be reused in multiple sequences

### 5. **Test Sequences**

```bash
# Before saving: Check frame count
seq status

# After saving: Check sequence info
seq info "MySequence"

# Play and watch
seq play "MySequence"
```

### 6. **Create Backups**

Sequences are stored on ESP32 Flash. They will be lost on flash reset!

**Backup Method:**
1. Sequences are JSON files in `/sequences/*.seq`
2. Can be backed up via Serial/Web upload (future feature)

---

## 🔧 Troubleshooting

### "⚠️ LittleFS mount failed"

**Problem:** Filesystem not initialized

**Solution:**
```bash
# Restart
reset

# Format LittleFS (WARNING: Deletes all sequences!)
# → Via Arduino IDE: Tools → Erase Flash → "All Flash Contents"
```

### "❌ Sequence not found"

**Problem:** Name misspelled or sequence doesn't exist

**Solution:**
```bash
# List all sequences
seq list

# Use correct spelling (case-sensitive!)
seq play "Greeting"  # ✅
seq play "greeting"  # ❌ (lowercase)
```

### "❌ Not recording"

**Problem:** Trying to add frame without starting recording

**Solution:**
```bash
# Start recording first!
seq new "MyName"

# Then add frames
seq frame 1000
```

### "❌ Maximum frames reached"

**Problem:** More than 200 frames in one sequence

**Solution:**
→ Split sequence into multiple smaller sequences
→ Combine via playlist

### Sequence Plays Differently Than Expected

**Problem:** Frame durations not optimally set

**Solution:**
```bash
# Check sequence info
seq info "MySequence"

# Re-record with adjusted frame durations
seq delete "MySequence"
# → Type 'yes' to confirm deletion
seq new "MySequence"
# ... frames with better durations ...
```

---

## 🔬 Technical Details

### File Format

Sequences are stored as JSON on LittleFS:

**File Path:** `/sequences/Greeting.seq`

**Format:**
```json
{
  "name": "Greeting",
  "version": 1,
  "frameCount": 4,
  "totalDuration": 5000,
  "frames": [
    {
      "d": 1000,
      "s": {"ep": 90, "et": 90, "hp": 90, "ht": 90},
      "em": 5,
      "ec": 32767,
      "eb": 150,
      "dm": 3,
      "dc": 32767,
      "db": 150,
      "sf": 0,
      "so": 0,
      "v": 20
    }
  ]
}
```

### Frame Structure

| Field | Type | Description | Values |
|-------|------|-------------|--------|
| `d` | uint16 | Duration (ms) | 1-60000 |
| `s.ep` | uint8 | Eye Pan | 0-180 |
| `s.et` | uint8 | Eye Tilt | 0-180 |
| `s.hp` | uint8 | Head Pan | 0-180 |
| `s.ht` | uint8 | Head Tilt | 0-180 |
| `em` | uint8 | Eye Mode | 0-13 |
| `ec` | uint32 | Eye Color (RGB) | 0x000000-0xFFFFFF |
| `eb` | uint8 | Eye Brightness | 0-255 |
| `dm` | uint8 | Detail Mode | 0-4 |
| `dc` | uint32 | Detail Color | 0x000000-0xFFFFFF |
| `db` | uint8 | Detail Brightness | 0-255 |
| `sf` | uint8 | Sound File | 0-255 |
| `so` | uint8 | Sound Folder | 0-99 |
| `v` | uint8 | Volume | 0-30 |

### Memory Management

**Flash Distribution (ESP32-S3-Zero 4MB):**
```
┌─────────────────────────────────────┐
│ Bootloader        (~32 KB)          │
│ Partitions        (~4 KB)           │
│ App/Firmware      (~1.2-1.5 MB)     │
│ LittleFS          (~1.5-2 MB)  ← Sequences
│ Reserved          (~Rest)           │
└─────────────────────────────────────┘
```

**LittleFS Capacity:**
- Small sequence (10 frames): ~1 KB → 1500+ possible
- Medium sequence (50 frames): ~4 KB → 375+ possible
- Large sequence (200 frames): ~16 KB → 90+ possible

> Note: JSON overhead (keys, brackets, whitespace) adds ~80 bytes per frame on flash.
> In RAM during playback, each frame occupies ~24 bytes (struct with alignment padding).

### Playback Engine

**Interpolation:**
- Servos move smoothly from position A to B
- Movement speed determined by frame duration
- Uses non-blocking movements in main loop

**Timing:**
- Frame timer starts when frame is activated
- After duration expires: Switch to next frame
- Playlist auto-advance: Automatic transition to next sequence

**Priority:**
- Sequence playback has priority over autonomous movements
- IR trigger can stop active playback and start new sequence
- During recording: Current state is frozen for frame capture

---

## 📚 Advanced Commands

### Sequence Management

```bash
# Rename
seq rename "OldName" "NewName"

# Delete (requires confirmation)
seq delete "MySequence"
# Output:
# ⚠️ Delete sequence "MySequence"? Type 'yes' to confirm:
# → Type 'yes' + Enter within 30 seconds, or deletion is cancelled

# Show status (currently running recording/playback)
seq status
```

### Debug & Monitoring

```bash
# Global status
status

# System monitor (live updates)
monitor

# Show config
config
```

---

## 🎓 Example Workflow: Complete Performance

### Goal

A 30-second performance with:
1. Greeting
2. Look around
3. Alert reaction
4. Back to idle

### Step-by-Step

```bash
# ========== 1. Greeting (5s) ==========
seq new "Greeting"

# Frame 1: Start position
# (Web: Center servos)
seq frame 1000

# Frame 2: Raise head + blue eyes
# (Web: Head Tilt 110°, Color Picker #007FFF, Pulse)
seq frame 1500

# Frame 3: Greeting sound
# (Web: Sound File 1 Folder 4 trigger)
seq frame 500

# Frame 4: Back to center
# (Web: Center servos)
seq frame 2000

seq save


# ========== 2. LookAround (10s) ==========
seq new "LookAround"

# Frame 1-5: Look left
seq frame 1000  # Left start
seq frame 2000  # Hold left
seq frame 1000  # Back center
seq frame 2000  # Right start
seq frame 2000  # Hold right
seq frame 1000  # Back center
seq frame 2000  # Look down
seq frame 1000  # Back center

seq save


# ========== 3. Alert (8s) ==========
seq new "Alert"

# Frame 1: Red eyes + scanner
# (Web: Color #FF0000, Animation Scanner)
seq frame 500

# Frame 2: Alert sound
# (Web: Sound File 1 Folder 2)
seq frame 1000

# Frame 3: Quick head movement left
seq frame 400

# Frame 4: Quick head movement right
seq frame 400

# Frame 5: Center
seq frame 1500

# Frame 6: Back to blue
# (Web: Color #007FFF)
seq frame 2000

seq save


# ========== 4. Idle (7s) ==========
seq new "Idle"

# Frame 1: Amber eyes + pulse
# (Web: Color #FFAA00, Pulse)
seq frame 2000

# Frame 2: Slow breathing movement
seq frame 3000

# Frame 3: Minimal movement
seq frame 2000

seq save


# ========== 5. Create Playlist ==========
seq playlist clear
seq playlist add "Greeting"
seq playlist add "LookAround"
seq playlist add "Alert"
seq playlist add "Idle"

# Show
seq playlist


# ========== 6. Play! ==========
seq playlist play

# ✓ Total duration: ~30 seconds
# ✓ Automatic transition between sequences
# ✓ No pauses between sections


# ========== 7. IR Remote Mapping (Optional) ==========
# Button 7 for complete playlist
seq map 7 "Greeting"  # Only starts Greeting
# For complete playlist: Use playlist play via separate button


# ========== 8. Looping (Optional) ==========
seq playlist loop
# → Performance runs endlessly!
```

---

## 🌐 Web API Reference (v1.2.5)

The Web Interface provides full sequence control. All endpoints require HTTP Basic Authentication (same as the main web UI).

### Sequence Endpoints

| Endpoint | Method | Description | Parameters |
|----------|--------|-------------|------------|
| `/seq/list` | GET | List all sequences | -- |
| `/seq/play` | GET | Play a sequence once | `name` |
| `/seq/loop` | GET | Play a sequence in loop mode | `name` |
| `/seq/stop` | GET | Stop active playback | -- |
| `/seq/pause` | GET | Pause active playback | -- |
| `/seq/resume` | GET | Resume paused playback | -- |
| `/seq/delete` | GET | Delete a sequence | `name` |

### Playlist Endpoints

| Endpoint | Method | Description | Parameters |
|----------|--------|-------------|------------|
| `/seq/playlist/add` | GET | Add sequence to playlist | `name` |
| `/seq/playlist/clear` | GET | Clear playlist | -- |
| `/seq/playlist/play` | GET | Play playlist once | -- |
| `/seq/playlist/loop` | GET | Play playlist in loop | -- |
| `/seq/playlist/get` | GET | Get current playlist (JSON) | -- |

### IR Mapping Endpoints

| Endpoint | Method | Description | Parameters |
|----------|--------|-------------|------------|
| `/seq/map/list` | GET | Get all IR mappings (JSON) | -- |
| `/seq/map/set` | GET | Map IR button to sequence | `button`, `name` |
| `/seq/map/clear` | GET | Clear IR button mapping | `button` |

### Response Format

**`/seq/list` response:**
```json
{
  "sequences": [
    {"name": "Greeting"},
    {"name": "LookAround"}
  ],
  "count": 2,
  "maxDisplayed": 20
}
```

> When `count >= maxDisplayed`, the web UI shows a truncation warning. The list shows the first 20 sequences.

### Web UI Features (v1.2.5)

The web interface includes the following sequence controls:

- **Playback Controls**: Pause, Resume, and Stop buttons
- **Per-Sequence Actions**: Play (▶), Loop (🔁), Add to Playlist (+), Delete (🗑️)
- **Playlist Management**: Add, Clear, Play, Loop controls
- **IR Mapping**: View, assign, and clear button-to-sequence mappings
- **Auto-refresh**: Sequence list and playlist status update automatically

> **Note:** Pause/Resume is only available via the Web UI -- there are no serial commands for these actions.

---

## 🎉 Done!

You now have all the tools to create complex K-2SO performances!

### Next Steps

1. ✅ Experiment with different frame durations
2. ✅ Create a library of reusable sequences
3. ✅ Combine sequences into playlists
4. ✅ Map frequently used sequences to IR buttons
5. ✅ Share your best sequences with the community!

### Support

- **GitHub:** https://github.com/PrintedDroid/K-2SO-Animatronic
- **Community:** Printed-Droid.com Forums

**Have fun creating your K-2SO performances! 🤖**

