/*
================================================================================
// K-2SO Controller Configuration Header - UPDATED WITH STATUS LED
// Contains hardware definitions, pin assignments, and system constants
================================================================================
*/

#ifndef K2SO_CONFIG_H
#define K2SO_CONFIG_H

#include <Arduino.h>

//========================================
// HARDWARE CONFIGURATION
//========================================

// Board: Droid Logic Motion Board v1.2 with ESP32-S3-Zero
// Pin assignments for the board

// Audio System (DFPlayer Mini)
#define DFPLAYER_RX_PIN     12      // GP12 - DFPlayer RX
#define DFPLAYER_TX_PIN     11      // GP11 - DFPlayer RX

// I2C Bus (for future expansion)
#define I2C_SDA_PIN         1       // GP1 - SDA
#define I2C_SCL_PIN         2       // GP2 - SCL

// NeoPixel Eye LEDs
#define LEFT_EYE_PIN        3       // GP3 - Left eye NeoPixel
#define RIGHT_EYE_PIN       4       // GP4 - Right eye NeoPixel
#define NUM_EYE_PIXELS      7       // Number of pixels per eye

// Servo Control Pins
#define EYE_PAN_PIN         5       // GP5 - Eye pan servo
#define EYE_TILT_PIN        6       // GP6 - Eye tilt servo  
#define HEAD_PAN_PIN        7       // GP7 - Head pan servo
#define HEAD_TILT_PIN       8       // GP8 - Head tilt servo

// Sensor and Control
#define IR_RECEIVER_PIN     9       // GP9 - IR receiver

// Detail LEDs for random blinking
#define DETAIL_LED_1_PIN    10      // GP10 - Detail LED 1
#define DETAIL_LED_2_PIN    13      // GP13 - Detail LED 2

// Status LED (NEW)
#define STATUS_LED_PIN      21      // GP21 - Single WS2812 status LED
#define STATUS_LED_COUNT    1       // Single LED

// Detail LED count
#define DETAIL_LED_COUNT 2

//========================================
// WIFI CONFIGURATION - MUST BE CHANGED!
//========================================
#warning "IMPORTANT: Update WiFi credentials in config.h!"
#define WIFI_SSID           "Your Homewifi SSID"    // CHANGE THIS!
#define WIFI_PASSWORD       "Your Homewifi Password" // CHANGE THIS!

//========================================
// SYSTEM CONSTANTS
//========================================
#define EEPROM_SIZE         2048    // EEPROM size for configuration storage
#define EEPROM_MAGIC        0xC0    // K2SO version identifier (192 in decimal)
#define ENABLE_SERIAL_DEBUG true    // Enable detailed serial debugging

// Timing constants
#define AUTO_SLEEP_TIME     3600000  // Auto-sleep after 60 minutes of inactivity
#define DEFAULT_BRIGHTNESS  150     // Default LED brightness (0-255)

//========================================
// STATUS LED CONFIGURATION (NEW)
//========================================
// Status LED animation timings
#define STATUS_LED_BRIGHTNESS       50      // Status LED brightness (0-255)
#define STATUS_PULSE_SPEED          3000    // Pulse animation speed (ms)
#define STATUS_BLINK_FAST           200     // Fast blink interval (ms)
#define STATUS_BLINK_SLOW           1000    // Slow blink interval (ms)
#define STATUS_FLASH_DURATION       100     // Flash duration (ms)
#define STATUS_FADE_STEPS           50      // Fade animation steps

// Status LED states
enum StatusLEDState {
  STATUS_OFF,                 // LED off
  STATUS_BOOT,                // Boot sequence (blue blinking)
  STATUS_WIFI_CONNECTING,     // WiFi connecting (yellow blinking)
  STATUS_WIFI_CONNECTED,      // WiFi connected (green solid)
  STATUS_WIFI_DISCONNECTED,   // WiFi disconnected (red solid)
  STATUS_MODE_SCANNING,       // Scanning mode (ice blue pulse)
  STATUS_MODE_ALERT,          // Alert mode (red pulse)
  STATUS_MODE_IDLE,           // Idle mode (amber pulse)
  STATUS_IR_ACTIVITY,         // IR command received (white flash)
  STATUS_SERVO_ACTIVITY,      // Servo movement (blue flash)
  STATUS_AUDIO_ACTIVITY,      // Audio playing (green flash)
  STATUS_ERROR,               // Error state (fast red blink)
  STATUS_LEARNING_MODE,       // IR learning mode (purple blink)
  STATUS_CONFIG_MODE          // Configuration mode (cyan pulse)
};

//========================================
// OPERATING MODES
//========================================
enum OperatingMode {
  MODE_NORMAL,
  MODE_IR_SCANNER,
  MODE_IR_LEARNING,
  MODE_SETUP_WIZARD,
  MODE_MONITOR,
  MODE_TEST
};

enum PersonalityMode {
  MODE_SCANNING,    // Slow, methodical observation behavior
  MODE_ALERT,       // Fast, reactive behavior
  MODE_IDLE         // Minimal movement, power saving
};

