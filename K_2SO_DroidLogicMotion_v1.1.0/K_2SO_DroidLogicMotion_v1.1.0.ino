/*
================================================================================
PROJECT:      K-2SO Professional Animatronics Controller
VERSION:      1.1.0
AUTHOR:       Printed-Droid.com
DATE:         2025-10-23
DESCRIPTION:  Advanced ESP32-S3 based control system for Star Wars K-2SO droid builds
              with film-accurate behaviors and professional-grade features

HARDWARE:     ESP32-S3-Zero + DFPlayer Mini + PAM8406 Amplifier
COMPATIBILITY: Works on ANY ESP32-S3 board - no PWM boards needed!
BOARD:        ESP32S3 Dev Module (select in Arduino IDE)
BAUD RATE:    115200

**Important Upload Notes:**
Always enable "USB CDC On Boot" - without this, serial communication will fail
If upload fails, try holding BOOT button while connecting USB

================================================================================
CORE FEATURES:
================================================================================

MOTION CONTROL:
• 4x Servo motors (Eye Pan/Tilt + Head Pan/Tilt)
• Smooth interpolation algorithms with film-accurate timing
• Three personality modes: SCANNING, ALERT, IDLE
• Autonomous movement patterns with anti-repetition logic
• Complete calibration system for center positions and ranges

VISUAL SYSTEM:
• Dual 7-pixel NeoPixel eyes (16.7M colors)
• Advanced animations: Fade, Flicker, Pulse, Scanner effects
• Mode-specific eye colors (Ice Blue/Red/Amber)
• Detail LED random blinking system
• Brightness control (0-255 levels)

AUDIO SYSTEM:
• DFPlayer Mini with 24-bit DAC output
• PAM8406 stereo amplifier (5W per channel at 4Ω)
• 30-level volume control with folder organization
• Mode-specific audio from organized SD card structure
• Random sound selection with configurable pause intervals

CONTROL INTERFACES:
• IR Remote (17-button learning system with any remote)
• Professional Web Interface (mobile responsive)
• Advanced Serial CLI (115200 baud configuration)
• Real-time system monitoring and diagnostics

================================================================================
IR REMOTE LAYOUT:
================================================================================

MOVEMENT:        UP/DOWN/LEFT/RIGHT (direct eye positioning), OK (center)
PERSONALITY:     1=Scanning(Ice Blue), 2=Alert(Red), 3=Idle(Amber)
AUDIO:           4=Random Scan, 5=Random Alert, 6=Random Voice
VISUAL:          *=Color Forward, #=Color Backward, 0=Eyes On/Off

================================================================================
SERIAL COMMANDS (examples):
================================================================================

CALIBRATION:     servo eye center 90 90, led brightness 200, sound volume 25
TIMING:          timing scan move 20 40, timing alert wait 500 1500
PROFILES:        profile save "MyK2SO", profile load 0, profile list
IR SETUP:        learn, scan, default, show
DIAGNOSTICS:     status, monitor, test, backup, restore

================================================================================
WEB INTERFACE FEATURES:
================================================================================

• Live servo positioning gamepad with visual feedback
• Color picker with real-time preview
• Animation controls (flicker, pulse, scanner)
• Audio management and volume slider
• System status monitoring with uptime/memory display
• Mobile-optimized touch interface

================================================================================
HARDWARE SETUP:
================================================================================

ESP32-S3 PINS:
• GP3  → Left Eye NeoPixel (7 pixels)
• GP4  → Right Eye NeoPixel (7 pixels)
• GP5  → Eye Pan Servo
• GP6  → Eye Tilt Servo
• GP7  → Head Pan Servo
• GP8  → Head Tilt Servo
• GP9  → IR Receiver (TSOP38238)
• GP10 → Detail LED 1
• GP11 → DFPlayer TX
• GP12 → DFPlayer RX
• GP13 → Detail LED 2

POWER REQUIREMENTS:
• 5V/3A minimum power supply
• Servos: 5V direct connection
• Audio: PAM8406 amplifier (5W per channel)
• LEDs: 5V with current limiting

SD CARD STRUCTURE:
/01/001.mp3 → Scanning mode sounds
/02/001.mp3 → Alert mode sounds
/03/001.mp3 → Boot sequence audio
/04/001.mp3 → Voice lines/responses

================================================================================
CONFIGURATION:
================================================================================

1. Update WiFi credentials in config.h
2. Load MP3 files to microSD card (001.mp3, 002.mp3 format)
3. Program IR remote: Type 'learn' in serial monitor
4. Calibrate servos: Use 'servo' commands for center positions
5. Adjust timing: Use 'timing' commands for movement speeds

================================================================================
EXPANSION READY:
================================================================================

PREPARED SENSOR INPUTS:
• Human presence microwave sensor
• PIR motion detection  
• Top proximity sensor
• I2C expansion bus (GP1/GP2)

FUTURE FEATURES:
• Over-the-air updates
• Advanced sensor integration
• Custom behavior programming
• Third-party API integration

================================================================================
TECHNICAL SPECIFICATIONS:
================================================================================

PROCESSOR:       ESP32-S3-Zero (240MHz dual-core Xtensa LX7)
MEMORY:          512KB SRAM + 8MB Flash + EEPROM storage
CONNECTIVITY:    WiFi 802.11n, mDNS (k2so.local)
REAL-TIME:       Non-blocking operation, smooth multi-tasking
RELIABILITY:     Wear leveling, checksum validation, error recovery

================================================================================
COMPATIBILITY & REQUIREMENTS:
================================================================================

ARDUINO IDE:     ESP32 board package 3.3.1+
LIBRARIES:       Adafruit NeoPixel, ESP32Servo, DFPlayer Mini Mp3 by Makuna
BOARD SETTING:   ESP32S3 Dev Module
NO PWM BOARDS:   Direct ESP32-S3 servo control - external PWM boards unnecessary!

================================================================================
DISCLAIMER:
================================================================================

DISCLAIMER OF LIABILITY:
THIS SOFTWARE AND HARDWARE DESIGN IS PROVIDED "AS IS" WITHOUT WARRANTY OF ANY 
KIND. THE AUTHOR DISCLAIMS ALL WARRANTIES, EXPRESS OR IMPLIED, INCLUDING BUT NOT 
LIMITED TO WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE, AND 
NON-INFRINGEMENT.

IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY CLAIM, DAMAGES, OR OTHER LIABILITY,
WHETHER IN AN ACTION OF CONTRACT, TORT, OR OTHERWISE, ARISING FROM, OUT OF, OR IN
CONNECTION WITH THE SOFTWARE OR HARDWARE DESIGN OR THE USE OR OTHER DEALINGS IN
THE SOFTWARE OR HARDWARE.

SAFETY WARNING:
This project involves electrical components, servo motors, and audio amplifiers.
Users are responsible for:
• Proper electrical safety and insulation
• Adequate power supply sizing and protection
• Safe mechanical assembly and operation
• Compliance with local electrical codes and regulations
• Testing all functions before final installation

BUILD AT YOUR OWN RISK. Ensure proper knowledge of electronics and safety 
practices before attempting this build. The author assumes no responsibility 
for damage to property, injury, or death resulting from the use of this design.

USE OF THIS CODE CONSTITUTES ACCEPTANCE OF THESE TERMS.

================================================================================
*/

