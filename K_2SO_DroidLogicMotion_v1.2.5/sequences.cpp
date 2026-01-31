#include "sequences.h"
#include "globals.h"
#include "animations.h"   // For PixelMode enum
#include <ArduinoJson.h>
#include <ESP32Servo.h>   // For Servo class methods

// Global instance
SequenceManager sequenceManager;

SequenceManager::SequenceManager() {
  recording.state = REC_IDLE;
  recording.frameCount = 0;
  recording.name[0] = '\0';
  recording.frames = nullptr;  // Dynamic allocation

  playback.isPlaying = false;
  playback.isPaused = false;
  playback.currentFrameIndex = 0;
  playback.totalFrames = 0;
  playback.frames = nullptr;
  playback.loop = false;
  playback.soundTriggered = false;
  playback.pauseElapsed = 0;

  // Initialize playlist
  playback.playlist.count = 0;
  playback.playlist.currentIndex = 0;
  playback.playlist.loop = false;
  playback.playlist.active = false;

  sdAvailable = false;
}

SequenceManager::~SequenceManager() {
  if (playback.frames != nullptr) {
    delete[] playback.frames;
    playback.frames = nullptr;
  }
  if (recording.frames != nullptr) {
    delete[] recording.frames;
    recording.frames = nullptr;
  }
}

bool SequenceManager::begin() {
  // Initialize LittleFS
  if (!LittleFS.begin(true)) {  // true = format on failure
    Serial.println(F("⚠️ LittleFS mount failed - sequences disabled"));
    sdAvailable = false;
    return false;
  }

  sdAvailable = true;

  // Ensure sequences directory exists
  if (!ensureSequenceDir()) {
    Serial.println(F("⚠️ Could not create sequences directory"));
    return false;
  }

  Serial.println(F("✓ Sequence manager initialized (LittleFS)"));
  Serial.println(F("  Storage: ESP32-S3-Zero 4MB Flash (~1.5-2MB for sequences)"));
  return true;
}

bool SequenceManager::ensureSequenceDir() {
  if (!LittleFS.exists(SEQUENCES_DIR)) {
    return LittleFS.mkdir(SEQUENCES_DIR);
  }
  return true;
}

String SequenceManager::getSequencePath(const char* name) {
  String path = String(SEQUENCES_DIR) + "/" + String(name) + ".seq";
  return path;
}

bool SequenceManager::validateFrame(const SequenceFrame& frame) {
  // Validate servo positions (0-180)
  if (frame.eyePan > 180 || frame.eyeTilt > 180 ||
      frame.headPan > 180 || frame.headTilt > 180) {
    return false;
  }

  // Validate duration (1ms - 60 seconds)
  if (frame.duration < 1 || frame.duration > 60000) {
    return false;
  }

  // Validate brightness values
  if (frame.eyeBrightness > 255 || frame.detailBrightness > 255) {
    return false;
  }

  // Validate volume (0-30 for DFPlayer)
  if (frame.volume > 30) {
    return false;
  }

  return true;
}

// ============================================================================
// RECORDING FUNCTIONS
// ============================================================================

bool SequenceManager::startRecording(const char* name) {
  if (!sdAvailable) {
    Serial.println(F("❌ SD card not available"));
    return false;
  }

  if (recording.state == REC_RECORDING) {
    Serial.println(F("❌ Already recording. Stop current recording first."));
    return false;
  }

  if (playback.isPlaying) {
    Serial.println(F("❌ Cannot record while playing. Stop playback first."));
    return false;
  }

  if (strlen(name) == 0 || strlen(name) >= MAX_SEQUENCE_NAME_LENGTH) {
    Serial.println(F("❌ Invalid sequence name"));
    return false;
  }

  // Check if sequence already exists
  if (sequenceExists(name)) {
    Serial.print(F("⚠️ Sequence '"));
    Serial.print(name);
    Serial.println(F("' already exists. It will be overwritten."));
  }

  // Free any existing recording buffer
  if (recording.frames != nullptr) {
    delete[] recording.frames;
    recording.frames = nullptr;
  }

  // Allocate recording buffer dynamically
  size_t requiredMemory = sizeof(SequenceFrame) * MAX_RECORDING_FRAMES;
  if (ESP.getFreeHeap() < requiredMemory + 10000) {  // Keep 10KB safety margin
    Serial.print(F("❌ Insufficient memory for recording buffer: "));
    Serial.print(ESP.getFreeHeap());
    Serial.print(F(" bytes free, need "));
    Serial.println(requiredMemory + 10000);
    return false;
  }

  recording.frames = new SequenceFrame[MAX_RECORDING_FRAMES];
  if (recording.frames == nullptr) {
    Serial.println(F("❌ Failed to allocate recording buffer"));
    return false;
  }

  // Initialize recording session
  recording.state = REC_RECORDING;
  strncpy(recording.name, name, MAX_SEQUENCE_NAME_LENGTH - 1);
  recording.name[MAX_SEQUENCE_NAME_LENGTH - 1] = '\0';
  recording.frameCount = 0;
  recording.startTime = millis();

  Serial.print(F("🎬 Recording started: "));
  Serial.println(recording.name);
  Serial.print(F("   Buffer allocated: "));
  Serial.print(requiredMemory);
  Serial.println(F(" bytes"));

  return true;
}

