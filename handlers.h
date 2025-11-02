/*
================================================================================
// K-2SO Controller Handler Function Declarations
// Contains all function prototypes for web handlers and command processors
// Clean structure without circular dependencies
================================================================================
*/

#ifndef K2SO_HANDLERS_H
#define K2SO_HANDLERS_H

#include <Arduino.h>
#include "config.h"  // For Command enum and structures
void initializeIR();
// #include "Mp3Notify.h" // <-- THIS LINE MUST BE REMOVED OR COMMENTED OUT

//========================================
// WEB SERVER HANDLERS
//========================================

// Main web interface handlers
void handleRoot();                    // Main web page
void handleWebStatus();              // System status via web
void handleNotFound();               // 404 handler

// Web authentication helper
bool checkWebAuth();                 // Check web authentication

// LED control handlers
void handleRed();                    // Set eyes to red
void handleGreen();                  // Set eyes to green
void handleBlue();                   // Set eyes to blue
void handleWhite();                  // Set eyes to white
void handleOff();                    // Turn eyes off
void handleBrightness();             // Set eye brightness
void handleFlicker();                // Activate flicker mode
void handlePulse();                  // Activate pulse mode

// Audio control handlers
void handleVolume();                 // Set volume
void handlePlaySound();              // Play specific sound file

// Servo control handlers
void handleSetServos();              // Set multiple servo positions
void handleWebMode();                // Change personality mode via web

// Detail LED web handlers
void handleDetailCount();            // Set detail LED count via web
void handleDetailBrightnessWeb();    // Set detail LED brightness via web
void handleDetailPatternWeb();       // Set detail LED pattern via web
void handleDetailEnabledWeb();       // Enable/disable detail LEDs via web

//========================================
// COMMAND PROCESSING FUNCTIONS
//========================================

// Core command processing
Command parseCommand(String cmd);
void processCommand(String fullCommand);

// Specialized command handlers
void handleServoCommand(String params);
void handleLEDCommand(String params);
void handleDetailCommand(String params);  // NEW: Detail LED control (WS2812)
void handleSoundCommand(String params);
void handleTimingCommand(String params);
void handleProfileCommand(String params);
void handleWiFiCommand(String params);    // WiFi configuration
void handleAPCommand(String params);       // Access Point configuration

//========================================
// SYSTEM STATUS AND HELP FUNCTIONS
//========================================

// Information display functions
void showHelp();                     // Display complete command help
void showStatus();                   // Display system status
void showConfiguration();            // Display current configuration
void showSavedCodes();               // Display saved IR codes

//========================================
// IR REMOTE FUNCTIONS
//========================================

// IR learning and configuration
void enterLearningMode();            // Start IR remote learning
void handleScannerMode();            // IR code scanner mode
void handleLearningMode();           // Process IR learning steps
void loadDefaultCodes();             // Load standard IR codes
void clearAllData();                 // Clear all configuration data

// IR command processing
void handleIRCommand(uint32_t code);
void executeButtonCommand(const char* buttonName);

//========================================
// CONFIGURATION MANAGEMENT
//========================================

// EEPROM functions
void loadConfiguration();            // Load config from EEPROM
void saveConfiguration();            // Save config to EEPROM
void smartSaveToEEPROM();           // Save only if changed (wear leveling)
void applyConfiguration();           // Apply loaded config to hardware
uint32_t calculateChecksum();        // Calculate config data checksum

// Backup and restore
void backupToSerial();               // Output config as hex dump
void restoreFromSerial();            // Restore config from hex input

//========================================
// SYSTEM OPERATION MODES
//========================================

// Mode handlers
void handleNormalOperation();        // Main operation loop
void handleBootSequence(unsigned long currentMillis);
void handleMonitorMode();            // Live monitoring mode
void handleTestMode();               // Hardware test mode
void handleDemoMode();               // Demo mode showing all features

// Mode management
void enterMonitorMode();             // Enter monitoring mode
void enterDemoMode();                // Enter demo mode
void runTestSequence(String params); // Start hardware test

// Sensor handling
void handleSensors();                // Process all sensor inputs

//========================================
// SERVO CONTROL FUNCTIONS
//========================================

// Servo movement and control
void updateServos(unsigned long currentMillis);
void updateServo(ServoState& servo, unsigned long currentMillis);
void setServoParameters();           // Set movement parameters based on mode

// Individual servo control
void moveServoTo(int servoIndex, int position);
void centerAllServos();              // Move all servos to center positions

//========================================
// LED AND ANIMATION FUNCTIONS
//========================================

// Basic LED control (implemented in animations.cpp)
void setEyeColor(uint32_t leftColor, uint32_t rightColor);
void setEyeBrightness(uint8_t brightness);

// Animation control (implemented in animations.cpp)
void startColorFade(uint32_t leftTarget, uint32_t rightTarget);
void startFlickerMode();
void startPulseMode();
void stopAllAnimations();

// Animation handling
void handlePixelAnimations();        // Main animation update loop

//========================================
// AUDIO SYSTEM FUNCTIONS
//========================================

// Audio control and management
void updateAudio();                  // Update audio system state
void playSound(int fileNumber);      // Play specific sound file
void playRandomSound(int folder);    // Play random sound from folder
void setVolume(uint8_t volume);      // Set audio volume

//========================================
// HARDWARE INITIALIZATION
//========================================

// Detail LED system
void initializeDetailBlinkers();     // Initialize detail LED system
void updateDetailBlinkers(unsigned long now); // Update detail LED blinkers

//========================================
// UTILITY FUNCTIONS
//========================================

// Validation functions
bool isValidServoPosition(int position);     // Validate servo position
bool isValidBrightness(uint8_t brightness);  // Validate LED brightness
bool isValidVolume(uint8_t volume);          // Validate audio volume

// String utilities
String getModeName(PersonalityMode mode);    // Get mode name as string

//========================================
// DIAGNOSTIC FUNCTIONS
//========================================

// System diagnostics
void runSystemDiagnostics();         // Complete system check
void testAllServos();                // Test all servo movement
void testAllLEDs();                  // Test all LED functions
void testAudioSystem();              // Test audio system

// Performance monitoring
void updateSystemStats();            // Update performance statistics
void logSystemEvent(const char* event); // Log system events

//========================================
// IR SYSTEM FUNCTIONS
//========================================

// IR System Management
void initializeIR();                    // Initialize IR receiver (called from main)
void stopIR();                          // Stop IR receiver
void setIREnabled(bool enabled);        // Enable/disable IR
bool checkForIRCommand(uint32_t& code); // Check for IR input

// IR Command Processing
void handleIRCommand(uint32_t code);     // Process received IR code
void executeButtonCommand(const char* buttonName); // Execute button action

#endif // K2SO_HANDLERS_H