//========================================
// SYSTEM LIBRARIES FIRST
//========================================
#include <Arduino.h>
#include <WiFi.h>
#include <WebServer.h>
#include <ESPmDNS.h>
#include <ESP32Servo.h>
#include <Adafruit_NeoPixel.h>
#include <EEPROM.h>

// IR Library with correct defines
// #define DISABLE_IR_SEND
// #include <IRremote.hpp>

// MP3 Library
#include <DFMiniMp3.h>

//========================================
// CUSTOM HEADERS IN CORRECT ORDER
//========================================
#include "config.h"       // Base configuration and structures
#include "Mp3Notify.h"    // MP3 Notification class (needs DFMiniMp3.h)
#include "statusled.h"    // Status LED control system
#include "detailleds.h"   // Detail LED control system (WS2812)
#include "animations.h"   // LED animations
#include "webpage.h"      // Web interface
#include "handlers.h"     // Command handlers
#include "globals.h"      // Global variables (LAST!)

//========================================
// GLOBAL VARIABLE DEFINITIONS
//========================================
// Legacy detail LED pins (DEPRECATED - kept for compatibility)
const uint8_t DETAIL_LED_PINS[2] = {10, 13};
const char* standard17Buttons[17] = {
  "0", "1", "2", "3", "4", "5", "6", "7", "8", "9",
  "*", "#", "UP", "DOWN", "LEFT", "RIGHT", "OK"
};