bool SequenceManager::addFrame(const SequenceFrame& frame) {
  if (recording.state != REC_RECORDING) {
    Serial.println(F("❌ Not recording"));
    return false;
  }

  if (recording.frames == nullptr) {
    Serial.println(F("❌ Recording buffer not allocated"));
    return false;
  }

  if (recording.frameCount >= MAX_RECORDING_FRAMES) {
    Serial.println(F("❌ Maximum frames reached"));
    return false;
  }

  if (!validateFrame(frame)) {
    Serial.println(F("❌ Invalid frame data"));
    return false;
  }

  // Add frame to recording
  recording.frames[recording.frameCount] = frame;
  recording.frameCount++;

  Serial.print(F("✓ Frame "));
  Serial.print(recording.frameCount);
  Serial.print(F(" added ("));
  Serial.print(frame.duration);
  Serial.println(F("ms)"));

  return true;
}

bool SequenceManager::addCurrentStateAsFrame(uint16_t duration) {
  SequenceFrame frame;
  captureCurrentState(frame);
  frame.duration = duration;
  return addFrame(frame);
}

void SequenceManager::captureCurrentState(SequenceFrame& frame) {
  // Capture current servo positions from ServoState structures
  frame.eyePan = eyePan.currentPosition;
  frame.eyeTilt = eyeTilt.currentPosition;
  frame.headPan = headPan.currentPosition;
  frame.headTilt = headTilt.currentPosition;

  // Capture eye animation settings
  frame.eyeMode = static_cast<uint8_t>(currentEyeMode);
  frame.eyeColor = currentEyeColor;
  frame.eyeBrightness = config.eyeBrightness;

  // Capture detail LED settings
  frame.detailMode = static_cast<uint8_t>(currentDetailPattern);
  frame.detailColor = currentDetailColor;
  frame.detailBrightness = detailBrightness;

  // Capture audio settings (no active sound trigger)
  frame.soundFile = 0;
  frame.soundFolder = 0;
  frame.volume = currentVolume;
}

bool SequenceManager::saveRecording() {
  if (recording.state != REC_RECORDING) {
    Serial.println(F("❌ Not recording"));
    return false;
  }

  if (recording.frameCount == 0) {
    Serial.println(F("❌ No frames to save"));
    return false;
  }

  // Save to SD card
  bool success = saveSequenceToSD(recording.name, recording.frames, recording.frameCount);

  if (success) {
    unsigned long duration = millis() - recording.startTime;
    Serial.print(F("💾 Sequence saved: "));
    Serial.print(recording.name);
    Serial.print(F(" ("));
    Serial.print(recording.frameCount);
    Serial.print(F(" frames, "));
    Serial.print(duration / 1000.0, 1);
    Serial.println(F("s recording time)"));

    // Reset recording state
    recording.state = REC_IDLE;
    recording.frameCount = 0;
  } else {
    Serial.println(F("❌ Failed to save sequence"));
  }

  // Free recording buffer
  if (recording.frames != nullptr) {
    delete[] recording.frames;
    recording.frames = nullptr;
    Serial.println(F("   Recording buffer freed"));
  }

  return success;
}

