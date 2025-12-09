/*
================================================================================
// K-2SO Controller Animation System Header 
// Contains LED animation function declarations and constants
================================================================================
*/

#ifndef K2SO_ANIMATIONS_H
#define K2SO_ANIMATIONS_H

#include <Arduino.h>
#include <Adafruit_NeoPixel.h>  // Full include instead of forward declaration
#include "config.h"  // For PixelMode enum and constants

//========================================
// ANIMATION CONSTANTS
//========================================

// Fade animation settings
#define FADE_DURATION_MS            1000    // Duration for color fades
#define FADE_STEPS                  50      // Number of steps in fade

// Flicker animation settings
#define FLICKER_INTENSITY_MIN       0.3     // Minimum flicker intensity (30%)
#define FLICKER_INTENSITY_MAX       1.0     // Maximum flicker intensity (100%)
#define FLICKER_UPDATE_INTERVAL_MS  50      // Flicker update rate

// Pulse animation settings  
#define PULSE_MIN_BRIGHTNESS        0.2     // Minimum pulse brightness (20%)
#define PULSE_MAX_BRIGHTNESS        1.0     // Maximum pulse brightness (100%)
#define PULSE_SPEED_MS              3000    // Full pulse cycle time

// Scanner animation settings
#define SCANNER_SPEED               100     // Scanner sweep speed in milliseconds
#define SCANNER_TAIL_LENGTH         3       // Length of scanner trail

//========================================
// ANIMATION STATE TRACKING - KORRIGIERT: Nur einmal definiert
//========================================

struct AnimationState {
  // Fade animation state
  uint32_t fadeStartColorLeft;
  uint32_t fadeStartColorRight;
  uint32_t fadeTargetColorLeft;
  uint32_t fadeTargetColorRight;
  unsigned long fadeStartTime;
  
  // Flicker animation state
  unsigned long lastFlickerUpdate;
  float flickerIntensityLeft;
  float flickerIntensityRight;
  
  // Pulse animation state
  unsigned long pulseStartTime;
  bool pulseDirection; // true = brightening, false = dimming
  
  // Scanner animation state
  unsigned long lastScannerUpdate;
  int scannerPosition;
  bool scannerDirection; // true = forward, false = reverse

  // Multi-purpose animation step counter
  int animationStep;

  // General animation state
  bool animationActive;
  PixelMode currentMode;
  uint32_t baseColorLeft;
  uint32_t baseColorRight;
  uint8_t baseBrightness;
};

//========================================
// EXTERNAL REFERENCES - KORRIGIERT: Bereinigt
//========================================

// Hardware references (defined in main file - forward declared here)
class Adafruit_NeoPixel;
extern Adafruit_NeoPixel leftEye;
extern Adafruit_NeoPixel rightEye;

// External variables (defined in main file)
extern PixelMode currentPixelMode;
extern uint32_t leftEyeCurrentColor;
extern uint32_t rightEyeCurrentColor;
extern uint8_t currentBrightness;
extern unsigned long animationStartTime;
extern unsigned long lastAnimationUpdateTime;

// Animation state (defined in animations.cpp)
extern AnimationState animState;

//========================================
// FUNCTION DECLARATIONS - Grouped by Functionality
//========================================

// Core animation functions
void initializeAnimations();                           // Initialize animation system
void handlePixelAnimations();                          // Main animation update loop
void stopAllAnimations();                              // Stop all animations
void resetAnimationState();                            // Reset animation state

// Color and brightness control
void setEyeColor(uint32_t leftColor, uint32_t rightColor);     // Set eye colors directly
void setEyeBrightness(uint8_t brightness);                     // Set eye brightness
void setEyeColorAndBrightness(uint32_t leftColor, uint32_t rightColor, uint8_t brightness);

// Basic animation modes
void startSolidColor(uint32_t leftColor, uint32_t rightColor); // Static color mode
void startColorFade(uint32_t targetLeft, uint32_t targetRight); // Fade to new colors
void startBrightnessFade(uint8_t targetBrightness);            // Fade brightness only
void startFadeOff();                                           // Fade to off

