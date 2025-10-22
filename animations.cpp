/*
================================================================================
// K-2SO Controller Animation System Implementation
// Advanced LED animation functions for dual eye control
// FIXED VERSION - All printf format specifiers corrected
================================================================================
*/

#include <Arduino.h>      // System zuerst
#include <math.h>         // System library
#include "animations.h"   // DANN custom
#include "config.h"

//========================================
// GLOBAL ANIMATION STATE
//========================================
AnimationState animState;

//========================================
// CORE ANIMATION FUNCTIONS
//========================================

void initializeAnimations() {
  // Initialize animation state
  resetAnimationState();
  
  // Set initial eye state
  animState.baseColorLeft = leftEye.Color(255, 255, 255);  // White
  animState.baseColorRight = rightEye.Color(255, 255, 255); // White  
  animState.baseBrightness = currentBrightness;
  
  Serial.println("Animation system initialized");
}

void resetAnimationState() {
  memset(&animState, 0, sizeof(animState));
  animState.currentMode = SOLID_COLOR;
  animState.baseBrightness = DEFAULT_BRIGHTNESS;
  animState.pulseDirection = true;
  animState.scannerDirection = true;
  animState.flickerIntensityLeft = 1.0;
  animState.flickerIntensityRight = 1.0;
}

void handlePixelAnimations() {
  if (!animState.animationActive && currentPixelMode == SOLID_COLOR) {
    return; // No animation needed
  }
  
  switch (currentPixelMode) {
    case SOLID_COLOR:
      // Static color, no animation needed
      break;
      
    case FADE_COLOR:
    case FADE_OFF:
      updateFadeAnimation();
      break;
      
    case FLICKER:
      updateFlickerAnimation();
      break;
      
    case PULSE:
      updatePulseAnimation();
      break;

    case SCANNER:
      updateScannerAnimation();
      break;
      
    default:
      Serial.println("Warning: Unknown animation mode");
      currentPixelMode = SOLID_COLOR;
      break;
  }
}

void stopAllAnimations() {
  currentPixelMode = SOLID_COLOR;
  animState.animationActive = false;
  resetAnimationState();
  Serial.println("All animations stopped");
}

//========================================
// COLOR AND BRIGHTNESS CONTROL
//========================================

void setEyeColor(uint32_t leftColor, uint32_t rightColor) {
  leftEyeCurrentColor = leftColor;
  rightEyeCurrentColor = rightColor;
  animState.baseColorLeft = leftColor;
  animState.baseColorRight = rightColor;

  // Clear all LEDs first
  leftEye.clear();
  rightEye.clear();

  // Set only active LEDs based on eye version
  for (int i = 0; i < activeEyeLEDCount; i++) {
    leftEye.setPixelColor(i, leftColor);
    rightEye.setPixelColor(i, rightColor);
  }

  leftEye.show();
  rightEye.show();

  currentPixelMode = SOLID_COLOR;
  animState.animationActive = false;
}

void setEyeBrightness(uint8_t brightness) {
  currentBrightness = brightness;
  animState.baseBrightness = brightness;
  
  leftEye.setBrightness(brightness);
  rightEye.setBrightness(brightness);
  leftEye.show();
  rightEye.show();
}

void setEyeColorAndBrightness(uint32_t leftColor, uint32_t rightColor, uint8_t brightness) {
  setEyeBrightness(brightness);
  setEyeColor(leftColor, rightColor);
}

//========================================
// BASIC ANIMATION MODES
//========================================

void startSolidColor(uint32_t leftColor, uint32_t rightColor) {
  setEyeColor(leftColor, rightColor);
}

void startColorFade(uint32_t targetLeft, uint32_t targetRight) {
  // Store current colors as start colors
  animState.fadeStartColorLeft = leftEyeCurrentColor;
  animState.fadeStartColorRight = rightEyeCurrentColor;
  animState.fadeTargetColorLeft = targetLeft;
  animState.fadeTargetColorRight = targetRight;
  animState.fadeStartTime = millis();
  
  currentPixelMode = FADE_COLOR;
  animState.animationActive = true;
  animState.currentMode = FADE_COLOR;
  
  Serial.printf("Starting fade: L(0x%06X->0x%06X) R(0x%06X->0x%06X)\n", 
              (unsigned int)animState.fadeStartColorLeft, (unsigned int)targetLeft,
              (unsigned int)animState.fadeStartColorRight, (unsigned int)targetRight);
}

void startBrightnessFade(uint8_t targetBrightness) {
  // Fade brightness while keeping current colors
  startColorFade(leftEyeCurrentColor, rightEyeCurrentColor);
  // The brightness change will be handled in the update function
}

