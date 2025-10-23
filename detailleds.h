/*
================================================================================
// K-2SO Detail LED System Header - WS2812 Strip Control
// Supports configurable LED count and multiple animation patterns
// Includes two eye versions: 8-LED strip and 13-LED circle with center
================================================================================
*/

#ifndef K2SO_DETAILLEDS_H
#define K2SO_DETAILLEDS_H

#include <Arduino.h>
#include <Adafruit_NeoPixel.h>
#include "config.h"

//========================================
// DETAIL LED CONFIGURATION
//========================================

// Hardware configuration
#define DETAIL_LED_PIN          10      // GP10 - WS2812 strip
#define MAX_DETAIL_LEDS         8       // Maximum LEDs (8-LED strip)
#define DEFAULT_DETAIL_COUNT    5       // Default active LEDs
#define DEFAULT_DETAIL_BRIGHTNESS 150   // Default brightness (0-255)

//========================================
// ANIMATION PATTERNS
//========================================

enum DetailPattern {
  DETAIL_PATTERN_BLINK,     // Blink on/off
  DETAIL_PATTERN_FADE,      // Smooth fade in/out
  DETAIL_PATTERN_CHASE,     // Chase/sequential lighting
  DETAIL_PATTERN_PULSE,     // Breathing pulse effect
  DETAIL_PATTERN_RANDOM     // Random flickering
};

// Note: Eye version configuration removed - detail LEDs only support strip mode (1-8 LEDs)
// Main eye version configuration is in animations.h/config.h

//========================================
// ANIMATION TIMING CONSTANTS
//========================================

#define DETAIL_BLINK_ON_MS      500     // Blink on duration (0.5 seconds)
#define DETAIL_BLINK_OFF_MS     500     // Blink off duration (0.5 seconds)
#define DETAIL_FADE_SPEED_MS    1500    // Fade cycle duration
#define DETAIL_CHASE_SPEED_MS   100     // Chase animation speed
#define DETAIL_PULSE_SPEED_MS   2000    // Pulse cycle duration
#define DETAIL_RANDOM_MIN_MS    400     // Random flicker minimum interval (slower, calmer)
#define DETAIL_RANDOM_MAX_MS    1000    // Random flicker maximum interval (slower, calmer)

//========================================
// DATA STRUCTURES
//========================================

// Detail LED configuration and state
struct DetailLEDState {
  // Hardware configuration
  uint8_t activeCount;              // Number of active LEDs (1-8)
  uint8_t brightness;               // Brightness (0-255)

  // Color control
  uint8_t red;                      // Red component (0-255)
  uint8_t green;                    // Green component (0-255)
  uint8_t blue;                     // Blue component (0-255)

  // Animation control
  DetailPattern pattern;            // Current animation pattern
  bool enabled;                     // Master enable/disable

  // Animation state
  unsigned long lastUpdate;         // Last animation update time
  int animationStep;                // Current animation step
  bool animationDirection;          // Animation direction (for reversible patterns)
  float animationProgress;          // Animation progress (0.0 - 1.0)

  // Mode-dependent behavior
  bool autoColorMode;               // Automatically change color based on personality mode
};

// Mode-specific color configuration
struct DetailModeColors {
  uint8_t scanningR;
  uint8_t scanningG;
  uint8_t scanningB;

  uint8_t alertR;
  uint8_t alertG;
  uint8_t alertB;

  uint8_t idleR;
  uint8_t idleG;
  uint8_t idleB;
};

//========================================
// EXTERNAL REFERENCES
//========================================

// Hardware object (defined in main file)
extern Adafruit_NeoPixel detailLEDs;

// State variables (defined in detailleds.cpp)
extern DetailLEDState detailState;
extern DetailModeColors detailModeColors;

//========================================
// FUNCTION DECLARATIONS
//========================================

// Initialization and core functions
void initializeDetailLEDs();                           // Initialize detail LED system
void updateDetailLEDs();                               // Main update loop (call in main loop)
void resetDetailLEDs();                                // Reset to default state

// Configuration functions
void setDetailCount(uint8_t count);                    // Set number of active LEDs (1-8)
void setDetailBrightness(uint8_t brightness);          // Set brightness (0-255)
void setDetailColor(uint8_t r, uint8_t g, uint8_t b); // Set RGB color
void setDetailPattern(DetailPattern pattern);          // Set animation pattern
void setDetailEnabled(bool enabled);                   // Enable/disable detail LEDs
void setDetailAutoColorMode(bool enabled);             // Enable/disable auto color mode

// Pattern control functions
void startDetailBlink();                               // Start blink pattern
void startDetailFade();                                // Start fade pattern
void startDetailChase();                               // Start chase pattern
void startDetailPulse();                               // Start pulse pattern
void startDetailRandom();                              // Start random pattern

// Animation update functions (internal, called by updateDetailLEDs)
void updateDetailBlink();                              // Update blink animation
void updateDetailFade();                               // Update fade animation
void updateDetailChase();                              // Update chase animation
void updateDetailPulse();                              // Update pulse animation
void updateDetailRandom();                             // Update random animation

// Mode integration
void updateDetailColorForMode(PersonalityMode mode);   // Update color based on personality mode
void setDetailModeColors(uint8_t scanR, uint8_t scanG, uint8_t scanB,
                        uint8_t alertR, uint8_t alertG, uint8_t alertB,
                        uint8_t idleR, uint8_t idleG, uint8_t idleB);

// Utility functions
void detailLedsOff();                                  // Turn off all detail LEDs
void detailLedsOn();                                   // Turn on all detail LEDs
void setDetailLED(uint8_t index, uint8_t r, uint8_t g, uint8_t b); // Set individual LED
void showDetailLEDs();                                 // Update LED strip (calls show())
String getDetailPatternName();                         // Get current pattern name as string

// Status and information
void printDetailLEDStatus();                           // Print current status to Serial
bool isDetailLEDEnabled();                             // Check if enabled
uint8_t getDetailCount();                              // Get active LED count
DetailPattern getDetailPattern();                      // Get current pattern

// Preset configurations
void setDetailDefaultRed();                            // Set to default red blinking
void setDetailModeScanningBlue();                      // Set to scanning mode blue
void setDetailModeAlertRed();                          // Set to alert mode red
void setDetailModeIdleAmber();                         // Set to idle mode amber

#endif // K2SO_DETAILLEDS_H