/*
================================================================================
// K-2SO Controller Handler Implementation - UPDATED WITH STATUS LED
// UPDATED: Added status LED activity indicators throughout command processing
================================================================================
*/

// System libraries FIRST
#include <Arduino.h>
#include <WiFi.h>
#include <WebServer.h>
#include <ESPmDNS.h>
#include <Adafruit_NeoPixel.h>
#include <ESP32Servo.h>
#include <EEPROM.h>

// IR Library with correct defines
#define DISABLE_IR_SEND
#include <IRremote.hpp>

// MP3 Library (Makuna)
#include <DFMiniMp3.h>

// Custom headers AFTER system libraries
#include "config.h"
#include "handlers.h"
#include "animations.h"
#include "statusled.h"    // Status LED functions
#include "detailleds.h"   // Detail LED functions (WS2812)
#include "webpage.h"
#include "globals.h"
#include "Mp3Notify.h"    

// Forward declaration to access mp3 object from main .ino
extern DFMiniMp3<HardwareSerial, Mp3Notify> mp3;

int currentColorIndex = 0;
const int COLOR_COUNT = 6;

//========================================
// IR SYSTEM MANAGEMENT FUNCTIONS
//========================================

void initializeIR() {
  if (config.irEnabled) {
    IrReceiver.begin(IR_RECEIVER_PIN, false);
    Serial.println("- IR Receiver: OK");
  } else {
    Serial.println("- IR Receiver: Disabled");
  }
}

void stopIR() {
  IrReceiver.end();
}

void setIREnabled(bool enabled) {
  if (enabled && !config.irEnabled) {
    IrReceiver.begin(IR_RECEIVER_PIN, false);
    config.irEnabled = true;
    Serial.println("IR enabled.");
  } else if (!enabled && config.irEnabled) {
    IrReceiver.end();
    config.irEnabled = false;
    Serial.println("IR disabled.");
  }
}

bool checkForIRCommand(uint32_t& code) {
  if (!config.irEnabled) return false;
  
  if (IrReceiver.decode()) {
    code = IrReceiver.decodedIRData.decodedRawData;
    IrReceiver.resume();
    return (code != 0xFFFFFFFF && code != 0);
  }
  return false;
}

//========================================
// WEB SERVER HANDLERS - UPDATED WITH STATUS LED + AUTHENTICATION
//========================================

// Helper function to check web authentication
bool checkWebAuth() {
  // If auth is disabled (empty username), allow all requests
  if (strlen(WEB_AUTH_USER) == 0) {
    return true;
  }

  // Check if authentication is valid
  if (!server.authenticate(WEB_AUTH_USER, WEB_AUTH_PASS)) {
    server.requestAuthentication();
    return false;
  }
  return true;
}

void handleRoot() {
  if (!checkWebAuth()) return;
  Serial.println("Web request: Root page");
  server.send(200, "text/html", getIndexPage());
}

void handleWebStatus() {
  if (!checkWebAuth()) return;
  Serial.println("Web request: Status");
  String status = "{\n";
  status += "  \"mode\": \"" + getModeName(currentMode) + "\",\n";
  status += "  \"awake\": " + String(isAwake ? "true" : "false") + ",\n";
  status += "  \"volume\": " + String(config.savedVolume) + ",\n";
  status += "  \"brightness\": " + String(currentBrightness) + ",\n";
  status += "  \"uptime\": " + String((millis() - uptimeStart) / 1000) + ",\n";
  status += "  \"freeMemory\": " + String(ESP.getFreeHeap()) + ",\n";
  status += "  \"ir_commands\": " + String(irCommandCount) + ",\n";
  status += "  \"servo_movements\": " + String(servoMovements) + "\n";
  status += "}";
  server.send(200, "application/json", status);
}

void handleSetServos() {
  if (!checkWebAuth()) return;
  Serial.println("Web request: Set servos");

  if (server.hasArg("eyePan") && server.hasArg("eyeTilt") && 
      server.hasArg("headPan") && server.hasArg("headTilt")) {
    
    int eyePanPos = server.arg("eyePan").toInt();
    int eyeTiltPos = server.arg("eyeTilt").toInt();
    int headPanPos = server.arg("headPan").toInt();
    int headTiltPos = server.arg("headTilt").toInt();
    
    // Validate positions
    eyePanPos = constrain(eyePanPos, config.eyePanMin, config.eyePanMax);
    eyeTiltPos = constrain(eyeTiltPos, config.eyeTiltMin, config.eyeTiltMax);
    headPanPos = constrain(headPanPos, config.headPanMin, config.headPanMax);
    headTiltPos = constrain(headTiltPos, config.headTiltMin, config.headTiltMax);
    
    // Set servo targets
    eyePan.targetPosition = eyePanPos;
    eyeTilt.targetPosition = eyeTiltPos;
    headPan.targetPosition = headPanPos;
    headTilt.targetPosition = headTiltPos;
    
    // Move servos immediately for web interface responsiveness
    eyePanServo.write(eyePanPos);
    eyeTiltServo.write(eyeTiltPos);
    headPanServo.write(headPanPos);
    headTiltServo.write(headTiltPos);
    
    // Update current positions
    eyePan.currentPosition = eyePanPos;
    eyeTilt.currentPosition = eyeTiltPos;
    headPan.currentPosition = headPanPos;
    headTilt.currentPosition = headTiltPos;
    
    // Wake up if sleeping and mark activity
    if (!isAwake) {
      isAwake = true;
      currentMode = MODE_ALERT;
      setServoParameters();
    }
    lastActivityTime = millis();
    servoMovements++;
    
    statusLEDServoActivity(); // NEW: Flash blue for servo activity
    
    Serial.printf("Servos set: EP:%d ET:%d HP:%d HT:%d\n", eyePanPos, eyeTiltPos, headPanPos, headTiltPos);
    server.send(200, "text/plain", "OK");
  } else {
    server.send(400, "text/plain", "Missing parameters");
  }
}

void handleRed() {
  if (!checkWebAuth()) return;
  Serial.println("Web request: Red eyes");
  uint32_t red = Adafruit_NeoPixel::Color(255, 0, 0);
  setEyeColor(red, red);
  currentPixelMode = SOLID_COLOR;
  lastActivityTime = millis();
  server.send(200, "text/plain", "OK");
}

void handleGreen() {
  if (!checkWebAuth()) return;
  Serial.println("Web request: Green eyes");
  uint32_t green = Adafruit_NeoPixel::Color(0, 255, 0);
  setEyeColor(green, green);
  currentPixelMode = SOLID_COLOR;
  lastActivityTime = millis();
  server.send(200, "text/plain", "OK");
}

void handleBlue() {
  if (!checkWebAuth()) return;
  Serial.println("Web request: Blue eyes");
  uint32_t blue = Adafruit_NeoPixel::Color(0, 0, 255);
  setEyeColor(blue, blue);
  currentPixelMode = SOLID_COLOR;
  lastActivityTime = millis();
  server.send(200, "text/plain", "OK");
}

void handleWhite() {
  if (!checkWebAuth()) return;
  Serial.println("Web request: White eyes");
  uint32_t white = Adafruit_NeoPixel::Color(255, 255, 255);
  setEyeColor(white, white);
  currentPixelMode = SOLID_COLOR;
  lastActivityTime = millis();
  server.send(200, "text/plain", "OK");
}

void handleOff() {
  if (!checkWebAuth()) return;
  Serial.println("Web request: Eyes off");
  uint32_t off = Adafruit_NeoPixel::Color(0, 0, 0);
  setEyeColor(off, off);
  currentPixelMode = SOLID_COLOR;
  server.send(200, "text/plain", "OK");
}

void handleBrightness() {
  if (!checkWebAuth()) return;
  if (server.hasArg("value")) {
    int brightness = server.arg("value").toInt();
    brightness = constrain(brightness, 0, 255);
    setEyeBrightness(brightness);
    Serial.printf("Web request: Brightness set to %d\n", brightness);
    server.send(200, "text/plain", "OK");
  } else {
    server.send(400, "text/plain", "Missing value parameter");
  }
}

void handleFlicker() {
  if (!checkWebAuth()) return;
  Serial.println("Web request: Flicker mode");
  startFlickerMode();
  lastActivityTime = millis();
  server.send(200, "text/plain", "OK");
}

void handlePulse() {
  if (!checkWebAuth()) return;
  Serial.println("Web request: Pulse mode");
  startPulseMode();
  lastActivityTime = millis();
  server.send(200, "text/plain", "OK");
}

void handleVolume() {
  if (!checkWebAuth()) return;
  if (server.hasArg("value")) {
    int volume = server.arg("value").toInt();
    volume = constrain(volume, 0, 30);
    setVolume(volume);
    Serial.printf("Web request: Volume set to %d\n", volume);
    server.send(200, "text/plain", "OK");
  } else {
    server.send(400, "text/plain", "Missing value parameter");
  }
}

void handlePlaySound() {
  if (!checkWebAuth()) return;
  if (server.hasArg("file")) {
    int fileNum = server.arg("file").toInt();
    playSound(fileNum);
    statusLEDAudioActivity(); // NEW: Flash green for audio activity
    Serial.printf("Web request: Playing sound %d\n", fileNum);
    server.send(200, "text/plain", "OK");
  } else {
    server.send(400, "text/plain", "Missing file parameter");
  }
}

void handleWebMode() {
  if (!checkWebAuth()) return;
  if (server.hasArg("mode")) {
    String mode = server.arg("mode");
    mode.toLowerCase();
    
    if (mode == "scanning") {
      currentMode = MODE_SCANNING;
      statusLEDScanningMode(); // Update status LED
    } else if (mode == "alert") {
      currentMode = MODE_ALERT;
      statusLEDAlertMode(); // Update status LED
    } else if (mode == "idle") {
      currentMode = MODE_IDLE;
      statusLEDIdleMode(); // Update status LED
    } else {
      server.send(400, "text/plain", "Invalid mode");
      return;
    }

    setServoParameters();
    updateDetailColorForMode(currentMode); // NEW: Update detail LEDs for mode
    config.savedMode = currentMode;
    
    if (!isAwake) {
      isAwake = true;
    }
    lastActivityTime = millis();
    
    Serial.printf("Web request: Mode changed to %s\n", getModeName(currentMode).c_str());
    server.send(200, "text/plain", "OK");
  } else {
    server.send(400, "text/plain", "Missing mode parameter");
  }
}

//========================================
// DETAIL LED WEB HANDLERS
//========================================

void handleDetailCount() {
  if (!checkWebAuth()) return;
  if (server.hasArg("value")) {
    int count = constrain(server.arg("value").toInt(), 1, 8);
    setDetailCount(count);
    Serial.printf("Web request: Detail LED count set to %d\n", count);
    server.send(200, "text/plain", "OK");
  } else {
    server.send(400, "text/plain", "Missing value parameter");
  }
}

void handleDetailBrightnessWeb() {
  if (!checkWebAuth()) return;
  if (server.hasArg("value")) {
    int brightness = constrain(server.arg("value").toInt(), 0, 255);
    setDetailBrightness(brightness);
    Serial.printf("Web request: Detail LED brightness set to %d\n", brightness);
    server.send(200, "text/plain", "OK");
  } else {
    server.send(400, "text/plain", "Missing value parameter");
  }
}

void handleDetailPatternWeb() {
  if (!checkWebAuth()) return;
  if (server.hasArg("pattern")) {
    String pattern = server.arg("pattern");
    pattern.toLowerCase();

    if (pattern == "blink") {
      startDetailBlink();
    } else if (pattern == "fade") {
      startDetailFade();
    } else if (pattern == "chase") {
      startDetailChase();
    } else if (pattern == "pulse") {
      startDetailPulse();
    } else if (pattern == "random") {
      startDetailRandom();
    } else {
      server.send(400, "text/plain", "Invalid pattern");
      return;
    }

    Serial.printf("Web request: Detail LED pattern set to %s\n", pattern.c_str());
    server.send(200, "text/plain", "OK");
  } else {
    server.send(400, "text/plain", "Missing pattern parameter");
  }
}

void handleDetailEnabledWeb() {
  if (!checkWebAuth()) return;
  if (server.hasArg("state")) {
    String state = server.arg("state");
    state.toLowerCase();

    if (state == "off") {
      setDetailEnabled(false);
      Serial.println("Web request: Detail LEDs disabled");
      server.send(200, "text/plain", "OK");
    } else if (state == "on") {
      setDetailEnabled(true);
      Serial.println("Web request: Detail LEDs enabled");
      server.send(200, "text/plain", "OK");
    } else {
      server.send(400, "text/plain", "Invalid state");
    }
  } else {
    server.send(400, "text/plain", "Missing state parameter");
  }
}

void handleNotFound() {
  String message = "404 - Not Found\n\n";
  message += "URI: " + server.uri() + "\n";
  message += "Method: ";
  message += (server.method() == HTTP_GET) ? "GET" : "POST";
  message += "\n";
  message += "Arguments: " + String(server.args()) + "\n";
  
  for (uint8_t i = 0; i < server.args(); i++) {
    message += " " + server.argName(i) + ": " + server.arg(i) + "\n";
  }
  
  server.send(404, "text/plain", message);
  Serial.println("Web request: 404 - " + server.uri());
}

//========================================
// COMMAND PROCESSING - UPDATED WITH STATUS LED
//========================================

Command parseCommand(String cmd) {
  cmd.toLowerCase();
  
  if (cmd == "help") return CMD_HELP;
  if (cmd == "status") return CMD_STATUS;
  if (cmd == "reset") return CMD_RESET;
  if (cmd == "save") return CMD_SAVE;
  if (cmd == "config") return CMD_CONFIG;
  if (cmd == "learn") return CMD_LEARN;
  if (cmd == "scan") return CMD_SCAN;
  if (cmd == "show") return CMD_SHOW;
  if (cmd == "clear") return CMD_CLEAR;
  if (cmd == "default") return CMD_DEFAULT;
  if (cmd == "servo") return CMD_SERVO;
  if (cmd == "led") return CMD_LED;
  if (cmd == "sound") return CMD_SOUND;
  if (cmd == "timing") return CMD_TIMING;
  if (cmd == "profile") return CMD_PROFILE;
  if (cmd == "monitor") return CMD_MONITOR;
  if (cmd == "test") return CMD_TEST;
  if (cmd == "demo") return CMD_DEMO;
  if (cmd == "backup") return CMD_BACKUP;
  if (cmd == "restore") return CMD_RESTORE;
  if (cmd == "exit" || cmd == "normal") return CMD_EXIT;
  if (cmd == "ir on") return CMD_IR_ON;
  if (cmd == "ir off") return CMD_IR_OFF;
  if (cmd == "mode") return CMD_MODE;
  if (cmd == "detail") return CMD_DETAIL;
  if (cmd == "wifi") return CMD_WIFI;
  if (cmd == "ap") return CMD_AP;

  return CMD_UNKNOWN;
}

void processCommand(String fullCommand) {
  int spaceIndex = fullCommand.indexOf(' ');
  String cmd = (spaceIndex > 0) ? fullCommand.substring(0, spaceIndex) : fullCommand;
  String params = (spaceIndex > 0) ? fullCommand.substring(spaceIndex + 1) : "";
  
  String cmdLower = cmd;
  cmdLower.toLowerCase();
  if (cmdLower == "ir") {
    String paramsLower = params;
    paramsLower.toLowerCase();
    if (paramsLower == "on") {
      cmd = "ir on";
      params = "";
    } else if (paramsLower == "off") {
      cmd = "ir off";
      params = "";
    }
  }
  
  Command command = parseCommand(cmd);
  
  switch (command) {
    case CMD_HELP:
      showHelp();
      break;
      
    case CMD_STATUS:
      showStatus();
      break;
      
    case CMD_CONFIG:
      showConfiguration();
      break;
      
    case CMD_RESET:
      Serial.println("Restarting system...");
      delay(1000);
      ESP.restart();
      break;
      
    case CMD_SAVE:
      smartSaveToEEPROM();
      break;
      
    case CMD_LEARN:
      enterLearningMode();
      break;
      
    case CMD_SCAN:
      operatingMode = MODE_IR_SCANNER;
      statusLEDConfigMode(); // NEW: Show config mode
      Serial.println("\n=== IR SCANNER MODE ===");
      Serial.println("Press any remote button. Type 'exit' to quit.");
      break;
      
    case CMD_SHOW:
      showSavedCodes();
      break;
      
    case CMD_CLEAR: {
      Serial.print("Clear all data? Type 'YES' to confirm: ");
      while (!Serial.available()) { delay(10); }
      String confirmation = Serial.readStringUntil('\n');
      confirmation.trim();
      if (confirmation == "YES") {
        clearAllData();
      } else {
        Serial.println("Operation cancelled.");
      }
      break;
    }
          
    case CMD_DEFAULT:
      loadDefaultCodes();
      break;
      
    case CMD_SERVO:
      handleServoCommand(params);
      break;
      
    case CMD_LED:
      handleLEDCommand(params);
      break;
      
    case CMD_SOUND:
      handleSoundCommand(params);
      break;
      
    case CMD_TIMING:
      handleTimingCommand(params);
      break;
      
    case CMD_PROFILE:
      handleProfileCommand(params);
      break;
      
    case CMD_MONITOR:
      enterMonitorMode();
      break;
      
    case CMD_TEST:
      runTestSequence(params);
      break;

    case CMD_DEMO:
      enterDemoMode();
      break;

    case CMD_BACKUP:
      backupToSerial();
      break;
      
    case CMD_RESTORE:
      restoreFromSerial();
      break;
      
    case CMD_EXIT:
      operatingMode = MODE_NORMAL;
      autoUpdateStatusLED(); // NEW: Return to normal status
      Serial.println("Returning to normal operation.");
      break;
      
    case CMD_IR_ON:
      config.irEnabled = true;
      IrReceiver.begin(IR_RECEIVER_PIN, false);
      smartSaveToEEPROM();
      Serial.println("IR enabled.");
      break;
      
    case CMD_IR_OFF:
      config.irEnabled = false;
      IrReceiver.end();
      smartSaveToEEPROM();
      Serial.println("IR disabled.");
      break;
      
    case CMD_MODE:
      if (params.length() == 0) {
        Serial.println("Current mode: " + getModeName(currentMode));
        Serial.println("Available modes: scanning, alert, idle");
      } else {
        params.toLowerCase();
        if (params == "scanning") {
          currentMode = MODE_SCANNING;
          statusLEDScanningMode(); // Update status LED
          Serial.println("Mode set to SCANNING");
        } else if (params == "alert") {
          currentMode = MODE_ALERT;
          statusLEDAlertMode(); // Update status LED
          Serial.println("Mode set to ALERT");
        } else if (params == "idle") {
          currentMode = MODE_IDLE;
          statusLEDIdleMode(); // Update status LED
          Serial.println("Mode set to IDLE");
        } else {
          Serial.println("Invalid mode. Use: scanning, alert, or idle");
          break;
        }
        setServoParameters();
        updateDetailColorForMode(currentMode); // NEW: Update detail LEDs for mode
        config.savedMode = currentMode;
        if (!isAwake) {
          isAwake = true;
        }
        lastActivityTime = millis();
      }
      break;

    case CMD_DETAIL:
      handleDetailCommand(params);
      break;

    case CMD_WIFI:
      handleWiFiCommand(params);
      break;

    case CMD_AP:
      handleAPCommand(params);
      break;

    default:
      Serial.println("Unknown command. Type 'help' for available commands.");
      break;
  }
}

