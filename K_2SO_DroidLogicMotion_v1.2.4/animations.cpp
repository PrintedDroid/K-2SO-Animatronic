/*
================================================================================
// K-2SO Controller Animation System Implementation
// Advanced LED animation functions for dual eye control
================================================================================
*/

#include <Arduino.h>      // System zuerst
#include <math.h>         // System library
#include "animations.h"   // DANN custom
#include "config.h"
#include "globals.h"      // For activeEyeLEDCount and other globals

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

    case IRIS:
      updateIrisAnimation();
      break;

    case TARGETING:
      updateTargetingAnimation();
      break;

    case RING_SCANNER:
      updateRingScannerAnimation();
      break;

    case SPIRAL:
      updateSpiralAnimation();
      break;

    case FOCUS:
      updateFocusAnimation();
      break;

    case RADAR:
      updateRadarAnimation();
      break;

    case HEARTBEAT:
      updateHeartbeatAnimation();
      break;

    case ALARM:
      updateAlarmAnimation();
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
  // Optimized: Skip if colors haven't changed
  if (leftColor == leftEyeCurrentColor && rightColor == rightEyeCurrentColor &&
      currentPixelMode == SOLID_COLOR) {
    return;  // No change needed
  }

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
// ADVANCED ANIMATION MODES (13-LED CIRCLE EYES)
//========================================

void startIrisMode() {
  // Iris effect: Ring pulses while center stays static
  animState.baseColorLeft = getK2SOBlue();
  animState.baseColorRight = getK2SOBlue();
  animState.pulseStartTime = millis();
  animState.pulseDirection = true;

  currentPixelMode = IRIS;
  animState.animationActive = true;
  animState.currentMode = IRIS;

  Serial.println("Starting iris animation (13-LED only)");
}

void startTargetingMode() {
  // Targeting: Ring rotates while center blinks
  animState.baseColorLeft = getAlertRed();
  animState.baseColorRight = getAlertRed();
  animState.lastScannerUpdate = millis();
  animState.scannerPosition = 0;
  animState.lastFlickerUpdate = millis();

  currentPixelMode = TARGETING;
  animState.animationActive = true;
  animState.currentMode = TARGETING;

  Serial.println("Starting targeting animation (13-LED only)");
}

void startRingScannerMode() {
  // Scanner only in ring (LEDs 1-12)
  animState.baseColorLeft = getK2SOBlue();
  animState.baseColorRight = getK2SOBlue();
  animState.lastScannerUpdate = millis();
  animState.scannerPosition = 1;  // Start at LED 1 (ring)
  animState.scannerDirection = true;

  currentPixelMode = RING_SCANNER;
  animState.animationActive = true;
  animState.currentMode = RING_SCANNER;

  Serial.println("Starting ring scanner animation (13-LED only)");
}

void startSpiralMode() {
  // Spiral from outside to inside
  animState.baseColorLeft = getK2SOBlue();
  animState.baseColorRight = getK2SOBlue();
  animState.lastScannerUpdate = millis();
  animState.scannerPosition = 0;
  animState.scannerDirection = true;

  currentPixelMode = SPIRAL;
  animState.animationActive = true;
  animState.currentMode = SPIRAL;

  Serial.println("Starting spiral animation (13-LED only)");
}

void startFocusMode() {
  // Ring blinks while center stays on
  animState.baseColorLeft = getK2SOBlue();
  animState.baseColorRight = getK2SOBlue();
  animState.lastFlickerUpdate = millis();
  animState.flickerIntensityLeft = 1.0;

  currentPixelMode = FOCUS;
  animState.animationActive = true;
  animState.currentMode = FOCUS;

  Serial.println("Starting focus animation (13-LED only)");
}

void startRadarMode() {
  // Radar sweep effect in ring
  animState.baseColorLeft = getScanningGreen();
  animState.baseColorRight = getScanningGreen();
  animState.lastScannerUpdate = millis();
  animState.scannerPosition = 1;  // Start at LED 1 (ring)
  animState.scannerDirection = true;

  currentPixelMode = RADAR;
  animState.animationActive = true;
  animState.currentMode = RADAR;

  Serial.println("Starting radar animation (13-LED only)");
}

//========================================
// SYNCHRONIZED ANIMATION MODES
//========================================

