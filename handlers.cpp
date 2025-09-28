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
#include "statusled.h"    // NEW: Status LED functions
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
// WEB SERVER HANDLERS - UPDATED WITH STATUS LED
//========================================

void handleRoot() {
  Serial.println("Web request: Root page");
  server.send(200, "text/html", getIndexPage());
}

void handleWebStatus() {
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
  Serial.println("Web request: Red eyes");
  uint32_t red = Adafruit_NeoPixel::Color(255, 0, 0);
  setEyeColor(red, red);
  currentPixelMode = SOLID_COLOR;
  lastActivityTime = millis();
  server.send(200, "text/plain", "OK");
}

void handleGreen() {
  Serial.println("Web request: Green eyes");
  uint32_t green = Adafruit_NeoPixel::Color(0, 255, 0);
  setEyeColor(green, green);
  currentPixelMode = SOLID_COLOR;
  lastActivityTime = millis();
  server.send(200, "text/plain", "OK");
}

void handleBlue() {
  Serial.println("Web request: Blue eyes");
  uint32_t blue = Adafruit_NeoPixel::Color(0, 0, 255);
  setEyeColor(blue, blue);
  currentPixelMode = SOLID_COLOR;
  lastActivityTime = millis();
  server.send(200, "text/plain", "OK");
}

void handleWhite() {
  Serial.println("Web request: White eyes");
  uint32_t white = Adafruit_NeoPixel::Color(255, 255, 255);
  setEyeColor(white, white);
  currentPixelMode = SOLID_COLOR;
  lastActivityTime = millis();
  server.send(200, "text/plain", "OK");
}

void handleOff() {
  Serial.println("Web request: Eyes off");
  uint32_t off = Adafruit_NeoPixel::Color(0, 0, 0);
  setEyeColor(off, off);
  currentPixelMode = SOLID_COLOR;
  server.send(200, "text/plain", "OK");
}

void handleBrightness() {
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
  Serial.println("Web request: Flicker mode");
  startFlickerMode();
  lastActivityTime = millis();
  server.send(200, "text/plain", "OK");
}

void handlePulse() {
  Serial.println("Web request: Pulse mode");
  startPulseMode();
  lastActivityTime = millis();
  server.send(200, "text/plain", "OK");
}

void handleVolume() {
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
  if (server.hasArg("mode")) {
    String mode = server.arg("mode");
    mode.toLowerCase();
    
    if (mode == "scanning") {
      currentMode = MODE_SCANNING;
      statusLEDScanningMode(); // NEW: Update status LED
    } else if (mode == "alert") {
      currentMode = MODE_ALERT;
      statusLEDAlertMode(); // NEW: Update status LED
    } else if (mode == "idle") {
      currentMode = MODE_IDLE;
      statusLEDIdleMode(); // NEW: Update status LED
    } else {
      server.send(400, "text/plain", "Invalid mode");
      return;
    }
    
    setServoParameters();
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
  if (cmd == "backup") return CMD_BACKUP;
  if (cmd == "restore") return CMD_RESTORE;
  if (cmd == "exit" || cmd == "normal") return CMD_EXIT;
  if (cmd == "ir on") return CMD_IR_ON;
  if (cmd == "ir off") return CMD_IR_OFF;
  if (cmd == "mode") return CMD_MODE;
  
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
          statusLEDScanningMode(); // NEW: Update status LED
          Serial.println("Mode set to SCANNING");
        } else if (params == "alert") {
          currentMode = MODE_ALERT;
          statusLEDAlertMode(); // NEW: Update status LED
          Serial.println("Mode set to ALERT");
        } else if (params == "idle") {
          currentMode = MODE_IDLE;
          statusLEDIdleMode(); // NEW: Update status LED
          Serial.println("Mode set to IDLE");
        } else {
          Serial.println("Invalid mode. Use: scanning, alert, or idle");
          break;
        }
        setServoParameters();
        config.savedMode = currentMode;
        if (!isAwake) {
          isAwake = true;
        }
        lastActivityTime = millis();
      }
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
      strcpy(config.buttons[i].name, standard17Buttons[i]);
      config.buttons[i].isConfigured = false;
    }
    
    for (int i = 17; i < config.buttonCount; i++) {
      sprintf(config.buttons[i].name, "BTN%d", i + 1);
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
    Serial.println("  sound volume [0-30]          - Set volume");
    Serial.println("  sound play [file_number]     - Play specific file");
    Serial.println("  sound folder [folder] [track] - Play from folder");
    Serial.println("  sound stop                   - Stop playback");
    Serial.println("  sound show                   - Show settings");
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
    Serial.println("  servo eye center [pan] [tilt]   - Set eye center positions");
    Serial.println("  servo eye limits [minP] [maxP] [minT] [maxT] - Set eye limits");
    Serial.println("  servo head center [pan] [tilt] - Set head center positions");
    Serial.println("  servo head limits [minP] [maxP] [minT] [maxT] - Set head limits");
    Serial.println("  servo test [eye/head/all]      - Test servo movement");
    Serial.println("  servo show                     - Show all servo settings");
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
    Serial.println("  led brightness [0-255]       - Set eye brightness");
    Serial.println("  led color [r] [g] [b]        - Set eye color (0-255 each)");
    Serial.println("  led mode [solid/flicker/pulse] - Set animation mode");
    Serial.println("  led test [left/right/both]  - Test LEDs");
    Serial.println("  led show                     - Show current settings");
    Serial.println("  led status [on/off]          - Enable/disable status LED");
    Serial.println("  led status brightness [0-255] - Set status LED brightness");
    Serial.println("  led status test              - Test status LED");
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
    } else {
      Serial.println("Invalid mode. Use: solid, flicker, or pulse");
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
      
      setLeftEyeColor(red); delay(500);
      setLeftEyeColor(green); delay(500);
      setLeftEyeColor(blue); delay(500);
      setLeftEyeColor(off); delay(500);
    }
    
    if (target == "right" || target == "both") {
      Serial.println("Testing right eye...");
      uint32_t red = Adafruit_NeoPixel::Color(255, 0, 0);
      uint32_t green = Adafruit_NeoPixel::Color(0, 255, 0);
      uint32_t blue = Adafruit_NeoPixel::Color(0, 0, 255);
      uint32_t white = Adafruit_NeoPixel::Color(255, 255, 255);
      
      setRightEyeColor(red); delay(500);
      setRightEyeColor(green); delay(500);
      setRightEyeColor(blue); delay(500);
      setRightEyeColor(white);
    }
    
    Serial.println("LED test complete");
  }
}