//========================================
// AUDIO SYSTEM FUNCTIONS - UPDATED WITH STATUS LED
//========================================

void playSound(int fileNumber) {
  if (!isAudioReady) {
    Serial.println("Audio system not ready");
    statusLEDError(); // NEW: Show error
    return;
  }
  
  if (fileNumber < 1 || fileNumber > 255) {
    Serial.printf("Invalid file number: %d\n", fileNumber);
    return;
  }
  
  mp3.playFolderTrack(4, fileNumber);
  lastActivityTime = millis();
  statusLEDAudioActivity(); // NEW: Flash green for audio
  Serial.printf("Playing sound file %d\n", fileNumber);
}

void playRandomSound(int folder) {
  if (!isAudioReady) {
    Serial.println("Audio system not ready");
    statusLEDError(); // NEW: Show error
    return;
  }
  
  int trackCount = mp3.getFolderTrackCount(folder);
  if (trackCount > 0) {
    int track = random(1, trackCount + 1);
    mp3.playFolderTrack(folder, track);
    lastActivityTime = millis();
    statusLEDAudioActivity(); // NEW: Flash green for audio
    Serial.printf("Playing random sound: folder %d, track %d\n", folder, track);
  } else {
    Serial.printf("No tracks found in folder %d\n", folder);
  }
}

void setVolume(uint8_t volume) {
  if (!isValidVolume(volume)) {
    Serial.printf("Invalid volume level: %d\n", volume);
    return;
  }
  
  config.savedVolume = volume;
  if (isAudioReady) {
    mp3.setVolume(volume);
    Serial.printf("Volume set to %d\n", volume);
  } else {
    Serial.println("Audio system not ready, volume setting saved");
  }
}

void updateAudio() {
  if (!isAudioReady || !isAwake) {
    return;
  }
  
  mp3.loop();
  
  if (isWaitingForNextTrack && millis() >= nextPlayTime) {
    isWaitingForNextTrack = false;
    
    int folder = 1;
    switch (currentMode) {
      case MODE_SCANNING:
        folder = 1;
        break;
      case MODE_ALERT:
        folder = 2;
        break;
      case MODE_IDLE:
        return;
    }
    
    playRandomSound(folder);
  }
}

//========================================
// IR LEARNING AND SCANNING - UPDATED WITH STATUS LED
//========================================

void enterLearningMode() {
  operatingMode = MODE_IR_LEARNING;
  currentButtonIndex = 0;
  waitingForIR = false;
  
  statusLEDLearningMode(); // NEW: Show learning mode
  
  Serial.println("\n=== IR LEARNING MODE ===");
  
  if (config.buttonCount == 0) {
    Serial.print("How many buttons does your remote have? (1-21): ");
    String buttonInput;
    while (Serial.available() == 0) {
      delay(50); 
    }
    buttonInput = Serial.readStringUntil('\n');
    buttonInput.trim();

    config.buttonCount = constrain(buttonInput.toInt(), 1, 21);
    Serial.printf("Learning %d buttons.\n", config.buttonCount);
    
    for (int i = 0; i < config.buttonCount && i < 17; i++) {
      strncpy(config.buttons[i].name, standard17Buttons[i], sizeof(config.buttons[i].name) - 1);
      config.buttons[i].name[sizeof(config.buttons[i].name) - 1] = '\0';  // Ensure null termination
      config.buttons[i].isConfigured = false;
    }

    for (int i = 17; i < config.buttonCount; i++) {
      snprintf(config.buttons[i].name, sizeof(config.buttons[i].name), "BTN%d", i + 1);
      config.buttons[i].isConfigured = false;
    }
  }
  
  Serial.println("\nPress each button when prompted.");
  Serial.println("Type 'exit' to cancel learning.");
  Serial.printf("\nPress button '%s'\n", config.buttons[0].name);
  
  waitingForIR = true;
  learningTimeout = millis();
}

void handleLearningMode() {
  if (waitingForIR && millis() - learningTimeout > 30000) {
    Serial.println("\nLearning timeout!");
    operatingMode = MODE_NORMAL;
    autoUpdateStatusLED(); // NEW: Return to normal status
    return;
  }
  
  if (Serial.available()) {
    String cmd = Serial.readStringUntil('\n');
    if (cmd.indexOf("exit") >= 0) {
      operatingMode = MODE_NORMAL;
      autoUpdateStatusLED(); // NEW: Return to normal status
      Serial.println("Learning cancelled");
      return;
    }
  }
  
  uint32_t code;
  if (waitingForIR && checkForIRCommand(code)) {
    Serial.printf("Learned: 0x%08X\n", (unsigned int)code);
    
    config.buttons[currentButtonIndex].code = code;
    config.buttons[currentButtonIndex].isConfigured = true;
    
    currentButtonIndex++;
    waitingForIR = false;
    
    if (currentButtonIndex < config.buttonCount) {
      Serial.printf("\nPress button '%s'\n", config.buttons[currentButtonIndex].name);
      waitingForIR = true;
      learningTimeout = millis();
    } else {
      Serial.println("\n=== Learning Complete! ===");
      Serial.printf("Successfully programmed %d buttons:\n", config.buttonCount);
      for (int i = 0; i < config.buttonCount; i++) {
        Serial.printf("  %s = 0x%08X\n", config.buttons[i].name, (unsigned int)config.buttons[i].code);
      }
      smartSaveToEEPROM();
      operatingMode = MODE_NORMAL;
      autoUpdateStatusLED(); // NEW: Return to normal status
    }
  }
}

void handleScannerMode() {
  if (Serial.available()) {
    String cmd = Serial.readStringUntil('\n');
    if (cmd.indexOf("exit") >= 0 || cmd.indexOf("normal") >= 0) {
      operatingMode = MODE_NORMAL;
      autoUpdateStatusLED(); // NEW: Return to normal status
      Serial.println("Exiting scanner mode");
      return;
    }
  }
  
  uint32_t code;
  if (checkForIRCommand(code)) {
    statusLEDIRActivity(); // NEW: Flash white for IR activity
    Serial.printf("Received: 0x%08X (Protocol: %s)\n", (unsigned int)code,
                  IrReceiver.getProtocolString());
  }
}

void handleIRCommand(uint32_t code) {
  statusLEDIRActivity(); // NEW: Flash white for IR command
  
  Serial.printf("IR: 0x%08X, Protocol: %s\n", (unsigned int)code, 
                IrReceiver.getProtocolString());

  for (int i = 0; i < config.buttonCount; i++) {
    if (config.buttons[i].isConfigured && config.buttons[i].code == code) {
      Serial.printf("Executing command for button: %s\n", config.buttons[i].name);
      executeButtonCommand(config.buttons[i].name);
      return;
    }
  }
  
  Serial.println("Unknown IR code");
}

void executeButtonCommand(const char* buttonName) {
  Serial.printf("Executing button command: %s\n", buttonName);
  
  if (!isAwake) {
    isAwake = true;
  }
  lastActivityTime = millis();

  // Servo movement commands
  if (strcmp(buttonName, "UP") == 0) {
    eyePan.targetPosition = config.eyePanCenter;
    eyeTilt.targetPosition = config.eyeTiltMax;
    eyePanServo.write(eyePan.targetPosition);
    eyeTiltServo.write(eyeTilt.targetPosition);
    statusLEDServoActivity(); // NEW: Flash blue for servo
    return;
  }
  else if (strcmp(buttonName, "DOWN") == 0) {
    eyePan.targetPosition = config.eyePanCenter;
    eyeTilt.targetPosition = config.eyeTiltMin;
    eyePanServo.write(eyePan.targetPosition);
    eyeTiltServo.write(eyeTilt.targetPosition);
    statusLEDServoActivity(); // NEW: Flash blue for servo
    return;
  } 
  else if (strcmp(buttonName, "LEFT") == 0) {
    eyePan.targetPosition = config.eyePanMax;
    eyeTilt.targetPosition = config.eyeTiltCenter;
    eyePanServo.write(eyePan.targetPosition);
    eyeTiltServo.write(eyeTilt.targetPosition);
    statusLEDServoActivity(); // NEW: Flash blue for servo
    return;
  } 
  else if (strcmp(buttonName, "RIGHT") == 0) {
    eyePan.targetPosition = config.eyePanMin;
    eyeTilt.targetPosition = config.eyeTiltCenter;
    eyePanServo.write(eyePan.targetPosition);
    eyeTiltServo.write(eyeTilt.targetPosition);
    statusLEDServoActivity(); // NEW: Flash blue for servo
    return;
  } 
  else if (strcmp(buttonName, "OK") == 0) {
    centerAllServos();
    statusLEDServoActivity(); // NEW: Flash blue for servo
    return;
  } 
  // Mode changes with status LED updates
  else if (strcmp(buttonName, "1") == 0) {
    currentMode = MODE_SCANNING;
    setServoParameters();
    statusLEDScanningMode(); // NEW: Update status LED
    uint32_t iceBlue = Adafruit_NeoPixel::Color(80, 150, 255);
    setEyeColor(iceBlue, iceBlue);
    Serial.println("Scanning mode: Eyes set to ice blue");
    return;
  } 
  else if (strcmp(buttonName, "2") == 0) {
    currentMode = MODE_ALERT;
    setServoParameters();
    statusLEDAlertMode(); // NEW: Update status LED
    uint32_t alertRed = Adafruit_NeoPixel::Color(255, 0, 0);
    setEyeColor(alertRed, alertRed);
    Serial.println("Alert mode: Eyes set to red");
    return;
  }
  else if (strcmp(buttonName, "3") == 0) {
    currentMode = MODE_IDLE;
    setServoParameters();
    statusLEDIdleMode(); // NEW: Update status LED
    uint32_t dimAmber = Adafruit_NeoPixel::Color(100, 60, 0);
    setEyeColor(dimAmber, dimAmber);
    Serial.println("Idle mode: Eyes set to dim amber");
    return;
  } 
  // Audio commands with status LED flash
  else if (strcmp(buttonName, "4") == 0) {
    if (isAudioReady) {
      playRandomSound(1);
      Serial.println("Playing random scanning sound");
    }
    return;
  }
  else if (strcmp(buttonName, "5") == 0) {
    if (isAudioReady) {
      playRandomSound(2);
      Serial.println("Playing random alert sound");
    }
    return;
  }
  else if (strcmp(buttonName, "6") == 0) {
    if (isAudioReady) {
      playRandomSound(4);
      Serial.println("Playing random voice line");
    }
    return;
  }
  // Button 7: Start Demo Mode
  else if (strcmp(buttonName, "7") == 0) {
    enterDemoMode();
    Serial.println("Starting comprehensive demo mode");
    return;
  }
  // Button 8: Toggle Detail LEDs
  else if (strcmp(buttonName, "8") == 0) {
    setDetailEnabled(!detailState.enabled);
    Serial.printf("Detail LEDs: %s\n", detailState.enabled ? "ON" : "OFF");
    return;
  }
  // Button 9: Cycle Eye Animation Modes
  else if (strcmp(buttonName, "9") == 0) {
    static int animationModeIndex = 0;
    const PixelMode modes[] = {SOLID_COLOR, FLICKER, PULSE, SCANNER, HEARTBEAT, ALARM};
    const char* modeNames[] = {"Solid", "Flicker", "Pulse", "Scanner", "Heartbeat", "Alarm"};
    const int modeCount = 6;

    animationModeIndex = (animationModeIndex + 1) % modeCount;

    switch(modes[animationModeIndex]) {
      case SOLID_COLOR:
        setEyeColor(getK2SOBlue(), getK2SOBlue());
        break;
      case FLICKER:
        startFlickerMode();
        break;
      case PULSE:
        startPulseMode();
        break;
      case SCANNER:
        startScannerMode();
        break;
      case HEARTBEAT:
        startHeartbeatMode();
        break;
      case ALARM:
        startAlarmMode();
        break;
      default:
        break;
    }

    Serial.printf("Eye Animation: %s\n", modeNames[animationModeIndex]);
    return;
  }
  // Color cycling commands
  else if (strcmp(buttonName, "*") == 0) {
    uint32_t colors[COLOR_COUNT] = {
      Adafruit_NeoPixel::Color(80, 150, 255),   // Ice blue
      Adafruit_NeoPixel::Color(255, 0, 0),     // Red
      Adafruit_NeoPixel::Color(0, 255, 0),     // Green
      Adafruit_NeoPixel::Color(255, 255, 0),   // Yellow
      Adafruit_NeoPixel::Color(255, 0, 255),   // Magenta
      Adafruit_NeoPixel::Color(255, 255, 255)  // White
    };
    
    currentColorIndex = (currentColorIndex + 1) % COLOR_COUNT;
    setEyeColor(colors[currentColorIndex], colors[currentColorIndex]);
    Serial.printf("Color forward: %d\n", currentColorIndex);
    return;
  }
  else if (strcmp(buttonName, "#") == 0) {
    uint32_t colors[COLOR_COUNT] = {
      Adafruit_NeoPixel::Color(80, 150, 255),   // Ice blue
      Adafruit_NeoPixel::Color(255, 0, 0),     // Red
      Adafruit_NeoPixel::Color(0, 255, 0),     // Green
      Adafruit_NeoPixel::Color(255, 255, 0),   // Yellow
      Adafruit_NeoPixel::Color(255, 0, 255),   // Magenta
      Adafruit_NeoPixel::Color(255, 255, 255)  // White
    };
    
    currentColorIndex = (currentColorIndex - 1 + COLOR_COUNT) % COLOR_COUNT;
    setEyeColor(colors[currentColorIndex], colors[currentColorIndex]);
    Serial.printf("Color backward: %d\n", currentColorIndex);
    return;
  }
  // Eyes on/off toggle
  else if (strcmp(buttonName, "0") == 0) {
    if (leftEyeCurrentColor == 0 && rightEyeCurrentColor == 0) {
      uint32_t white = Adafruit_NeoPixel::Color(255, 255, 255);
      setEyeColor(white, white);
      Serial.println("Eyes ON");
    } else {
      uint32_t off = Adafruit_NeoPixel::Color(0, 0, 0);
      setEyeColor(off, off);
      Serial.println("Eyes OFF");
    }
    return;
  }
  else {
    Serial.printf("Unknown button command: %s\n", buttonName);
    return;
  }
}

//========================================
// REST OF EXISTING FUNCTIONS (unchanged except for status LED calls)
//========================================

// [All other existing functions remain the same, but I'll add status LED calls 
// where appropriate for servo movements, test sequences, etc.]

void centerAllServos() {
  Serial.println("Centering all servos");
  
  eyePan.targetPosition = config.eyePanCenter;
  eyeTilt.targetPosition = config.eyeTiltCenter;
  headPan.targetPosition = config.headPanCenter;
  headTilt.targetPosition = config.headTiltCenter;
  
  eyePanServo.write(eyePan.targetPosition);
  eyeTiltServo.write(eyeTilt.targetPosition);
  headPanServo.write(headPan.targetPosition);
  headTiltServo.write(headTilt.targetPosition);
  
  eyePan.currentPosition = eyePan.targetPosition;
  eyeTilt.currentPosition = eyeTilt.targetPosition;
  headPan.currentPosition = headPan.targetPosition;
  headTilt.currentPosition = headTilt.targetPosition;
  
  servoMovements++;
  statusLEDServoActivity(); // NEW: Flash blue for servo activity
}

