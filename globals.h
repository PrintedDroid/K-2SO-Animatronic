/*
================================================================================
// K-2SO Controller Global Variables Header - UPDATED WITH STATUS LED
================================================================================
*/

#ifndef K2SO_GLOBALS_H
#define K2SO_GLOBALS_H

#include <Arduino.h>
#include "config.h"      // For struct definitions

//========================================
// FORWARD DECLARATIONS (avoids includes)
//========================================

// Hardware classes - forward declared
class Adafruit_NeoPixel;
class WebServer;
class Servo;
class HardwareSerial;

// Simple class forward declarations (no templates)
class Mp3Notify;

//========================================
// HARDWARE OBJECT DECLARATIONS
//========================================

// NeoPixel objects
extern Adafruit_NeoPixel leftEye;
extern Adafruit_NeoPixel rightEye;
extern Adafruit_NeoPixel statusLED;  // NEW: Status LED

// Web server
extern WebServer server;

// Servo objects  
extern Servo eyePanServo;
extern Servo eyeTiltServo;
extern Servo headPanServo;
extern Servo headTiltServo;

// Hardware Serial for DFPlayer
extern HardwareSerial dfSerial;

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
// DETAIL LED SYSTEM
//========================================

extern DetailBlinker blinkers[DETAIL_LED_COUNT];

// Detail LED pin array (constant)
extern const uint8_t DETAIL_LED_PINS[DETAIL_LED_COUNT];

//========================================
// IR REMOTE CONSTANTS
//========================================

// Standard 17-button remote layout
extern const char* standard17Buttons[17];

#endif // K2SO_GLOBALS_H