bool SequenceManager::cancelRecording() {
  if (recording.state != REC_RECORDING) {
    return false;
  }

  recording.state = REC_IDLE;
  recording.frameCount = 0;

  // Free recording buffer
  if (recording.frames != nullptr) {
    delete[] recording.frames;
    recording.frames = nullptr;
  }

  Serial.println(F("🚫 Recording cancelled, buffer freed"));
  return true;
}

// ============================================================================
// PLAYBACK FUNCTIONS
// ============================================================================

bool SequenceManager::playSequence(const char* name, bool loop) {
  if (!sdAvailable) {
    Serial.println(F("❌ SD card not available"));
    return false;
  }

  if (recording.state == REC_RECORDING) {
    Serial.println(F("❌ Cannot play while recording"));
    return false;
  }

  if (playback.isPlaying) {
    stopPlayback();
  }

  // Load sequence from SD
  SequenceFrame* frames = nullptr;
  uint16_t frameCount = 0;

  if (!loadSequenceFromSD(name, frames, frameCount)) {
    Serial.print(F("❌ Failed to load sequence: "));
    Serial.println(name);
    return false;
  }

  // Initialize playback
  playback.isPlaying = true;
  playback.isPaused = false;
  playback.soundTriggered = false;
  playback.pauseElapsed = 0;
  strncpy(playback.currentSequenceName, name, MAX_SEQUENCE_NAME_LENGTH - 1);
  playback.currentSequenceName[MAX_SEQUENCE_NAME_LENGTH - 1] = '\0';
  playback.currentFrameIndex = 0;
  playback.totalFrames = frameCount;
  playback.frameStartTime = millis();
  playback.loop = loop;
  playback.frames = frames;

  Serial.print(F("▶️ Playing: "));
  Serial.print(name);
  if (loop) {
    Serial.println(F(" (looping)"));
  } else {
    Serial.println();
  }

  return true;
}

void SequenceManager::updatePlayback() {
  if (!playback.isPlaying || playback.frames == nullptr) {
    return;
  }

  // Skip update if paused
  if (playback.isPaused) {
    return;
  }

  // Bounds check - prevent array out-of-bounds access
  if (playback.currentFrameIndex >= playback.totalFrames) {
    stopPlayback();
    Serial.println(F("⚠️ Frame index out of bounds, stopping playback"));
    return;
  }

  // Check if current frame duration has elapsed
  SequenceFrame& currentFrame = playback.frames[playback.currentFrameIndex];
  unsigned long elapsed = millis() - playback.frameStartTime;

  if (elapsed >= currentFrame.duration) {
    // Move to next frame
    playback.currentFrameIndex++;
    playback.soundTriggered = false;  // Reset sound trigger for new frame

    // Check if sequence is complete
    if (playback.currentFrameIndex >= playback.totalFrames) {
      // Check if playlist is active
      if (playback.playlist.active) {
        // Move to next sequence in playlist
        playback.playlist.currentIndex++;

        if (playback.playlist.currentIndex >= playback.playlist.count) {
          // Playlist complete
          if (playback.playlist.loop) {
            // Loop playlist
            playback.playlist.currentIndex = 0;
          } else {
            // Stop playback
            playback.playlist.active = false;
            stopPlayback();
            Serial.println(F("✓ Playlist complete"));
            return;
          }
        }

        // Play next sequence in playlist - with bounds validation
        if (playback.playlist.count == 0 ||
            playback.playlist.currentIndex >= MAX_PLAYLIST_ITEMS ||
            playback.playlist.currentIndex >= playback.playlist.count) {
          playback.playlist.active = false;
          stopPlayback();
          Serial.println(F("⚠️ Playlist empty or index out of bounds"));
          return;
        }
        const char* nextSeq = playback.playlist.sequences[playback.playlist.currentIndex];
        Serial.print(F("⏭️ Next in playlist: "));
        Serial.println(nextSeq);

        // Stop current and play next
        if (playback.frames != nullptr) {
          delete[] playback.frames;
          playback.frames = nullptr;
        }
        playSequence(nextSeq, false);
        return;

      } else if (playback.loop) {
        // Loop single sequence
        playback.currentFrameIndex = 0;
        playback.frameStartTime = millis();
        playback.soundTriggered = false;
      } else {
        // Stop playback
        stopPlayback();
        Serial.println(F("✓ Sequence complete"));
        return;
      }
    } else {
      playback.frameStartTime = millis();
    }
  }

  // Apply current frame settings
  SequenceFrame& frame = playback.frames[playback.currentFrameIndex];

  // Apply servo positions - use smooth interpolation via ServoState system
  // Only update target if position changed to avoid unnecessary movement resets
  if (eyePan.targetPosition != frame.eyePan) {
    eyePan.targetPosition = frame.eyePan;
    eyePan.isMoving = true;
  }
  if (eyeTilt.targetPosition != frame.eyeTilt) {
    eyeTilt.targetPosition = frame.eyeTilt;
    eyeTilt.isMoving = true;
  }
  if (headPan.targetPosition != frame.headPan) {
    headPan.targetPosition = frame.headPan;
    headPan.isMoving = true;
  }
  if (headTilt.targetPosition != frame.headTilt) {
    headTilt.targetPosition = frame.headTilt;
    headTilt.isMoving = true;
  }
  // Note: Actual servo movement is handled by updateServo() in main loop
  // This provides smooth interpolated movement instead of instant jumps

  // Apply eye animation
  if (frame.eyeMode < 14) {
    currentEyeMode = static_cast<PixelMode>(frame.eyeMode);
    currentEyeColor = frame.eyeColor;
    config.eyeBrightness = frame.eyeBrightness;
  }

  // Apply detail LED
  if (frame.detailMode < 5) {
    currentDetailPattern = static_cast<DetailPattern>(frame.detailMode);
    currentDetailColor = frame.detailColor;
    detailBrightness = frame.detailBrightness;
  }

  // Trigger sound if specified - only once per frame using flag
  if (frame.soundFile > 0 && !playback.soundTriggered) {
    mp3.playFolderTrack(frame.soundFolder, frame.soundFile);
    if (frame.volume > 0) {
      mp3.setVolume(frame.volume);
      currentVolume = frame.volume;
    }
    playback.soundTriggered = true;  // Prevent re-triggering
  }
}