void runTestSequence(String params) {
  Serial.println("\n=== HARDWARE TEST SEQUENCE ===");
  if (params.length() > 0) {
    Serial.println("Test options: servo, led, audio, ir, all");
    Serial.println("Usage: test [option]");
  }
  
  operatingMode = MODE_TEST;
  statusLEDTestMode(); // NEW: Set test mode status
  testStep = 0;
  testTimer = millis();
}

// [Include all other existing functions from the original handlers.cpp]
// [They remain unchanged except where status LED calls are appropriate]

// For brevity, I'm not including every single function, but the pattern is:
// - Add statusLEDServoActivity() for servo movements
// - Add statusLEDAudioActivity() for audio operations  
// - Add statusLEDError() for error conditions
// - Add appropriate mode status calls when modes change

//========================================
// SPECIALIZED COMMAND HANDLERS - CONTINUED
//========================================

void handleSoundCommand(String params) {
  if (params.length() == 0) {
    Serial.println("Sound commands:");
    Serial.println(F("  sound volume [0-30]          - Set volume"));
    Serial.println(F("  sound play [file_number]     - Play specific file"));
    Serial.println(F("  sound folder [folder] [track] - Play from folder"));
    Serial.println(F("  sound stop                   - Stop playback"));
    Serial.println(F("  sound show                   - Show settings"));
    return;
  }
  
  String args[3];
  int argCount = 0;
  int startIdx = 0;
  
  for (int i = 0; i <= params.length() && argCount < 3; i++) {
    if (i == params.length() || params[i] == ' ') {
      if (i > startIdx) {
        args[argCount++] = params.substring(startIdx, i);
      }
      startIdx = i + 1;
    }
  }
  
  if (args[0] == "show") {
    Serial.println("\n=== SOUND SETTINGS ===");
    Serial.printf("Volume: %d\n", config.savedVolume);
    Serial.printf("Audio ready: %s\n", isAudioReady ? "Yes" : "No");
    Serial.printf("Pause range: %d-%d ms\n", config.soundPauseMin, config.soundPauseMax);
  }
  else if (args[0] == "volume" && argCount >= 2) {
    int volume = constrain(args[1].toInt(), 0, 30);
    setVolume(volume);
    Serial.printf("Volume set to: %d\n", volume);
  }
  else if (args[0] == "play" && argCount >= 2) {
    int fileNum = args[1].toInt();
    playSound(fileNum);
    Serial.printf("Playing file: %d\n", fileNum);
  }
  else if (args[0] == "folder" && argCount >= 3) {
    int folder = args[1].toInt();
    int track = args[2].toInt();
    if (isAudioReady) {
      mp3.playFolderTrack(folder, track);
      statusLEDAudioActivity(); // NEW: Flash green for audio
      Serial.printf("Playing folder %d, track %d\n", folder, track);
    } else {
      Serial.println("Audio system not ready");
      statusLEDError(); // NEW: Show error
    }
  }
  else if (args[0] == "stop") {
    if (isAudioReady) {
      mp3.stop();
      Serial.println("Playback stopped");
    }
  }
}

void handleServoCommand(String params) {
  if (params.length() == 0) {
    Serial.println("Servo commands:");
    Serial.println(F("  servo eye center [pan] [tilt]   - Set eye center positions"));
    Serial.println(F("  servo eye limits [minP] [maxP] [minT] [maxT] - Set eye limits"));
    Serial.println(F("  servo head center [pan] [tilt] - Set head center positions"));
    Serial.println(F("  servo head limits [minP] [maxP] [minT] [maxT] - Set head limits"));
    Serial.println(F("  servo test [eye/head/all]      - Test servo movement"));
    Serial.println(F("  servo show                     - Show all servo settings"));
    return;
  }
  
  String args[6];
  int argCount = 0;
  int startIdx = 0;
  
  for (int i = 0; i <= params.length() && argCount < 6; i++) {
    if (i == params.length() || params[i] == ' ') {
      if (i > startIdx) {
        args[argCount++] = params.substring(startIdx, i);
      }
      startIdx = i + 1;
    }
  }
  
  if (args[0] == "show") {
    Serial.println("\n=== SERVO SETTINGS ===");
    Serial.printf("Eye Pan: Center=%d, Range=%d-%d\n", config.eyePanCenter, config.eyePanMin, config.eyePanMax);
    Serial.printf("Eye Tilt: Center=%d, Range=%d-%d\n", config.eyeTiltCenter, config.eyeTiltMin, config.eyeTiltMax);
    Serial.printf("Head Pan: Center=%d, Range=%d-%d\n", config.headPanCenter, config.headPanMin, config.headPanMax);
    Serial.printf("Head Tilt: Center=%d, Range=%d-%d\n", config.headTiltCenter, config.headTiltMin, config.headTiltMax);
  }
  else if (args[0] == "eye" && args[1] == "center" && argCount >= 4) {
    config.eyePanCenter = constrain(args[2].toInt(), 0, 180);
    config.eyeTiltCenter = constrain(args[3].toInt(), 0, 180);
    eyePan.currentPosition = config.eyePanCenter;
    eyeTilt.currentPosition = config.eyeTiltCenter;
    eyePanServo.write(config.eyePanCenter);
    eyeTiltServo.write(config.eyeTiltCenter);
    statusLEDServoActivity(); // NEW: Flash blue for servo
    Serial.printf("Eye centers set: Pan=%d, Tilt=%d\n", config.eyePanCenter, config.eyeTiltCenter);
    smartSaveToEEPROM();
  }
  else if (args[0] == "eye" && args[1] == "limits" && argCount >= 6) {
    config.eyePanMin = constrain(args[2].toInt(), 0, 180);
    config.eyePanMax = constrain(args[3].toInt(), 0, 180);
    config.eyeTiltMin = constrain(args[4].toInt(), 0, 180);
    config.eyeTiltMax = constrain(args[5].toInt(), 0, 180);
    
    eyePan.minRange = config.eyePanMin;
    eyePan.maxRange = config.eyePanMax;
    eyeTilt.minRange = config.eyeTiltMin;
    eyeTilt.maxRange = config.eyeTiltMax;
    
    Serial.printf("Eye limits set: Pan=%d-%d, Tilt=%d-%d\n", 
                  config.eyePanMin, config.eyePanMax, config.eyeTiltMin, config.eyeTiltMax);
    smartSaveToEEPROM();
  }
  else if (args[0] == "head" && args[1] == "center" && argCount >= 4) {
    config.headPanCenter = constrain(args[2].toInt(), 0, 180);
    config.headTiltCenter = constrain(args[3].toInt(), 0, 180);
    headPan.currentPosition = config.headPanCenter;
    headTilt.currentPosition = config.headTiltCenter;
    headPanServo.write(config.headPanCenter);
    headTiltServo.write(config.headTiltCenter);
    statusLEDServoActivity(); // NEW: Flash blue for servo
    Serial.printf("Head centers set: Pan=%d, Tilt=%d\n", config.headPanCenter, config.headTiltCenter);
    smartSaveToEEPROM();
  }
  else if (args[0] == "head" && args[1] == "limits" && argCount >= 6) {
    config.headPanMin = constrain(args[2].toInt(), 0, 180);
    config.headPanMax = constrain(args[3].toInt(), 0, 180);
    config.headTiltMin = constrain(args[4].toInt(), 0, 180);
    config.headTiltMax = constrain(args[5].toInt(), 0, 180);
    
    headPan.minRange = config.headPanMin;
    headPan.maxRange = config.headPanMax;
    headTilt.minRange = config.headTiltMin;
    headTilt.maxRange = config.headTiltMax;
    
    Serial.printf("Head limits set: Pan=%d-%d, Tilt=%d-%d\n", 
                  config.headPanMin, config.headPanMax, config.headTiltMin, config.headTiltMax);
    smartSaveToEEPROM();
  }
  else if (args[0] == "test") {
    if (argCount < 2 || args[1] == "all") {
      Serial.println("Testing all servos...");
      centerAllServos();
      delay(1000);
      
      // Test eye servos
      eyePan.targetPosition = eyePan.minRange;
      eyeTilt.targetPosition = eyeTilt.minRange;
      eyePanServo.write(eyePan.targetPosition);
      eyeTiltServo.write(eyeTilt.targetPosition);
      statusLEDServoActivity(); // NEW: Flash blue for servo
      delay(1000);
      
      eyePan.targetPosition = eyePan.maxRange;
      eyeTilt.targetPosition = eyeTilt.maxRange;
      eyePanServo.write(eyePan.targetPosition);
      eyeTiltServo.write(eyeTilt.targetPosition);
      statusLEDServoActivity(); // NEW: Flash blue for servo
      delay(1000);
      
      // Test head servos
      headPan.targetPosition = headPan.minRange;
      headTilt.targetPosition = headTilt.minRange;
      headPanServo.write(headPan.targetPosition);
      headTiltServo.write(headTilt.targetPosition);
      statusLEDServoActivity(); // NEW: Flash blue for servo
      delay(1000);
      
      headPan.targetPosition = headPan.maxRange;
      headTilt.targetPosition = headTilt.maxRange;
      headPanServo.write(headPan.targetPosition);
      headTiltServo.write(headTilt.targetPosition);
      statusLEDServoActivity(); // NEW: Flash blue for servo
      delay(1000);
      
      centerAllServos();
      Serial.println("Servo test complete");
    }
  }
}

void handleLEDCommand(String params) {
  if (params.length() == 0) {
    Serial.println("LED commands:");
    Serial.println(F("  led brightness [0-255]       - Set eye brightness"));
    Serial.println(F("  led color [r] [g] [b]        - Set eye color (0-255 each)"));
    Serial.println(F("  led mode [mode]              - Set animation mode"));
    Serial.println("    Modes: solid, flicker, pulse, scanner, heartbeat, alarm");
    Serial.println("    13-LED only: iris, targeting, ring_scanner, spiral, focus, radar");
    Serial.println(F("  led eye [7led/13led]         - Set eye hardware version"));
    Serial.println("    7led:  7-LED version (LEDs 0-6)");
    Serial.println("    13led: 13-LED version (LED 0=center, 1-12=ring) - DEFAULT");
    Serial.println(F("  led test [left/right/both]  - Test LEDs"));
    Serial.println(F("  led show                     - Show current settings"));
    Serial.println(F("  led status [on/off]          - Enable/disable status LED"));
    Serial.println(F("  led status brightness [0-255] - Set status LED brightness"));
    Serial.println(F("  led status test              - Test status LED"));
    return;
  }
  
  String args[4];
  int argCount = 0;
  int startIdx = 0;
  
  for (int i = 0; i <= params.length() && argCount < 4; i++) {
    if (i == params.length() || params[i] == ' ') {
      if (i > startIdx) {
        args[argCount++] = params.substring(startIdx, i);
      }
      startIdx = i + 1;
    }
  }
  
  if (args[0] == "show") {
    Serial.println("\n=== LED SETTINGS ===");
    Serial.printf("Eye Hardware Version: %s\n", getEyeHardwareVersionName().c_str());
    Serial.printf("Active LEDs per Eye: %d\n", getActiveEyeLEDCount());
    Serial.printf("Eye Brightness: %d/255\n", currentBrightness);
    Serial.printf("Current mode: %s\n", getAnimationModeName().c_str());
    Serial.printf("Left eye color: 0x%06lX\n", (unsigned long)leftEyeCurrentColor);
    Serial.printf("Right eye color: 0x%06lX\n", (unsigned long)rightEyeCurrentColor);
    Serial.printf("Status LED: %s (Brightness: %d)\n",
                  config.statusLedEnabled ? "Enabled" : "Disabled",
                  config.statusLedBrightness);
    Serial.printf("Status LED State: %s\n", getStatusLEDStateName(getCurrentStatusLEDState()).c_str());
  }
  else if (args[0] == "brightness" && argCount >= 2) {
    int brightness = constrain(args[1].toInt(), 0, 255);
    setEyeBrightness(brightness);
    config.eyeBrightness = brightness;
    Serial.printf("Eye brightness set to: %d\n", brightness);
  }
  else if (args[0] == "color" && argCount >= 4) {
    int r = constrain(args[1].toInt(), 0, 255);
    int g = constrain(args[2].toInt(), 0, 255);
    int b = constrain(args[3].toInt(), 0, 255);
    
    uint32_t color = Adafruit_NeoPixel::Color(r, g, b);
    setEyeColor(color, color);
    Serial.printf("Eye color set to RGB(%d, %d, %d)\n", r, g, b);
  }
  else if (args[0] == "mode" && argCount >= 2) {
    String mode = args[1];
    mode.toLowerCase();

    if (mode == "solid") {
      currentPixelMode = SOLID_COLOR;
      stopAllAnimations();
      Serial.println("Mode set to solid color");
    } else if (mode == "flicker") {
      startFlickerMode();
      Serial.println("Mode set to flicker");
    } else if (mode == "pulse") {
      startPulseMode();
      Serial.println("Mode set to pulse");
    } else if (mode == "scanner") {
      startScannerMode();
      Serial.println("Mode set to scanner");
    } else if (mode == "iris") {
      if (activeEyeLEDCount == 13) {
        startIrisMode();
        Serial.println("Mode set to iris (13-LED)");
      } else {
        Serial.println("Error: Iris mode requires 13-LED eyes. Use 'led eye 13led' first.");
      }
    } else if (mode == "targeting") {
      if (activeEyeLEDCount == 13) {
        startTargetingMode();
        Serial.println("Mode set to targeting (13-LED)");
      } else {
        Serial.println("Error: Targeting mode requires 13-LED eyes. Use 'led eye 13led' first.");
      }
    } else if (mode == "ring_scanner") {
      if (activeEyeLEDCount == 13) {
        startRingScannerMode();
        Serial.println("Mode set to ring scanner (13-LED)");
      } else {
        Serial.println("Error: Ring scanner mode requires 13-LED eyes. Use 'led eye 13led' first.");
      }
    } else if (mode == "spiral") {
      if (activeEyeLEDCount == 13) {
        startSpiralMode();
        Serial.println("Mode set to spiral (13-LED)");
      } else {
        Serial.println("Error: Spiral mode requires 13-LED eyes. Use 'led eye 13led' first.");
      }
    } else if (mode == "focus") {
      if (activeEyeLEDCount == 13) {
        startFocusMode();
        Serial.println("Mode set to focus (13-LED)");
      } else {
        Serial.println("Error: Focus mode requires 13-LED eyes. Use 'led eye 13led' first.");
      }
    } else if (mode == "radar") {
      if (activeEyeLEDCount == 13) {
        startRadarMode();
        Serial.println("Mode set to radar (13-LED)");
      } else {
        Serial.println("Error: Radar mode requires 13-LED eyes. Use 'led eye 13led' first.");
      }
    } else if (mode == "heartbeat") {
      startHeartbeatMode();
      Serial.println("Mode set to heartbeat (synchronized)");
    } else if (mode == "alarm") {
      startAlarmMode();
      Serial.println("Mode set to alarm (synchronized)");
    } else {
      Serial.println("Invalid mode.");
      Serial.println("Available: solid, flicker, pulse, scanner, heartbeat, alarm");
      Serial.println("13-LED only: iris, targeting, ring_scanner, spiral, focus, radar");
    }
  }
  else if (args[0] == "eye" && argCount >= 2) {
    String eyeVersion = args[1];
    eyeVersion.toLowerCase();

    if (eyeVersion == "7led") {
      setEyeHardwareVersion(EYE_VERSION_7LED);
    } else if (eyeVersion == "13led") {
      setEyeHardwareVersion(EYE_VERSION_13LED);
    } else {
      Serial.println("Invalid eye version. Use: 7led or 13led");
    }
  }
  else if (args[0] == "status" && argCount >= 2) {
    if (args[1] == "on") {
      enableStatusLED(true);
      Serial.println("Status LED enabled");
    } else if (args[1] == "off") {
      enableStatusLED(false);
      Serial.println("Status LED disabled");
    } else if (args[1] == "brightness" && argCount >= 3) {
      int brightness = constrain(args[2].toInt(), 0, 255);
      setStatusLEDBrightness(brightness);
      Serial.printf("Status LED brightness set to: %d\n", brightness);
    } else if (args[1] == "test") {
      statusLEDSystemTest();
    }
  }
  else if (args[0] == "test") {
    String target = (argCount >= 2) ? args[1] : "both";
    
    Serial.println("LED test sequence starting...");
    
    if (target == "left" || target == "both") {
      Serial.println("Testing left eye...");
      uint32_t red = Adafruit_NeoPixel::Color(255, 0, 0);
      uint32_t green = Adafruit_NeoPixel::Color(0, 255, 0);
      uint32_t blue = Adafruit_NeoPixel::Color(0, 0, 255);
      uint32_t off = Adafruit_NeoPixel::Color(0, 0, 0);

      // Reduced delays for better responsiveness during testing
      setLeftEyeColor(red); delay(300);
      setLeftEyeColor(green); delay(300);
      setLeftEyeColor(blue); delay(300);
      setLeftEyeColor(off); delay(300);
    }

    if (target == "right" || target == "both") {
      Serial.println("Testing right eye...");
      uint32_t red = Adafruit_NeoPixel::Color(255, 0, 0);
      uint32_t green = Adafruit_NeoPixel::Color(0, 255, 0);
      uint32_t blue = Adafruit_NeoPixel::Color(0, 0, 255);
      uint32_t white = Adafruit_NeoPixel::Color(255, 255, 255);

      // Reduced delays for better responsiveness during testing
      setRightEyeColor(red); delay(300);
      setRightEyeColor(green); delay(300);
      setRightEyeColor(blue); delay(300);
      setRightEyeColor(white);
    }
    
    Serial.println("LED test complete");
  }
}

