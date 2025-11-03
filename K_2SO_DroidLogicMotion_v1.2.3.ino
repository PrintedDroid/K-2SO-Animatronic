/*
================================================================================
PROJECT:      K-2SO Professional Animatronics Controller
VERSION:      1.2.2
AUTHOR:       Printed-Droid.com
DATE:         2025-11-02
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
  // Wait for USB CDC to be ready (max 2 seconds, then continue)
  unsigned long startTime = millis();
  while (!Serial && (millis() - startTime < 2000)) {
    delay(10);
  }
  delay(100);  // Short delay for stability
  uptimeStart = millis();

  Serial.println(F("\n========================================="));
  Serial.println(F("   K-2SO Professional Controller v1.2.3"));
  Serial.println(F("     Droid Logic Motion Board v1.3"));
  Serial.println(F("       www.printed-droid.com"));
  Serial.println(F("========================================="));

  if (strcmp(WIFI_SSID, "YOUR_WIFI_SSID") == 0) {
    Serial.println(F("\n[WARNING] WiFi not configured in config.h!"));
  }

  EEPROM.begin(EEPROM_SIZE);
  loadConfiguration();
  initializeHardware();
  initializeWiFi();
  setupWebServer();
  applyConfiguration();

  Serial.println(F("\nSystem ready. Type 'help' for commands."));
  if (WiFi.status() == WL_CONNECTED) {
    Serial.print(F("Web Interface: http://"));
    Serial.println(WiFi.localIP());
  }
  if (config.buttonCount == 0) {
    Serial.println(F("\n[!] No IR remote configured. Type 'learn' to setup."));
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

  // Handle web server requests in both WiFi and AP mode
  if (WiFi.status() == WL_CONNECTED || WiFi.getMode() == WIFI_AP || WiFi.getMode() == WIFI_AP_STA) {
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
  Serial.println(F("- NeoPixel LEDs: OK"));

  // Initialize Status LED
  initializeStatusLED();

  // Initialize Detail LEDs (WS2812 Strip)
  initializeDetailLEDs();

  // OPTIMIZED: Faster Audio System initialization
  Serial.println(F("Initializing DFPlayer..."));
  dfSerial.begin(9600, SERIAL_8N1, DFPLAYER_RX_PIN, DFPLAYER_TX_PIN);
  delay(100);  // Reduced from 500ms

  mp3.begin();
  delay(500);  // Reduced from 2000ms to 500ms

  int attempts = 0;
  uint16_t fileCount = 0;
  const int maxAttempts = 5;  // Reduced from 15 to 5 (quick startup)

  while (attempts < maxAttempts && fileCount == 0) {
    mp3.loop();
    delay(150);  // Reduced from 300ms to 150ms
    fileCount = mp3.getTotalTrackCount();
    attempts++;
    if (fileCount == 0 && attempts < maxAttempts) {
      Serial.printf("- DFPlayer attempt %d/%d\n", attempts, maxAttempts);
    }
  }

  if (fileCount > 0) {
    isAudioReady = true;
    mp3.setVolume(config.savedVolume);
    Serial.printf("- DFPlayer: OK (%d files found)\n", fileCount);
  } else {
    Serial.println(F("- DFPlayer: Not ready or no SD card"));
    Serial.println(F("  Note: Audio will retry during boot sequence"));
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

  // Detach servos first if already attached (prevents PWM channel conflicts)
  if (eyePanServo.attached()) {
    eyePanServo.detach();
  }
  if (eyeTiltServo.attached()) {
    eyeTiltServo.detach();
  }
  if (headPanServo.attached()) {
    headPanServo.detach();
  }
  if (headTiltServo.attached()) {
    headTiltServo.detach();
  }

  eyePanServo.attach(EYE_PAN_PIN, 500, 2500);
  eyeTiltServo.attach(EYE_TILT_PIN, 500, 2500);
  headPanServo.attach(HEAD_PAN_PIN, 500, 2500);
  headTiltServo.attach(HEAD_TILT_PIN, 500, 2500);
  Serial.println(F("- Servos: OK"));

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

void startAccessPoint() {
  // Generate default AP name if not configured
  String apSSID;
  String apPassword;

  if (config.apConfigured && strlen(config.apSSID) > 0) {
    apSSID = String(config.apSSID);
    apPassword = String(config.apPassword);
    Serial.print(F("Starting Access Point (from EEPROM): "));
  } else {
    // Generate AP name from MAC address
    String macAddr = WiFi.macAddress();
    macAddr.replace(":", "");
    apSSID = "K2SO-" + macAddr.substring(6);
    apPassword = DEFAULT_AP_PASSWORD;
    Serial.print(F("Starting Access Point (default): "));
  }

  Serial.println(apSSID);

  // Disable WiFi sleep to prevent RMT conflicts with NeoPixels
  WiFi.setSleep(false);

  // Start AP mode
  bool apStarted = WiFi.softAP(apSSID.c_str(), apPassword.c_str());

  if (apStarted) {
    Serial.println(F("Access Point started successfully!"));
    Serial.print(F("AP SSID: "));
    Serial.println(apSSID);
    Serial.print(F("AP IP Address: "));
    Serial.println(WiFi.softAPIP());
    Serial.println(F("Connect to this network to access the web interface."));
    Serial.print(F("Web Interface: http://"));
    Serial.println(WiFi.softAPIP());

    statusLEDWiFiConnected(); // Show AP as connected
  } else {
    Serial.println(F("Failed to start Access Point!"));
    statusLEDError();
  }
}

void initializeWiFi() {
  bool wifiConnected = false;

  // Check if WiFi is configured in EEPROM (takes priority over config.h)
  if (config.wifiConfigured && strlen(config.wifiSSID) > 0) {
    Serial.print(F("Connecting to WiFi (from EEPROM): "));
    Serial.println(config.wifiSSID);

    statusLEDWiFiConnecting();

    WiFi.mode(WIFI_STA);
    // Disable WiFi sleep to prevent RMT conflicts with NeoPixels
    WiFi.setSleep(false);
    WiFi.begin(config.wifiSSID, config.wifiPassword);
    int attempts = 0;
    // Reduced timeout: 10 attempts * 300ms = 3 seconds max
    while (WiFi.status() != WL_CONNECTED && attempts < 10) {
      delay(300);  // Reduced from 500ms
      Serial.print(F("."));
      attempts++;
    }
    Serial.println();  // New line after dots

    if (WiFi.status() == WL_CONNECTED) {
      wifiConnected = true;
    }
  }
  // Fallback to config.h if not configured in EEPROM
  else if (strcmp(WIFI_SSID, "YOUR_WIFI_SSID") != 0 && strcmp(WIFI_SSID, "Your Homewifi SSID") != 0) {
    Serial.print(F("Connecting to WiFi (from config.h): "));
    Serial.println(WIFI_SSID);

    statusLEDWiFiConnecting();

    WiFi.mode(WIFI_STA);
    // Disable WiFi sleep to prevent RMT conflicts with NeoPixels
    WiFi.setSleep(false);
    WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
    int attempts = 0;
    // Reduced timeout: 10 attempts * 300ms = 3 seconds max
    while (WiFi.status() != WL_CONNECTED && attempts < 10) {
      delay(300);  // Reduced from 500ms
      Serial.print(F("."));
      attempts++;
    }
    Serial.println();  // New line after dots

    if (WiFi.status() == WL_CONNECTED) {
      wifiConnected = true;
    }
  }

  if (wifiConnected) {
    Serial.println(F("\nWiFi connected!"));
    Serial.print(F("IP Address: "));
    Serial.println(WiFi.localIP());
    statusLEDWiFiConnected();
    wifiWasConnected = true;
    if (MDNS.begin("k2so")) {
      Serial.println(F("MDNS responder started (k2so.local)"));
    }
  } else {
    Serial.println(F("\nWiFi connection failed!"));
    statusLEDWiFiDisconnected();
    wifiWasConnected = false;

    // Start AP mode if enabled
    if (config.apEnabled) {
      Serial.println(F("AP mode enabled - starting Access Point..."));
      startAccessPoint();
    } else {
      Serial.println(F("AP mode disabled. Use 'ap enable' to enable fallback AP mode."));
      Serial.println(F("Or use 'wifi set \"ssid\" \"password\"' to reconfigure WiFi (quotes for spaces)."));
    }
  }
}

//========================================
// VOICE ASSISTANT TRIGGER HANDLERS
//========================================
// These endpoints are designed for Google Home, Alexa, and other voice assistants
// via IFTTT webhooks or similar services. Each returns a simple HTTP 200 response.

// Trigger 1: "K2SO wake up" - Activate scanning mode
void handleTriggerWakeup() {
  if (!checkWebAuth()) return;

  Serial.println(F("[VOICE] Trigger: Wake Up"));

  // Activate scanning mode
  currentMode = MODE_SCANNING;
  setServoParameters();
  statusLEDScanningMode();

  // Blue scanner eyes
  startScannerMode();

  // Play random scanning sound
  if (isAudioReady) {
    playRandomSound(1); // Scanning sounds folder
  }

  // Wake up message
  isAwake = true;

  server.send(200, F("text/plain"), F("K2SO activated - Scanning mode"));
  Serial.println(F("[VOICE] K2SO activated"));
}

// Trigger 2: "K2SO standby" - Quiet mode with gentle blue glow
void handleTriggerStandby() {
  if (!checkWebAuth()) return;

  Serial.println(F("[VOICE] Trigger: Standby"));

  // Idle mode
  currentMode = MODE_IDLE;
  setServoParameters();
  statusLEDIdleMode();

  // Center all servos (no movement)
  centerAllServos();

  // Gentle blue solid eyes
  uint32_t dimBlue = Adafruit_NeoPixel::Color(30, 70, 120);
  setEyeColor(dimBlue, dimBlue);

  server.send(200, F("text/plain"), F("K2SO standby mode"));
  Serial.println(F("[VOICE] Standby mode active"));
}

// Trigger 3: "K2SO sleep" - Everything off
void handleTriggerSleep() {
  if (!checkWebAuth()) return;

  Serial.println(F("[VOICE] Trigger: Sleep"));

  // Idle mode
  currentMode = MODE_IDLE;
  isAwake = false;

  // Turn off eyes
  uint32_t off = Adafruit_NeoPixel::Color(0, 0, 0);
  setEyeColor(off, off);

  // Turn off detail LEDs
  setDetailEnabled(false);

  // Center servos
  centerAllServos();

  // Status LED off
  statusLED.setPixelColor(0, 0, 0, 0);
  statusLED.show();

  server.send(200, F("text/plain"), F("K2SO sleep mode"));
  Serial.println(F("[VOICE] Sleep mode - all systems off"));
}

// Trigger 4: "K2SO demo" - Full demonstration
void handleTriggerDemo() {
  if (!checkWebAuth()) return;

  Serial.println(F("[VOICE] Trigger: Demo"));

  // Start demo mode
  enterDemoMode();

  server.send(200, F("text/plain"), F("K2SO demo started"));
  Serial.println(F("[VOICE] Demo mode started"));
}

// Trigger 5: "K2SO speak" - Random voice line
void handleTriggerSpeak() {
  if (!checkWebAuth()) return;

  Serial.println(F("[VOICE] Trigger: Speak"));

  if (isAudioReady) {
    playRandomSound(4); // Voice lines folder
    server.send(200, F("text/plain"), F("K2SO speaking"));
    Serial.println(F("[VOICE] Playing random voice line"));
  } else {
    server.send(503, F("text/plain"), F("Audio system not ready"));
    Serial.println(F("[VOICE] Audio not available"));
  }
}

// Trigger 6: "K2SO alert mode" - Alert personality
void handleTriggerAlert() {
  if (!checkWebAuth()) return;

  Serial.println(F("[VOICE] Trigger: Alert Mode"));

  // Alert mode
  currentMode = MODE_ALERT;
  setServoParameters();
  statusLEDAlertMode();

  // Amber/yellow alert eyes
  uint32_t amber = Adafruit_NeoPixel::Color(255, 140, 0);
  setEyeColor(amber, amber);

  // Play alert sound
  if (isAudioReady) {
    playRandomSound(2); // Alert sounds folder
  }

  server.send(200, F("text/plain"), F("K2SO alert mode activated"));
  Serial.println(F("[VOICE] Alert mode active"));
}

// Trigger 7: "K2SO scanner eyes" - Classic scanner animation
void handleTriggerScanner() {
  if (!checkWebAuth()) return;

  Serial.println(F("[VOICE] Trigger: Scanner Eyes"));

  // Start scanner animation
  startScannerMode();

  server.send(200, F("text/plain"), F("K2SO scanner animation"));
  Serial.println(F("[VOICE] Scanner animation active"));
}

// Trigger 8: "K2SO alarm" - Red flashing alarm
void handleTriggerAlarm() {
  if (!checkWebAuth()) return;

  Serial.println(F("[VOICE] Trigger: Alarm"));

  // Alert mode
  currentMode = MODE_ALERT;
  setServoParameters();

  // Red alarm animation
  startAlarmMode();

  // Play alert sound
  if (isAudioReady) {
    playRandomSound(2); // Alert sounds
  }

  server.send(200, F("text/plain"), F("K2SO alarm activated"));
  Serial.println(F("[VOICE] Alarm mode active"));
}

// Trigger 9: "K2SO center" - Center all servos
void handleTriggerCenter() {
  if (!checkWebAuth()) return;

  Serial.println(F("[VOICE] Trigger: Center"));

  // Center all servos
  centerAllServos();

  server.send(200, F("text/plain"), F("K2SO servos centered"));
  Serial.println(F("[VOICE] All servos centered"));
}

// Trigger 10: "K2SO patrol" - Patrol mode with movements and sounds
void handleTriggerPatrol() {
  if (!checkWebAuth()) return;

  Serial.println(F("[VOICE] Trigger: Patrol"));

  // Scanning mode for patrol
  currentMode = MODE_SCANNING;
  setServoParameters();
  statusLEDScanningMode();

  // Scanner eyes
  startScannerMode();

  // Random movement
  isAwake = true;

  // Play scanning sound
  if (isAudioReady) {
    playRandomSound(1);
  }

  server.send(200, F("text/plain"), F("K2SO patrol mode"));
  Serial.println(F("[VOICE] Patrol mode - active scanning"));
}

void setupWebServer() {
  // Only start web server if WiFi is connected OR AP mode is active
  if (WiFi.status() != WL_CONNECTED && WiFi.getMode() != WIFI_AP && WiFi.getMode() != WIFI_AP_STA) {
    return;
  }

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

  // Voice Assistant Trigger Endpoints (Google Home, Alexa via IFTTT)
  server.on("/trigger/wakeup", handleTriggerWakeup);
  server.on("/trigger/standby", handleTriggerStandby);
  server.on("/trigger/sleep", handleTriggerSleep);
  server.on("/trigger/demo", handleTriggerDemo);
  server.on("/trigger/speak", handleTriggerSpeak);
  server.on("/trigger/alert", handleTriggerAlert);
  server.on("/trigger/scanner", handleTriggerScanner);
  server.on("/trigger/alarm", handleTriggerAlarm);
  server.on("/trigger/center", handleTriggerCenter);
  server.on("/trigger/patrol", handleTriggerPatrol);

  server.onNotFound(handleNotFound);
  server.begin();
  Serial.println(F("Web server started"));
  Serial.println(F("Voice triggers: /trigger/[wakeup|standby|sleep|demo|speak|alert|scanner|alarm|center|patrol]"));
}

//========================================
// NEW: SYSTEM STATUS UPDATE FUNCTION
//========================================
void updateSystemStatus() {
  static unsigned long lastSystemCheck = 0;
  unsigned long currentTime = millis();

  // Update every 5 seconds (reduced frequency to prevent WiFi task conflicts)
  if (currentTime - lastSystemCheck < 5000) {
    return;
  }
  lastSystemCheck = currentTime;

  // Auto-update status LED based on system state
  autoUpdateStatusLED();

  // Check WiFi status changes (with error handling)
  static bool lastWifiStatus = false;
  static bool wifiCheckFailed = false;

  // Safe WiFi status check
  wl_status_t wifiStatus = WiFi.status();
  if (wifiStatus != WL_NO_SHIELD) {  // Check if WiFi is initialized
    bool currentWifiStatus = (wifiStatus == WL_CONNECTED);

    if (currentWifiStatus != lastWifiStatus) {
      lastWifiStatus = currentWifiStatus;
      wifiCheckFailed = false;
      if (currentWifiStatus) {
        statusLEDWiFiConnected();
        Serial.println("WiFi reconnected");
      } else {
        statusLEDWiFiDisconnected();
        Serial.println("WiFi disconnected");
      }
    }
  }

  // Check for errors
  if (bootSequenceComplete && !isAudioReady) {
    statusLEDError();
  }
}