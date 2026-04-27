#include "sequences.h"
#include "globals.h"
#include "animations.h"   // For PixelMode enum, setEyeColor, setEyeBrightness
#include "detailleds.h"   // For detailState, setDetailColor, setDetailBrightness, setDetailPattern
#include <ArduinoJson.h>
#include <ESP32Servo.h>   // For Servo class methods

// Global instance
SequenceManager sequenceManager;

namespace {
bool loadFramesFromJsonDocument(const JsonDocument& doc,
                                SequenceFrame*& frames,
                                uint16_t& frameCount,
                                String& errorMessage) {
  frameCount = doc["frameCount"] | 0;
  if (frameCount == 0 || frameCount > MAX_FRAMES_PER_SEQUENCE) {
    errorMessage = "Invalid frame count in sequence JSON.";
    frames = nullptr;
    frameCount = 0;
    return false;
  }

  JsonArrayConst framesArray = doc["frames"].as<JsonArrayConst>();
  if (framesArray.isNull()) {
    errorMessage = "Missing frames array in sequence JSON.";
    frames = nullptr;
    frameCount = 0;
    return false;
  }

  uint16_t actualFrameCount = framesArray.size();
  if (actualFrameCount == 0) {
    errorMessage = "Sequence JSON contains no frames.";
    frames = nullptr;
    frameCount = 0;
    return false;
  }

  if (actualFrameCount != frameCount) {
    errorMessage = "Declared frame count does not match actual frames array.";
    frames = nullptr;
    frameCount = actualFrameCount;
    return false;
  }

  frames = new SequenceFrame[frameCount];
  if (frames == nullptr) {
    errorMessage = "Memory allocation failed for imported sequence.";
    frameCount = 0;
    return false;
  }

  uint16_t idx = 0;
  for (JsonObjectConst frameObj : framesArray) {
    JsonObjectConst servos = frameObj["s"].as<JsonObjectConst>();

    frames[idx].duration = frameObj["d"] | 1000;
    frames[idx].eyePan = servos["ep"] | 90;
    frames[idx].eyeTilt = servos["et"] | 90;
    frames[idx].headPan = servos["hp"] | 90;
    frames[idx].headTilt = servos["ht"] | 90;
    frames[idx].eyeMode = frameObj["em"] | 0;
    frames[idx].eyeColor = frameObj["ec"] | 0x007FFF;
    frames[idx].eyeBrightness = frameObj["eb"] | 150;
    frames[idx].detailMode = frameObj["dm"] | 0;
    frames[idx].detailColor = frameObj["dc"] | 0x007FFF;
    frames[idx].detailBrightness = frameObj["db"] | 150;
    frames[idx].soundFile = frameObj["sf"] | 0;
    frames[idx].soundFolder = frameObj["so"] | 0;
    frames[idx].volume = frameObj["v"] | 20;
    idx++;
  }

  return true;
}
}

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
  if (!LittleFS.begin(false)) {
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

bool SequenceManager::formatStorage() {
  if (playback.isPlaying) {
    stopPlayback();
  }

  if (recording.state == REC_RECORDING) {
    cancelRecording();
  }

  LittleFS.end();

  if (!LittleFS.format()) {
    Serial.println(F("Failed to format LittleFS."));
    sdAvailable = false;
    return false;
  }

  if (!LittleFS.begin(false)) {
    Serial.println(F("LittleFS mount failed after format."));
    sdAvailable = false;
    return false;
  }

  sdAvailable = ensureSequenceDir();
  if (!sdAvailable) {
    Serial.println(F("Could not recreate /sequences after format."));
    return false;
  }

  playback.playlist.count = 0;
  playback.playlist.currentIndex = 0;
  playback.playlist.loop = false;
  playback.playlist.active = false;

  Serial.println(F("LittleFS formatted. Sequence storage is ready."));
  return true;
}

bool SequenceManager::ensureSequenceDir() {
  if (!LittleFS.exists(SEQUENCES_DIR)) {
    return LittleFS.mkdir(SEQUENCES_DIR);
  }
  return true;
}

bool SequenceManager::isValidSequenceName(const char* name) const {
  if (name == nullptr) {
    return false;
  }

  size_t len = strlen(name);
  if (len < 1 || len > 31) {
    return false;
  }

  for (size_t i = 0; i < len; i++) {
    char c = name[i];
    bool ok = (c >= 'a' && c <= 'z') ||
              (c >= 'A' && c <= 'Z') ||
              (c >= '0' && c <= '9') ||
              c == '_' || c == '-';
    if (!ok) {
      return false;
    }
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

  if (!isValidSequenceName(name)) {
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

  // Capture eye animation settings from actual system state
  frame.eyeMode = static_cast<uint8_t>(currentPixelMode);
  frame.eyeColor = leftEyeCurrentColor;
  frame.eyeBrightness = currentBrightness;

  // Capture detail LED settings from actual detailState
  frame.detailMode = static_cast<uint8_t>(detailState.pattern);
  frame.detailColor = ((uint32_t)detailState.red << 16) | ((uint32_t)detailState.green << 8) | detailState.blue;
  frame.detailBrightness = detailState.brightness;

  // Capture audio settings (no active sound trigger)
  frame.soundFile = 0;
  frame.soundFolder = 0;
  frame.volume = config.savedVolume;
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

bool SequenceManager::playSequence(const char* name, bool loop, bool preservePlaylist) {
  if (!sdAvailable) {
    Serial.println(F("❌ SD card not available"));
    return false;
  }

  if (!isValidSequenceName(name)) {
    Serial.println(F("Invalid sequence name. Use only A-Z, a-z, 0-9, _ and - (1-31 chars)."));
    return false;
  }

  if (recording.state == REC_RECORDING) {
    Serial.println(F("❌ Cannot play while recording"));
    return false;
  }

  if (playback.isPlaying) {
    stopPlayback(preservePlaylist);
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

  // Apply first frame immediately (LEDs, details, sound, servos)
  if (frameCount > 0) {
    SequenceFrame& first = frames[0];
    eyePan.targetPosition = first.eyePan;
    eyePan.isMoving = true;
    eyeTilt.targetPosition = first.eyeTilt;
    eyeTilt.isMoving = true;
    headPan.targetPosition = first.headPan;
    headPan.isMoving = true;
    headTilt.targetPosition = first.headTilt;
    headTilt.isMoving = true;

    if (first.eyeMode < 14) {
      currentPixelMode = static_cast<PixelMode>(first.eyeMode);
      setEyeColor(first.eyeColor, first.eyeColor);
      setEyeBrightness(first.eyeBrightness);
    }
    if (first.detailMode < 5) {
      setDetailPattern(static_cast<DetailPattern>(first.detailMode));
      setDetailColor((first.detailColor >> 16) & 0xFF,
                     (first.detailColor >> 8) & 0xFF,
                     first.detailColor & 0xFF);
      setDetailBrightness(first.detailBrightness);
    }
    if (first.soundFile > 0) {
      mp3.playFolderTrack(first.soundFolder, first.soundFile);
      if (first.volume > 0) mp3.setVolume(first.volume);
      playback.soundTriggered = true;
    }
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
  bool newFrame = false;

  if (elapsed >= currentFrame.duration) {
    // Move to next frame
    playback.currentFrameIndex++;
    playback.soundTriggered = false;  // Reset sound trigger for new frame
    newFrame = true;

    // Check if sequence is complete
    if (playback.currentFrameIndex >= playback.totalFrames) {
      // Check if playlist is active
      if (playback.playlist.active) {
        // Move to next sequence in playlist
        uint8_t nextIndex = playback.playlist.currentIndex + 1;

        if (nextIndex >= playback.playlist.count) {
          // Playlist complete
          if (playback.playlist.loop) {
            // Loop playlist
            nextIndex = 0;
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
            nextIndex >= MAX_PLAYLIST_ITEMS ||
            nextIndex >= playback.playlist.count) {
          playback.playlist.active = false;
          stopPlayback();
          Serial.println(F("⚠️ Playlist empty or index out of bounds"));
          return;
        }
        const char* nextSeq = playback.playlist.sequences[nextIndex];
        Serial.print(F("⏭️ Next in playlist: "));
        Serial.println(nextSeq);

        // Stop current and play next
        if (playback.frames != nullptr) {
          delete[] playback.frames;
          playback.frames = nullptr;
        }
        if (!playSequence(nextSeq, false, true)) {
          playback.playlist.active = false;
          playback.playlist.loop = false;
          playback.playlist.currentIndex = 0;
          playback.isPaused = false;
          playback.pauseElapsed = 0;
          playback.soundTriggered = false;
          playback.currentFrameIndex = 0;
          playback.totalFrames = 0;
          playback.loop = false;
          playback.currentSequenceName[0] = '\0';
          Serial.println(F("Playlist stopped - failed to load next sequence."));
          return;
        }
        playback.playlist.currentIndex = nextIndex;
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

  // Get current frame
  SequenceFrame& frame = playback.frames[playback.currentFrameIndex];

  // Servo targets are set every loop for smooth interpolated movement
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

  // LED, detail, and sound only applied on frame transitions (not every loop)
  // This avoids fighting with the animation system and wasting CPU on .show() calls
  if (!newFrame) {
    return;
  }

  // Apply eye animation via actual system functions
  if (frame.eyeMode < 14) {
    currentPixelMode = static_cast<PixelMode>(frame.eyeMode);
    setEyeColor(frame.eyeColor, frame.eyeColor);
    setEyeBrightness(frame.eyeBrightness);
  }

  // Apply detail LED via actual system functions
  if (frame.detailMode < 5) {
    setDetailPattern(static_cast<DetailPattern>(frame.detailMode));
    setDetailColor((frame.detailColor >> 16) & 0xFF,
                   (frame.detailColor >> 8) & 0xFF,
                   frame.detailColor & 0xFF);
    setDetailBrightness(frame.detailBrightness);
  }

  // Trigger sound if specified
  if (frame.soundFile > 0 && !playback.soundTriggered) {
    mp3.playFolderTrack(frame.soundFolder, frame.soundFile);
    if (frame.volume > 0) {
      mp3.setVolume(frame.volume);
    }
    playback.soundTriggered = true;
  }
}

bool SequenceManager::stopPlayback(bool preservePlaylist) {
  if (!playback.isPlaying) {
    return false;
  }

  playback.isPlaying = false;
  if (!preservePlaylist) {
    playback.playlist.active = false;  // Deactivate playlist if active
  }

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

  if (!isValidSequenceName(name)) {
    Serial.println(F("Invalid sequence name."));
    return false;
  }

  if (playback.isPlaying && strcmp(playback.currentSequenceName, name) == 0) {
    Serial.println(F("Stopping playback before delete."));
    stopPlayback();
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

  if (!isValidSequenceName(name)) {
    return false;
  }

  String path = getSequencePath(name);
  return LittleFS.exists(path);
}

bool SequenceManager::renameSequence(const char* oldName, const char* newName) {
  if (!sdAvailable) {
    return false;
  }

  if (!isValidSequenceName(oldName) || !isValidSequenceName(newName)) {
    Serial.println(F("Invalid sequence name."));
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

bool SequenceManager::verifySequence(const char* name, SequenceVerifyInfo& info, String& errorMessage) {
  memset(&info, 0, sizeof(info));

  if (!sdAvailable) {
    errorMessage = "Sequence storage is not available.";
    return false;
  }

  if (!isValidSequenceName(name)) {
    errorMessage = "Invalid sequence name.";
    return false;
  }

  String path = getSequencePath(name);
  File file = LittleFS.open(path, "r");
  if (!file) {
    errorMessage = "Sequence file could not be opened.";
    return false;
  }

  info.fileSize = file.size();
  strncpy(info.name, name, MAX_SEQUENCE_NAME_LENGTH - 1);
  info.name[MAX_SEQUENCE_NAME_LENGTH - 1] = '\0';

  DynamicJsonDocument doc(16384);
  if (doc.capacity() == 0) {
    file.close();
    errorMessage = "Out of memory for sequence verification.";
    return false;
  }

  DeserializationError error = deserializeJson(doc, file);
  file.close();
  if (error) {
    errorMessage = String("JSON parse error: ") + error.c_str();
    return false;
  }

  info.declaredFrameCount = doc["frameCount"] | 0;
  info.totalDuration = doc["totalDuration"] | 0;
  info.version = doc["version"] | 1;

  JsonArrayConst framesArray = doc["frames"].as<JsonArrayConst>();
  if (framesArray.isNull()) {
    errorMessage = "Missing frames array.";
    return false;
  }

  info.actualFrameCount = framesArray.size();
  if (info.actualFrameCount == 0) {
    errorMessage = "Sequence contains no frames.";
    return false;
  }

  if (info.declaredFrameCount == 0 || info.declaredFrameCount > MAX_FRAMES_PER_SEQUENCE) {
    errorMessage = "Declared frame count is invalid.";
    return false;
  }

  if (info.actualFrameCount != info.declaredFrameCount) {
    errorMessage = "Declared frame count does not match actual frames array.";
    return false;
  }

  for (JsonObjectConst frameObj : framesArray) {
    uint16_t duration = frameObj["d"] | 0;
    if (duration == 0) {
      errorMessage = "At least one frame has an invalid duration.";
      return false;
    }
  }

  errorMessage = "Sequence verified successfully.";
  return true;
}

bool SequenceManager::exportSequenceJson(const char* name, String& jsonOut, String& errorMessage) {
  jsonOut = "";

  if (!sdAvailable) {
    errorMessage = "Sequence storage is not available.";
    return false;
  }

  if (!isValidSequenceName(name)) {
    errorMessage = "Invalid sequence name.";
    return false;
  }

  String path = getSequencePath(name);
  File file = LittleFS.open(path, "r");
  if (!file) {
    errorMessage = "Sequence file could not be opened.";
    return false;
  }

  jsonOut = file.readString();
  file.close();

  if (jsonOut.length() == 0) {
    errorMessage = "Sequence file is empty.";
    return false;
  }

  errorMessage = "Sequence exported successfully.";
  return true;
}

bool SequenceManager::importSequenceJson(const String& json, String& importedName, String& errorMessage) {
  importedName = "";

  if (!sdAvailable) {
    errorMessage = "Sequence storage is not available.";
    return false;
  }

  if (json.length() == 0) {
    errorMessage = "No sequence JSON received.";
    return false;
  }

  DynamicJsonDocument doc(16384);
  if (doc.capacity() == 0) {
    errorMessage = "Out of memory for sequence import.";
    return false;
  }

  DeserializationError error = deserializeJson(doc, json);
  if (error) {
    errorMessage = String("JSON parse error: ") + error.c_str();
    return false;
  }

  const char* name = doc["name"] | "";
  if (!isValidSequenceName(name)) {
    errorMessage = "Invalid sequence name in JSON.";
    return false;
  }

  if (sequenceExists(name)) {
    errorMessage = "Sequence already exists. Delete or rename it first.";
    return false;
  }

  SequenceFrame* frames = nullptr;
  uint16_t frameCount = 0;
  if (!loadFramesFromJsonDocument(doc, frames, frameCount, errorMessage)) {
    if (frames != nullptr) {
      delete[] frames;
    }
    return false;
  }

  bool saved = saveSequenceToSD(name, frames, frameCount);
  delete[] frames;

  if (!saved) {
    errorMessage = "Failed to save imported sequence.";
    return false;
  }

  importedName = String(name);
  errorMessage = "Sequence imported successfully.";
  return true;
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

  if (!isValidSequenceName(name)) {
    return false;
  }

  String path = getSequencePath(name);
  File file = LittleFS.open(path, "r");

  if (!file) {
    return false;
  }

  StaticJsonDocument<64> filter;
  filter["frameCount"] = true;
  filter["totalDuration"] = true;
  filter["version"] = true;

  DynamicJsonDocument doc(256);
  if (doc.capacity() == 0) {
    file.close();
    return false;
  }

  DeserializationError error = deserializeJson(doc, file, DeserializationOption::Filter(filter));
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

  if (!isValidSequenceName(name)) {
    Serial.println(F("Invalid sequence name."));
    return false;
  }

  // Check available heap memory before allocating JSON buffer
  size_t freeHeap = ESP.getFreeHeap();
  size_t requiredHeap = 40000;
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
  String tempPath = path + ".tmp";
  String backupPath = path + ".bak";

  // Calculate total duration
  uint32_t totalDuration = 0;
  for (uint16_t i = 0; i < frameCount; i++) {
    totalDuration += frames[i].duration;
  }

  // Create JSON document
  DynamicJsonDocument doc(16384); // Large buffer for up to ~200 frames
  if (doc.capacity() == 0) {
    Serial.println(F("Error: Out of memory for JSON document."));
    return false;
  }

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

  if (LittleFS.exists(tempPath)) {
    LittleFS.remove(tempPath);
  }
  if (LittleFS.exists(backupPath)) {
    LittleFS.remove(backupPath);
  }

  // Write to temporary file first so the last good sequence is preserved
  File file = LittleFS.open(tempPath, "w");
  if (!file) {
    Serial.println(F("❌ Failed to open file for writing"));
    return false;
  }

  if (serializeJson(doc, file) == 0) {
    Serial.println(F("❌ Failed to write JSON"));
    file.close();
    LittleFS.remove(tempPath);
    return false;
  }

  file.close();

  bool hadExistingFile = LittleFS.exists(path);
  if (hadExistingFile && !LittleFS.rename(path, backupPath)) {
    LittleFS.remove(tempPath);
    Serial.println(F("Failed to protect existing sequence before replace."));
    return false;
  }

  if (!LittleFS.rename(tempPath, path)) {
    if (hadExistingFile && LittleFS.exists(backupPath)) {
      LittleFS.rename(backupPath, path);
    }
    LittleFS.remove(tempPath);
    Serial.println(F("Failed to replace sequence file."));
    return false;
  }

  if (LittleFS.exists(backupPath)) {
    LittleFS.remove(backupPath);
  }

  return true;
}

bool SequenceManager::loadSequenceFromSD(const char* name, SequenceFrame*& frames, uint16_t& frameCount) {
  if (!sdAvailable) {
    return false;
  }

  if (!isValidSequenceName(name)) {
    Serial.println(F("Invalid sequence name."));
    return false;
  }

  // Check available heap memory before allocating JSON buffer
  size_t freeHeap = ESP.getFreeHeap();
  size_t requiredHeap = 40000;
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
  if (doc.capacity() == 0) {
    file.close();
    Serial.println(F("Error: Out of memory for JSON document."));
    return false;
  }
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

  if (idx == 0) {
    delete[] frames;
    frames = nullptr;
    frameCount = 0;
    Serial.println(F("Error: Sequence file contains no frames."));
    return false;
  }

  frameCount = idx;
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
  if (!isValidSequenceName(name)) {
    Serial.println(F("Invalid sequence name."));
    return false;
  }

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

bool SequenceManager::playlistRemove(uint8_t index) {
  if (index >= playback.playlist.count) {
    Serial.println(F("Invalid playlist index."));
    return false;
  }

  if (playback.playlist.active) {
    Serial.println(F("Stop playlist playback before removing items."));
    return false;
  }

  for (uint8_t i = index; i + 1 < playback.playlist.count; i++) {
    strncpy(playback.playlist.sequences[i],
            playback.playlist.sequences[i + 1],
            MAX_SEQUENCE_NAME_LENGTH - 1);
    playback.playlist.sequences[i][MAX_SEQUENCE_NAME_LENGTH - 1] = '\0';
  }

  if (playback.playlist.count > 0) {
    playback.playlist.sequences[playback.playlist.count - 1][0] = '\0';
    playback.playlist.count--;
  }

  if (playback.playlist.currentIndex >= playback.playlist.count) {
    playback.playlist.currentIndex = 0;
  }

  Serial.println(F("Playlist item removed."));
  return true;
}

bool SequenceManager::playlistMove(uint8_t fromIndex, uint8_t toIndex) {
  if (fromIndex >= playback.playlist.count || toIndex >= playback.playlist.count) {
    Serial.println(F("Invalid playlist index."));
    return false;
  }

  if (fromIndex == toIndex) {
    return true;
  }

  if (playback.playlist.active) {
    Serial.println(F("Stop playlist playback before reordering items."));
    return false;
  }

  char movedName[MAX_SEQUENCE_NAME_LENGTH];
  strncpy(movedName, playback.playlist.sequences[fromIndex], MAX_SEQUENCE_NAME_LENGTH - 1);
  movedName[MAX_SEQUENCE_NAME_LENGTH - 1] = '\0';

  if (fromIndex < toIndex) {
    for (uint8_t i = fromIndex; i < toIndex; i++) {
      strncpy(playback.playlist.sequences[i],
              playback.playlist.sequences[i + 1],
              MAX_SEQUENCE_NAME_LENGTH - 1);
      playback.playlist.sequences[i][MAX_SEQUENCE_NAME_LENGTH - 1] = '\0';
    }
  } else {
    for (uint8_t i = fromIndex; i > toIndex; i--) {
      strncpy(playback.playlist.sequences[i],
              playback.playlist.sequences[i - 1],
              MAX_SEQUENCE_NAME_LENGTH - 1);
      playback.playlist.sequences[i][MAX_SEQUENCE_NAME_LENGTH - 1] = '\0';
    }
  }

  strncpy(playback.playlist.sequences[toIndex], movedName, MAX_SEQUENCE_NAME_LENGTH - 1);
  playback.playlist.sequences[toIndex][MAX_SEQUENCE_NAME_LENGTH - 1] = '\0';

  Serial.println(F("Playlist item moved."));
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
  bool started = playSequence(playback.playlist.sequences[0], false, true);
  if (!started) {
    playback.playlist.active = false;
    playback.playlist.loop = false;
    playback.playlist.currentIndex = 0;
    playback.isPaused = false;
    playback.pauseElapsed = 0;
    playback.soundTriggered = false;
    playback.currentFrameIndex = 0;
    playback.totalFrames = 0;
    playback.loop = false;
    playback.currentSequenceName[0] = '\0';
    Serial.println(F("Playlist start failed - first sequence could not load."));
  }
  return started;
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