//========================================
// DETAIL LED COMMAND HANDLER (NEW - WS2812)
//========================================

void handleDetailCommand(String params) {
  if (params.length() == 0) {
    Serial.println("\n=== Detail LED Commands ===");
    Serial.println(F("  detail show                     - Show current settings"));
    Serial.println(F("  detail count [1-8]              - Set number of active LEDs (default: 5)"));
    Serial.println(F("  detail brightness [0-255]       - Set brightness"));
    Serial.println(F("  detail color [r] [g] [b]        - Set RGB color (0-255 each)"));
    Serial.println(F("  detail pattern [name]           - Set animation pattern"));
    Serial.println("    Patterns: blink, fade, chase, pulse, random");
    Serial.println(F("  detail on                       - Enable detail LEDs"));
    Serial.println(F("  detail off                      - Disable detail LEDs"));
    Serial.println(F("  detail auto [on/off]            - Auto color based on mode"));
    Serial.println(F("  detail test                     - Run test sequence"));
    Serial.println(F("===========================\n"));
    return;
  }

  String args[4];
  int argCount = 0;
  int startIdx = 0;

  for (int i = 0; i <= params.length() && argCount < 4; i++) {
    if (i == params.length() || params[i] == ' ') {
      if (i > startIdx) {
        args[argCount++] = params.substring(startIdx, i);
      }
      startIdx = i + 1;
    }
  }

  if (args[0] == "show") {
    printDetailLEDStatus();
  }
  else if (args[0] == "count" && argCount >= 2) {
    int count = args[1].toInt();
    setDetailCount(count);
  }
  else if (args[0] == "brightness" && argCount >= 2) {
    int brightness = constrain(args[1].toInt(), 0, 255);
    setDetailBrightness(brightness);
  }
  else if (args[0] == "color" && argCount >= 4) {
    int r = constrain(args[1].toInt(), 0, 255);
    int g = constrain(args[2].toInt(), 0, 255);
    int b = constrain(args[3].toInt(), 0, 255);
    setDetailColor(r, g, b);
  }
  else if (args[0] == "pattern" && argCount >= 2) {
    String pattern = args[1];
    pattern.toLowerCase();

    if (pattern == "blink") {
      startDetailBlink();
    } else if (pattern == "fade") {
      startDetailFade();
    } else if (pattern == "chase") {
      startDetailChase();
    } else if (pattern == "pulse") {
      startDetailPulse();
    } else if (pattern == "random") {
      startDetailRandom();
    } else {
      Serial.println("Invalid pattern. Use: blink, fade, chase, pulse, or random");
    }
  }
  else if (args[0] == "on") {
    setDetailEnabled(true);
  }
  else if (args[0] == "off") {
    setDetailEnabled(false);
  }
  else if (args[0] == "auto" && argCount >= 2) {
    String autoMode = args[1];
    autoMode.toLowerCase();

    if (autoMode == "on") {
      setDetailAutoColorMode(true);
    } else if (autoMode == "off") {
      setDetailAutoColorMode(false);
    } else {
      Serial.println("Use: detail auto on/off");
    }
  }
  else if (args[0] == "test") {
    Serial.println("\n=== Detail LED Test Sequence ===");
    Serial.println("Running quick pattern tests (1s each)...");

    // Test all patterns with reduced delays for better responsiveness
    Serial.println("Testing BLINK pattern (red)...");
    setDetailColor(255, 0, 0);
    startDetailBlink();
    delay(1000);  // Reduced from 2000ms

    Serial.println("Testing FADE pattern (green)...");
    setDetailColor(0, 255, 0);
    startDetailFade();
    delay(1000);  // Reduced from 2000ms

    Serial.println("Testing PULSE pattern (blue)...");
    setDetailColor(0, 0, 255);
    startDetailPulse();
    delay(1000);  // Reduced from 2000ms

    Serial.println("Testing CHASE pattern (yellow)...");
    setDetailColor(255, 255, 0);
    startDetailChase();
    delay(1000);  // Reduced from 2000ms

    Serial.println("Testing RANDOM pattern (purple)...");
    setDetailColor(255, 0, 255);
    startDetailRandom();
    delay(1000);  // Reduced from 2000ms

    // Return to default
    Serial.println("Returning to default (red blink)...");
    setDetailDefaultRed();

    Serial.println("Detail LED test complete!\n");
  }
  else {
    Serial.println("Invalid detail command. Type 'detail' for help.");
  }
}

//========================================
// WIFI CONFIGURATION COMMAND HANDLER
//========================================

void handleWiFiCommand(String params) {
  if (params.length() == 0) {
    Serial.println(F("\n=== WiFi Configuration ==="));
    Serial.println(F("  wifi set <ssid> <password>       - Configure WiFi credentials"));
    Serial.println(F("  wifi set \"ssid\" \"password\"       - Use quotes for spaces"));
    Serial.println(F("  wifi show                        - Show current WiFi settings"));
    Serial.println(F("  wifi reset                       - Clear WiFi configuration"));
    Serial.println(F("  wifi reconnect                   - Reconnect to WiFi"));
    Serial.println(F("\nExample: wifi set \"HONOR Magoc V2\" \"my password\""));
    Serial.println(F("===========================\n"));
    return;
  }

  // Smart parser that supports quoted strings with spaces
  String args[3];
  int argCount = 0;
  int i = 0;

  while (i < params.length() && argCount < 3) {
    // Skip leading spaces
    while (i < params.length() && params[i] == ' ') {
      i++;
    }

    if (i >= params.length()) break;

    // Check if this argument starts with a quote
    if (params[i] == '"') {
      i++; // Skip opening quote
      int startIdx = i;
      // Find closing quote
      while (i < params.length() && params[i] != '"') {
        i++;
      }
      args[argCount++] = params.substring(startIdx, i);
      if (i < params.length()) i++; // Skip closing quote
    } else {
      // No quote, read until next space
      int startIdx = i;
      while (i < params.length() && params[i] != ' ') {
        i++;
      }
      args[argCount++] = params.substring(startIdx, i);
    }
  }

  if (argCount == 0) {
    Serial.println(F("Invalid wifi command. Type 'wifi' for help."));
    return;
  }

  String subCmd = args[0];
  subCmd.toLowerCase();

  if (subCmd == "show") {
    Serial.println(F("\n=== WiFi Configuration ==="));
    if (config.wifiConfigured && strlen(config.wifiSSID) > 0) {
      Serial.print(F("SSID: "));
      Serial.println(config.wifiSSID);
      Serial.print(F("Password: "));
      // Show masked password
      for (int i = 0; i < strlen(config.wifiPassword); i++) {
        Serial.print('*');
      }
      Serial.println();
      Serial.println(F("Source: EEPROM (configured via serial)"));
    }
    else if (strcmp(WIFI_SSID, "YOUR_WIFI_SSID") != 0 && strcmp(WIFI_SSID, "Your Homewifi SSID") != 0) {
      Serial.print(F("SSID: "));
      Serial.println(WIFI_SSID);
      Serial.println(F("Password: ********"));
      Serial.println(F("Source: config.h (fallback)"));
    }
    else {
      Serial.println(F("WiFi not configured"));
      Serial.println(F("Use 'wifi set \"ssid\" \"password\"' to configure (quotes for spaces)"));
    }

    Serial.print(F("Status: "));
    if (WiFi.status() == WL_CONNECTED) {
      Serial.println(F("Connected"));
      Serial.print(F("IP Address: "));
      Serial.println(WiFi.localIP());
      Serial.print(F("mDNS: http://k2so.local"));
    } else {
      Serial.println(F("Disconnected"));
    }
    Serial.println(F("===========================\n"));
  }
  else if (subCmd == "set") {
    if (argCount < 3) {
      Serial.println(F("\n=== WiFi Set Command ==="));
      Serial.println(F("Usage: wifi set <ssid> <password>"));
      Serial.println(F("\nFor SSIDs or passwords with spaces, use quotes:"));
      Serial.println(F("  wifi set \"HONOR Magoc V2\" \"my password\""));
      Serial.println(F("\nFor SSIDs without spaces:"));
      Serial.println(F("  wifi set MyNetwork MyPassword123"));
      Serial.println(F("===========================\n"));
      return;
    }

    String ssid = args[1];
    String password = args[2];

    // Check length limits
    if (ssid.length() >= sizeof(config.wifiSSID)) {
      Serial.printf("Error: SSID too long (max %d characters)\n", sizeof(config.wifiSSID) - 1);
      return;
    }
    if (password.length() >= sizeof(config.wifiPassword)) {
      Serial.printf("Error: Password too long (max %d characters)\n", sizeof(config.wifiPassword) - 1);
      return;
    }

    // Save to config
    strncpy(config.wifiSSID, ssid.c_str(), sizeof(config.wifiSSID) - 1);
    config.wifiSSID[sizeof(config.wifiSSID) - 1] = '\0';

    strncpy(config.wifiPassword, password.c_str(), sizeof(config.wifiPassword) - 1);
    config.wifiPassword[sizeof(config.wifiPassword) - 1] = '\0';

    config.wifiConfigured = true;

    Serial.println(F("\n=== WiFi Configuration Saved ==="));
    Serial.print(F("SSID: "));
    Serial.println(config.wifiSSID);
    Serial.println(F("Password: ********"));
    Serial.println(F("Saved to EEPROM"));

    smartSaveToEEPROM();

    Serial.println(F("\nUse 'wifi reconnect' or restart to apply changes."));
    Serial.println(F("================================\n"));
  }
  else if (subCmd == "reset") {
    Serial.print(F("Clear WiFi configuration? Type 'YES' to confirm: "));
    while (!Serial.available()) { delay(10); }
    String confirmation = Serial.readStringUntil('\n');
    confirmation.trim();

    if (confirmation == "YES") {
      strcpy(config.wifiSSID, "");
      strcpy(config.wifiPassword, "");
      config.wifiConfigured = false;

      smartSaveToEEPROM();

      Serial.println(F("WiFi configuration cleared."));
      Serial.println(F("Disconnecting WiFi..."));
      WiFi.disconnect();
      statusLEDWiFiDisconnected();
    } else {
      Serial.println(F("Operation cancelled."));
    }
  }
  else if (subCmd == "reconnect") {
    Serial.println(F("Reconnecting to WiFi..."));

    // Stop web server first to prevent race conditions
    server.stop();
    Serial.println(F("Web server stopped"));

    // Disconnect WiFi and wait for proper cleanup
    WiFi.disconnect(true);  // true = wifioff
    delay(1000);  // Increased delay to allow proper cleanup

    // Stop MDNS service
    MDNS.end();
    delay(100);

    // Call the WiFi init function from main .ino (need to declare it extern)
    extern void initializeWiFi();
    extern void setupWebServer();

    initializeWiFi();
    setupWebServer();

    Serial.println(F("WiFi reconnection complete"));
  }
  else {
    Serial.println(F("Invalid wifi command. Type 'wifi' for help."));
  }
}

//========================================
// ACCESS POINT (AP) CONFIGURATION COMMAND HANDLER
//========================================
void handleAPCommand(String params) {
  if (params.length() == 0) {
    Serial.println(F("\n=== Access Point Configuration ==="));
    Serial.println(F("  ap set <ssid> <password>       - Configure AP credentials (password min 8 chars)"));
    Serial.println(F("  ap set \"ssid\" \"password\"       - Use quotes for spaces"));
    Serial.println(F("  ap show                        - Show current AP settings"));
    Serial.println(F("  ap reset                       - Reset to default AP settings"));
    Serial.println(F("  ap enable                      - Enable AP mode fallback"));
    Serial.println(F("  ap disable                     - Disable AP mode fallback"));
    Serial.println(F("  ap start                       - Start AP mode now"));
    Serial.println(F("\nExample: ap set \"My K2SO\" \"password123\""));
    Serial.println(F("===================================\n"));
    return;
  }

  // Smart parser that supports quoted strings with spaces (same as WiFi command)
  String args[3];
  int argCount = 0;
  int i = 0;

  while (i < params.length() && argCount < 3) {
    // Skip leading spaces
    while (i < params.length() && params[i] == ' ') {
      i++;
    }

    if (i >= params.length()) break;

    // Check if this argument starts with a quote
    if (params[i] == '"') {
      i++; // Skip opening quote
      int startIdx = i;
      // Find closing quote
      while (i < params.length() && params[i] != '"') {
        i++;
      }
      args[argCount++] = params.substring(startIdx, i);
      if (i < params.length()) i++; // Skip closing quote
    } else {
      // No quote, read until next space
      int startIdx = i;
      while (i < params.length() && params[i] != ' ') {
        i++;
      }
      args[argCount++] = params.substring(startIdx, i);
    }
  }

  if (argCount == 0) {
    Serial.println(F("Invalid ap command. Type 'ap' for help."));
    return;
  }

  String subCmd = args[0];
  subCmd.toLowerCase();

  if (subCmd == "show") {
    Serial.println(F("\n=== Access Point Configuration ==="));
    if (config.apConfigured && strlen(config.apSSID) > 0) {
      Serial.print(F("AP SSID: "));
      Serial.println(config.apSSID);
      Serial.print(F("AP Password: "));
      // Show masked password
      for (int i = 0; i < strlen(config.apPassword); i++) {
        Serial.print('*');
      }
      Serial.println();
      Serial.println(F("Source: EEPROM (configured via serial)"));
    } else {
      // Show default AP settings
      Serial.println(F("Using default AP settings:"));
      String defaultAPName = "K2SO-" + WiFi.macAddress().substring(12);
      defaultAPName.replace(":", "");
      Serial.print(F("AP SSID: "));
      Serial.println(defaultAPName);
      Serial.println(F("AP Password: k2so2024 (default)"));
      Serial.println(F("Use 'ap set <ssid> <password>' to customize"));
    }

    Serial.print(F("AP Mode: "));
    Serial.println(config.apEnabled ? F("Enabled (fallback)") : F("Disabled"));

    Serial.print(F("Status: "));
    if (WiFi.getMode() == WIFI_AP || WiFi.getMode() == WIFI_AP_STA) {
      Serial.println(F("AP Active"));
      Serial.print(F("AP IP Address: "));
      Serial.println(WiFi.softAPIP());
      Serial.print(F("Connected clients: "));
      Serial.println(WiFi.softAPgetStationNum());
    } else {
      Serial.println(F("AP Inactive"));
    }
    Serial.println(F("===================================\n"));
  }
  else if (subCmd == "set") {
    if (argCount < 3) {
      Serial.println(F("\n=== AP Set Command ==="));
      Serial.println(F("Usage: ap set <ssid> <password>"));
      Serial.println(F("\nFor SSIDs or passwords with spaces, use quotes:"));
      Serial.println(F("  ap set \"My K2SO\" \"my password\""));
      Serial.println(F("\nFor SSIDs without spaces:"));
      Serial.println(F("  ap set K2SO-Droid MySecurePass123"));
      Serial.println(F("\nNote: Password must be at least 8 characters for WPA2"));
      Serial.println(F("===========================\n"));
      return;
    }

    String ssid = args[1];
    String password = args[2];

    // Validate password length (WPA2 requirement)
    if (password.length() < 8) {
      Serial.println(F("Error: Password must be at least 8 characters for WPA2"));
      return;
    }

    // Check length limits
    if (ssid.length() >= sizeof(config.apSSID)) {
      Serial.printf("Error: SSID too long (max %d characters)\n", sizeof(config.apSSID) - 1);
      return;
    }
    if (password.length() >= sizeof(config.apPassword)) {
      Serial.printf("Error: Password too long (max %d characters)\n", sizeof(config.apPassword) - 1);
      return;
    }

    // Save to config
    strncpy(config.apSSID, ssid.c_str(), sizeof(config.apSSID) - 1);
    config.apSSID[sizeof(config.apSSID) - 1] = '\0';

    strncpy(config.apPassword, password.c_str(), sizeof(config.apPassword) - 1);
    config.apPassword[sizeof(config.apPassword) - 1] = '\0';

    config.apConfigured = true;

    Serial.println(F("\n=== AP Configuration Saved ==="));
    Serial.print(F("AP SSID: "));
    Serial.println(config.apSSID);
    Serial.println(F("AP Password: ********"));
    Serial.println(F("Saved to EEPROM"));

    smartSaveToEEPROM();

    Serial.println(F("\nAP will use these settings on next activation."));
    Serial.println(F("Use 'ap start' to activate AP mode now."));
    Serial.println(F("===============================\n"));
  }
  else if (subCmd == "reset") {
    Serial.print(F("Reset AP configuration to defaults? Type 'YES' to confirm: "));
    while (!Serial.available()) { delay(10); }
    String confirmation = Serial.readStringUntil('\n');
    confirmation.trim();

    if (confirmation == "YES") {
      strcpy(config.apSSID, "");
      strcpy(config.apPassword, "");
      config.apConfigured = false;

      smartSaveToEEPROM();

      Serial.println(F("AP configuration reset to defaults."));
      Serial.println(F("Default AP will be K2SO-XXXXXX with password: k2so2024"));
    } else {
      Serial.println(F("Operation cancelled."));
    }
  }
  else if (subCmd == "enable") {
    config.apEnabled = true;
    smartSaveToEEPROM();
    Serial.println(F("AP mode fallback enabled."));
    Serial.println(F("AP will start automatically if WiFi connection fails."));
  }
  else if (subCmd == "disable") {
    config.apEnabled = false;
    smartSaveToEEPROM();
    Serial.println(F("AP mode fallback disabled."));
  }
  else if (subCmd == "start") {
    Serial.println(F("Starting Access Point mode..."));

    // Declare external function
    extern void startAccessPoint();
    startAccessPoint();
  }
  else {
    Serial.println(F("Invalid ap command. Type 'ap' for help."));
  }
}