//========================================
// HARDWARE OBJECT DEFINITIONS
//========================================
Adafruit_NeoPixel leftEye(NUM_EYE_PIXELS, LEFT_EYE_PIN, NEO_GRB + NEO_KHZ800);
Adafruit_NeoPixel rightEye(NUM_EYE_PIXELS, RIGHT_EYE_PIN, NEO_GRB + NEO_KHZ800);
Adafruit_NeoPixel statusLED(STATUS_LED_COUNT, STATUS_LED_PIN, NEO_GRB + NEO_KHZ800);
// Note: detailLEDs NeoPixel object is defined in detailleds.cpp
HardwareSerial dfSerial(2);
DFMiniMp3<HardwareSerial, Mp3Notify> mp3(dfSerial);
WebServer server(80);
Servo eyePanServo;
Servo eyeTiltServo;
Servo headPanServo;
Servo headTiltServo;

//========================================
// SYSTEM STATE VARIABLE DEFINITIONS
//========================================
ConfigData config;
ConfigData lastSavedConfig;
OperatingMode operatingMode = MODE_NORMAL;
PersonalityMode currentMode = MODE_SCANNING;
bool isAwake = false;
bool bootSequenceComplete = false;
unsigned long lastActivityTime = 0;
unsigned long animationStartTime = 0;
unsigned long lastAnimationUpdateTime = 0;
uint32_t leftEyeCurrentColor = 0;
uint32_t rightEyeCurrentColor = 0;
uint8_t currentBrightness = DEFAULT_BRIGHTNESS;
PixelMode currentPixelMode = SOLID_COLOR;
uint8_t activeEyeLEDCount = 13;  // Active LED count based on eye version (default 13)
bool isAudioReady = false;
bool isWaitingForNextTrack = false;
unsigned long nextPlayTime = 0;
int currentTrackFolder = 1;
unsigned long uptimeStart = 0;
unsigned long irCommandCount = 0;
unsigned long servoMovements = 0;
int bootSequenceStep = 0;
unsigned long bootSequenceTimer = 0;
bool monitorMode = false;
unsigned long lastMonitorUpdate = 0;
int learningStep = 0;
int currentButtonIndex = 0;
unsigned long learningTimeout = 0;
bool waitingForIR = false;
int testStep = 0;
unsigned long testTimer = 0;
ServoState eyePan;
ServoState eyeTilt;
ServoState headPan;
ServoState headTilt;
DetailBlinker blinkers[2];  // Legacy: kept for compatibility

// Status LED variables (NEW)
StatusLEDAnimation statusLEDAnim;
unsigned long lastWifiCheck = 0;
unsigned long lastStatusUpdate = 0;
bool wifiWasConnected = false;

//========================================
// FORWARD DECLARATIONS FOR LOCAL FUNCTIONS
//========================================
void initializeHardware();
void initializeServos();
void initializeWiFi();
void setupWebServer();
void updateSystemStatus();  // NEW: Status LED system updates

//========================================
// SETUP FUNCTION
//========================================
void setup() {
  Serial.begin(115200);
  delay(200);
  uptimeStart = millis();

  Serial.println("\n=========================================");
  Serial.println("   K-2SO Professional Controller v1.1.0");
  Serial.println("     Droid Logic Motion Board v1.3");
  Serial.println("       www.printed-droid.com");
  Serial.println("=========================================");

  if (strcmp(WIFI_SSID, "YOUR_WIFI_SSID") == 0) {
    Serial.println("\n[WARNING] WiFi not configured in config.h!");
  }

  EEPROM.begin(EEPROM_SIZE);
  loadConfiguration();
  initializeHardware();
  initializeWiFi();
  setupWebServer();
  applyConfiguration();

  Serial.println("\nSystem ready. Type 'help' for commands.");
  if (WiFi.status() == WL_CONNECTED) {
    Serial.print("Web Interface: http://");
    Serial.println(WiFi.localIP());
  }
  if (config.buttonCount == 0) {
    Serial.println("\n[!] No IR remote configured. Type 'learn' to setup.");
  }

  bootSequenceTimer = millis();
  statusLEDBootSequence(); // NEW: Start boot sequence LED animation
  logSystemEvent("System startup complete");
}

