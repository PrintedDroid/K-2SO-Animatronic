/*
================================================================================
// K-2SO Controller Status LED System Implementation
// Controls single WS2812 LED for comprehensive system status indication
================================================================================
*/

#include <Arduino.h>
#include <math.h>
#include <WiFi.h>         // For WiFi.status() in autoUpdateStatusLED
#include "statusled.h"
#include "animations.h"   // For interpolateColor function
#include "config.h"
#include "globals.h"

//========================================
// CORE STATUS LED FUNCTIONS
//========================================

void initializeStatusLED() {
  // Initialize status LED hardware
  statusLED.begin();
  statusLED.setBrightness(config.statusLedBrightness);
  statusLED.show();
  
  // Initialize animation state
  resetStatusLED();
  
  Serial.println("- Status LED: Initialized");
  
  // Brief initialization flash
  setStatusLEDColor(statusColorBlue());
  delay(100);
  statusLEDOff();
}

void resetStatusLED() {
  // Reset animation state to defaults
  statusLEDAnim.currentState = STATUS_OFF;
  statusLEDAnim.targetState = STATUS_OFF;
  statusLEDAnim.animationStart = 0;
  statusLEDAnim.lastUpdate = 0;
  statusLEDAnim.animationProgress = 0.0;
  statusLEDAnim.currentColor = 0;
  statusLEDAnim.targetColor = 0;
  statusLEDAnim.isAnimating = false;
  statusLEDAnim.blinkState = false;
  statusLEDAnim.pulseDirection = 1;
  statusLEDAnim.flashStartTime = 0;
  statusLEDAnim.flashDuration = 0;
  
  statusLEDOff();
}

void updateStatusLED() {
  if (!config.statusLedEnabled) {
    return;
  }
  
  unsigned long currentTime = millis();
  
  // Handle flash animations (highest priority)
  if (statusLEDAnim.flashDuration > 0) {
    if (currentTime - statusLEDAnim.flashStartTime >= statusLEDAnim.flashDuration) {
      // Flash complete, return to previous state
      statusLEDAnim.flashDuration = 0;
      statusLEDAnim.currentState = statusLEDAnim.targetState;
    } else {
      // Continue flash
      return;
    }
  }
  
  // Update animations based on current state
  switch (statusLEDAnim.currentState) {
    case STATUS_OFF:
      statusLEDOff();
      break;
      
    case STATUS_BOOT:
      {
        // Blue pulsing during boot
        float intensity = calculatePulseIntensity(currentTime, 1000); // 1 second pulse
        uint32_t color = fadeColor(statusColorBlue(), intensity);
        setStatusLEDColor(color);
      }
      break;
      
    case STATUS_WIFI_CONNECTING:
      {
        // Yellow blinking
        bool blinkState = calculateBlinkState(currentTime, STATUS_BLINK_FAST);
        if (blinkState != statusLEDAnim.blinkState) {
          statusLEDAnim.blinkState = blinkState;
          if (blinkState) {
            setStatusLEDColor(statusColorYellow());
          } else {
            statusLEDOff();
          }
        }
      }
      break;
      
    case STATUS_WIFI_CONNECTED:
      setStatusLEDColor(statusColorGreen());
      break;
      
    case STATUS_WIFI_DISCONNECTED:
      setStatusLEDColor(statusColorRed());
      break;
      
    case STATUS_MODE_SCANNING:
      {
        // Ice blue pulsing
        float intensity = calculatePulseIntensity(currentTime, STATUS_PULSE_SPEED);
        uint32_t color = fadeColor(statusColorIceBlue(), intensity);
        setStatusLEDColor(color);
      }
      break;
      
    case STATUS_MODE_ALERT:
      {
        // Red pulsing
        float intensity = calculatePulseIntensity(currentTime, STATUS_PULSE_SPEED);
        uint32_t color = fadeColor(statusColorRed(), intensity);
        setStatusLEDColor(color);
      }
      break;
      
    case STATUS_MODE_IDLE:
      {
        // Amber pulsing
        float intensity = calculatePulseIntensity(currentTime, STATUS_PULSE_SPEED);
        uint32_t color = fadeColor(statusColorAmber(), intensity);
        setStatusLEDColor(color);
      }
      break;
      
    case STATUS_ERROR:
      {
        // Fast red blinking
        bool blinkState = calculateBlinkState(currentTime, STATUS_BLINK_FAST);
        if (blinkState != statusLEDAnim.blinkState) {
          statusLEDAnim.blinkState = blinkState;
          if (blinkState) {
            setStatusLEDColor(statusColorRed());
          } else {
            statusLEDOff();
          }
        }
      }
      break;
      
    case STATUS_LEARNING_MODE:
      {
        // Purple blinking
        bool blinkState = calculateBlinkState(currentTime, STATUS_BLINK_SLOW);
        if (blinkState != statusLEDAnim.blinkState) {
          statusLEDAnim.blinkState = blinkState;
          if (blinkState) {
            setStatusLEDColor(statusColorPurple());
          } else {
            statusLEDOff();
          }
        }
      }
      break;
      
    case STATUS_CONFIG_MODE:
      {
        // Cyan pulsing
        float intensity = calculatePulseIntensity(currentTime, STATUS_PULSE_SPEED);
        uint32_t color = fadeColor(statusColorCyan(), intensity);
        setStatusLEDColor(color);
      }
      break;
      
    default:
      statusLEDOff();
      break;
  }
  
  statusLEDAnim.lastUpdate = currentTime;
}