void startFadeOff() {
  startColorFade(leftEye.Color(0, 0, 0), rightEye.Color(0, 0, 0));
  currentPixelMode = FADE_OFF;
  Serial.println("Starting fade to off");
}

//========================================
// ADVANCED ANIMATION MODES
//========================================

void startFlickerMode() {
  startFlickerMode(animState.baseColorLeft); // Use current left eye color
}

void startFlickerMode(uint32_t baseColor) {
  animState.baseColorLeft = baseColor;
  animState.baseColorRight = baseColor;
  animState.lastFlickerUpdate = millis();
  animState.flickerIntensityLeft = 1.0;
  animState.flickerIntensityRight = 1.0;
  
  currentPixelMode = FLICKER;
  animState.animationActive = true;
  animState.currentMode = FLICKER;
  
  Serial.println("Starting flicker animation");
}

void startPulseMode() {
  startPulseMode(animState.baseColorLeft); // Use current left eye color
}

void startPulseMode(uint32_t baseColor) {
  animState.baseColorLeft = baseColor;
  animState.baseColorRight = baseColor;
  animState.pulseStartTime = millis();
  animState.pulseDirection = true;
  
  currentPixelMode = PULSE;
  animState.animationActive = true;
  animState.currentMode = PULSE;
  
  Serial.println("Starting pulse animation");
}

void startScannerMode() {
  startScannerMode(getK2SOBlue());
}

void startScannerMode(uint32_t scanColor) {
  // Scanner mode creates a sweeping effect across both eyes
  animState.baseColorLeft = scanColor;
  animState.baseColorRight = scanColor;
  animState.lastScannerUpdate = millis();
  animState.scannerPosition = 0;
  animState.scannerDirection = true;
  
  currentPixelMode = SCANNER; // Use pulse for scanner base
  animState.animationActive = true;
  animState.currentMode = SCANNER;
  
  Serial.println("Starting scanner animation");
}

//========================================
// ANIMATION UPDATE FUNCTIONS
//========================================

void updateFadeAnimation() {
  unsigned long currentTime = millis();
  unsigned long elapsed = currentTime - animState.fadeStartTime;
  
  if (elapsed >= FADE_DURATION_MS) {
    // Fade complete
    leftEyeCurrentColor = animState.fadeTargetColorLeft;
    rightEyeCurrentColor = animState.fadeTargetColorRight;
    setEyeColor(leftEyeCurrentColor, rightEyeCurrentColor);
    
    currentPixelMode = SOLID_COLOR;
    animState.animationActive = false;
    
    Serial.println("Fade animation complete");
    return;
  }
  
  // Calculate fade progress (0.0 to 1.0)
  float progress = (float)elapsed / (float)FADE_DURATION_MS;
  
  // Smooth the progress using easing function (optional)
  progress = progress * progress * (3.0f - 2.0f * progress); // Smoothstep
  
  // Interpolate colors
  uint32_t currentLeft = interpolateColor(animState.fadeStartColorLeft, 
                                          animState.fadeTargetColorLeft, progress);
  uint32_t currentRight = interpolateColor(animState.fadeStartColorRight, 
                                           animState.fadeTargetColorRight, progress);
  
  // Update display
  leftEye.fill(currentLeft);
  rightEye.fill(currentRight);
  leftEye.show();
  rightEye.show();
  
  // Update current color tracking
  leftEyeCurrentColor = currentLeft;
  rightEyeCurrentColor = currentRight;
}

void updateFlickerAnimation() {
  unsigned long currentTime = millis();
  
  if (currentTime - animState.lastFlickerUpdate >= FLICKER_UPDATE_INTERVAL_MS) {
    animState.lastFlickerUpdate = currentTime;
    
    // Generate random flicker intensities for each eye
    animState.flickerIntensityLeft = random(FLICKER_INTENSITY_MIN * 100, 
                                            FLICKER_INTENSITY_MAX * 100) / 100.0;
    animState.flickerIntensityRight = random(FLICKER_INTENSITY_MIN * 100, 
                                             FLICKER_INTENSITY_MAX * 100) / 100.0;
    
    // Apply flicker to base colors
    uint32_t flickerLeft = adjustColorBrightness(animState.baseColorLeft, 
                                                  animState.flickerIntensityLeft);
    uint32_t flickerRight = adjustColorBrightness(animState.baseColorRight, 
                                                   animState.flickerIntensityRight);
    
    // Update display
    leftEye.fill(flickerLeft);
    rightEye.fill(flickerRight);
    leftEye.show();
    rightEye.show();
    
    // Update current color tracking
    leftEyeCurrentColor = flickerLeft;
    rightEyeCurrentColor = flickerRight;
  }
}