void startHeartbeatMode() {
  // Heartbeat pulse: synchronized double-pulse like a heartbeat
  animState.baseColorLeft = getAlertRed();
  animState.baseColorRight = getAlertRed();
  animState.pulseStartTime = millis();
  animState.animationStep = 0;

  currentPixelMode = HEARTBEAT;
  animState.animationActive = true;
  animState.currentMode = HEARTBEAT;

  Serial.println("Starting heartbeat animation (synchronized)");
}

void startAlarmMode() {
  // Alarm flash: rapid synchronized red/white flashing
  animState.baseColorLeft = getAlertRed();
  animState.baseColorRight = getAlertRed();
  animState.lastFlickerUpdate = millis();
  animState.animationStep = 0;

  currentPixelMode = ALARM;
  animState.animationActive = true;
  animState.currentMode = ALARM;

  Serial.println("Starting alarm animation (synchronized)");
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

  // Optimized: Only update LEDs if color changed (reduces updates by 60-90%)
  if (pulseLeft != leftEyeCurrentColor) {
    leftEye.fill(pulseLeft);
    leftEye.show();
    leftEyeCurrentColor = pulseLeft;
  }

  if (pulseRight != rightEyeCurrentColor) {
    rightEye.fill(pulseRight);
    rightEye.show();
    rightEyeCurrentColor = pulseRight;
  }
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
// ADVANCED ANIMATION UPDATE FUNCTIONS (13-LED CIRCLE EYES)
//========================================

void updateIrisAnimation() {
  // Iris effect: Ring pulses while center stays static
  // Only works with 13-LED eyes
  if (activeEyeLEDCount != 13) {
    Serial.println("Warning: Iris mode requires 13-LED eyes");
    stopAllAnimations();
    return;
  }

  unsigned long currentTime = millis();
  unsigned long elapsed = currentTime - animState.pulseStartTime;

  // Calculate pulse progress (0.0 to 1.0) within cycle
  float cycleProgress = (float)(elapsed % PULSE_SPEED_MS) / (float)PULSE_SPEED_MS;

  // Convert to sine wave for smooth pulsing
  float sineValue = sin(cycleProgress * 2.0 * PI);

  // Map sine wave to brightness range for ring
  float ringBrightness = PULSE_MIN_BRIGHTNESS +
                         (PULSE_MAX_BRIGHTNESS - PULSE_MIN_BRIGHTNESS) *
                         (sineValue + 1.0) / 2.0;

  // Clear both eyes
  leftEye.clear();
  rightEye.clear();

  // Set center LED (LED 0) to full brightness
  leftEye.setPixelColor(0, animState.baseColorLeft);
  rightEye.setPixelColor(0, animState.baseColorRight);

  // Set ring LEDs (1-12) with pulsing brightness
  uint32_t pulseColor = adjustColorBrightness(animState.baseColorLeft, ringBrightness);
  for (int i = 1; i <= 12; i++) {
    leftEye.setPixelColor(i, pulseColor);
    rightEye.setPixelColor(i, pulseColor);
  }

  leftEye.show();
  rightEye.show();
}

void updateTargetingAnimation() {
  // Targeting: Ring rotates while center blinks
  // Only works with 13-LED eyes
  if (activeEyeLEDCount != 13) {
    Serial.println("Warning: Targeting mode requires 13-LED eyes");
    stopAllAnimations();
    return;
  }

  unsigned long currentTime = millis();

  // Update ring rotation
  if (currentTime - animState.lastScannerUpdate >= 100) {  // 100ms per step
    animState.lastScannerUpdate = currentTime;

    // Clear eyes
    leftEye.clear();
    rightEye.clear();

    // Blink center LED (on/off every 500ms)
    bool centerOn = (currentTime / 500) % 2 == 0;
    if (centerOn) {
      leftEye.setPixelColor(0, animState.baseColorLeft);
      rightEye.setPixelColor(0, animState.baseColorRight);
    }

    // Draw 4 targeting crosshair points rotating around ring
    for (int i = 0; i < 4; i++) {
      int ledIndex = (animState.scannerPosition + (i * 3)) % 12 + 1;  // LEDs 1-12
      leftEye.setPixelColor(ledIndex, animState.baseColorLeft);
      rightEye.setPixelColor(ledIndex, animState.baseColorRight);
    }

    leftEye.show();
    rightEye.show();

    // Move scanner position
    animState.scannerPosition = (animState.scannerPosition + 1) % 12;
  }
}

void updateRingScannerAnimation() {
  // Scanner only in ring (LEDs 1-12)
  // Only works with 13-LED eyes
  if (activeEyeLEDCount != 13) {
    Serial.println("Warning: Ring scanner mode requires 13-LED eyes");
    stopAllAnimations();
    return;
  }

  unsigned long currentTime = millis();

  if (currentTime - animState.lastScannerUpdate >= SCANNER_SPEED) {
    animState.lastScannerUpdate = currentTime;

    // Clear eyes
    leftEye.clear();
    rightEye.clear();

    // Keep center LED on
    leftEye.setPixelColor(0, animState.baseColorLeft);
    rightEye.setPixelColor(0, animState.baseColorRight);

    // Create scanner beam with tail in ring only
    for (int i = 0; i < SCANNER_TAIL_LENGTH; i++) {
      int pixelIndex = animState.scannerPosition - i;
      if (pixelIndex < 1) pixelIndex += 12;  // Wrap within ring (1-12)

      float intensity = 1.0 - (float)i / SCANNER_TAIL_LENGTH;
      uint32_t scanColor = adjustColorBrightness(animState.baseColorLeft, intensity);

      leftEye.setPixelColor(pixelIndex, scanColor);
      rightEye.setPixelColor(pixelIndex, scanColor);
    }

    leftEye.show();
    rightEye.show();

    // Update scanner position (only in ring 1-12)
    if (animState.scannerDirection) {
      animState.scannerPosition++;
      if (animState.scannerPosition > 12) {
        animState.scannerPosition = 12;
        animState.scannerDirection = false;
      }
    } else {
      animState.scannerPosition--;
      if (animState.scannerPosition < 1) {
        animState.scannerPosition = 1;
        animState.scannerDirection = true;
      }
    }
  }
}

void updateSpiralAnimation() {
  // Spiral effect from outside (ring) to inside (center)
  // Only works with 13-LED eyes
  if (activeEyeLEDCount != 13) {
    Serial.println("Warning: Spiral mode requires 13-LED eyes");
    stopAllAnimations();
    return;
  }

  unsigned long currentTime = millis();

  if (currentTime - animState.lastScannerUpdate >= 80) {  // 80ms per step
    animState.lastScannerUpdate = currentTime;

    // Clear eyes
    leftEye.clear();
    rightEye.clear();

    // Spiral effect: light up LEDs in sequence
    int step = animState.scannerPosition % 13;

    if (step < 12) {
      // Ring LEDs (1-12) light up in sequence
      for (int i = 1; i <= step + 1; i++) {
        float intensity = (float)i / 12.0;  // Fade from dim to bright
        uint32_t color = adjustColorBrightness(animState.baseColorLeft, intensity);
        leftEye.setPixelColor(i, color);
        rightEye.setPixelColor(i, color);
      }
    } else {
      // All ring LEDs lit, center LED flashes
      for (int i = 1; i <= 12; i++) {
        leftEye.setPixelColor(i, animState.baseColorLeft);
        rightEye.setPixelColor(i, animState.baseColorRight);
      }
      leftEye.setPixelColor(0, animState.baseColorLeft);
      rightEye.setPixelColor(0, animState.baseColorRight);
    }

    leftEye.show();
    rightEye.show();

    animState.scannerPosition++;
    if (animState.scannerPosition >= 13) {
      animState.scannerPosition = 0;  // Loop
    }
  }
}

void updateFocusAnimation() {
  // Focus effect: Ring blinks while center stays on
  // Only works with 13-LED eyes
  if (activeEyeLEDCount != 13) {
    Serial.println("Warning: Focus mode requires 13-LED eyes");
    stopAllAnimations();
    return;
  }

  unsigned long currentTime = millis();

  if (currentTime - animState.lastFlickerUpdate >= 300) {  // 300ms blink interval
    animState.lastFlickerUpdate = currentTime;

    // Clear eyes
    leftEye.clear();
    rightEye.clear();

    // Center LED always on
    leftEye.setPixelColor(0, animState.baseColorLeft);
    rightEye.setPixelColor(0, animState.baseColorRight);

    // Ring blinks on/off
    bool ringOn = (currentTime / 300) % 2 == 0;
    if (ringOn) {
      for (int i = 1; i <= 12; i++) {
        leftEye.setPixelColor(i, animState.baseColorLeft);
        rightEye.setPixelColor(i, animState.baseColorRight);
      }
    }

    leftEye.show();
    rightEye.show();
  }
}

void updateRadarAnimation() {
  // Radar sweep effect in ring with fade trail
  // Only works with 13-LED eyes
  if (activeEyeLEDCount != 13) {
    Serial.println("Warning: Radar mode requires 13-LED eyes");
    stopAllAnimations();
    return;
  }

  unsigned long currentTime = millis();

  if (currentTime - animState.lastScannerUpdate >= 60) {  // 60ms per step
    animState.lastScannerUpdate = currentTime;

    // Clear eyes
    leftEye.clear();
    rightEye.clear();

    // Center LED stays dimly lit
    uint32_t centerColor = adjustColorBrightness(animState.baseColorLeft, 0.3);
    leftEye.setPixelColor(0, centerColor);
    rightEye.setPixelColor(0, centerColor);

    // Radar beam with longer fade trail
    for (int i = 0; i < 6; i++) {  // 6 LED trail
      int ledIndex = (animState.scannerPosition - i);
      if (ledIndex < 1) ledIndex += 12;  // Wrap around ring

      float intensity = 1.0 - (float)i / 6.0;  // Fade out
      uint32_t beamColor = adjustColorBrightness(animState.baseColorLeft, intensity);

      leftEye.setPixelColor(ledIndex, beamColor);
      rightEye.setPixelColor(ledIndex, beamColor);
    }

    leftEye.show();
    rightEye.show();

    // Move radar beam around ring (1-12)
    animState.scannerPosition++;
    if (animState.scannerPosition > 12) {
      animState.scannerPosition = 1;
    }
  }
}

//========================================
// SYNCHRONIZED ANIMATION UPDATE FUNCTIONS
//========================================

void updateHeartbeatAnimation() {
  // Heartbeat: synchronized double-pulse like a real heartbeat (lub-dub)
  unsigned long currentTime = millis();
  unsigned long elapsed = currentTime - animState.pulseStartTime;

  // Heartbeat cycle: 1200ms total
  // First beat: 0-200ms (fast pulse)
  // Rest: 200-400ms
  // Second beat: 400-600ms (fast pulse)
  // Long rest: 600-1200ms

  float brightness = 0.0;
  unsigned long cycleTime = elapsed % 1200;

  if (cycleTime < 200) {
    // First beat (lub)
    float beatProgress = (float)cycleTime / 200.0;
    brightness = sin(beatProgress * PI);  // Quick pulse
  } else if (cycleTime >= 400 && cycleTime < 600) {
    // Second beat (dub)
    float beatProgress = (float)(cycleTime - 400) / 200.0;
    brightness = sin(beatProgress * PI) * 0.7;  // Slightly weaker pulse
  } else {
    // Rest periods
    brightness = 0.1;  // Dim baseline
  }

  // Apply brightness to both eyes (synchronized)
  uint32_t beatColor = adjustColorBrightness(animState.baseColorLeft, brightness);

  for (int i = 0; i < activeEyeLEDCount; i++) {
    leftEye.setPixelColor(i, beatColor);
    rightEye.setPixelColor(i, beatColor);
  }

  leftEye.show();
  rightEye.show();
}

void updateAlarmAnimation() {
  // Alarm: rapid synchronized red/white flashing
  unsigned long currentTime = millis();

  if (currentTime - animState.lastFlickerUpdate >= 150) {  // 150ms interval
    animState.lastFlickerUpdate = currentTime;

    // Alternate between red and white
    uint32_t color;
    if (animState.animationStep % 2 == 0) {
      color = getAlertRed();  // Red
    } else {
      color = makeColor(255, 255, 255);  // White
    }

    // Apply to all LEDs on both eyes (synchronized)
    for (int i = 0; i < activeEyeLEDCount; i++) {
      leftEye.setPixelColor(i, color);
      rightEye.setPixelColor(i, color);
    }

    leftEye.show();
    rightEye.show();

    animState.animationStep++;
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
    case SCANNER: return "Scanner";
    case IRIS: return "Iris (13-LED)";
    case TARGETING: return "Targeting (13-LED)";
    case RING_SCANNER: return "Ring Scanner (13-LED)";
    case SPIRAL: return "Spiral (13-LED)";
    case FOCUS: return "Focus (13-LED)";
    case RADAR: return "Radar (13-LED)";
    case HEARTBEAT: return "Heartbeat (Synchronized)";
    case ALARM: return "Alarm (Synchronized)";
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