bool SequenceManager::stopPlayback() {
  if (!playback.isPlaying) {
    return false;
  }

  playback.isPlaying = false;
  playback.playlist.active = false;  // Deactivate playlist if active

  // Free memory
  if (playback.frames != nullptr) {
    delete[] playback.frames;
    playback.frames = nullptr;
  }

  Serial.println(F("⏹️ Playback stopped"));
  return true;
}

bool SequenceManager::pausePlayback() {
  if (!playback.isPlaying || playback.isPaused) {
    return false;
  }

  // Store elapsed time in current frame before pausing
  playback.pauseElapsed = millis() - playback.frameStartTime;
  playback.isPaused = true;
  Serial.println(F("⏸️ Playback paused"));
  return true;
}

bool SequenceManager::resumePlayback() {
  if (!playback.isPlaying || !playback.isPaused) {
    return false;
  }

  // Restore frame timing accounting for elapsed time before pause
  playback.frameStartTime = millis() - playback.pauseElapsed;
  playback.isPaused = false;
  Serial.println(F("▶️ Playback resumed"));
  return true;
}

float SequenceManager::getPlaybackProgress() {
  if (!playback.isPlaying || playback.totalFrames == 0) {
    return 0.0;
  }
  return (float)playback.currentFrameIndex / (float)playback.totalFrames;
}

// ============================================================================
// SEQUENCE MANAGEMENT
// ============================================================================

bool SequenceManager::deleteSequence(const char* name) {
  if (!sdAvailable) {
    return false;
  }

  String path = getSequencePath(name);
  if (!LittleFS.exists(path)) {
    Serial.print(F("❌ Sequence not found: "));
    Serial.println(name);
    return false;
  }

  if (LittleFS.remove(path)) {
    Serial.print(F("🗑️ Deleted: "));
    Serial.println(name);
    return true;
  }

  return false;
}

bool SequenceManager::sequenceExists(const char* name) {
  if (!sdAvailable) {
    return false;
  }

  String path = getSequencePath(name);
  return LittleFS.exists(path);
}

bool SequenceManager::renameSequence(const char* oldName, const char* newName) {
  if (!sdAvailable) {
    return false;
  }

  String oldPath = getSequencePath(oldName);
  String newPath = getSequencePath(newName);

  if (!LittleFS.exists(oldPath)) {
    return false;
  }

  if (LittleFS.exists(newPath)) {
    Serial.println(F("❌ Target name already exists"));
    return false;
  }

  return LittleFS.rename(oldPath, newPath);
}

