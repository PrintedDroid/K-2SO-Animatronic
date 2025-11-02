/*
================================================================================
// K-2SO Detail LED System Implementation - WS2812 Strip Control
// Full implementation of detail LED control with multiple patterns and eye versions
================================================================================
*/

#include "detailleds.h"

//========================================
// HARDWARE OBJECT DEFINITION
//========================================

// Define the NeoPixel strip object (max 8 LEDs)
Adafruit_NeoPixel detailLEDs = Adafruit_NeoPixel(MAX_DETAIL_LEDS, DETAIL_LED_PIN, NEO_GRB + NEO_KHZ800);

//========================================
// STATE VARIABLES
//========================================

// Current detail LED state
DetailLEDState detailState;

// Mode-specific colors (defaults)
DetailModeColors detailModeColors = {
  255, 0, 0,    // Scanning: Red (default)
  255, 0, 0,    // Alert: Red (default)
  255, 0, 0     // Idle: Red (default)
};

//========================================
// INITIALIZATION
//========================================

void initializeDetailLEDs() {
  // Initialize hardware
  detailLEDs.begin();
  detailLEDs.clear();
  detailLEDs.show();

  // Set default configuration
  detailState.activeCount = DEFAULT_DETAIL_COUNT;
  detailState.brightness = DEFAULT_DETAIL_BRIGHTNESS;

  // Set default color (red)
  detailState.red = 255;
  detailState.green = 0;
  detailState.blue = 0;

  // Set default animation (Random pattern with slow, organic flicker)
  detailState.pattern = DETAIL_PATTERN_RANDOM;
  detailState.enabled = true;
  detailState.autoColorMode = false;

  // Initialize animation state
  detailState.lastUpdate = millis();
  detailState.animationStep = 0;
  detailState.animationDirection = true;
  detailState.animationProgress = 0.0;

  // Set brightness
  detailLEDs.setBrightness(detailState.brightness);

  Serial.println("- Detail LEDs: OK (WS2812 Strip, Random Pattern)");
  Serial.printf("  Active LEDs: %d/%d\n", detailState.activeCount, MAX_DETAIL_LEDS);
  Serial.printf("  Pattern: %s\n", getDetailPatternName().c_str());
}

//========================================
// MAIN UPDATE LOOP
//========================================

void updateDetailLEDs() {
  if (!detailState.enabled) {
    return;
  }

  unsigned long now = millis();

  // Update animation based on current pattern
  switch (detailState.pattern) {
    case DETAIL_PATTERN_BLINK:
      updateDetailBlink();
      break;

    case DETAIL_PATTERN_FADE:
      updateDetailFade();
      break;

    case DETAIL_PATTERN_CHASE:
      updateDetailChase();
      break;

    case DETAIL_PATTERN_PULSE:
      updateDetailPulse();
      break;

    case DETAIL_PATTERN_RANDOM:
      updateDetailRandom();
      break;
  }
}

//========================================
// ANIMATION UPDATE FUNCTIONS
//========================================

void updateDetailBlink() {
  unsigned long now = millis();
  unsigned long elapsed = now - detailState.lastUpdate;

  // Determine on/off state
  bool shouldBeOn = (detailState.animationStep % 2 == 0);
  unsigned long duration = shouldBeOn ? DETAIL_BLINK_ON_MS : DETAIL_BLINK_OFF_MS;

  if (elapsed >= duration) {
    detailState.animationStep++;
    detailState.lastUpdate = now;

    // Toggle between on and off
    shouldBeOn = (detailState.animationStep % 2 == 0);

    if (shouldBeOn) {
      // Turn on all active LEDs
      for (int i = 0; i < detailState.activeCount; i++) {
        detailLEDs.setPixelColor(i, detailLEDs.Color(detailState.red, detailState.green, detailState.blue));
      }
    } else {
      // Turn off all LEDs
      for (int i = 0; i < detailState.activeCount; i++) {
        detailLEDs.setPixelColor(i, 0);
      }
    }

    detailLEDs.show();
  }
}

void updateDetailFade() {
  unsigned long now = millis();
  unsigned long elapsed = now - detailState.lastUpdate;

  if (elapsed >= 20) {  // Update every 20ms for smooth animation
    detailState.lastUpdate = now;

    // Calculate fade progress (0.0 - 1.0 - 0.0)
    unsigned long cycleTime = now % DETAIL_FADE_SPEED_MS;
    float progress = (float)cycleTime / DETAIL_FADE_SPEED_MS;

    // Create triangle wave (fade in, then fade out)
    float brightness;
    if (progress < 0.5) {
      brightness = progress * 2.0;  // Fade in
    } else {
      brightness = (1.0 - progress) * 2.0;  // Fade out
    }

    // Apply brightness to all active LEDs
    uint8_t r = detailState.red * brightness;
    uint8_t g = detailState.green * brightness;
    uint8_t b = detailState.blue * brightness;

    for (int i = 0; i < detailState.activeCount; i++) {
      detailLEDs.setPixelColor(i, detailLEDs.Color(r, g, b));
    }

    detailLEDs.show();
  }
}