//========================================
// STATE MANAGEMENT FUNCTIONS
//========================================

void setStatusLEDState(StatusLEDState newState) {
  if (!config.statusLedEnabled) {
    return;
  }
  
  // Don't interrupt flash animations unless it's an error or higher priority
  if (statusLEDAnim.flashDuration > 0 && newState != STATUS_ERROR) {
    statusLEDAnim.targetState = newState;
    return;
  }
  
  statusLEDAnim.currentState = newState;
  statusLEDAnim.targetState = newState;
  statusLEDAnim.animationStart = millis();
  statusLEDAnim.isAnimating = true;
  
//  Serial.printf("Status LED: %s\n", getStatusLEDStateName(newState).c_str());
}

void setStatusLEDColor(uint32_t color) {
  if (!config.statusLedEnabled) {
    return;
  }
  
  statusLEDAnim.currentColor = color;
  statusLED.setPixelColor(0, color);
  statusLED.show();
}

void setStatusLEDBrightness(uint8_t brightness) {
  config.statusLedBrightness = brightness;
  statusLED.setBrightness(brightness);
  statusLED.show();
}

void statusLEDOff() {
  statusLED.setPixelColor(0, 0);
  statusLED.show();
  statusLEDAnim.currentColor = 0;
}

//========================================
// SYSTEM STATUS INDICATORS
//========================================

void statusLEDBootSequence() {
  setStatusLEDState(STATUS_BOOT);
}

void statusLEDWiFiConnecting() {
  setStatusLEDState(STATUS_WIFI_CONNECTING);
}

void statusLEDWiFiConnected() {
  setStatusLEDState(STATUS_WIFI_CONNECTED);
}

void statusLEDWiFiDisconnected() {
  setStatusLEDState(STATUS_WIFI_DISCONNECTED);
}

void statusLEDError() {
  setStatusLEDState(STATUS_ERROR);
}

//========================================
// MODE STATUS INDICATORS
//========================================

void statusLEDScanningMode() {
  setStatusLEDState(STATUS_MODE_SCANNING);
}

void statusLEDAlertMode() {
  setStatusLEDState(STATUS_MODE_ALERT);
}

void statusLEDIdleMode() {
  setStatusLEDState(STATUS_MODE_IDLE);
}

//========================================
// ACTIVITY INDICATORS (BRIEF FLASHES)
//========================================

void statusLEDIRActivity() {
  startStatusLEDFlash(statusColorWhite(), STATUS_FLASH_DURATION);
}