int SequenceManager::listSequences(char names[][MAX_SEQUENCE_NAME_LENGTH], int maxCount) {
  if (!sdAvailable) {
    return 0;
  }

  File dir = LittleFS.open(SEQUENCES_DIR);
  if (!dir) {
    return 0;
  }

  int count = 0;
  File entry = dir.openNextFile();

  while (entry && count < maxCount) {
    if (!entry.isDirectory()) {
      String filename = String(entry.name());
      // Remove directory path if present
      int lastSlash = filename.lastIndexOf('/');
      if (lastSlash >= 0) {
        filename = filename.substring(lastSlash + 1);
      }
      if (filename.endsWith(".seq")) {
        // Remove .seq extension
        filename.remove(filename.length() - 4);
        strncpy(names[count], filename.c_str(), MAX_SEQUENCE_NAME_LENGTH - 1);
        names[count][MAX_SEQUENCE_NAME_LENGTH - 1] = '\0';
        count++;
      }
    }
    entry.close();
    entry = dir.openNextFile();
  }

  dir.close();
  return count;
}

bool SequenceManager::getSequenceInfo(const char* name, SequenceInfo& info) {
  if (!sdAvailable) {
    return false;
  }

  String path = getSequencePath(name);
  File file = LittleFS.open(path, "r");

  if (!file) {
    return false;
  }

  // Parse JSON to get metadata
  DynamicJsonDocument doc(1024);
  DeserializationError error = deserializeJson(doc, file);
  file.close();

  if (error) {
    return false;
  }

  // Fill info structure
  strncpy(info.name, name, MAX_SEQUENCE_NAME_LENGTH - 1);
  info.name[MAX_SEQUENCE_NAME_LENGTH - 1] = '\0';
  info.frameCount = doc["frameCount"] | 0;
  info.totalDuration = doc["totalDuration"] | 0;
  info.version = doc["version"] | 1;

  return true;
}

// ============================================================================
// FILE I/O
// ============================================================================

bool SequenceManager::saveSequenceToSD(const char* name, const SequenceFrame* frames, uint16_t frameCount) {
  if (!sdAvailable || frames == nullptr || frameCount == 0) {
    return false;
  }

  // Check available heap memory before allocating JSON buffer
  size_t freeHeap = ESP.getFreeHeap();
  size_t requiredHeap = 20000;  // 16KB for JSON + safety margin
  if (freeHeap < requiredHeap) {
    Serial.print(F("❌ Insufficient heap memory: "));
    Serial.print(freeHeap);
    Serial.print(F(" bytes free, need "));
    Serial.println(requiredHeap);
    return false;
  }

  // Check available LittleFS space (estimate ~80 bytes per frame)
  size_t estimatedSize = 200 + (frameCount * 80);  // Header + frames
  size_t totalBytes = LittleFS.totalBytes();
  size_t usedBytes = LittleFS.usedBytes();
  size_t freeSpace = totalBytes - usedBytes;

  if (freeSpace < estimatedSize + 1024) {  // Keep 1KB safety margin
    Serial.print(F("❌ Insufficient storage: "));
    Serial.print(freeSpace);
    Serial.print(F(" bytes free, need ~"));
    Serial.println(estimatedSize);
    return false;
  }

  String path = getSequencePath(name);

  // Calculate total duration
  uint32_t totalDuration = 0;
  for (uint16_t i = 0; i < frameCount; i++) {
    totalDuration += frames[i].duration;
  }

  // Create JSON document
  DynamicJsonDocument doc(16384); // Large buffer for up to ~200 frames

  doc["name"] = name;
  doc["version"] = 1;
  doc["frameCount"] = frameCount;
  doc["totalDuration"] = totalDuration;

  JsonArray framesArray = doc.createNestedArray("frames");

  for (uint16_t i = 0; i < frameCount; i++) {
    JsonObject frame = framesArray.createNestedObject();
    frame["d"] = frames[i].duration;

    // Servos (compact notation)
    JsonObject servos = frame.createNestedObject("s");
    servos["ep"] = frames[i].eyePan;
    servos["et"] = frames[i].eyeTilt;
    servos["hp"] = frames[i].headPan;
    servos["ht"] = frames[i].headTilt;

    // Eyes
    frame["em"] = frames[i].eyeMode;
    frame["ec"] = frames[i].eyeColor;
    frame["eb"] = frames[i].eyeBrightness;

    // Detail LED
    frame["dm"] = frames[i].detailMode;
    frame["dc"] = frames[i].detailColor;
    frame["db"] = frames[i].detailBrightness;

    // Audio
    if (frames[i].soundFile > 0) {
      frame["sf"] = frames[i].soundFile;
      frame["so"] = frames[i].soundFolder;
      frame["v"] = frames[i].volume;
    }
  }

  // Write to file
  File file = LittleFS.open(path, "w");
  if (!file) {
    Serial.println(F("❌ Failed to open file for writing"));
    return false;
  }

  if (serializeJson(doc, file) == 0) {
    Serial.println(F("❌ Failed to write JSON"));
    file.close();
    return false;
  }

  file.close();
  return true;
}