void updateDetailChase() {
  unsigned long now = millis();
  unsigned long elapsed = now - detailState.lastUpdate;

  if (elapsed >= DETAIL_CHASE_SPEED_MS) {
    detailState.lastUpdate = now;

    // Clear all LEDs
    detailLEDs.clear();

    // For strip version: chase along the strip
    int ledIndex = detailState.animationStep % detailState.activeCount;
    detailLEDs.setPixelColor(ledIndex, detailLEDs.Color(detailState.red, detailState.green, detailState.blue));

    detailState.animationStep++;
    detailLEDs.show();
  }
}

void updateDetailPulse() {
  unsigned long now = millis();
  unsigned long elapsed = now - detailState.lastUpdate;

  if (elapsed >= 20) {  // Update every 20ms for smooth animation
    detailState.lastUpdate = now;

    // Calculate pulse progress using sine wave
    unsigned long cycleTime = now % DETAIL_PULSE_SPEED_MS;
    float progress = (float)cycleTime / DETAIL_PULSE_SPEED_MS;
    float angle = progress * 2.0 * PI;
    float brightness = (sin(angle) + 1.0) / 2.0;  // 0.0 to 1.0

    // Apply minimum brightness threshold (don't go completely off)
    brightness = 0.2 + (brightness * 0.8);

    // Apply brightness to all active LEDs
    uint8_t r = detailState.red * brightness;
    uint8_t g = detailState.green * brightness;
    uint8_t b = detailState.blue * brightness;

    for (int i = 0; i < detailState.activeCount; i++) {
      detailLEDs.setPixelColor(i, detailLEDs.Color(r, g, b));
    }

    detailLEDs.show();
  }
}

void updateDetailRandom() {
  unsigned long now = millis();
  unsigned long elapsed = now - detailState.lastUpdate;

  // Random interval between updates (faster for more dynamic effect)
  unsigned long interval = random(DETAIL_RANDOM_MIN_MS, DETAIL_RANDOM_MAX_MS);

  if (elapsed >= interval) {
    detailState.lastUpdate = now;

    // Clear all LEDs first
    detailLEDs.clear();

    // Randomly decide how many LEDs to light up (1 to activeCount)
    int numLEDsToLight = random(1, detailState.activeCount + 1);

    // Create array to track which LEDs are already selected
    bool ledUsed[MAX_DETAIL_LEDS] = {false};

    // Randomly select LEDs to light up
    for (int i = 0; i < numLEDsToLight; i++) {
      int ledIndex;

      // Find an unused LED
      do {
        ledIndex = random(0, detailState.activeCount);
      } while (ledUsed[ledIndex]);

      ledUsed[ledIndex] = true;

      // Random brightness for this LED (20% to 100%)
      float brightness = random(20, 101) / 100.0;

      uint8_t r = detailState.red * brightness;
      uint8_t g = detailState.green * brightness;
      uint8_t b = detailState.blue * brightness;

      detailLEDs.setPixelColor(ledIndex, detailLEDs.Color(r, g, b));
    }

    detailLEDs.show();
  }
}

//========================================
// CONFIGURATION FUNCTIONS
//========================================

void setDetailCount(uint8_t count) {
  if (count >= 1 && count <= MAX_DETAIL_LEDS) {
    detailState.activeCount = count;

    // Clear all LEDs and restart animation
    detailLEDs.clear();
    detailLEDs.show();
    detailState.animationStep = 0;

    Serial.printf("Detail LED count set to: %d/%d\n", count, MAX_DETAIL_LEDS);
  } else {
    Serial.printf("Error: LED count must be between 1 and %d\n", MAX_DETAIL_LEDS);
  }
}

void setDetailBrightness(uint8_t brightness) {
  detailState.brightness = brightness;
  detailLEDs.setBrightness(brightness);
  detailLEDs.show();
  Serial.printf("Detail LED brightness set to: %d\n", brightness);
}

void setDetailColor(uint8_t r, uint8_t g, uint8_t b) {
  detailState.red = r;
  detailState.green = g;
  detailState.blue = b;
  Serial.printf("Detail LED color set to: RGB(%d, %d, %d)\n", r, g, b);
}

void setDetailPattern(DetailPattern pattern) {
  detailState.pattern = pattern;
  detailState.animationStep = 0;
  detailState.lastUpdate = millis();
  detailState.animationProgress = 0.0;

  // Clear LEDs when changing pattern
  detailLEDs.clear();
  detailLEDs.show();

  Serial.printf("Detail LED pattern set to: %s\n", getDetailPatternName().c_str());
}

void setDetailEnabled(bool enabled) {
  detailState.enabled = enabled;

  if (!enabled) {
    detailLedsOff();
  }

  Serial.printf("Detail LEDs: %s\n", enabled ? "Enabled" : "Disabled");
}

void setDetailAutoColorMode(bool enabled) {
  detailState.autoColorMode = enabled;
  Serial.printf("Detail LED auto color mode: %s\n", enabled ? "Enabled" : "Disabled");
}

//========================================
// PATTERN CONTROL FUNCTIONS
//========================================