void handleTimingCommand(String params) {
  if (params.length() == 0) {
    Serial.println("Timing commands:");
    Serial.println(F("  timing scan move [min] [max]  - Set scan eye movement timing"));
    Serial.println(F("  timing scan wait [min] [max]  - Set scan eye wait timing"));
    Serial.println(F("  timing alert move [min] [max] - Set alert eye movement timing"));
    Serial.println(F("  timing alert wait [min] [max] - Set alert eye wait timing"));
    Serial.println(F("  timing sound [min] [max]      - Set sound pause timing"));
    Serial.println(F("  timing show                   - Show all timing settings"));
    return;
  }
  
  String args[4];
  int argCount = 0;
  int startIdx = 0;
  
  for (int i = 0; i <= params.length() && argCount < 4; i++) {
    if (i == params.length() || params[i] == ' ') {
      if (i > startIdx) {
        args[argCount++] = params.substring(startIdx, i);
      }
      startIdx = i + 1;
    }
  }
  
  if (args[0] == "show") {
    Serial.println("\n=== TIMING SETTINGS ===");
    Serial.printf("Scan Eye Move: %d-%d ms\n", config.scanEyeMoveMin, config.scanEyeMoveMax);
    Serial.printf("Scan Eye Wait: %d-%d ms\n", config.scanEyeWaitMin, config.scanEyeWaitMax);
    Serial.printf("Alert Eye Move: %d-%d ms\n", config.alertEyeMoveMin, config.alertEyeMoveMax);
    Serial.printf("Alert Eye Wait: %d-%d ms\n", config.alertEyeWaitMin, config.alertEyeWaitMax);
    Serial.printf("Sound Pause: %d-%d ms\n", config.soundPauseMin, config.soundPauseMax);
    Serial.printf("Boot Sequence Delay: %d ms\n", config.bootSequenceDelay);
  }
  else if (args[0] == "scan" && args[1] == "move" && argCount >= 4) {
    config.scanEyeMoveMin = constrain(args[2].toInt(), 1, 1000);
    config.scanEyeMoveMax = constrain(args[3].toInt(), config.scanEyeMoveMin, 2000);
    Serial.printf("Scan eye move timing: %d-%d ms\n", config.scanEyeMoveMin, config.scanEyeMoveMax);
    smartSaveToEEPROM();
  }
  else if (args[0] == "scan" && args[1] == "wait" && argCount >= 4) {
    config.scanEyeWaitMin = constrain(args[2].toInt(), 100, 30000);
    config.scanEyeWaitMax = constrain(args[3].toInt(), config.scanEyeWaitMin, 60000);
    Serial.printf("Scan eye wait timing: %d-%d ms\n", config.scanEyeWaitMin, config.scanEyeWaitMax);
    smartSaveToEEPROM();
  }
  else if (args[0] == "alert" && args[1] == "move" && argCount >= 4) {
    config.alertEyeMoveMin = constrain(args[2].toInt(), 1, 500);
    config.alertEyeMoveMax = constrain(args[3].toInt(), config.alertEyeMoveMin, 1000);
    Serial.printf("Alert eye move timing: %d-%d ms\n", config.alertEyeMoveMin, config.alertEyeMoveMax);
    smartSaveToEEPROM();
  }
  else if (args[0] == "alert" && args[1] == "wait" && argCount >= 4) {
    config.alertEyeWaitMin = constrain(args[2].toInt(), 50, 10000);
    config.alertEyeWaitMax = constrain(args[3].toInt(), config.alertEyeWaitMin, 20000);
    Serial.printf("Alert eye wait timing: %d-%d ms\n", config.alertEyeWaitMin, config.alertEyeWaitMax);
    smartSaveToEEPROM();
  }
  else if (args[0] == "sound" && argCount >= 3) {
    config.soundPauseMin = constrain(args[1].toInt(), 1000, 120000);
    config.soundPauseMax = constrain(args[2].toInt(), config.soundPauseMin, 300000);
    Serial.printf("Sound pause timing: %d-%d ms\n", config.soundPauseMin, config.soundPauseMax);
    smartSaveToEEPROM();
  }
}

void handleProfileCommand(String params) {
  if (params.length() == 0) {
    Serial.println("Profile commands:");
    Serial.println(F("  profile save [name]    - Save current settings as profile"));
    Serial.println(F("  profile load [0-4]     - Load saved profile"));
    Serial.println(F("  profile list           - List all profiles"));
    Serial.println(F("  profile delete [0-4]   - Delete profile"));
    Serial.println(F("  profile show [0-4]     - Show profile details"));
    return;
  }
  
  String args[2];
  int argCount = 0;
  int startIdx = 0;
  
  for (int i = 0; i <= params.length() && argCount < 2; i++) {
    if (i == params.length() || params[i] == ' ') {
      if (i > startIdx) {
        args[argCount++] = params.substring(startIdx, i);
      }
      startIdx = i + 1;
    }
  }
  
  if (args[0] == "list") {
    Serial.println("\n=== SAVED PROFILES ===");
    for (int i = 0; i < 5; i++) {
      if (config.profiles[i].active) {
        Serial.printf("%d: %s\n", i, config.profiles[i].name);
      } else {
        Serial.printf("%d: [Empty]\n", i);
      }
    }
    Serial.printf("Current profile: %d\n", config.currentProfile);
  }
  else if (args[0] == "save" && argCount >= 2) {
    int slot = 0;
    for (int i = 0; i < 5; i++) {
      if (!config.profiles[i].active) {
        slot = i;
        break;
      }
    }
    
    Profile& profile = config.profiles[slot];
    profile.active = true;
    strncpy(profile.name, args[1].c_str(), 15);
    profile.name[15] = '\0';
    
    profile.mode = currentMode;
    profile.volume = config.savedVolume;
    profile.eyeBrightness = currentBrightness;
    
    profile.eyePanCenter = config.eyePanCenter;
    profile.eyeTiltCenter = config.eyeTiltCenter;
    profile.headPanCenter = config.headPanCenter;
    profile.headTiltCenter = config.headTiltCenter;
    
    profile.scanEyeMoveMin = config.scanEyeMoveMin;
    profile.scanEyeMoveMax = config.scanEyeMoveMax;
    profile.scanEyeWaitMin = config.scanEyeWaitMin;
    profile.scanEyeWaitMax = config.scanEyeWaitMax;
    profile.alertEyeMoveMin = config.alertEyeMoveMin;
    profile.alertEyeMoveMax = config.alertEyeMoveMax;
    profile.soundPauseMin = config.soundPauseMin;
    profile.soundPauseMax = config.soundPauseMax;
    
    config.currentProfile = slot;
    smartSaveToEEPROM();
    
    Serial.printf("Profile saved as '%s' in slot %d\n", profile.name, slot);
  }
  else if (args[0] == "load" && argCount >= 2) {
    int slot = constrain(args[1].toInt(), 0, 4);
    
    if (!config.profiles[slot].active) {
      Serial.printf("Profile slot %d is empty\n", slot);
      return;
    }
    
    Profile& profile = config.profiles[slot];
    
    currentMode = (PersonalityMode)profile.mode;
    config.savedVolume = profile.volume;
    currentBrightness = profile.eyeBrightness;
    
    config.eyePanCenter = profile.eyePanCenter;
    config.eyeTiltCenter = profile.eyeTiltCenter;
    config.headPanCenter = profile.headPanCenter;
    config.headTiltCenter = profile.headTiltCenter;
    
    config.scanEyeMoveMin = profile.scanEyeMoveMin;
    config.scanEyeMoveMax = profile.scanEyeMoveMax;
    config.scanEyeWaitMin = profile.scanEyeWaitMin;
    config.scanEyeWaitMax = profile.scanEyeWaitMax;
    config.alertEyeMoveMin = profile.alertEyeMoveMin;
    config.alertEyeMoveMax = profile.alertEyeMoveMax;
    config.soundPauseMin = profile.soundPauseMin;
    config.soundPauseMax = profile.soundPauseMax;
    
    config.currentProfile = slot;
    
    applyConfiguration();
    autoUpdateStatusLED(); // NEW: Update status LED for loaded mode
    smartSaveToEEPROM();
    
    Serial.printf("Profile '%s' loaded from slot %d\n", profile.name, slot);
  }
  else if (args[0] == "delete" && argCount >= 2) {
    int slot = constrain(args[1].toInt(), 0, 4);
    
    if (!config.profiles[slot].active) {
      Serial.printf("Profile slot %d is already empty\n", slot);
      return;
    }
    
    String profileName = config.profiles[slot].name;
    config.profiles[slot].active = false;
    memset(&config.profiles[slot], 0, sizeof(Profile));
    
    if (config.currentProfile == slot) {
      config.currentProfile = 255;
    }
    
    smartSaveToEEPROM();
    Serial.printf("Profile '%s' deleted from slot %d\n", profileName.c_str(), slot);
  }
}

//========================================
// SYSTEM STATUS AND HELP FUNCTIONS
//========================================

void showHelp() {
  Serial.println("\n=== K-2SO COMMAND REFERENCE ===");
  Serial.println("\nBASIC:");
  Serial.println(F("  help      - Show this help"));
  Serial.println(F("  status    - System status and statistics"));
  Serial.println(F("  config    - Show current configuration"));
  Serial.println(F("  save      - Save current settings to EEPROM"));
  Serial.println(F("  reset     - Restart the system"));
  
  Serial.println("\nMODES:");
  Serial.println(F("  mode [scanning/alert/idle] - Change personality mode"));
  
  Serial.println("\nIR CONTROL:");
  Serial.println(F("  learn     - Program IR remote buttons"));
  Serial.println(F("  scan      - IR code scanner mode"));
  Serial.println(F("  show      - Show programmed IR codes"));
  Serial.println(F("  clear     - Clear all IR codes (requires confirmation)"));
  Serial.println(F("  default   - Load standard IR remote codes"));
  Serial.println(F("  ir on/off - Enable/disable IR receiver"));
  
  Serial.println("\nHARDWARE CONFIGURATION:");
  Serial.println(F("  servo [options]  - Configure servo settings"));
  Serial.println(F("  led [options]    - Configure LED settings (eyes + status)"));
  Serial.println(F("  detail [options] - Configure detail LEDs (WS2812 strip)"));
  Serial.println(F("  sound [options]  - Configure audio settings"));
  Serial.println(F("  timing [options] - Configure movement timing"));
  
  Serial.println("\nPROFILE MANAGEMENT:");
  Serial.println(F("  profile save [name]  - Save current settings as profile"));
  Serial.println(F("  profile load [index] - Load saved profile (0-4)"));
  Serial.println(F("  profile list         - List all profiles"));
  Serial.println(F("  profile delete [idx] - Delete profile"));
  
  Serial.println("\nNETWORK CONFIGURATION:");
  Serial.println(F("  wifi [options]  - Configure WiFi connection"));
  Serial.println(F("  ap [options]    - Configure Access Point mode"));

  Serial.println("\nSYSTEM TOOLS:");
  Serial.println(F("  monitor   - Live system monitoring mode"));
  Serial.println(F("  test      - Hardware test sequence"));
  Serial.println(F("  demo      - Comprehensive demo of all features"));
  Serial.println(F("  backup    - Export configuration as hex"));
  Serial.println(F("  restore   - Import configuration from hex"));
  Serial.println(F("  exit      - Exit special modes"));

  Serial.println("\nIR REMOTE BUTTONS:");
  Serial.println("  1-3: Personality modes (Scanning, Alert, Idle)");
  Serial.println("  4-6: Audio (Random Scan, Random Alert, Random Voice)");
  Serial.println("  7:   Start Demo Mode (showcases all features)");
  Serial.println("  8:   Toggle Detail LEDs on/off");
  Serial.println("  9:   Cycle Eye Animations (Solid/Flicker/Pulse/Scanner/etc)");
  Serial.println("  UP/DOWN/LEFT/RIGHT: Eye movement, OK: Center all servos");
  Serial.println("  */#: Color backward/forward, 0: Eyes on/off");

  Serial.println("\nSTATUS LED FEATURES:");
  Serial.println("  Status LED shows: WiFi status, operating modes, activities");
  Serial.println("  Blue pulse=scanning, Red pulse=alert, Amber pulse=idle");
  Serial.println("  Green=WiFi connected, Red=disconnected, White flash=IR activity");

  Serial.println("\nType any command without parameters for detailed help.");
  Serial.println("Web interface available at: http://" + WiFi.localIP().toString());
}

void showStatus() {
  Serial.println("\n=== K-2SO SYSTEM STATUS ===");
  
  unsigned long uptime = (millis() - uptimeStart) / 1000;
  Serial.printf("Uptime: %02lu:%02lu:%02lu\n", uptime/3600, (uptime%3600)/60, uptime%60);
  Serial.printf("Free RAM: %lu bytes\n", (unsigned long)ESP.getFreeHeap());
  Serial.printf("EEPROM Writes: %lu\n", (unsigned long)config.writeCount);
  Serial.printf("WiFi IP: %s\n", WiFi.localIP().toString().c_str());
  
  Serial.printf("Mode: %s\n", getModeName(currentMode).c_str());
  Serial.printf("Status: %s\n", isAwake ? "AWAKE" : "SLEEPING");
  Serial.printf("Boot Complete: %s\n", bootSequenceComplete ? "Yes" : "No");
  
  Serial.print("Profile: ");
  if (config.currentProfile < 5 && config.profiles[config.currentProfile].active) {
    Serial.println(config.profiles[config.currentProfile].name);
  } else {
    Serial.println("Default");
  }
  
  Serial.printf("IR Receiver: %s\n", config.irEnabled ? "Enabled" : "Disabled");
  Serial.printf("Audio System: %s\n", isAudioReady ? "Ready" : "Not Ready");
  Serial.printf("Volume: %d/30\n", config.savedVolume);
  Serial.printf("Eye Brightness: %d/255\n", currentBrightness);
  
  // NEW: Status LED information
  Serial.printf("Status LED: %s (State: %s)\n", 
                config.statusLedEnabled ? "Enabled" : "Disabled",
                getStatusLEDStateName(getCurrentStatusLEDState()).c_str());
  Serial.printf("Status LED Brightness: %d/255\n", config.statusLedBrightness);
  
  Serial.printf("IR Commands: %lu\n", irCommandCount);
  Serial.printf("Servo Movements: %lu\n", servoMovements);
  Serial.printf("Last Activity: %lu seconds ago\n", (millis() - lastActivityTime) / 1000);
  
  Serial.println("\nServo Positions:");
  Serial.printf("  Eye Pan: %d/%d  Eye Tilt: %d/%d\n", eyePan.currentPosition, eyePan.targetPosition, eyeTilt.currentPosition, eyeTilt.targetPosition);
  Serial.printf("  Head Pan: %d/%d  Head Tilt: %d/%d\n", headPan.currentPosition, headPan.targetPosition, headTilt.currentPosition, headTilt.targetPosition);
}

void showConfiguration() {
  Serial.println("\n=== CURRENT CONFIGURATION ===");
  
  Serial.println("\n[SERVO SETTINGS]");
  Serial.printf("Eye Pan: Center=%d, Range=%d-%d\n", config.eyePanCenter, config.eyePanMin, config.eyePanMax);
  Serial.printf("Eye Tilt: Center=%d, Range=%d-%d\n", config.eyeTiltCenter, config.eyeTiltMin, config.eyeTiltMax);
  Serial.printf("Head Pan: Center=%d, Range=%d-%d\n", config.headPanCenter, config.headPanMin, config.headPanMax);
  Serial.printf("Head Tilt: Center=%d, Range=%d-%d\n", config.headTiltCenter, config.headTiltMin, config.headTiltMax);
  
  Serial.println("\n[LED SETTINGS]");
  Serial.printf("Eye Brightness: %d/255\n", currentBrightness);
  Serial.printf("Eye Animation Mode: %s\n", getAnimationModeName().c_str());
  Serial.printf("Status LED: %s (Brightness: %d/255)\n", 
                config.statusLedEnabled ? "Enabled" : "Disabled", 
                config.statusLedBrightness);
  
  Serial.println("\n[TIMING SETTINGS]");
  Serial.printf("Scan Move: %d-%d ms\n", config.scanEyeMoveMin, config.scanEyeMoveMax);
  Serial.printf("Scan Wait: %d-%d ms\n", config.scanEyeWaitMin, config.scanEyeWaitMax);
  Serial.printf("Alert Move: %d-%d ms\n", config.alertEyeMoveMin, config.alertEyeMoveMax);
  Serial.printf("Alert Wait: %d-%d ms\n", config.alertEyeWaitMin, config.alertEyeWaitMax);
  Serial.printf("Sound Pause: %d-%d ms\n", config.soundPauseMin, config.soundPauseMax);
  
  Serial.println("\n[IR BUTTONS]");
  int configuredButtons = 0;
  for (int i = 0; i < config.buttonCount && i < 5; i++) {
    if (config.buttons[i].isConfigured) {
      Serial.printf("  %s = 0x%08X\n", config.buttons[i].name, (unsigned int)config.buttons[i].code);
      configuredButtons++;
    }
  }
  if (config.buttonCount > 5) {
    Serial.printf("  ... and %d more buttons\n", config.buttonCount - 5);
  }
  Serial.printf("Total configured: %d buttons\n", configuredButtons);
}