bool SequenceManager::loadSequenceFromSD(const char* name, SequenceFrame*& frames, uint16_t& frameCount) {
  if (!sdAvailable) {
    return false;
  }

  // Check available heap memory before allocating JSON buffer
  size_t freeHeap = ESP.getFreeHeap();
  size_t requiredHeap = 24000;  // 16KB for JSON + frames array + safety margin
  if (freeHeap < requiredHeap) {
    Serial.print(F("❌ Insufficient heap memory: "));
    Serial.print(freeHeap);
    Serial.print(F(" bytes free, need "));
    Serial.println(requiredHeap);
    return false;
  }

  String path = getSequencePath(name);
  File file = LittleFS.open(path, "r");

  if (!file) {
    Serial.print(F("❌ File not found: "));
    Serial.println(path);
    return false;
  }

  // Parse JSON
  DynamicJsonDocument doc(16384);
  DeserializationError error = deserializeJson(doc, file);
  file.close();

  if (error) {
    Serial.print(F("❌ JSON parse error in '"));
    Serial.print(name);
    Serial.print(F("': "));
    Serial.println(error.c_str());
    return false;
  }

  // Get frame count
  frameCount = doc["frameCount"] | 0;
  if (frameCount == 0 || frameCount > MAX_FRAMES_PER_SEQUENCE) {
    Serial.print(F("❌ Invalid frame count in '"));
    Serial.print(name);
    Serial.print(F("': "));
    Serial.println(frameCount);
    return false;
  }

  // Allocate memory
  frames = new SequenceFrame[frameCount];
  if (frames == nullptr) {
    Serial.print(F("❌ Memory allocation failed for '"));
    Serial.print(name);
    Serial.print(F("' ("));
    Serial.print(frameCount);
    Serial.println(F(" frames)"));
    return false;
  }

  // Load frames
  JsonArray framesArray = doc["frames"];
  uint16_t idx = 0;

  for (JsonObject frameObj : framesArray) {
    if (idx >= frameCount) break;

    frames[idx].duration = frameObj["d"] | 1000;

    // Servos
    JsonObject servos = frameObj["s"];
    frames[idx].eyePan = servos["ep"] | 90;
    frames[idx].eyeTilt = servos["et"] | 90;
    frames[idx].headPan = servos["hp"] | 90;
    frames[idx].headTilt = servos["ht"] | 90;

    // Eyes
    frames[idx].eyeMode = frameObj["em"] | 0;
    frames[idx].eyeColor = frameObj["ec"] | 0x007FFF;
    frames[idx].eyeBrightness = frameObj["eb"] | 150;

    // Detail LED
    frames[idx].detailMode = frameObj["dm"] | 0;
    frames[idx].detailColor = frameObj["dc"] | 0x007FFF;
    frames[idx].detailBrightness = frameObj["db"] | 150;

    // Audio
    frames[idx].soundFile = frameObj["sf"] | 0;
    frames[idx].soundFolder = frameObj["so"] | 0;
    frames[idx].volume = frameObj["v"] | 20;

    idx++;
  }

  return true;
}

// ============================================================================
// UTILITY
// ============================================================================