void statusLEDServoActivity() {
  startStatusLEDFlash(statusColorBlue(), STATUS_FLASH_DURATION);
}

void statusLEDAudioActivity() {
  startStatusLEDFlash(statusColorGreen(), STATUS_FLASH_DURATION);
}

//========================================
// SPECIAL MODE INDICATORS
//========================================

void statusLEDLearningMode() {
  setStatusLEDState(STATUS_LEARNING_MODE);
}

void statusLEDConfigMode() {
  setStatusLEDState(STATUS_CONFIG_MODE);
}

void statusLEDTestMode() {
  // Cycle through colors for test mode
  static int testColorIndex = 0;
  uint32_t testColors[] = {
    statusColorRed(), statusColorGreen(), statusColorBlue(),
    statusColorYellow(), statusColorPurple(), statusColorCyan(),
    statusColorWhite(), statusColorOrange()
  };
  
  setStatusLEDColor(testColors[testColorIndex]);
  testColorIndex = (testColorIndex + 1) % 8;
}

//========================================
// ANIMATION CONTROL FUNCTIONS
//========================================

void startStatusLEDPulse(uint32_t color) {
  statusLEDAnim.targetColor = color;
  statusLEDAnim.animationStart = millis();
  statusLEDAnim.isAnimating = true;
  statusLEDAnim.pulseDirection = 1;
}

void startStatusLEDBlink(uint32_t color, unsigned long interval) {
  statusLEDAnim.targetColor = color;
  statusLEDAnim.animationStart = millis();
  statusLEDAnim.isAnimating = true;
  statusLEDAnim.blinkState = false;
}

void startStatusLEDFlash(uint32_t color, unsigned long duration) {
  statusLEDAnim.flashStartTime = millis();
  statusLEDAnim.flashDuration = duration;
  setStatusLEDColor(color);
}

void stopStatusLEDAnimation() {
  statusLEDAnim.isAnimating = false;
  statusLEDAnim.flashDuration = 0;
}

//========================================
// INTERNAL ANIMATION UPDATE FUNCTIONS
//========================================

static void updateBootAnimation(unsigned long currentTime) {
  // Blue pulsing during boot
  float intensity = calculatePulseIntensity(currentTime, 1000); // 1 second pulse
  uint32_t color = fadeColor(statusColorBlue(), intensity);
  setStatusLEDColor(color);
}

static void updatePulseAnimation(unsigned long currentTime, uint32_t baseColor) {
  float intensity = calculatePulseIntensity(currentTime, STATUS_PULSE_SPEED);
  uint32_t color = fadeColor(baseColor, intensity);
  setStatusLEDColor(color);
}

static void updateBlinkAnimation(unsigned long currentTime, uint32_t color, unsigned long interval) {
  bool blinkState = calculateBlinkState(currentTime, interval);
  if (blinkState != statusLEDAnim.blinkState) {
    statusLEDAnim.blinkState = blinkState;
    if (blinkState) {
      setStatusLEDColor(color);
    } else {
      statusLEDOff();
    }
  }
}

//========================================
// COLOR UTILITY FUNCTIONS
//========================================

uint32_t getStatusLEDColor(StatusLEDState state) {
  switch (state) {
    case STATUS_WIFI_CONNECTED:      return statusColorGreen();
    case STATUS_WIFI_DISCONNECTED:  return statusColorRed();
    case STATUS_WIFI_CONNECTING:    return statusColorYellow();
    case STATUS_BOOT:               return statusColorBlue();
    case STATUS_MODE_SCANNING:      return statusColorIceBlue();
    case STATUS_MODE_ALERT:         return statusColorRed();
    case STATUS_MODE_IDLE:          return statusColorAmber();
    case STATUS_ERROR:              return statusColorRed();
    case STATUS_LEARNING_MODE:      return statusColorPurple();
    case STATUS_CONFIG_MODE:        return statusColorCyan();
    case STATUS_IR_ACTIVITY:        return statusColorWhite();
    case STATUS_SERVO_ACTIVITY:     return statusColorBlue();
    case STATUS_AUDIO_ACTIVITY:     return statusColorGreen();
    default:                        return 0; // Off
  }
}