void showSavedCodes() {
  if (config.buttonCount == 0) {
    Serial.println("No IR remote configured.");
    Serial.println("Use 'learn' to program your remote or 'default' for standard codes.");
    return;
  }
  
  Serial.println("\n=== PROGRAMMED IR CODES ===");
  Serial.printf("Remote has %d buttons:\n", config.buttonCount);
  
  for (int i = 0; i < config.buttonCount; i++) {
    if (config.buttons[i].isConfigured) {
      Serial.printf("  %-8s = 0x%08X\n", config.buttons[i].name, (unsigned int)config.buttons[i].code);
    } else {
      Serial.printf("  %-8s = [Not programmed]\n", config.buttons[i].name);
    }
  }
  
  Serial.println("\nUse 'scan' mode to identify unknown remote codes.");
}

void loadDefaultCodes() {
  Serial.println("Loading default IR codes for standard 17-button remote...");
  
  config.buttonCount = 17;
  
  uint32_t defaultCodes[] = {
    0xE619FF00, // 0
    0xBA45FF00, // 1
    0xB946FF00, // 2
    0xB847FF00, // 3
    0xBB44FF00, // 4
    0xBF40FF00, // 5
    0xBC43FF00, // 6
    0xF807FF00, // 7
    0xEA15FF00, // 8
    0xF609FF00, // 9
    0xE916FF00, // *
    0xF20DFF00, // #
    0xE718FF00, // UP
    0xAD52FF00, // DOWN
    0xF708FF00, // LEFT
    0xA55AFF00, // RIGHT
    0xE31CFF00  // OK
  };
  
  for (int i = 0; i < 17; i++) {
    strncpy(config.buttons[i].name, standard17Buttons[i], sizeof(config.buttons[i].name) - 1);
    config.buttons[i].name[sizeof(config.buttons[i].name) - 1] = '\0';  // Ensure null termination
    config.buttons[i].code = defaultCodes[i];
    config.buttons[i].isConfigured = true;
  }
  
  smartSaveToEEPROM();
  Serial.println("Default codes loaded successfully!");
  Serial.println("You can now use a standard NEC remote or run 'learn' to program your own.");
}

void clearAllData() {
  Serial.println(F("WARNING:"));
  Serial.println("All servo calibration, IR codes, profiles, and settings will be lost.");
  Serial.print("Are you absolutely sure? Type 'YES' to confirm: ");
  
  while (!Serial.available()) { delay(10); }
  String confirmation = Serial.readStringUntil('\n');
  confirmation.trim();
  
  if (confirmation == "YES") {
    for (int i = 0; i < EEPROM_SIZE; i++) {
      EEPROM.write(i, 0xFF);
    }
    EEPROM.commit();
    
    Serial.println("All data cleared. System will restart...");
    delay(2000);
    ESP.restart();
  } else {
    Serial.println("Operation cancelled. No data was cleared.");
  }
}

//========================================
// ADDITIONAL STATUS LED INTEGRATION
//========================================

void enterMonitorMode() {
  operatingMode = MODE_MONITOR;
  monitorMode = true;
  lastMonitorUpdate = 0;
  statusLEDConfigMode(); // NEW: Show config mode
  Serial.println("\n=== LIVE MONITOR MODE ===");
  Serial.println("Real-time system monitoring. Press any key to exit.");
  Serial.println("Time  | Mode | Eye P/T | Head P/T | IR    | Audio | Free RAM | Status LED");
  Serial.println("------|------|---------|----------|-------|-------|----------|----------");
}

void handleMonitorMode() {
  if (Serial.available()) {
    Serial.readString();
    monitorMode = false;
    operatingMode = MODE_NORMAL;
    autoUpdateStatusLED(); // NEW: Return to normal status
    Serial.println("\nMonitor mode ended.");
    return;
  }
  
  unsigned long currentMillis = millis();
  if (currentMillis - lastMonitorUpdate >= 1000) {
    lastMonitorUpdate = currentMillis;
    
    Serial.printf("%5lu | %4s | %3d/%3d | %3d/%3d  | ",
                  currentMillis / 1000,
                  getModeName(currentMode).substring(0, 4).c_str(),
                  eyePan.currentPosition, eyeTilt.currentPosition,
                  headPan.currentPosition, headTilt.currentPosition);
    
    if (IrReceiver.decode()) {
      Serial.printf("0x%04X", (unsigned int)(IrReceiver.decodedIRData.decodedRawData & 0xFFFF));
      IrReceiver.resume();
      statusLEDIRActivity(); // NEW: Flash for IR activity in monitor
    } else {
      Serial.print("  --  ");
    }
    
    Serial.printf(" | %s | %6lu | %s\n",
                  isAudioReady ? " OK " : "ERR ",
                  (unsigned long)ESP.getFreeHeap(),
                  getStatusLEDStateName(getCurrentStatusLEDState()).substring(0, 8).c_str());
  }
}

void handleTestMode() {
  unsigned long currentMillis = millis();
  
  switch (testStep) {
    case 0:
      Serial.println("Testing LEDs - Red");
      {
        uint32_t red = Adafruit_NeoPixel::Color(255, 0, 0);
        setEyeColor(red, red);
      }
      statusLEDTestMode(); // NEW: Update status LED for test
      testStep++;
      testTimer = currentMillis;
      break;
      
    case 1:
      if (currentMillis - testTimer > 1000) {
        Serial.println("Testing LEDs - Green");
        {
          uint32_t green = Adafruit_NeoPixel::Color(0, 255, 0);
          setEyeColor(green, green);
        }
        statusLEDTestMode(); // NEW: Update status LED
        testStep++;
        testTimer = currentMillis;
      }
      break;
      
    case 2:
      if (currentMillis - testTimer > 1000) {
        Serial.println("Testing LEDs - Blue");
        {
          uint32_t blue = Adafruit_NeoPixel::Color(0, 0, 255);
          setEyeColor(blue, blue);
        }
        statusLEDTestMode(); // NEW: Update status LED
        testStep++;
        testTimer = currentMillis;
      }
      break;
      
    case 3:
      if (currentMillis - testTimer > 1000) {
        Serial.println("Testing Eye Servos");
        {
          uint32_t white = Adafruit_NeoPixel::Color(255, 255, 255);
          setEyeColor(white, white);
        }
        eyePan.targetPosition = eyePan.minRange;
        eyeTilt.targetPosition = eyeTilt.minRange;
        eyePanServo.write(eyePan.targetPosition);
        eyeTiltServo.write(eyeTilt.targetPosition);
        statusLEDServoActivity(); // NEW: Flash for servo activity
        testStep++;
        testTimer = currentMillis;
      }
      break;
      
    case 4:
      if (currentMillis - testTimer > 2000) {
        eyePan.targetPosition = eyePan.maxRange;
        eyeTilt.targetPosition = eyeTilt.maxRange;
        eyePanServo.write(eyePan.targetPosition);
        eyeTiltServo.write(eyeTilt.targetPosition);
        statusLEDServoActivity(); // NEW: Flash for servo activity
        testStep++;
        testTimer = currentMillis;
      }
      break;
      
    case 5:
      if (currentMillis - testTimer > 2000) {
        Serial.println("Testing Head Servos");
        eyePan.targetPosition = config.eyePanCenter;
        eyeTilt.targetPosition = config.eyeTiltCenter;
        eyePanServo.write(eyePan.targetPosition);
        eyeTiltServo.write(eyeTilt.targetPosition);
        headPan.targetPosition = headPan.minRange;
        headTilt.targetPosition = headTilt.minRange;
        headPanServo.write(headPan.targetPosition);
        headTiltServo.write(headTilt.targetPosition);
        statusLEDServoActivity(); // NEW: Flash for servo activity
        testStep++;
        testTimer = currentMillis;
      }
      break;
      
    case 6:
      if (currentMillis - testTimer > 2000) {
        headPan.targetPosition = headPan.maxRange;
        headTilt.targetPosition = headTilt.maxRange;
        headPanServo.write(headPan.targetPosition);
        headTiltServo.write(headTilt.targetPosition);
        statusLEDServoActivity(); // NEW: Flash for servo activity
        testStep++;
        testTimer = currentMillis;
      }
      break;
      
    case 7:
      if (currentMillis - testTimer > 2000) {
        Serial.println("Testing Audio System");
        headPan.targetPosition = config.headPanCenter;
        headTilt.targetPosition = config.headTiltCenter;
        headPanServo.write(headPan.targetPosition);
        headTiltServo.write(headTilt.targetPosition);
        if (isAudioReady) {
          mp3.playFolderTrack(4, 1);
          statusLEDAudioActivity(); // NEW: Flash for audio activity
        }
        testStep++;
        testTimer = currentMillis;
      }
      break;
      
    case 8:
      if (currentMillis - testTimer > 3000) {
        Serial.println("Testing Detail LEDs (WS2812)");
        // Test new WS2812 detail LEDs
        setDetailColor(255, 0, 0);
        setDetailEnabled(true);
        startDetailBlink();
        testStep++;
        testTimer = currentMillis;
      }
      break;

    case 9:
      if (currentMillis - testTimer > 2000) {
        // Turn off detail LEDs
        setDetailEnabled(false);
        Serial.println("Testing Status LED System");
        statusLEDSystemTest(); // Test status LED
        testStep++;
        testTimer = currentMillis;
      }
      break;
      
    case 10:
      if (currentMillis - testTimer > 3000) {
        Serial.println(F("=== Hardware Test Complete ==="));
        Serial.println("All systems tested successfully!");
        operatingMode = MODE_NORMAL;
        autoUpdateStatusLED(); // NEW: Return to normal status
      }
      break;
  }
}

//========================================
// DEMO MODE - Comprehensive Feature Demonstration
//========================================

void enterDemoMode() {
  Serial.println("\n╔═══════════════════════════════════════╗");
  Serial.println("║  K-2SO COMPREHENSIVE DEMO MODE        ║");
  Serial.println("║  Showcasing all features              ║");
  Serial.println("╚═══════════════════════════════════════╝\n");

  operatingMode = MODE_DEMO;
  testStep = 0;
  testTimer = millis();
  isAwake = true;

  Serial.println("Demo will show:");
  Serial.println("• All 12 Eye Animation Modes");
  Serial.println("• All 5 Detail LED Patterns");
  Serial.println("• Color Changes");
  Serial.println("• Servo Movements");
  Serial.println("• Audio System\n");
  Serial.println("Press any key to exit demo...\n");
}

void handleDemoMode() {
  unsigned long currentMillis = millis();

  // Exit demo if serial input detected
  if (Serial.available() > 0) {
    Serial.read();  // Clear buffer
    Serial.println("\n=== Demo Mode Stopped ===");
    operatingMode = MODE_NORMAL;
    setEyeColor(getK2SOBlue(), getK2SOBlue());
    // Restore detail LED defaults
    setDetailColor(255, 0, 0);  // Red
    startDetailRandom();
    autoUpdateStatusLED();
    return;
  }

  switch (testStep) {
    // ==== EYE ANIMATIONS ====
    case 0:
      Serial.println("\n▶ Demonstrating: EYE ANIMATIONS");
      Serial.println("1/12: Solid Color (K-2SO Blue)");
      setEyeColor(getK2SOBlue(), getK2SOBlue());
      setEyeHardwareVersion(EYE_VERSION_13LED);  // Ensure 13-LED mode
      testStep++;
      testTimer = currentMillis;
      break;

    case 1:
      if (currentMillis - testTimer > 2000) {
        Serial.println("2/12: Flicker Animation");
        startFlickerMode(getK2SOBlue());
        testStep++;
        testTimer = currentMillis;
      }
      break;

    case 2:
      if (currentMillis - testTimer > 3000) {
        Serial.println("3/12: Pulse Animation");
        startPulseMode(getK2SOBlue());
        testStep++;
        testTimer = currentMillis;
      }
      break;

    case 3:
      if (currentMillis - testTimer > 3000) {
        Serial.println("4/12: Scanner Animation");
        startScannerMode();
        testStep++;
        testTimer = currentMillis;
      }
      break;

    case 4:
      if (currentMillis - testTimer > 4000) {
        Serial.println("5/12: Heartbeat Animation (Synchronized)");
        startHeartbeatMode();
        testStep++;
        testTimer = currentMillis;
      }
      break;

    case 5:
      if (currentMillis - testTimer > 4000) {
        Serial.println("6/12: Alarm Animation (Synchronized)");
        startAlarmMode();
        testStep++;
        testTimer = currentMillis;
      }
      break;

    case 6:
      if (currentMillis - testTimer > 3000) {
        Serial.println("7/12: Iris Animation (13-LED)");
        startIrisMode();
        testStep++;
        testTimer = currentMillis;
      }
      break;

    case 7:
      if (currentMillis - testTimer > 4000) {
        Serial.println("8/12: Targeting Animation (13-LED)");
        startTargetingMode();
        testStep++;
        testTimer = currentMillis;
      }
      break;

    case 8:
      if (currentMillis - testTimer > 4000) {
        Serial.println("9/12: Ring Scanner Animation (13-LED)");
        startRingScannerMode();
        testStep++;
        testTimer = currentMillis;
      }
      break;

    case 9:
      if (currentMillis - testTimer > 4000) {
        Serial.println("10/12: Spiral Animation (13-LED)");
        startSpiralMode();
        testStep++;
        testTimer = currentMillis;
      }
      break;

    case 10:
      if (currentMillis - testTimer > 4000) {
        Serial.println("11/12: Focus Animation (13-LED)");
        startFocusMode();
        testStep++;
        testTimer = currentMillis;
      }
      break;

    case 11:
      if (currentMillis - testTimer > 4000) {
        Serial.println("12/12: Radar Animation (13-LED)");
        startRadarMode();
        testStep++;
        testTimer = currentMillis;
      }
      break;

    // ==== DETAIL LED PATTERNS ====
    case 12:
      if (currentMillis - testTimer > 4000) {
        Serial.println("\n▶ Demonstrating: DETAIL LED PATTERNS");
        Serial.println("1/5: Blink Pattern");
        setDetailColor(255, 0, 0);  // Red
        startDetailBlink();
        setDetailEnabled(true);
        testStep++;
        testTimer = currentMillis;
      }
      break;

    case 13:
      if (currentMillis - testTimer > 3000) {
        Serial.println("2/5: Fade Pattern");
        startDetailFade();
        testStep++;
        testTimer = currentMillis;
      }
      break;

    case 14:
      if (currentMillis - testTimer > 3000) {
        Serial.println("3/5: Chase Pattern");
        setDetailColor(0, 255, 0);  // Green
        startDetailChase();
        testStep++;
        testTimer = currentMillis;
      }
      break;

    case 15:
      if (currentMillis - testTimer > 3000) {
        Serial.println("4/5: Pulse Pattern");
        setDetailColor(0, 0, 255);  // Blue
        startDetailPulse();
        testStep++;
        testTimer = currentMillis;
      }
      break;

    case 16:
      if (currentMillis - testTimer > 3000) {
        Serial.println("5/5: Random Pattern (Multiple LEDs)");
        setDetailColor(255, 100, 0);  // Orange
        startDetailRandom();
        testStep++;
        testTimer = currentMillis;
      }
      break;

    // ==== COLOR CHANGES ====
    case 17:
      if (currentMillis - testTimer > 4000) {
        Serial.println("\n▶ Demonstrating: COLOR PALETTE");
        Serial.println("Ice Blue");
        setEyeColor(getIceBlue(), getIceBlue());
        testStep++;
        testTimer = currentMillis;
      }
      break;

    case 18:
      if (currentMillis - testTimer > 2000) {
        Serial.println("Alert Red");
        setEyeColor(getAlertRed(), getAlertRed());
        testStep++;
        testTimer = currentMillis;
      }
      break;

    case 19:
      if (currentMillis - testTimer > 2000) {
        Serial.println("Scanning Green");
        setEyeColor(getScanningGreen(), getScanningGreen());
        testStep++;
        testTimer = currentMillis;
      }
      break;

    case 20:
      if (currentMillis - testTimer > 2000) {
        Serial.println("Idle Amber");
        setEyeColor(getIdleAmber(), getIdleAmber());
        testStep++;
        testTimer = currentMillis;
      }
      break;

    // ==== SERVO MOVEMENTS ====
    case 21:
      if (currentMillis - testTimer > 2000) {
        Serial.println("\n▶ Demonstrating: SERVO MOVEMENTS");
        Serial.println("Eye Movement Pattern");
        setEyeColor(getK2SOBlue(), getK2SOBlue());
        eyePan.targetPosition = eyePan.minRange;
        eyeTilt.targetPosition = eyeTilt.minRange;
        eyePanServo.write(eyePan.targetPosition);
        eyeTiltServo.write(eyeTilt.targetPosition);
        testStep++;
        testTimer = currentMillis;
      }
      break;

    case 22:
      if (currentMillis - testTimer > 1500) {
        eyePan.targetPosition = eyePan.maxRange;
        eyeTilt.targetPosition = eyeTilt.maxRange;
        eyePanServo.write(eyePan.targetPosition);
        eyeTiltServo.write(eyeTilt.targetPosition);
        testStep++;
        testTimer = currentMillis;
      }
      break;

    case 23:
      if (currentMillis - testTimer > 1500) {
        eyePan.targetPosition = config.eyePanCenter;
        eyeTilt.targetPosition = config.eyeTiltCenter;
        eyePanServo.write(eyePan.targetPosition);
        eyeTiltServo.write(eyeTilt.targetPosition);
        Serial.println("Head Movement Pattern");
        headPan.targetPosition = headPan.minRange;
        headTilt.targetPosition = headTilt.maxRange;
        headPanServo.write(headPan.targetPosition);
        headTiltServo.write(headTilt.targetPosition);
        testStep++;
        testTimer = currentMillis;
      }
      break;

    case 24:
      if (currentMillis - testTimer > 2000) {
        headPan.targetPosition = headPan.maxRange;
        headTilt.targetPosition = headTilt.minRange;
        headPanServo.write(headPan.targetPosition);
        headTiltServo.write(headTilt.targetPosition);
        testStep++;
        testTimer = currentMillis;
      }
      break;

    case 25:
      if (currentMillis - testTimer > 2000) {
        headPan.targetPosition = config.headPanCenter;
        headTilt.targetPosition = config.headTiltCenter;
        headPanServo.write(headPan.targetPosition);
        headTiltServo.write(headTilt.targetPosition);
        testStep++;
        testTimer = currentMillis;
      }
      break;

    // ==== AUDIO SYSTEM ====
    case 26:
      if (currentMillis - testTimer > 1500) {
        Serial.println("\n▶ Demonstrating: AUDIO SYSTEM");
        if (isAudioReady) {
          Serial.println("Playing K-2SO voice line");
          mp3.playFolderTrack(4, 1);
        } else {
          Serial.println("Audio system not available");
        }
        testStep++;
        testTimer = currentMillis;
      }
      break;

    // ==== DEMO COMPLETE ====
    case 27:
      if (currentMillis - testTimer > 4000) {
        Serial.println("\n╔═══════════════════════════════════════╗");
        Serial.println("║  DEMO COMPLETE!                       ║");
        Serial.println("║  All features demonstrated            ║");
        Serial.println("╚═══════════════════════════════════════╝\n");
        Serial.println("Returning to normal operation...\n");

        // Restore normal state
        operatingMode = MODE_NORMAL;
        setEyeColor(getK2SOBlue(), getK2SOBlue());
        // Restore detail LED defaults
        setDetailColor(255, 0, 0);  // Red
        startDetailRandom();
        autoUpdateStatusLED();
      }
      break;
  }
}