void updatePulseAnimation() {
  unsigned long currentTime = millis();
  unsigned long elapsed = currentTime - animState.pulseStartTime;
  
  // Calculate pulse progress (0.0 to 1.0) within cycle
  float cycleProgress = (float)(elapsed % PULSE_SPEED_MS) / (float)PULSE_SPEED_MS;
  
  // Convert to sine wave for smooth pulsing (0 to 2π)
  float sineValue = sin(cycleProgress * 2.0 * PI);
  
  // Map sine wave (-1 to 1) to brightness range
  float brightness = PULSE_MIN_BRIGHTNESS + 
                     (PULSE_MAX_BRIGHTNESS - PULSE_MIN_BRIGHTNESS) * 
                     (sineValue + 1.0) / 2.0;
  
  // Apply brightness to base colors
  uint32_t pulseLeft = adjustColorBrightness(animState.baseColorLeft, brightness);
  uint32_t pulseRight = adjustColorBrightness(animState.baseColorRight, brightness);
  
  // Update display
  leftEye.fill(pulseLeft);
  rightEye.fill(pulseRight);
  leftEye.show();
  rightEye.show();
  
  // Update current color tracking
  leftEyeCurrentColor = pulseLeft;
  rightEyeCurrentColor = pulseRight;
}

void updateScannerAnimation() {
  unsigned long currentTime = millis();
  
  if (currentTime - animState.lastScannerUpdate >= SCANNER_SPEED) {
    animState.lastScannerUpdate = currentTime;
    
    // Clear both eyes
    leftEye.clear();
    rightEye.clear();
    
    // Total pixels across both eyes
    int totalPixels = NUM_EYE_PIXELS * 2;
    
    // Create scanner beam with tail
    for (int i = 0; i < SCANNER_TAIL_LENGTH; i++) {
      int pixelIndex = animState.scannerPosition - i;
      if (pixelIndex < 0) pixelIndex += totalPixels;
      
      float intensity = 1.0 - (float)i / SCANNER_TAIL_LENGTH;
      uint32_t scanColor = adjustColorBrightness(animState.baseColorLeft, intensity);
      
      if (pixelIndex < NUM_EYE_PIXELS) {
        leftEye.setPixelColor(pixelIndex, scanColor);
      } else {
        rightEye.setPixelColor(pixelIndex - NUM_EYE_PIXELS, scanColor);
      }
    }
    
    leftEye.show();
    rightEye.show();
    
    // Update scanner position
    if (animState.scannerDirection) {
      animState.scannerPosition++;
      if (animState.scannerPosition >= totalPixels) {
        animState.scannerPosition = totalPixels - 1;
        animState.scannerDirection = false;
      }
    } else {
      animState.scannerPosition--;
      if (animState.scannerPosition < 0) {
        animState.scannerPosition = 0;
        animState.scannerDirection = true;
      }
    }
  }
}

//========================================
// UTILITY FUNCTIONS
//========================================

uint32_t interpolateColor(uint32_t startColor, uint32_t endColor, float progress) {
  // Extract RGB components from start color
  uint8_t startR = getRedComponent(startColor);
  uint8_t startG = getGreenComponent(startColor);
  uint8_t startB = getBlueComponent(startColor);
  
  // Extract RGB components from end color
  uint8_t endR = getRedComponent(endColor);
  uint8_t endG = getGreenComponent(endColor);  
  uint8_t endB = getBlueComponent(endColor);
  
  // Interpolate each component
  uint8_t interpR = startR + (endR - startR) * progress;
  uint8_t interpG = startG + (endG - startG) * progress;
  uint8_t interpB = startB + (endB - startB) * progress;
  
  return makeColor(interpR, interpG, interpB);
}

uint32_t adjustColorBrightness(uint32_t color, float brightness) {
  uint8_t r = getRedComponent(color) * brightness;
  uint8_t g = getGreenComponent(color) * brightness;
  uint8_t b = getBlueComponent(color) * brightness;
  
  return makeColor(r, g, b);
}

uint8_t getRedComponent(uint32_t color) {
  return (color >> 16) & 0xFF;
}

uint8_t getGreenComponent(uint32_t color) {
  return (color >> 8) & 0xFF;
}

uint8_t getBlueComponent(uint32_t color) {
  return color & 0xFF;
}

uint32_t makeColor(uint8_t r, uint8_t g, uint8_t b) {
  return ((uint32_t)r << 16) | ((uint32_t)g << 8) | b;
}

//========================================
// EYE-SPECIFIC UTILITY FUNCTIONS
//========================================

void setLeftEyeColor(uint32_t color) {
  leftEyeCurrentColor = color;
  leftEye.fill(color);
  leftEye.show();
}

void setRightEyeColor(uint32_t color) {
  rightEyeCurrentColor = color;
  rightEye.fill(color);
  rightEye.show();
}