uint32_t statusColorRed() {
  return statusLED.Color(255, 0, 0);
}

uint32_t statusColorGreen() {
  return statusLED.Color(0, 255, 0);
}

uint32_t statusColorBlue() {
  return statusLED.Color(0, 0, 255);
}

uint32_t statusColorYellow() {
  return statusLED.Color(255, 255, 0);
}

uint32_t statusColorPurple() {
  return statusLED.Color(128, 0, 128);
}

uint32_t statusColorCyan() {
  return statusLED.Color(0, 255, 255);
}

uint32_t statusColorWhite() {
  return statusLED.Color(255, 255, 255);
}

uint32_t statusColorOrange() {
  return statusLED.Color(255, 165, 0);
}

uint32_t statusColorAmber() {
  return statusLED.Color(255, 191, 0);
}

uint32_t statusColorIceBlue() {
  return statusLED.Color(80, 150, 255);
}

//========================================
// ANIMATION UTILITY FUNCTIONS
//========================================

float calculatePulseIntensity(unsigned long currentTime, unsigned long period) {
  float phase = (float)((currentTime) % period) / (float)period;
  return (sin(phase * 2.0 * PI) + 1.0) / 2.0; // 0.0 to 1.0
}

float calculateBlinkState(unsigned long currentTime, unsigned long interval) {
  return ((currentTime / interval) % 2) == 0;
}

uint32_t fadeColor(uint32_t color, float intensity) {
  // Extract RGB components
  uint8_t r = (color >> 16) & 0xFF;
  uint8_t g = (color >> 8) & 0xFF;
  uint8_t b = color & 0xFF;
  
  // Apply intensity
  r = (uint8_t)(r * intensity);
  g = (uint8_t)(g * intensity);
  b = (uint8_t)(b * intensity);
  
  return statusLED.Color(r, g, b);
}

// Note: interpolateColor is already defined in animations.cpp, so we don't redefine it here

//========================================
// STATUS CHECKING FUNCTIONS
//========================================

bool isStatusLEDAnimating() {
  return statusLEDAnim.isAnimating || statusLEDAnim.flashDuration > 0;
}

StatusLEDState getCurrentStatusLEDState() {
  return statusLEDAnim.currentState;
}

unsigned long getStatusLEDAnimationTime() {
  return millis() - statusLEDAnim.animationStart;
}

//========================================
// CONFIGURATION FUNCTIONS
//========================================

void enableStatusLED(bool enable) {
  config.statusLedEnabled = enable;
  if (!enable) {
    statusLEDOff();
  }
}

bool isStatusLEDEnabled() {
  return config.statusLedEnabled;
}

void setStatusLEDConfig(uint8_t brightness, bool enabled) {
  config.statusLedBrightness = brightness;
  config.statusLedEnabled = enabled;
  statusLED.setBrightness(brightness);
  
  if (!enabled) {
    statusLEDOff();
  }
}

//========================================
// DEBUG AND DIAGNOSTIC FUNCTIONS
//========================================

void statusLEDSystemTest() {
  Serial.println("Status LED System Test");
  
  // Test all basic colors
  uint32_t colors[] = {
    statusColorRed(), statusColorGreen(), statusColorBlue(),
    statusColorYellow(), statusColorPurple(), statusColorCyan(),
    statusColorWhite(), statusColorOrange(), statusColorAmber(),
    statusColorIceBlue()
  };
  
  const char* colorNames[] = {
    "Red", "Green", "Blue", "Yellow", "Purple", 
    "Cyan", "White", "Orange", "Amber", "Ice Blue"
  };
  
  for (int i = 0; i < 10; i++) {
    Serial.printf("- Testing %s\n", colorNames[i]);
    setStatusLEDColor(colors[i]);
    delay(500);
  }
  
  statusLEDOff();
  Serial.println("Status LED test complete");
}