//========================================
// UTILITY FUNCTIONS - UPDATED
//========================================

String getModeName(PersonalityMode mode) {
  switch (mode) {
    case MODE_SCANNING: return "SCANNING";
    case MODE_ALERT: return "ALERT";
    case MODE_IDLE: return "IDLE";
    default: return "UNKNOWN";
  }
}

void setServoParameters() {
  switch (currentMode) {
    case MODE_SCANNING:
      eyePan.stepSize = 2;
      eyeTilt.stepSize = 2;
      headPan.stepSize = 1;
      headTilt.stepSize = 1;
      eyePan.moveInterval = random(config.scanEyeMoveMin, config.scanEyeMoveMax);
      eyeTilt.moveInterval = random(config.scanEyeMoveMin, config.scanEyeMoveMax);
      break;
    case MODE_ALERT:
      eyePan.stepSize = 5;
      eyeTilt.stepSize = 5;
      headPan.stepSize = 3;
      headTilt.stepSize = 3;
      eyePan.moveInterval = random(config.alertEyeMoveMin, config.alertEyeMoveMax);
      eyeTilt.moveInterval = random(config.alertEyeMoveMin, config.alertEyeMoveMax);
      break;
    case MODE_IDLE:
      eyePan.stepSize = 1;
      eyeTilt.stepSize = 1;
      headPan.stepSize = 1;
      headTilt.stepSize = 1;
      break;
  }
}

bool isValidVolume(uint8_t volume) {
  return (volume <= 30);
}

void updateSystemStats() {
  static unsigned long lastStatsUpdate = 0;
  unsigned long currentTime = millis();
  
  if (currentTime - lastStatsUpdate >= 60000) {
    lastStatsUpdate = currentTime;
    
    Serial.printf("Stats: Uptime=%lu, IRCommands=%lu, ServoMoves=%lu, FreeHeap=%lu\n",
                  (currentTime - uptimeStart) / 1000,
                  irCommandCount,
                  servoMovements,
                  ESP.getFreeHeap());
  }
}

void logSystemEvent(const char* event) {
  unsigned long timestamp = (millis() - uptimeStart) / 1000;
  Serial.printf("[%lu] %s\n", timestamp, event);
}

//========================================
// SYSTEM OPERATION HANDLERS - UPDATED
//========================================

void handleNormalOperation() {
  unsigned long currentMillis = millis();
  
  handleSensors();
  updateServos(currentMillis);
  updateAudio();
}

void handleSensors() {
  uint32_t code;
  if (checkForIRCommand(code)) {
    handleIRCommand(code);
    irCommandCount++;
    lastActivityTime = millis();
  }
}

void updateServos(unsigned long currentMillis) {
  updateServo(eyePan, currentMillis);
  updateServo(eyeTilt, currentMillis);
  updateServo(headPan, currentMillis);
  updateServo(headTilt, currentMillis);
  
  static unsigned long nextMoveTime = 0;
  
  if (isAwake && currentMode != MODE_IDLE) {
    if (currentMillis >= nextMoveTime) {
      int moveType = random(0, 4);
      
      switch(moveType) {
        case 0:
          eyePan.targetPosition = random(eyePan.minRange, eyePan.maxRange + 1);
          eyePan.isMoving = true;
          statusLEDServoActivity(); // NEW: Flash for autonomous movement
          break;
        case 1:
          eyeTilt.targetPosition = random(eyeTilt.minRange, eyeTilt.maxRange + 1);
          eyeTilt.isMoving = true;
          statusLEDServoActivity(); // NEW: Flash for autonomous movement
          break;
        case 2:
          headPan.targetPosition = random(headPan.minRange, headPan.maxRange + 1);
          headPan.isMoving = true;
          statusLEDServoActivity(); // NEW: Flash for autonomous movement
          break;
        case 3:
          headTilt.targetPosition = random(headTilt.minRange, headTilt.maxRange + 1);
          headTilt.isMoving = true;
          statusLEDServoActivity(); // NEW: Flash for autonomous movement
          break;
      }
      
      unsigned long waitTime = (currentMode == MODE_SCANNING) ? 
                               random(config.scanEyeWaitMin, config.scanEyeWaitMax) :
                               random(config.alertEyeWaitMin, config.alertEyeWaitMax);
      nextMoveTime = currentMillis + waitTime;
    }
  }
}

void updateServo(ServoState& servo, unsigned long currentMillis) {
  if (!servo.isMoving) {
    return;
  }
  
  if (currentMillis - servo.previousMillis >= servo.moveInterval) {
    servo.previousMillis = currentMillis;
    
    int difference = servo.targetPosition - servo.currentPosition;
    
    if (abs(difference) <= servo.stepSize) {
      servo.currentPosition = servo.targetPosition;
      servo.servoObject->write(servo.currentPosition);
      servo.isMoving = false;
    } else {
      if (difference > 0) {
        servo.currentPosition += servo.stepSize;
      } else {
        servo.currentPosition -= servo.stepSize;
      }
      servo.servoObject->write(servo.currentPosition);
    }
    
    servoMovements++;
  }
}

void handleBootSequence(unsigned long currentMillis) {
  static unsigned long lastBootStep = 0;
  static bool firstRun = true;

  // Initialize on first run to avoid huge time difference
  if (firstRun) {
    lastBootStep = currentMillis;
    firstRun = false;
  }

  if (currentMillis - lastBootStep >= config.bootSequenceDelay) {
    lastBootStep = currentMillis;
    
    switch(bootSequenceStep) {
      // ==== DRAMATIC EYE AWAKENING WITH FLICKERING ====
      // Pupil flickers to life, then ring, then both brighten

      case 0:
        Serial.println(F("Boot: Initializing eye awakening sequence..."));
        // Complete darkness
        leftEye.clear();
        rightEye.clear();
        leftEye.show();
        rightEye.show();
        bootSequenceStep++;
        break;

      // === PUPIL FLICKERING (POWER SURGES) ===
      case 1:
        // First flicker - weak pulse
        {
          uint32_t weakPulse = Adafruit_NeoPixel::Color(10, 15, 18);
          leftEye.setPixelColor(0, weakPulse);
          rightEye.setPixelColor(0, weakPulse);
          leftEye.show();
          rightEye.show();
        }
        bootSequenceStep++;
        break;

      case 2:
        // Flicker off
        leftEye.setPixelColor(0, 0);
        rightEye.setPixelColor(0, 0);
        leftEye.show();
        rightEye.show();
        bootSequenceStep++;
        break;

      case 3:
        // Second flicker - stronger
        {
          uint32_t strongerPulse = Adafruit_NeoPixel::Color(25, 35, 40);
          leftEye.setPixelColor(0, strongerPulse);
          rightEye.setPixelColor(0, strongerPulse);
          leftEye.show();
          rightEye.show();
        }
        bootSequenceStep++;
        break;

      case 4:
        // Flicker off
        leftEye.setPixelColor(0, 0);
        rightEye.setPixelColor(0, 0);
        leftEye.show();
        rightEye.show();
        bootSequenceStep++;
        break;

      case 5:
        // Third pulse - stabilizing
        {
          uint32_t stable = Adafruit_NeoPixel::Color(40, 55, 65);
          leftEye.setPixelColor(0, stable);
          rightEye.setPixelColor(0, stable);
          leftEye.show();
          rightEye.show();
        }
        bootSequenceStep++;
        break;

      case 6:
        // Pupil stays on, gets slightly brighter
        {
          uint32_t brighter = Adafruit_NeoPixel::Color(55, 75, 90);
          leftEye.setPixelColor(0, brighter);
          rightEye.setPixelColor(0, brighter);
          leftEye.show();
          rightEye.show();
        }
        bootSequenceStep++;
        break;

      // === RING STARTS FLICKERING ===
      case 7:
        Serial.println(F("Boot: Ring LED activation..."));
        // Ring first flicker - very weak
        {
          uint32_t pupil = Adafruit_NeoPixel::Color(55, 75, 90);
          uint32_t weakRing = Adafruit_NeoPixel::Color(5, 8, 10);

          leftEye.setPixelColor(0, pupil);
          rightEye.setPixelColor(0, pupil);

          for (int i = 1; i <= 12; i++) {
            leftEye.setPixelColor(i, weakRing);
            rightEye.setPixelColor(i, weakRing);
          }
          leftEye.show();
          rightEye.show();
        }
        bootSequenceStep++;
        break;

      case 8:
        // Ring flicker off, pupil stays
        {
          uint32_t pupil = Adafruit_NeoPixel::Color(55, 75, 90);
          leftEye.setPixelColor(0, pupil);
          rightEye.setPixelColor(0, pupil);

          for (int i = 1; i <= 12; i++) {
            leftEye.setPixelColor(i, 0);
            rightEye.setPixelColor(i, 0);
          }
          leftEye.show();
          rightEye.show();
        }
        bootSequenceStep++;
        break;

      case 9:
        // Ring second flicker - stronger
        {
          uint32_t pupil = Adafruit_NeoPixel::Color(60, 80, 95);
          uint32_t medRing = Adafruit_NeoPixel::Color(15, 20, 25);

          leftEye.setPixelColor(0, pupil);
          rightEye.setPixelColor(0, pupil);

          for (int i = 1; i <= 12; i++) {
            leftEye.setPixelColor(i, medRing);
            rightEye.setPixelColor(i, medRing);
          }
          leftEye.show();
          rightEye.show();
        }
        bootSequenceStep++;
        break;

      case 10:
        // Ring flicker off again
        {
          uint32_t pupil = Adafruit_NeoPixel::Color(60, 80, 95);
          leftEye.setPixelColor(0, pupil);
          rightEye.setPixelColor(0, pupil);

          for (int i = 1; i <= 12; i++) {
            leftEye.setPixelColor(i, 0);
            rightEye.setPixelColor(i, 0);
          }
          leftEye.show();
          rightEye.show();
        }
        bootSequenceStep++;
        break;

      case 11:
        // Ring stabilizes and stays on
        {
          uint32_t pupil = Adafruit_NeoPixel::Color(70, 95, 115);
          uint32_t stableRing = Adafruit_NeoPixel::Color(25, 35, 45);

          leftEye.setPixelColor(0, pupil);
          rightEye.setPixelColor(0, pupil);

          for (int i = 1; i <= 12; i++) {
            leftEye.setPixelColor(i, stableRing);
            rightEye.setPixelColor(i, stableRing);
          }
          leftEye.show();
          rightEye.show();
        }
        bootSequenceStep++;
        break;

      // === SCHNELLE BLITZER ===
      case 12:
        // Blitz 1 - Alle heller
        {
          uint32_t flashPupil = Adafruit_NeoPixel::Color(120, 160, 195);
          uint32_t flashRing = Adafruit_NeoPixel::Color(60, 80, 100);

          leftEye.setPixelColor(0, flashPupil);
          rightEye.setPixelColor(0, flashPupil);

          for (int i = 1; i <= 12; i++) {
            leftEye.setPixelColor(i, flashRing);
            rightEye.setPixelColor(i, flashRing);
          }
          leftEye.show();
          rightEye.show();
        }
        bootSequenceStep++;
        break;

      case 13:
        // Zurück zu vorher
        {
          uint32_t pupil = Adafruit_NeoPixel::Color(70, 95, 115);
          uint32_t stableRing = Adafruit_NeoPixel::Color(25, 35, 45);

          leftEye.setPixelColor(0, pupil);
          rightEye.setPixelColor(0, pupil);

          for (int i = 1; i <= 12; i++) {
            leftEye.setPixelColor(i, stableRing);
            rightEye.setPixelColor(i, stableRing);
          }
          leftEye.show();
          rightEye.show();
        }
        bootSequenceStep++;
        break;

      case 14:
        // Blitz 2
        {
          uint32_t flashPupil = Adafruit_NeoPixel::Color(120, 160, 195);
          uint32_t flashRing = Adafruit_NeoPixel::Color(60, 80, 100);

          leftEye.setPixelColor(0, flashPupil);
          rightEye.setPixelColor(0, flashPupil);

          for (int i = 1; i <= 12; i++) {
            leftEye.setPixelColor(i, flashRing);
            rightEye.setPixelColor(i, flashRing);
          }
          leftEye.show();
          rightEye.show();
        }
        bootSequenceStep++;
        break;

      case 15:
        // Zurück
        {
          uint32_t pupil = Adafruit_NeoPixel::Color(70, 95, 115);
          uint32_t stableRing = Adafruit_NeoPixel::Color(25, 35, 45);

          leftEye.setPixelColor(0, pupil);
          rightEye.setPixelColor(0, pupil);

          for (int i = 1; i <= 12; i++) {
            leftEye.setPixelColor(i, stableRing);
            rightEye.setPixelColor(i, stableRing);
          }
          leftEye.show();
          rightEye.show();
        }
        bootSequenceStep++;
        break;

      case 16:
        // Blitz 3
        {
          uint32_t flashPupil = Adafruit_NeoPixel::Color(120, 160, 195);
          uint32_t flashRing = Adafruit_NeoPixel::Color(60, 80, 100);

          leftEye.setPixelColor(0, flashPupil);
          rightEye.setPixelColor(0, flashPupil);

          for (int i = 1; i <= 12; i++) {
            leftEye.setPixelColor(i, flashRing);
            rightEye.setPixelColor(i, flashRing);
          }
          leftEye.show();
          rightEye.show();
        }
        bootSequenceStep++;
        break;

      // === BOTH BRIGHTEN TOGETHER ===
      case 17:
        // 50% power
        {
          uint32_t pupil = Adafruit_NeoPixel::Color(90, 120, 145);
          uint32_t ring = Adafruit_NeoPixel::Color(45, 60, 75);

          leftEye.setPixelColor(0, pupil);
          rightEye.setPixelColor(0, pupil);

          for (int i = 1; i <= 12; i++) {
            leftEye.setPixelColor(i, ring);
            rightEye.setPixelColor(i, ring);
          }
          leftEye.show();
          rightEye.show();
        }
        bootSequenceStep++;
        break;

      case 18:
        // 70% power
        {
          uint32_t pupil = Adafruit_NeoPixel::Color(115, 150, 185);
          uint32_t ring = Adafruit_NeoPixel::Color(70, 95, 120);

          leftEye.setPixelColor(0, pupil);
          rightEye.setPixelColor(0, pupil);

          for (int i = 1; i <= 12; i++) {
            leftEye.setPixelColor(i, ring);
            rightEye.setPixelColor(i, ring);
          }
          leftEye.show();
          rightEye.show();
        }
        bootSequenceStep++;
        break;

      // === ROTATING RING EFFECT ===
      case 19:
        // Ring rotation - Position 0 (LEDs 1,3,5,7,9,11 an)
        {
          uint32_t pupil = Adafruit_NeoPixel::Color(115, 150, 185);
          uint32_t brightRing = Adafruit_NeoPixel::Color(90, 120, 150);
          uint32_t dimRing = Adafruit_NeoPixel::Color(30, 40, 50);

          leftEye.setPixelColor(0, pupil);
          rightEye.setPixelColor(0, pupil);

          for (int i = 1; i <= 12; i++) {
            if (i % 2 == 1) {  // Ungerade LEDs (1,3,5,7,9,11)
              leftEye.setPixelColor(i, brightRing);
              rightEye.setPixelColor(i, brightRing);
            } else {  // Gerade LEDs (2,4,6,8,10,12)
              leftEye.setPixelColor(i, dimRing);
              rightEye.setPixelColor(i, dimRing);
            }
          }
          leftEye.show();
          rightEye.show();
        }
        bootSequenceStep++;
        break;

      case 20:
        // Ring rotation - Position 1 (LEDs 2,4,6,8,10,12 an)
        {
          uint32_t pupil = Adafruit_NeoPixel::Color(115, 150, 185);
          uint32_t brightRing = Adafruit_NeoPixel::Color(90, 120, 150);
          uint32_t dimRing = Adafruit_NeoPixel::Color(30, 40, 50);

          leftEye.setPixelColor(0, pupil);
          rightEye.setPixelColor(0, pupil);

          for (int i = 1; i <= 12; i++) {
            if (i % 2 == 0) {  // Gerade LEDs
              leftEye.setPixelColor(i, brightRing);
              rightEye.setPixelColor(i, brightRing);
            } else {  // Ungerade LEDs
              leftEye.setPixelColor(i, dimRing);
              rightEye.setPixelColor(i, dimRing);
            }
          }
          leftEye.show();
          rightEye.show();
        }
        bootSequenceStep++;
        break;

      case 21:
        // Ring rotation - Position 0 again
        {
          uint32_t pupil = Adafruit_NeoPixel::Color(115, 150, 185);
          uint32_t brightRing = Adafruit_NeoPixel::Color(90, 120, 150);
          uint32_t dimRing = Adafruit_NeoPixel::Color(30, 40, 50);

          leftEye.setPixelColor(0, pupil);
          rightEye.setPixelColor(0, pupil);

          for (int i = 1; i <= 12; i++) {
            if (i % 2 == 1) {
              leftEye.setPixelColor(i, brightRing);
              rightEye.setPixelColor(i, brightRing);
            } else {
              leftEye.setPixelColor(i, dimRing);
              rightEye.setPixelColor(i, dimRing);
            }
          }
          leftEye.show();
          rightEye.show();
        }
        bootSequenceStep++;
        break;

      case 22:
        // Ring rotation - Position 1 again
        {
          uint32_t pupil = Adafruit_NeoPixel::Color(115, 150, 185);
          uint32_t brightRing = Adafruit_NeoPixel::Color(90, 120, 150);
          uint32_t dimRing = Adafruit_NeoPixel::Color(30, 40, 50);

          leftEye.setPixelColor(0, pupil);
          rightEye.setPixelColor(0, pupil);

          for (int i = 1; i <= 12; i++) {
            if (i % 2 == 0) {
              leftEye.setPixelColor(i, brightRing);
              rightEye.setPixelColor(i, brightRing);
            } else {
              leftEye.setPixelColor(i, dimRing);
              rightEye.setPixelColor(i, dimRing);
            }
          }
          leftEye.show();
          rightEye.show();
        }
        bootSequenceStep++;
        break;

      case 23:
        // 90% power - alle gleichmäßig
        {
          uint32_t pupil = Adafruit_NeoPixel::Color(135, 180, 220);
          uint32_t ring = Adafruit_NeoPixel::Color(105, 140, 175);

          leftEye.setPixelColor(0, pupil);
          rightEye.setPixelColor(0, pupil);

          for (int i = 1; i <= 12; i++) {
            leftEye.setPixelColor(i, ring);
            rightEye.setPixelColor(i, ring);
          }
          leftEye.show();
          rightEye.show();
        }
        bootSequenceStep++;
        break;

      case 24:
        Serial.println(F("Boot: Eyes at full power - Ice Blue activated"));
        // Full power - 100% Ice Blue!
        setEyeColor(getIceBlue(), getIceBlue());
        bootSequenceStep++;
        break;

      case 25:
        // Play boot sound when eyes are fully awake
        // Wait for audio system to be ready (with retry logic)
        {
          static uint8_t audioAttempts = 0;
          static bool messagePrinted = false;
          const uint8_t maxAttempts = 10;  // Try up to 10 times (3 seconds total)

          if (!messagePrinted) {
            Serial.println(F("Boot: Checking audio system..."));
            Serial.printf("  isAudioReady = %s\n", isAudioReady ? "TRUE" : "FALSE");
            messagePrinted = true;
          }

          if (isAudioReady) {
            // Play boot sound from folder 03, track 001
            Serial.println("  Attempting to play boot sound...");

            // Give DFPlayer extra time to be ready for commands
            mp3.loop();  // Process any pending DFPlayer events
            delay(500);  // Wait for DFPlayer to be fully ready

            // Check if folder 03 has files
            int folder03Count = mp3.getFolderTrackCount(3);
            Serial.printf("  Folder 03 has %d files\n", folder03Count);

            if (folder03Count > 0) {
              // Ensure volume is set
              mp3.setVolume(config.savedVolume);
              delay(100);

              // Play the boot sound
              Serial.println("  Sending playFolderTrack(3, 1) command...");
              mp3.playFolderTrack(3, 1);
              delay(200);  // Give time for command to be processed

              Serial.println("✓ Boot sound command sent (Folder 03/001.mp3)");
            } else {
              Serial.println("⚠ Warning: Folder 03 is empty or missing!");
            }

            bootSequenceStep++;
            audioAttempts = 0;
            messagePrinted = false;  // Reset for next boot
          } else if (audioAttempts >= maxAttempts) {
            Serial.printf("⚠ Audio system not ready after %d attempts - skipping boot sound\n", maxAttempts);
            bootSequenceStep++;
            audioAttempts = 0;
            messagePrinted = false;  // Reset for next boot
          } else {
            audioAttempts++;
            Serial.printf("  Waiting for audio system... (attempt %d/%d)\n", audioAttempts, maxAttempts);
            // Don't increment bootSequenceStep - stay in case 25 and retry
          }
        }
        break;

      case 26:
        // Center servos after eyes are awake and sound has played
        Serial.println(F("Boot: Centering servos..."));
        centerAllServos();
        bootSequenceStep++;
        break;

      case 27:
        // Final setup - boot complete
        isAwake = true;
        lastActivityTime = millis();
        bootSequenceComplete = true;
        autoUpdateStatusLED(); // Update status LED after boot complete
        logSystemEvent("Boot sequence complete");
        Serial.println("K-2SO is now ONLINE and ready for operation!");
        break;
    }
  }
}