void startDetailBlink() {
  setDetailPattern(DETAIL_PATTERN_BLINK);
}

void startDetailFade() {
  setDetailPattern(DETAIL_PATTERN_FADE);
}

void startDetailChase() {
  setDetailPattern(DETAIL_PATTERN_CHASE);
}

void startDetailPulse() {
  setDetailPattern(DETAIL_PATTERN_PULSE);
}

void startDetailRandom() {
  setDetailPattern(DETAIL_PATTERN_RANDOM);
}

//========================================
// MODE INTEGRATION
//========================================

void updateDetailColorForMode(PersonalityMode mode) {
  if (!detailState.autoColorMode) {
    return;
  }

  switch (mode) {
    case MODE_SCANNING:
      setDetailColor(detailModeColors.scanningR, detailModeColors.scanningG, detailModeColors.scanningB);
      break;

    case MODE_ALERT:
      setDetailColor(detailModeColors.alertR, detailModeColors.alertG, detailModeColors.alertB);
      break;

    case MODE_IDLE:
      setDetailColor(detailModeColors.idleR, detailModeColors.idleG, detailModeColors.idleB);
      break;
  }
}

void setDetailModeColors(uint8_t scanR, uint8_t scanG, uint8_t scanB,
                        uint8_t alertR, uint8_t alertG, uint8_t alertB,
                        uint8_t idleR, uint8_t idleG, uint8_t idleB) {
  detailModeColors.scanningR = scanR;
  detailModeColors.scanningG = scanG;
  detailModeColors.scanningB = scanB;

  detailModeColors.alertR = alertR;
  detailModeColors.alertG = alertG;
  detailModeColors.alertB = alertB;

  detailModeColors.idleR = idleR;
  detailModeColors.idleG = idleG;
  detailModeColors.idleB = idleB;

  Serial.println("Detail LED mode colors updated");
}

//========================================
// UTILITY FUNCTIONS
//========================================

void detailLedsOff() {
  detailLEDs.clear();
  detailLEDs.show();
}

void detailLedsOn() {
  for (int i = 0; i < detailState.activeCount; i++) {
    detailLEDs.setPixelColor(i, detailLEDs.Color(detailState.red, detailState.green, detailState.blue));
  }
  detailLEDs.show();
}

void setDetailLED(uint8_t index, uint8_t r, uint8_t g, uint8_t b) {
  if (index < MAX_DETAIL_LEDS) {
    detailLEDs.setPixelColor(index, detailLEDs.Color(r, g, b));
  }
}

void showDetailLEDs() {
  detailLEDs.show();
}

void resetDetailLEDs() {
  initializeDetailLEDs();
}

//========================================
// STATUS AND INFORMATION
//========================================

String getDetailPatternName() {
  switch (detailState.pattern) {
    case DETAIL_PATTERN_BLINK: return "Blink";
    case DETAIL_PATTERN_FADE: return "Fade";
    case DETAIL_PATTERN_CHASE: return "Chase";
    case DETAIL_PATTERN_PULSE: return "Pulse";
    case DETAIL_PATTERN_RANDOM: return "Random";
    default: return "Unknown";
  }
}

void printDetailLEDStatus() {
  Serial.println("\n=== Detail LED Status ===");
  Serial.printf("Status: %s\n", detailState.enabled ? "Enabled" : "Disabled");
  Serial.printf("Active LEDs: %d/%d\n", detailState.activeCount, MAX_DETAIL_LEDS);
  Serial.printf("Brightness: %d/255\n", detailState.brightness);
  Serial.printf("Color: RGB(%d, %d, %d)\n", detailState.red, detailState.green, detailState.blue);
  Serial.printf("Pattern: %s\n", getDetailPatternName().c_str());
  Serial.printf("Auto Color Mode: %s\n", detailState.autoColorMode ? "Enabled" : "Disabled");
  Serial.println("========================\n");
}

bool isDetailLEDEnabled() {
  return detailState.enabled;
}

uint8_t getDetailCount() {
  return detailState.activeCount;
}

DetailPattern getDetailPattern() {
  return detailState.pattern;
}

//========================================
// PRESET CONFIGURATIONS
//========================================

void setDetailDefaultRed() {
  setDetailColor(255, 0, 0);
  setDetailPattern(DETAIL_PATTERN_BLINK);
  setDetailBrightness(DEFAULT_DETAIL_BRIGHTNESS);
  Serial.println("Detail LEDs set to: Default Red Blinking");
}

void setDetailModeScanningBlue() {
  setDetailColor(80, 150, 255);
  setDetailPattern(DETAIL_PATTERN_PULSE);
  Serial.println("Detail LEDs set to: Scanning Mode Blue");
}

void setDetailModeAlertRed() {
  setDetailColor(255, 0, 0);
  setDetailPattern(DETAIL_PATTERN_BLINK);
  Serial.println("Detail LEDs set to: Alert Mode Red");
}

void setDetailModeIdleAmber() {
  setDetailColor(100, 60, 0);
  setDetailPattern(DETAIL_PATTERN_FADE);
  Serial.println("Detail LEDs set to: Idle Mode Amber");
}