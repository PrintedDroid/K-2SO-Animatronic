/*
================================================================================
// K-2SO Controller Global Variables Header - UPDATED WITH STATUS LED
================================================================================
*/

#ifndef K2SO_GLOBALS_H
#define K2SO_GLOBALS_H

#include <Arduino.h>
#include "config.h"      // For struct definitions
#include "Mp3Notify.h"   // For Mp3Notify class (needed for DFMiniMp3 template)

//========================================
// ENUM DEFINITIONS (used by multiple modules)
//========================================

// Detail LED animation patterns (used by sequences and detailleds)
enum DetailPattern {
  DETAIL_PATTERN_BLINK,     // Blink on/off
  DETAIL_PATTERN_FADE,      // Smooth fade in/out
  DETAIL_PATTERN_CHASE,     // Chase/sequential lighting
  DETAIL_PATTERN_PULSE,     // Breathing pulse effect
  DETAIL_PATTERN_RANDOM     // Random flickering
};

//========================================
// FORWARD DECLARATIONS (avoids includes)
//========================================

// Hardware classes - forward declared
class Adafruit_NeoPixel;
class WebServer;
class Servo;
class HardwareSerial;

//========================================
// HARDWARE OBJECT DECLARATIONS
//========================================

// NeoPixel objects
extern Adafruit_NeoPixel leftEye;
extern Adafruit_NeoPixel rightEye;
extern Adafruit_NeoPixel statusLED;  // Status LED
extern Adafruit_NeoPixel detailLEDs; // Detail LED strip (WS2812)

// Web server
extern WebServer server;

// Servo objects  
extern Servo eyePanServo;
extern Servo eyeTiltServo;
extern Servo headPanServo;
extern Servo headTiltServo;

// Hardware Serial for DFPlayer
extern HardwareSerial dfSerial;

// DFPlayer MP3 module
extern DFMiniMp3<HardwareSerial, Mp3Notify> mp3;

//========================================
// CONFIGURATION AND STATE VARIABLES
//========================================

// Configuration
extern ConfigData config;
extern ConfigData lastSavedConfig;

// Operating modes
extern OperatingMode operatingMode;
extern PersonalityMode currentMode;

// System status
extern bool isAwake;
extern bool bootSequenceComplete;
extern unsigned long lastActivityTime;

//========================================
// ANIMATION VARIABLES
//========================================

// Animation timing
extern unsigned long animationStartTime;
extern unsigned long lastAnimationUpdateTime;

// Current colors and settings
extern uint32_t leftEyeCurrentColor;
extern uint32_t rightEyeCurrentColor;
extern uint8_t currentBrightness;
extern PixelMode currentPixelMode;
extern uint8_t activeEyeLEDCount;  // Active LED count based on eye version

//========================================
// STATUS LED VARIABLES (NEW)
//========================================

// Status LED animation state
extern StatusLEDAnimation statusLEDAnim;

// Status LED activity tracking
extern unsigned long lastWifiCheck;
extern unsigned long lastStatusUpdate;
extern bool wifiWasConnected;

//========================================
// AUDIO SYSTEM VARIABLES
//========================================

extern bool isAudioReady;
extern bool isWaitingForNextTrack;
extern unsigned long nextPlayTime;
extern int currentTrackFolder;
extern uint8_t currentVolume;  // Current volume level (0-30)

//========================================
// SYSTEM STATISTICS
//========================================

extern unsigned long uptimeStart;
extern unsigned long irCommandCount;
extern unsigned long servoMovements;

//========================================
// BOOT SEQUENCE VARIABLES
//========================================

extern int bootSequenceStep;
extern unsigned long bootSequenceTimer;

//========================================
// OPERATING MODE VARIABLES
//========================================

// Monitor mode
extern bool monitorMode;
extern unsigned long lastMonitorUpdate;

// Learning mode
extern int learningStep;
extern int currentButtonIndex;
extern unsigned long learningTimeout;
extern bool waitingForIR;

// Test mode
extern int testStep;
extern unsigned long testTimer;

//========================================
// SERVO STATE STRUCTURES
//========================================

extern ServoState eyePan;
extern ServoState eyeTilt;
extern ServoState headPan;
extern ServoState headTilt;

//========================================
// DETAIL LED SYSTEM (LEGACY - DEPRECATED)
// Note: Old system with simple GPIO LEDs is deprecated
// See detailleds.h for new WS2812 strip system
//========================================

// Legacy DetailBlinker variables removed - now using WS2812 system (see detailleds.h)

//========================================
// NEW DETAIL LED SYSTEM (WS2812)
// Full declarations in detailleds.h
//========================================

// Forward declarations for new detail LED system
struct DetailLEDState;
struct DetailModeColors;

extern DetailLEDState detailState;
extern DetailModeColors detailModeColors;

//========================================
// IR REMOTE CONSTANTS
//========================================

// Standard 17-button remote layout
extern const char* standard17Buttons[17];

#endif // K2SO_GLOBALS_H