//========================================
// CONFIGURATION MANAGEMENT - UPDATED
//========================================

void loadConfiguration() {
  EEPROM.get(0, config);
  
  bool needsDefaults = false;
  
  if (config.magic != (uint8_t)EEPROM_MAGIC || config.version != 1) {
    Serial.println("Initializing configuration with defaults...");
    
    memset(&config, 0, sizeof(config));
    config.magic = (uint8_t)EEPROM_MAGIC;
    config.version = 1;
    config.writeCount = 0;
    
    // Default servo positions
    config.eyePanCenter = 90;
    config.eyeTiltCenter = 90;
    config.eyePanMin = 60;
    config.eyePanMax = 120;
    config.eyeTiltMin = 60;
    config.eyeTiltMax = 120;
    
    config.headPanCenter = 90;
    config.headTiltCenter = 90;
    config.headPanMin = 0;
    config.headPanMax = 180;
    config.headTiltMin = 0;
    config.headTiltMax = 180;
    
    // Default LED settings
    config.eyeBrightness = DEFAULT_BRIGHTNESS;
    config.ledEffectSpeed = 50;
    config.eyeVersion = EYE_VERSION_13LED;  // Default to 13-LED eyes

    // Default status LED settings
    config.statusLedBrightness = STATUS_LED_BRIGHTNESS;
    config.statusLedEnabled = true;
    
    // Default timing
    config.scanEyeMoveMin = 20;
    config.scanEyeMoveMax = 40;
    config.scanEyeWaitMin = 3000;
    config.scanEyeWaitMax = 6000;
    config.alertEyeMoveMin = 5;
    config.alertEyeMoveMax = 15;
    config.alertEyeWaitMin = 500;
    config.alertEyeWaitMax = 1500;
    config.soundPauseMin = 8000;
    config.soundPauseMax = 20000;
    config.bootSequenceDelay = 600;  // Slower for dramatic flickering effect
    
    // Default WiFi settings (empty - must be configured via serial)
    strcpy(config.wifiSSID, "");
    strcpy(config.wifiPassword, "");
    config.wifiConfigured = false;

    // Default settings
    config.savedVolume = 20;
    config.savedMode = MODE_SCANNING;
    config.irEnabled = true;
    config.currentProfile = 255;

    needsDefaults = true;
    saveConfiguration();
  }
  
  uint32_t storedChecksum = config.checksum;
  config.checksum = 0;
  uint32_t calculatedChecksum = calculateChecksum();

  if (storedChecksum != calculatedChecksum) {
    Serial.println("Configuration checksum mismatch, reloading defaults");
    Serial.printf("  Stored checksum: 0x%08X\n", storedChecksum);
    Serial.printf("  Calculated checksum: 0x%08X\n", calculatedChecksum);

    // Re-initialize with defaults instead of recursive call
    memset(&config, 0, sizeof(config));
    config.magic = (uint8_t)EEPROM_MAGIC;
    config.version = 1;
    config.writeCount = 0;

    // Default servo positions
    config.eyePanCenter = 90;
    config.eyeTiltCenter = 90;
    config.eyePanMin = 60;
    config.eyePanMax = 120;
    config.eyeTiltMin = 60;
    config.eyeTiltMax = 120;

    config.headPanCenter = 90;
    config.headTiltCenter = 90;
    config.headPanMin = 0;
    config.headPanMax = 180;
    config.headTiltMin = 0;
    config.headTiltMax = 180;

    // Default LED settings
    config.eyeBrightness = DEFAULT_BRIGHTNESS;
    config.ledEffectSpeed = 50;
    config.eyeVersion = EYE_VERSION_13LED;  // Default to 13-LED eyes

    // Default status LED settings
    config.statusLedBrightness = STATUS_LED_BRIGHTNESS;
    config.statusLedEnabled = true;

    // Default timing
    config.scanEyeMoveMin = 20;
    config.scanEyeMoveMax = 40;
    config.scanEyeWaitMin = 3000;
    config.scanEyeWaitMax = 6000;
    config.alertEyeMoveMin = 5;
    config.alertEyeMoveMax = 15;
    config.alertEyeWaitMin = 500;
    config.alertEyeWaitMax = 1500;
    config.soundPauseMin = 8000;
    config.soundPauseMax = 20000;
    config.bootSequenceDelay = 600;  // Slower for dramatic flickering effect

    // Default WiFi settings (empty - must be configured via serial)
    strcpy(config.wifiSSID, "");
    strcpy(config.wifiPassword, "");
    config.wifiConfigured = false;

    // Default settings
    config.savedVolume = 20;
    config.savedMode = MODE_SCANNING;
    config.irEnabled = true;
    config.currentProfile = 255;

    needsDefaults = true;
    saveConfiguration();
    Serial.println("Defaults loaded and saved after checksum mismatch");
  } else {
    Serial.printf("Configuration loaded (writes: %lu)\n", (unsigned long)config.writeCount);
    // Debug: Show WiFi configuration status
    if (config.wifiConfigured) {
      Serial.printf("  WiFi configured: SSID='%s'\n", config.wifiSSID);
    } else {
      Serial.println("  WiFi not configured (wifiConfigured=false)");
    }
  }

  if (config.buttonCount == 0 || needsDefaults) {
    Serial.println("No IR remote configured. Loading default codes...");
    loadDefaultCodes();
  }

  memcpy(&lastSavedConfig, &config, sizeof(config));
}

void saveConfiguration() {
  config.writeCount++;
  config.checksum = 0;
  config.checksum = calculateChecksum();

  // Optimized byte-level EEPROM writing
  // Only write bytes that have actually changed to reduce EEPROM wear
  uint8_t* configBytes = (uint8_t*)&config;
  uint8_t* lastSavedBytes = (uint8_t*)&lastSavedConfig;
  size_t bytesWritten = 0;

  for (size_t i = 0; i < sizeof(config); i++) {
    if (configBytes[i] != lastSavedBytes[i]) {
      EEPROM.write(i, configBytes[i]);
      bytesWritten++;
    }
  }

  EEPROM.commit();
  memcpy(&lastSavedConfig, &config, sizeof(config));

  // Log how many bytes were actually written
  Serial.printf("EEPROM: %u/%u bytes written (%.1f%% reduction)\n",
                bytesWritten, sizeof(config),
                100.0 * (1.0 - (float)bytesWritten / sizeof(config)));
}

void smartSaveToEEPROM() {
  if (memcmp(&config, &lastSavedConfig, sizeof(config) - sizeof(uint32_t)) != 0) {
    saveConfiguration();
    Serial.println("Configuration saved to EEPROM");
  } else {
    Serial.println("No configuration changes detected - skipping save");
  }
}

void applyConfiguration() {
  eyePan.currentPosition = config.eyePanCenter;
  eyeTilt.currentPosition = config.eyeTiltCenter;
  headPan.currentPosition = config.headPanCenter;
  headTilt.currentPosition = config.headTiltCenter;
  
  eyePan.minRange = config.eyePanMin;
  eyePan.maxRange = config.eyePanMax;
  eyeTilt.minRange = config.eyeTiltMin;
  eyeTilt.maxRange = config.eyeTiltMax;
  headPan.minRange = config.headPanMin;
  headPan.maxRange = config.headPanMax;
  headTilt.minRange = config.headTiltMin;
  headTilt.maxRange = config.headTiltMax;
  
  currentBrightness = config.eyeBrightness;
  setEyeBrightness(currentBrightness);

  // Apply eye hardware version
  updateEyeLEDCount();

  // Apply status LED configuration
  setStatusLEDConfig(config.statusLedBrightness, config.statusLedEnabled);
  
  if (isAudioReady) {
    mp3.setVolume(config.savedVolume);
  }
  
  currentMode = (PersonalityMode)config.savedMode;
  setServoParameters();

  lastActivityTime = millis();
}

uint32_t calculateChecksum() {
  uint32_t sum = 0;
  uint8_t* data = (uint8_t*)&config;
  for (size_t i = 0; i < sizeof(config) - sizeof(uint32_t); i++) {
    sum += data[i];
  }
  return sum;
}

void backupToSerial() {
  Serial.println("\n=== CONFIGURATION BACKUP ===");
  Serial.println("Copy the following data to save your configuration:");
  Serial.println(F("=== BACKUP START ==="));
  
  uint8_t* data = (uint8_t*)&config;
  for (size_t i = 0; i < sizeof(config); i++) {
    if (i % 32 == 0 && i > 0) Serial.println();
    if (data[i] < 0x10) Serial.print("0");
    Serial.print(data[i], HEX);
  }
  
  Serial.println("\n=== BACKUP END ===");
  Serial.println("Save this hex data to restore your configuration later.");
  Serial.printf("Total size: %d bytes\n", sizeof(config));
}

void restoreFromSerial() {
  Serial.println("\n=== CONFIGURATION RESTORE ===");
  Serial.println(F("WARNING:"));
  Serial.print("Continue? Type 'YES' to proceed: ");
  
  while (!Serial.available()) { delay(10); }
  String confirm = Serial.readStringUntil('\n');
  confirm.trim();
  
  if (confirm != "YES") {
    Serial.println("Restore cancelled.");
    return;
  }
  
  Serial.println("Paste your hex backup data and press Enter:");
  Serial.println("(You have 60 seconds to paste the data)");
  Serial.setTimeout(60000);
  
  String hexData = Serial.readStringUntil('\n');
  hexData.trim();
  
  if (hexData.length() < sizeof(config) * 2) {
    Serial.printf("Error: Data too short. Expected %d characters, got %d\n", 
                  sizeof(config) * 2, hexData.length());
    Serial.setTimeout(1000);
    return;
  }
  
  ConfigData tempConfig;
  uint8_t* data = (uint8_t*)&tempConfig;
  
  for (size_t i = 0; i < sizeof(config); i++) {
    String hexByte = hexData.substring(i * 2, i * 2 + 2);
    data[i] = strtol(hexByte.c_str(), NULL, 16);
  }
  
  if (tempConfig.magic == (uint8_t)EEPROM_MAGIC && tempConfig.version == 1) {
    memcpy(&config, &tempConfig, sizeof(config));
    saveConfiguration();
    applyConfiguration();
    
    Serial.println("Configuration restored successfully!");
    Serial.println(F("System"));
    delay(2000);
    ESP.restart();
  } else {
    Serial.println("Error: Invalid configuration data. Restore aborted.");
  }
  
  Serial.setTimeout(1000);
}

//========================================
// LEGACY DETAIL LED SYSTEM (DEPRECATED)
// This function is no longer used - Detail LEDs now use WS2812 system
// Kept for backward compatibility only
//========================================

void updateDetailBlinkers(unsigned long now) {
  // This function is deprecated and should not be called
  // WS2812 detail LEDs are updated via updateDetailLEDs() in detailleds.cpp
  // Legacy code kept for reference only
}