//========================================
// MAIN LOOP
//========================================
void loop() {
  static unsigned long lastCommandCheck = 0;
  unsigned long currentMillis = millis();

  if (WiFi.status() == WL_CONNECTED) {
    server.handleClient();
  }

  if (currentMillis - lastCommandCheck > 50) {
    if (Serial.available()) {
      String command = Serial.readStringUntil('\n');
      command.trim();
      if (command.length() > 0) {
        processCommand(command);
      }
    }
    lastCommandCheck = currentMillis;
  }

  switch (operatingMode) {
    case MODE_NORMAL:      handleNormalOperation(); break;
    case MODE_MONITOR:     handleMonitorMode();     break;
    case MODE_IR_SCANNER:  handleScannerMode();     break;
    case MODE_IR_LEARNING: handleLearningMode();    break;
    case MODE_TEST:        handleTestMode();        break;
    case MODE_DEMO:        handleDemoMode();        break;
    case MODE_SETUP_WIZARD: break;
  }

  updateDetailLEDs();       // NEW: Update detail LED animations (WS2812)
  handlePixelAnimations();
  updateSystemStats();
  updateSystemStatus();     // Update status LED system
  updateStatusLED();        // Handle status LED animations

  if (!bootSequenceComplete) {
    handleBootSequence(currentMillis);
  }
}

//========================================
// INITIALIZATION FUNCTIONS
//========================================
void initializeHardware() {
  Serial.println("Initializing hardware...");
  initializeServos();

  leftEye.begin();
  rightEye.begin();
  leftEye.show();
  rightEye.show();
  Serial.println("- NeoPixel LEDs: OK");

  // Initialize Status LED
  initializeStatusLED();

  // Initialize Detail LEDs (WS2812 Strip)
  initializeDetailLEDs();

  // IMPROVED: Audio System with better error handling and retry logic
  Serial.println("Initializing DFPlayer...");
  dfSerial.begin(9600, SERIAL_8N1, DFPLAYER_RX_PIN, DFPLAYER_TX_PIN);
  delay(500);

  mp3.begin();
  delay(2000);

  int attempts = 0;
  uint16_t fileCount = 0;
  const int maxAttempts = 15;  // Increased from 5 to 15 attempts

  while (attempts < maxAttempts && fileCount == 0) {
    mp3.loop();
    delay(300);  // Increased from 200ms to 300ms for slower SD cards
    fileCount = mp3.getTotalTrackCount();
    attempts++;
    if (fileCount == 0) {
      Serial.printf("- DFPlayer attempt %d/%d: No files detected\n", attempts, maxAttempts);
    }
  }

  if (fileCount > 0) {
    isAudioReady = true;
    mp3.setVolume(config.savedVolume);
    Serial.printf("- DFPlayer: OK (%d files found)\n", fileCount);
  } else {
    Serial.println("- DFPlayer: Not ready or no SD card");
    Serial.println("  Note: Audio will retry during boot sequence");
    statusLEDError(); // NEW: Show error on status LED
  }

  initializeIR();
}

// Legacy function - kept for compatibility but no longer used
// Detail LEDs now use WS2812 strip controlled by detailleds.cpp
void initializeDetailBlinkers() {
  // This function is deprecated and not called anymore
  // WS2812 detail LEDs are initialized via initializeDetailLEDs() in detailleds.cpp
}