String SequenceManager::getStatusString() {
  String status = "";

  if (recording.state == REC_RECORDING) {
    status = "Recording: " + String(recording.name) + " (" +
             String(recording.frameCount) + " frames)";
  } else if (playback.isPlaying) {
    status = "Playing: " + String(playback.currentSequenceName) + " [" +
             String(playback.currentFrameIndex + 1) + "/" +
             String(playback.totalFrames) + "]";
  } else {
    status = "Idle";
  }

  return status;
}

void SequenceManager::printSequenceInfo(const char* name) {
  SequenceInfo info;
  if (!getSequenceInfo(name, info)) {
    Serial.println(F("❌ Could not load sequence info"));
    return;
  }

  Serial.println(F("\n┌─ Sequence Info ────────────────────"));
  Serial.print(F("│ Name: "));
  Serial.println(info.name);
  Serial.print(F("│ Frames: "));
  Serial.println(info.frameCount);
  Serial.print(F("│ Duration: "));
  Serial.print(info.totalDuration / 1000.0, 2);
  Serial.println(F(" seconds"));
  Serial.print(F("│ Version: "));
  Serial.println(info.version);
  Serial.println(F("└────────────────────────────────────"));
}

// ============================================================================
// PLAYLIST FUNCTIONS (SEQUENCE CHAINING)
// ============================================================================

bool SequenceManager::playlistAdd(const char* name) {
  if (playback.playlist.count >= MAX_PLAYLIST_ITEMS) {
    Serial.println(F("❌ Playlist full (max 10 items)"));
    return false;
  }

  if (!sequenceExists(name)) {
    Serial.print(F("❌ Sequence not found: "));
    Serial.println(name);
    return false;
  }

  strncpy(playback.playlist.sequences[playback.playlist.count], name, MAX_SEQUENCE_NAME_LENGTH - 1);
  playback.playlist.sequences[playback.playlist.count][MAX_SEQUENCE_NAME_LENGTH - 1] = '\0';
  playback.playlist.count++;

  Serial.print(F("✓ Added to playlist: "));
  Serial.print(name);
  Serial.print(F(" ("));
  Serial.print(playback.playlist.count);
  Serial.println(F(" items)"));

  return true;
}

bool SequenceManager::playlistClear() {
  playback.playlist.count = 0;
  playback.playlist.currentIndex = 0;
  playback.playlist.active = false;
  Serial.println(F("✓ Playlist cleared"));
  return true;
}

bool SequenceManager::playlistStart(bool loop) {
  if (playback.playlist.count == 0) {
    Serial.println(F("❌ Playlist is empty"));
    return false;
  }

  playback.playlist.currentIndex = 0;
  playback.playlist.loop = loop;
  playback.playlist.active = true;

  Serial.print(F("▶️ Starting playlist ("));
  Serial.print(playback.playlist.count);
  Serial.print(F(" sequences)"));
  if (loop) {
    Serial.println(F(" [looping]"));
  } else {
    Serial.println();
  }

  // Start playing first sequence
  return playSequence(playback.playlist.sequences[0], false);
}

const char* SequenceManager::playlistGetCurrentName() {
  if (playback.playlist.active && playback.playlist.currentIndex < playback.playlist.count) {
    return playback.playlist.sequences[playback.playlist.currentIndex];
  }
  return "";
}

const char* SequenceManager::playlistGetName(uint8_t index) {
  if (index < playback.playlist.count) {
    return playback.playlist.sequences[index];
  }
  return "";
}

void SequenceManager::playlistPrint() {
  if (playback.playlist.count == 0) {
    Serial.println(F("📋 Playlist is empty"));
    return;
  }

  Serial.println(F("\n┌─ Playlist ─────────────────────────────────"));
  for (uint8_t i = 0; i < playback.playlist.count; i++) {
    Serial.print(F("│ "));
    Serial.print(i + 1);
    Serial.print(F(". "));
    Serial.println(playback.playlist.sequences[i]);
  }
  Serial.println(F("└────────────────────────────────────────────"));
  Serial.print(F("Total: "));
  Serial.print(playback.playlist.count);
  Serial.println(F(" sequence(s)"));

  if (playback.playlist.active) {
    Serial.print(F("▶️ Playing: #"));
    Serial.print(playback.playlist.currentIndex + 1);
    Serial.print(F(" ("));
    Serial.print(playback.playlist.sequences[playback.playlist.currentIndex]);
    Serial.println(F(")"));
  }
}