void setLeftEyeBrightness(uint8_t brightness) {
  leftEye.setBrightness(brightness);
  leftEye.show();
}

void setRightEyeBrightness(uint8_t brightness) {
  rightEye.setBrightness(brightness);
  rightEye.show();
}

//========================================
// ANIMATION MODE QUERIES
//========================================

bool isAnimationActive() {
  return animState.animationActive;
}

PixelMode getCurrentAnimationMode() {
  return currentPixelMode;
}

String getAnimationModeName() {
  switch (currentPixelMode) {
    case SOLID_COLOR: return "Solid Color";
    case FADE_COLOR: return "Color Fade";
    case FADE_OFF: return "Fade Off";
    case FLICKER: return "Flicker";
    case PULSE: return "Pulse";
    default: return "Unknown";
  }
}

//========================================
// PRESET COLOR FUNCTIONS
//========================================

void setEyesWhite() {
  setEyeColor(leftEye.Color(255, 255, 255), rightEye.Color(255, 255, 255));
}

void setEyesRed() {
  setEyeColor(leftEye.Color(255, 0, 0), rightEye.Color(255, 0, 0));
}

void setEyesBlue() {
  setEyeColor(leftEye.Color(0, 0, 255), rightEye.Color(0, 0, 255));
}

void setEyesGreen() {
  setEyeColor(leftEye.Color(0, 255, 0), rightEye.Color(0, 255, 0));
}

void setEyesOff() {
  setEyeColor(leftEye.Color(0, 0, 0), rightEye.Color(0, 0, 0));
}

void setEyesK2SOBlue() {
  uint32_t k2soBlue = getK2SOBlue();
  setEyeColor(k2soBlue, k2soBlue);
}

//========================================
// SPECIAL EFFECTS
//========================================

void startAlertFlash() {
  // Rapid red flashing for alerts
  animState.baseColorLeft = getAlertRed();
  animState.baseColorRight = getAlertRed();
  startFlickerMode(getAlertRed());
  
  // Override flicker settings for faster flash
  animState.lastFlickerUpdate = millis();
  Serial.println("Alert flash activated");
}

void startBootSequenceAnimation() {
  Serial.println("Starting boot sequence animation");
  
  // Progressive blue fade-in
  setEyesOff();
  delay(500);
  startColorFade(getK2SOBlue(), getK2SOBlue());
}

void startShutdownAnimation() {
  Serial.println("Starting shutdown animation");
  startFadeOff();
}

void startErrorIndicator() {
  Serial.println("Error indicator activated");
  
  // Fast red flashing pattern
  for (int i = 0; i < 6; i++) {
    setEyesRed();
    delay(150);
    setEyesOff();
    delay(150);
  }
  
  // Return to previous state
  setEyesK2SOBlue();
}

//========================================
// COLOR PALETTE FUNCTIONS
//========================================

uint32_t getIceBlue() {
  return makeColor(150, 200, 255); // Helles eisiges Blau
}

uint32_t getK2SOBlue() {
  return makeColor(0, 100, 255); // Signature K-2SO blue
}

uint32_t getAlertRed() {
  return makeColor(255, 0, 0); // Bright red for alerts
}

uint32_t getScanningGreen() {
  return makeColor(0, 255, 100); // Soft green for scanning
}

uint32_t getIdleAmber() {
  return makeColor(255, 150, 0); // Warm amber for idle state
}

//========================================
// EYE HARDWARE VERSION FUNCTIONS
//========================================

void setEyeHardwareVersion(EyeHardwareVersion version) {
  config.eyeVersion = version;
  updateEyeLEDCount();

  Serial.printf("Eye hardware version set to: %s\n", getEyeHardwareVersionName().c_str());
  Serial.printf("Active LEDs per eye: %d\n", activeEyeLEDCount);

  // Clear eyes and restart animation
  leftEye.clear();
  rightEye.clear();
  leftEye.show();
  rightEye.show();
}

EyeHardwareVersion getEyeHardwareVersion() {
  return config.eyeVersion;
}

uint8_t getActiveEyeLEDCount() {
  return activeEyeLEDCount;
}

String getEyeHardwareVersionName() {
  switch (config.eyeVersion) {
    case EYE_VERSION_7LED:
      return "7-LED (LEDs 0-6)";
    case EYE_VERSION_13LED:
      return "13-LED (LED 0=center, LEDs 1-12=ring)";
    default:
      return "Unknown";
  }
}

void updateEyeLEDCount() {
  switch (config.eyeVersion) {
    case EYE_VERSION_7LED:
      activeEyeLEDCount = 7;
      break;
    case EYE_VERSION_13LED:
      activeEyeLEDCount = 13;
      break;
    default:
      activeEyeLEDCount = 13; // Default to 13
      break;
  }
}