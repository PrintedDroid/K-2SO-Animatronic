#ifndef SEQUENCES_H
#define SEQUENCES_H

#include <Arduino.h>
#include <FS.h>
#include <LittleFS.h>
#include "config.h"
#include "globals.h"  // For ServoState, PixelMode, DetailPattern, mp3, etc.

// Maximum limits
#define MAX_FRAMES_PER_SEQUENCE 200
#define MAX_SEQUENCE_NAME_LENGTH 32
#define MAX_RECORDING_FRAMES 200
#define SEQUENCES_DIR "/sequences"

// Storage capacity (ESP32-S3-Zero: 4MB Flash)
// With default partition: ~1.5-2MB available for LittleFS
// Estimated capacity: 20-50+ sequences (depends on frame count)

// Recording states
enum RecordingState {
  REC_IDLE,
  REC_RECORDING,
  REC_PLAYING,
  REC_PAUSED
};

// Sequence frame structure - captures a single moment in time
struct SequenceFrame {
  uint16_t duration;        // Duration to hold this frame (ms)

  // Servo positions (0-180)
  uint8_t eyePan;
  uint8_t eyeTilt;
  uint8_t headPan;
  uint8_t headTilt;

  // Eye animation settings
  uint8_t eyeMode;          // Animation mode (0-13)
  uint32_t eyeColor;        // RGB color (24-bit)
  uint8_t eyeBrightness;    // 0-255

  // Detail LED settings
  uint8_t detailMode;       // Detail pattern (0-4)
  uint32_t detailColor;     // RGB color (24-bit)
  uint8_t detailBrightness; // 0-255

  // Audio settings
  uint8_t soundFile;        // 0 = no sound, 1-255 = file number
  uint8_t soundFolder;      // Folder number (1-99)
  uint8_t volume;           // 0-30
};

// Sequence metadata
struct SequenceInfo {
  char name[MAX_SEQUENCE_NAME_LENGTH];
  uint16_t frameCount;
  uint32_t totalDuration;  // Total duration in ms
  uint8_t version;          // Format version
};

struct SequenceVerifyInfo {
  char name[MAX_SEQUENCE_NAME_LENGTH];
  uint16_t declaredFrameCount;
  uint16_t actualFrameCount;
  uint32_t totalDuration;
  uint8_t version;
  size_t fileSize;
};

// Recording session state
struct RecordingSession {
  RecordingState state;
  char name[MAX_SEQUENCE_NAME_LENGTH];
  SequenceFrame* frames;        // Dynamic allocation - saves ~4.4KB RAM when not recording
  uint16_t frameCount;
  unsigned long startTime;
};

// Playlist for chaining sequences
#define MAX_PLAYLIST_ITEMS 10
struct Playlist {
  char sequences[MAX_PLAYLIST_ITEMS][MAX_SEQUENCE_NAME_LENGTH];
  uint8_t count;
  uint8_t currentIndex;
  bool loop;
  bool active;
};

// Playback state
struct PlaybackState {
  bool isPlaying;
  bool isPaused;                                      // Pause state for playback
  char currentSequenceName[MAX_SEQUENCE_NAME_LENGTH];
  uint16_t currentFrameIndex;
  uint16_t totalFrames;
  unsigned long frameStartTime;
  unsigned long pauseElapsed;                         // Time elapsed when paused
  bool loop;
  bool soundTriggered;                                // Prevent multiple sound triggers per frame
  SequenceFrame* frames;
  Playlist playlist;  // For chaining sequences
};

// Global sequence manager
class SequenceManager {
private:
  RecordingSession recording;
  PlaybackState playback;
  bool sdAvailable;

  // Helper functions
  bool ensureSequenceDir();
  String getSequencePath(const char* name);
  bool validateFrame(const SequenceFrame& frame);

public:
  SequenceManager();
  ~SequenceManager();

  // Initialization
  bool begin();
  bool isStorageAvailable() { return sdAvailable; }
  bool formatStorage();
  bool isValidSequenceName(const char* name) const;

  // Recording functions
  bool startRecording(const char* name);
  bool addFrame(const SequenceFrame& frame);
  bool addCurrentStateAsFrame(uint16_t duration);
  bool saveRecording();
  bool cancelRecording();
  bool isRecording() { return recording.state == REC_RECORDING; }
  uint16_t getRecordingFrameCount() { return recording.frameCount; }
  const char* getRecordingName() { return recording.name; }

  // Playback functions
  bool playSequence(const char* name, bool loop = false, bool preservePlaylist = false);
  bool stopPlayback(bool preservePlaylist = false);
  bool pausePlayback();
  bool resumePlayback();
  void updatePlayback(); // Call in main loop
  bool isPlaying() { return playback.isPlaying; }
  uint16_t getCurrentFrame() { return playback.currentFrameIndex; }
  uint16_t getTotalFrames() { return playback.totalFrames; }
  float getPlaybackProgress(); // Returns 0.0-1.0

  // Playlist functions (sequence chaining)
  bool playlistAdd(const char* name);
  bool playlistRemove(uint8_t index);
  bool playlistMove(uint8_t fromIndex, uint8_t toIndex);
  bool playlistClear();
  bool playlistStart(bool loop = false);
  bool playlistIsActive() { return playback.playlist.active; }
  uint8_t playlistGetCount() { return playback.playlist.count; }
  uint8_t playlistGetCurrentIndex() { return playback.playlist.currentIndex; }
  const char* playlistGetCurrentName();
  const char* playlistGetName(uint8_t index);
  void playlistPrint();

  // Sequence management
  bool deleteSequence(const char* name);
  bool sequenceExists(const char* name);
  bool renameSequence(const char* oldName, const char* newName);
  int listSequences(char names[][MAX_SEQUENCE_NAME_LENGTH], int maxCount);
  bool getSequenceInfo(const char* name, SequenceInfo& info);
  bool verifySequence(const char* name, SequenceVerifyInfo& info, String& errorMessage);
  bool exportSequenceJson(const char* name, String& jsonOut, String& errorMessage);
  bool importSequenceJson(const String& json, String& importedName, String& errorMessage);

  // File I/O
  bool saveSequenceToSD(const char* name, const SequenceFrame* frames, uint16_t frameCount);
  bool loadSequenceFromSD(const char* name, SequenceFrame*& frames, uint16_t& frameCount);

  // Utility
  void captureCurrentState(SequenceFrame& frame);
  String getStatusString();
  void printSequenceInfo(const char* name);
};

// Global instance
extern SequenceManager sequenceManager;

#endif // SEQUENCES_H
