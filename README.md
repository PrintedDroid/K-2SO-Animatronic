/*
================================================================================
// K-2SO Controller Status LED System Header
// Controls single WS2812 LED for system status indication
================================================================================
*/

#ifndef K2SO_STATUSLED_H
#define K2SO_STATUSLED_H

#include <Arduino.h>
#include <Adafruit_NeoPixel.h>
#include "config.h"

//========================================
// STATUS LED FUNCTION DECLARATIONS
//========================================

// Core status LED functions
void initializeStatusLED();                           // Initialize status LED system
void updateStatusLED();                               // Main status LED update loop
void resetStatusLED();                                // Reset LED to default state
void autoUpdateStatusLED();                           // Automatically determine and set status

// State management functions
void setStatusLEDState(StatusLEDState newState);      // Set specific status state
void setStatusLEDColor(uint32_t color);               // Set direct color
void setStatusLEDBrightness(uint8_t brightness);      // Set LED brightness
void statusLEDOff();                                  // Turn off status LED

// System status indicators
void statusLEDBootSequence();                         // Boot sequence animation
void statusLEDWiFiConnecting();                       // WiFi connecting indicator
void statusLEDWiFiConnected();                        // WiFi connected indicator
void statusLEDWiFiDisconnected();                     // WiFi disconnected indicator
void statusLEDError();                                // Error indication

// Mode status indicators
void statusLEDScanningMode();                         // Scanning mode pulse
void statusLEDAlertMode();                            // Alert mode pulse
void statusLEDIdleMode();                             // Idle mode pulse

// Activity indicators (brief flashes)
void statusLEDIRActivity();                           // IR command received flash
void statusLEDServoActivity();                        // Servo movement flash
void statusLEDAudioActivity();                        // Audio playing flash

// Special mode indicators
void statusLEDLearningMode();                         // IR learning mode
void statusLEDConfigMode();                           // Configuration mode
void statusLEDTestMode();                             // Test mode

// Animation control functions
void startStatusLEDPulse(uint32_t color);             // Start pulse animation
void startStatusLEDBlink(uint32_t color, unsigned long interval); // Start blink animation
void startStatusLEDFlash(uint32_t color, unsigned long duration); // Start flash
void stopStatusLEDAnimation();                        // Stop current animation

// Color utility functions
uint32_t getStatusLEDColor(StatusLEDState state);     // Get color for state
uint32_t statusColorRed();                            // Red color
uint32_t statusColorGreen();                          // Green color
uint32_t statusColorBlue();                           // Blue color
uint32_t statusColorYellow();                         // Yellow color
uint32_t statusColorPurple();                         // Purple color
uint32_t statusColorCyan();                           // Cyan color
uint32_t statusColorWhite();                          // White color
uint32_t statusColorOrange();                         // Orange color
uint32_t statusColorAmber();                          // Amber color
uint32_t statusColorIceBlue();                        // Ice blue color

// Animation utility functions
float calculatePulseIntensity(unsigned long currentTime, unsigned long period);
float calculateBlinkState(unsigned long currentTime, unsigned long interval);
uint32_t fadeColor(uint32_t color, float intensity);
uint32_t interpolateColor(uint32_t color1, uint32_t color2, float progress);

// Status checking functions
bool isStatusLEDAnimating();                          // Check if animation is active
StatusLEDState getCurrentStatusLEDState();            // Get current state
unsigned long getStatusLEDAnimationTime();           // Get animation duration

// Configuration functions
void enableStatusLED(bool enable);                    // Enable/disable status LED
bool isStatusLEDEnabled();                           // Check if enabled
void setStatusLEDConfig(uint8_t brightness, bool enabled); // Set configuration

// Debug and diagnostic functions
void statusLEDSystemTest();                          // Run LED test sequence
void statusLEDColorTest();                           // Test all colors
void statusLEDAnimationTest();                       // Test all animations
String getStatusLEDStateName(StatusLEDState state);  // Get state name as string

//========================================
// EXTERNAL REFERENCES
//========================================

// Status LED hardware object (defined in main)
extern Adafruit_NeoPixel statusLED;

// Status LED animation state (defined in main)
extern StatusLEDAnimation statusLEDAnim;

// System state variables for status indication
extern bool isAwake;
extern bool bootSequenceComplete;
extern OperatingMode operatingMode;
extern PersonalityMode currentMode;
extern bool isAudioReady;
extern ConfigData config;

#endif // K2SO_STATUSLED_H