void initializeServos() {
  eyePan.servoObject = &eyePanServo;
  eyeTilt.servoObject = &eyeTiltServo;
  headPan.servoObject = &headPanServo;
  headTilt.servoObject = &headTiltServo;

  eyePanServo.attach(EYE_PAN_PIN, 500, 2500);
  eyeTiltServo.attach(EYE_TILT_PIN, 500, 2500);
  headPanServo.attach(HEAD_PAN_PIN, 500, 2500);
  headTiltServo.attach(HEAD_TILT_PIN, 500, 2500);
  Serial.println("- Servos: OK");

  eyePan.minRange = config.eyePanMin;
  eyePan.maxRange = config.eyePanMax;
  eyePan.stepSize = 2;
  eyePan.moveInterval = 50;
  eyePan.isMoving = false;

  eyeTilt.minRange = config.eyeTiltMin;
  eyeTilt.maxRange = config.eyeTiltMax;
  eyeTilt.stepSize = 2;
  eyeTilt.moveInterval = 50;
  eyeTilt.isMoving = false;

  headPan.minRange = config.headPanMin;
  headPan.maxRange = config.headPanMax;
  headPan.stepSize = 1;
  headPan.moveInterval = 100;
  headPan.isMoving = false;

  headTilt.minRange = config.headTiltMin;
  headTilt.maxRange = config.headTiltMax;
  headTilt.stepSize = 1;
  headTilt.moveInterval = 100;
  headTilt.isMoving = false;
}

void initializeWiFi() {
  if (strcmp(WIFI_SSID, "YOUR_WIFI_SSID") == 0) return;
  
  Serial.print("Connecting to WiFi: ");
  Serial.println(WIFI_SSID);
  
  statusLEDWiFiConnecting(); // NEW: Show connecting status
  
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  int attempts = 0;
  while (WiFi.status() != WL_CONNECTED && attempts < 30) {
    delay(500);
    Serial.print(".");
    attempts++;
  }
  
  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("\nWiFi connected!");
    statusLEDWiFiConnected(); // NEW: Show connected status
    wifiWasConnected = true;
    if (MDNS.begin("k2so")) {
      Serial.println("MDNS responder started (k2so.local)");
    }
  } else {
    Serial.println("\nWiFi connection failed!");
    statusLEDWiFiDisconnected(); // NEW: Show disconnected status
    wifiWasConnected = false;
  }
}

void setupWebServer() {
  if (WiFi.status() != WL_CONNECTED) return;
  server.on("/", handleRoot);
  server.on("/status", handleWebStatus);
  server.on("/setServos", handleSetServos);
  server.on("/red", handleRed);
  server.on("/green", handleGreen);
  server.on("/blue", handleBlue);
  server.on("/white", handleWhite);
  server.on("/off", handleOff);
  server.on("/brightness", handleBrightness);
  server.on("/volume", handleVolume);
  server.on("/flicker", handleFlicker);
  server.on("/pulse", handlePulse);
  server.on("/playSound", handlePlaySound);
  server.on("/mode", handleWebMode);

  // Detail LED web handlers
  server.on("/detailCount", handleDetailCount);
  server.on("/detailBrightness", handleDetailBrightnessWeb);
  server.on("/detailPattern", handleDetailPatternWeb);
  server.on("/detailEnabled", handleDetailEnabledWeb);

  server.onNotFound(handleNotFound);
  server.begin();
  Serial.println("Web server started");
}

//========================================
// NEW: SYSTEM STATUS UPDATE FUNCTION
//========================================
void updateSystemStatus() {
  static unsigned long lastSystemCheck = 0;
  unsigned long currentTime = millis();
  
  // Update every 2 seconds
  if (currentTime - lastSystemCheck < 2000) {
    return;
  }
  lastSystemCheck = currentTime;
  
  // Auto-update status LED based on system state
  autoUpdateStatusLED();
  
  // Check WiFi status changes
  static bool lastWifiStatus = false;
  bool currentWifiStatus = (WiFi.status() == WL_CONNECTED);
  
  if (currentWifiStatus != lastWifiStatus) {
    lastWifiStatus = currentWifiStatus;
    if (currentWifiStatus) {
      statusLEDWiFiConnected();
      Serial.println("WiFi reconnected");
    } else {
      statusLEDWiFiDisconnected();
      Serial.println("WiFi disconnected");
    }
  }
  
  // Check for errors
  if (bootSequenceComplete && !isAudioReady) {
    statusLEDError();
  }
}