void handleTimingCommand(String params) {
  if (params.length() == 0) {
    Serial.println("Timing commands:");
    Serial.println("  timing scan move [min] [max]  - Set scan eye movement timing");
    Serial.println("  timing scan wait [min] [max]  - Set scan eye wait timing");
    Serial.println("  timing alert move [min] [max] - Set alert eye movement timing");
    Serial.println("  timing alert wait [min] [max] - Set alert eye wait timing");
    Serial.println("  timing sound [min] [max]      - Set sound pause timing");
    Serial.println("  timing show                   - Show all timing settings");
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
    Serial.println("  profile save [name]    - Save current settings as profile");
    Serial.println("  profile load [0-4]     - Load saved profile");
    Serial.println("  profile list           - List all profiles");
    Serial.println("  profile delete [0-4]   - Delete profile");
    Serial.println("  profile show [0-4]     - Show profile details");
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
  Serial.println("  help      - Show this help");
  Serial.println("  status    - System status and statistics");
  Serial.println("  config    - Show current configuration");
  Serial.println("  save      - Save current settings to EEPROM");
  Serial.println("  reset     - Restart the system");
  
  Serial.println("\nMODES:");
  Serial.println("  mode [scanning/alert/idle] - Change personality mode");
  
  Serial.println("\nIR CONTROL:");
  Serial.println("  learn     - Program IR remote buttons");
  Serial.println("  scan      - IR code scanner mode");
  Serial.println("  show      - Show programmed IR codes");
  Serial.println("  clear     - Clear all IR codes (requires confirmation)");
  Serial.println("  default   - Load standard IR remote codes");
  Serial.println("  ir on/off - Enable/disable IR receiver");
  
  Serial.println("\nHARDWARE CONFIGURATION:");
  Serial.println("  servo [options] - Configure servo settings");
  Serial.println("  led [options]   - Configure LED settings (eyes + status)");
  Serial.println("  sound [options] - Configure audio settings");
  Serial.println("  timing [options]- Configure movement timing");
  
  Serial.println("\nPROFILE MANAGEMENT:");
  Serial.println("  profile save [name]  - Save current settings as profile");
  Serial.println("  profile load [index] - Load saved profile (0-4)");
  Serial.println("  profile list         - List all profiles");
  Serial.println("  profile delete [idx] - Delete profile");
  
  Serial.println("\nSYSTEM TOOLS:");
  Serial.println("  monitor   - Live system monitoring mode");
  Serial.println("  test      - Hardware test sequence");
  Serial.println("  backup    - Export configuration as hex");
  Serial.println("  restore   - Import configuration from hex");
  Serial.println("  exit      - Exit special modes");
  
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
    strcpy(config.buttons[i].name, standard17Buttons[i]);
    config.buttons[i].code = defaultCodes[i];
    config.buttons[i].isConfigured = true;
  }
  
  smartSaveToEEPROM();
  Serial.println("Default codes loaded successfully!");
  Serial.println("You can now use a standard NEC remote or run 'learn' to program your own.");
}