//========================================
// LED ANIMATION MODES
//========================================
enum PixelMode {
  SOLID_COLOR,      // Static color
  FADE_OFF,         // Fade to off
  FADE_COLOR,       // Fade between colors
  FLICKER,          // Random brightness flicker
  PULSE,            // Smooth brightness pulse
  SCANNER           // Scanner effect
};

// Animation timing constants
#define PULSE_MIN_BRIGHTNESS_PCT    50      // Minimum brightness for pulse (50%)
#define PULSE_SPEED_MS              3000    // Full pulse cycle time
#define FLICKER_UPDATE_INTERVAL_MS  50      // Flicker update rate

//========================================
// DATA STRUCTURES
//========================================

// Forward declaration for Servo class (avoid including ESP32Servo.h here)
class Servo;

// IR Button configuration
struct IRButton {
  char name[16];
  uint32_t code;
  bool isConfigured;
};

// Servo state tracking
struct ServoState {
  Servo* servoObject;
  unsigned long previousMillis;
  unsigned long moveInterval;
  unsigned long waitInterval;
  int currentPosition;
  int targetPosition;
  int stepSize;
  int minRange;
  int maxRange;
  bool isMoving;
};

// Detail LED blinker state
struct DetailBlinker {
  uint8_t pin;
  bool state;
  unsigned long nextMs;
  uint16_t minOnMs;
  uint16_t maxOnMs;
  uint16_t minOffMs;
  uint16_t maxOffMs;
};

// Status LED animation state (NEW)
struct StatusLEDAnimation {
  StatusLEDState currentState;
  StatusLEDState targetState;
  unsigned long animationStart;
  unsigned long lastUpdate;
  float animationProgress;
  uint32_t currentColor;
  uint32_t targetColor;
  bool isAnimating;
  bool blinkState;
  int pulseDirection;  // 1 for increasing, -1 for decreasing
  unsigned long flashStartTime;
  unsigned long flashDuration;
};

// Profile system for storing complete configurations
struct Profile {
  char name[16];
  bool active;
  uint8_t mode;
  uint8_t volume;
  uint8_t eyeBrightness;
  
  // Servo center positions
  uint8_t eyePanCenter;
  uint8_t eyeTiltCenter;
  uint8_t headPanCenter;
  uint8_t headTiltCenter;
  
  // Timing settings
  uint16_t scanEyeMoveMin;
  uint16_t scanEyeMoveMax;
  uint16_t scanEyeWaitMin;
  uint16_t scanEyeWaitMax;
  uint16_t alertEyeMoveMin;
  uint16_t alertEyeMoveMax;
  uint16_t soundPauseMin;
  uint16_t soundPauseMax;
  
  // LED colors for different modes
  uint8_t scanColorR, scanColorG, scanColorB;
  uint8_t alertColorR, alertColorG, alertColorB;
};

// Main configuration structure
struct ConfigData {
  // System identification
  uint8_t magic;
  uint8_t version;
  uint32_t writeCount;  // EEPROM wear tracking
  
  // Remote configuration
  uint8_t remoteType;
  uint8_t buttonCount;
  IRButton buttons[21];  // Support up to 21 buttons
  
  // Current settings
  uint8_t currentProfile;
  uint8_t savedMode;
  uint8_t savedVolume;
  bool irEnabled;
  
  // Servo calibration - Eye servos
  uint8_t eyePanCenter;
  uint8_t eyeTiltCenter;
  uint8_t eyePanMin;
  uint8_t eyePanMax;
  uint8_t eyeTiltMin;
  uint8_t eyeTiltMax;
  
  // Servo calibration - Head servos  
  uint8_t headPanCenter;
  uint8_t headTiltCenter;
  uint8_t headPanMin;
  uint8_t headPanMax;
  uint8_t headTiltMin;
  uint8_t headTiltMax;
  
  // LED configuration
  uint8_t eyeBrightness;
  uint8_t ledEffectSpeed;
  
  // Status LED configuration (NEW)
  uint8_t statusLedBrightness;
  bool statusLedEnabled;
  
  // Timing configuration (milliseconds)
  uint16_t scanEyeMoveMin;
  uint16_t scanEyeMoveMax;
  uint16_t scanEyeWaitMin;
  uint16_t scanEyeWaitMax;
  uint16_t alertEyeMoveMin;
  uint16_t alertEyeMoveMax;
  uint16_t alertEyeWaitMin;
  uint16_t alertEyeWaitMax;
  uint16_t soundPauseMin;
  uint16_t soundPauseMax;
  uint16_t bootSequenceDelay;
  
  // Profile storage
  Profile profiles[5];
  
  // Data integrity
  uint32_t checksum;
};

//========================================
// COMMAND ENUMERATION
//========================================
enum Command {
  CMD_UNKNOWN,
  CMD_HELP, CMD_STATUS, CMD_RESET, CMD_SAVE, CMD_CONFIG,
  CMD_LEARN, CMD_SCAN, CMD_SHOW, CMD_CLEAR, CMD_DEFAULT,
  CMD_SERVO, CMD_LED, CMD_SOUND, CMD_TIMING,
  CMD_PROFILE, CMD_MONITOR, CMD_TEST,
  CMD_BACKUP, CMD_RESTORE, CMD_EXIT,
  CMD_IR_ON, CMD_IR_OFF, CMD_MODE
};

#endif // K2SO_CONFIG_H