// Advanced animation modes
void startFlickerMode();                               // Random brightness flicker
void startFlickerMode(uint32_t baseColor);             // Flicker with specific color
void startPulseMode();                                 // Smooth breathing pulse
void startPulseMode(uint32_t baseColor);               // Pulse with specific color
void startScannerMode();                               // K-2SO scanner effect
void startScannerMode(uint32_t scanColor);             // Scanner with specific color

// Advanced animation modes (13-LED Circle eyes)
void startIrisMode();                                  // Iris effect
void startTargetingMode();                             // Targeting crosshair
void startRingScannerMode();                           // Scanner in ring only
void startSpiralMode();                                // Spiral animation
void startFocusMode();                                 // Focus effect
void startRadarMode();                                 // Radar sweep

// Synchronized animation modes
void startHeartbeatMode();                             // Heartbeat pulse (synchronized)
void startAlarmMode();                                 // Alarm flash (synchronized)

// Animation update functions (called by main handler)
void updateFadeAnimation();                            // Update fade animation
void updateFlickerAnimation();                         // Update flicker animation
void updatePulseAnimation();                           // Update pulse animation
void updateScannerAnimation();                         // Update scanner animation
void updateIrisAnimation();                            // Update iris animation
void updateTargetingAnimation();                       // Update targeting animation
void updateRingScannerAnimation();                     // Update ring scanner
void updateSpiralAnimation();                          // Update spiral animation
void updateFocusAnimation();                           // Update focus animation
void updateRadarAnimation();                           // Update radar animation
void updateHeartbeatAnimation();                       // Update heartbeat animation
void updateAlarmAnimation();                           // Update alarm animation

// Utility functions
uint32_t interpolateColor(uint32_t startColor, uint32_t endColor, float progress);
uint32_t adjustColorBrightness(uint32_t color, float brightness);
uint8_t getRedComponent(uint32_t color);
uint8_t getGreenComponent(uint32_t color);  
uint8_t getBlueComponent(uint32_t color);
uint32_t makeColor(uint8_t r, uint8_t g, uint8_t b);

// Eye-specific utility functions
void setLeftEyeColor(uint32_t color);                  // Set left eye only
void setRightEyeColor(uint32_t color);                 // Set right eye only
void setLeftEyeBrightness(uint8_t brightness);         // Set left eye brightness
void setRightEyeBrightness(uint8_t brightness);        // Set right eye brightness

// Animation mode queries
bool isAnimationActive();                              // Check if any animation running
PixelMode getCurrentAnimationMode();                   // Get current animation mode
String getAnimationModeName();                         // Get animation mode as string

// Preset color functions
void setEyesWhite();                                   // Set both eyes to white
void setEyesRed();                                     // Set both eyes to red  
void setEyesBlue();                                    // Set both eyes to blue
void setEyesGreen();                                   // Set both eyes to green
void setEyesOff();                                     // Turn off both eyes
void setEyesK2SOBlue();                                // Signature K-2SO blue color

// Special effects
void startAlertFlash();                                // Alert strobe effect
void startBootSequenceAnimation();                     // Boot-up light sequence
void startShutdownAnimation();                         // Shutdown fade sequence
void startErrorIndicator();                            // Error flash pattern

// Color palette functions
uint32_t getK2SOBlue();                                // Get K-2SO signature blue
uint32_t getAlertRed();                                // Get alert red color
uint32_t getScanningGreen();                           // Get scanning green color
uint32_t getIdleAmber();                               // Get idle amber color
uint32_t getIceBlue();                                 // Get ice blue color

// Eye hardware configuration functions
void setEyeHardwareVersion(EyeHardwareVersion version); // Set eye hardware version (7 or 13 LEDs)
EyeHardwareVersion getEyeHardwareVersion();            // Get current eye hardware version
uint8_t getActiveEyeLEDCount();                        // Get active LED count based on version
String getEyeHardwareVersionName();                    // Get version name as string
void updateEyeLEDCount();                              // Update active LED count based on config

#endif // K2SO_ANIMATIONS_H