void clearAllData() {
  Serial.println("WARNING: This will erase ALL configuration data!");
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
        Serial.println("Testing Detail LEDs");
        for (int i = 0; i < DETAIL_LED_COUNT; i++) {
          digitalWrite(DETAIL_LED_PINS[i], HIGH);
        }
        testStep++;
        testTimer = currentMillis;
      }
      break;
      
    case 9:
      if (currentMillis - testTimer > 2000) {
        for (int i = 0; i < DETAIL_LED_COUNT; i++) {
          digitalWrite(DETAIL_LED_PINS[i], LOW);
        }
        Serial.println("Testing Status LED System");
        statusLEDSystemTest(); // NEW: Test status LED
        testStep++;
        testTimer = currentMillis;
      }
      break;
      
    case 10:
      if (currentMillis - testTimer > 3000) {
        Serial.println("=== Hardware Test Complete ===");
        Serial.println("All systems tested successfully!");
        operatingMode = MODE_NORMAL;
        autoUpdateStatusLED(); // NEW: Return to normal status
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
  
  if (currentMillis - lastBootStep >= config.bootSequenceDelay) {
    lastBootStep = currentMillis;
    
    switch(bootSequenceStep) {
      case 0:
        Serial.println("Boot: LED test");
        {
          uint32_t red = Adafruit_NeoPixel::Color(255, 0, 0);
          setEyeColor(red, red);
        }
        bootSequenceStep++;
        break;
        
      case 1:
        {
          uint32_t green = Adafruit_NeoPixel::Color(0, 255, 0);
          setEyeColor(green, green);
        }
        bootSequenceStep++;
        break;
        
      case 2:
        {
          uint32_t blue = Adafruit_NeoPixel::Color(0, 0, 255);
          setEyeColor(blue, blue);
        }
        bootSequenceStep++;
        break;
        
      case 3:
        Serial.println("Boot: Servo test");
        centerAllServos();
        bootSequenceStep++;
        break;
        
      case 4:
        Serial.println("Boot: Audio test");
        if (isAudioReady) {
          playSound(3);
        }
        bootSequenceStep++;
        break;
        
      case 5:
        Serial.println("Boot: Final setup");
        {
          uint32_t iceBlue = Adafruit_NeoPixel::Color(80, 150, 255);
          setEyeColor(iceBlue, iceBlue);
        }
        isAwake = true;
        lastActivityTime = millis();
        bootSequenceComplete = true;
        autoUpdateStatusLED(); // NEW: Update status LED after boot complete
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
    
    // NEW: Default status LED settings
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
    config.bootSequenceDelay = 300;
    
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
    loadConfiguration();
    return;
  } else {
    Serial.printf("Configuration loaded (writes: %lu)\n", (unsigned long)config.writeCount);
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
  EEPROM.put(0, config);
  EEPROM.commit();
  memcpy(&lastSavedConfig, &config, sizeof(config));
}

void smartSaveToEEPROM() {
  if (memcmp(&config, &lastSavedConfig, sizeof(config) - sizeof(uint32_t)) != 0) {
    saveConfiguration();
    Serial.println("Configuration saved to EEPROM");
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
  
  // NEW: Apply status LED configuration
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
  Serial.println("=== BACKUP START ===");
  
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
  Serial.println("WARNING: This will overwrite your current configuration!");
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
    Serial.println("System will restart to apply changes...");
    delay(2000);
    ESP.restart();
  } else {
    Serial.println("Error: Invalid configuration data. Restore aborted.");
  }
  
  Serial.setTimeout(1000);
}

//========================================
// DETAIL LED SYSTEM IMPLEMENTATION
//========================================

void updateDetailBlinkers(unsigned long now) {
  for (int i = 0; i < DETAIL_LED_COUNT; i++) {
    DetailBlinker& blinker = blinkers[i];
    
    if (now >= blinker.nextMs) {
      blinker.state = !blinker.state;
      digitalWrite(blinker.pin, blinker.state ? HIGH : LOW);
      
      if (blinker.state) {
        blinker.nextMs = now + random(blinker.minOnMs, blinker.maxOnMs + 1);
      } else {
        blinker.nextMs = now + random(blinker.minOffMs, blinker.maxOffMs + 1);
      }
    }
  }
}