void statusLEDColorTest() {
  Serial.println("Status LED Color Test - cycling through all colors");
  statusLEDSystemTest();
}

void statusLEDAnimationTest() {
  Serial.println("Status LED Animation Test");
  
  // Test pulse animation
  Serial.println("- Testing pulse animation");
  startStatusLEDPulse(statusColorBlue());
  delay(3000);
  
  // Test blink animation
  Serial.println("- Testing blink animation");
  startStatusLEDBlink(statusColorRed(), 250);
  delay(3000);
  
  // Test flash
  Serial.println("- Testing flash");
  for (int i = 0; i < 5; i++) {
    startStatusLEDFlash(statusColorWhite(), 100);
    delay(300);
  }
  
  statusLEDOff();
  Serial.println("Animation test complete");
}

String getStatusLEDStateName(StatusLEDState state) {
  switch (state) {
    case STATUS_OFF:                return "OFF";
    case STATUS_BOOT:               return "BOOT";
    case STATUS_WIFI_CONNECTING:    return "WIFI_CONNECTING";
    case STATUS_WIFI_CONNECTED:     return "WIFI_CONNECTED";
    case STATUS_WIFI_DISCONNECTED:  return "WIFI_DISCONNECTED";
    case STATUS_MODE_SCANNING:      return "MODE_SCANNING";
    case STATUS_MODE_ALERT:         return "MODE_ALERT";
    case STATUS_MODE_IDLE:          return "MODE_IDLE";
    case STATUS_IR_ACTIVITY:        return "IR_ACTIVITY";
    case STATUS_SERVO_ACTIVITY:     return "SERVO_ACTIVITY";
    case STATUS_AUDIO_ACTIVITY:     return "AUDIO_ACTIVITY";
    case STATUS_ERROR:              return "ERROR";
    case STATUS_LEARNING_MODE:      return "LEARNING_MODE";
    case STATUS_CONFIG_MODE:        return "CONFIG_MODE";
    default:                        return "UNKNOWN";
  }
}

//========================================
// AUTO STATUS UPDATE FUNCTIONS
//========================================

void autoUpdateStatusLED() {
  // Automatically determine status LED state based on system conditions
  static unsigned long lastAutoUpdate = 0;
  unsigned long currentTime = millis();
  
  if (currentTime - lastAutoUpdate < 1000) { // Update every second
    return;
  }
  lastAutoUpdate = currentTime;
  
  // Priority order: Error > Learning > Boot > WiFi > Mode
  
  // Check for error conditions
  if (!isAudioReady && bootSequenceComplete) {
    setStatusLEDState(STATUS_ERROR);
    return;
  }
  
  // Check for special operating modes
  if (operatingMode == MODE_IR_LEARNING) {
    setStatusLEDState(STATUS_LEARNING_MODE);
    return;
  }
  
  if (operatingMode == MODE_SETUP_WIZARD || operatingMode == MODE_MONITOR) {
    setStatusLEDState(STATUS_CONFIG_MODE);
    return;
  }
  
  // Check boot sequence
  if (!bootSequenceComplete) {
    setStatusLEDState(STATUS_BOOT);
    return;
  }
  
  // Check WiFi status
  if (WiFi.status() == WL_CONNECTED) {
    if (!wifiWasConnected) {
      wifiWasConnected = true;
      setStatusLEDState(STATUS_WIFI_CONNECTED);
      delay(2000); // Show connected state for 2 seconds
    }
  } else {
    if (wifiWasConnected) {
      wifiWasConnected = false;
      setStatusLEDState(STATUS_WIFI_DISCONNECTED);
      return;
    }
  }
  
  // Show current personality mode
  switch (currentMode) {
    case MODE_SCANNING:
      setStatusLEDState(STATUS_MODE_SCANNING);
      break;
    case MODE_ALERT:
      setStatusLEDState(STATUS_MODE_ALERT);
      break;
    case MODE_IDLE:
      setStatusLEDState(STATUS_MODE_IDLE);
      break;
  }
}