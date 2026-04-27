/*
================================================================================
// K-2SO Controller Web Interface Header
// Contains web page function declarations and constants
================================================================================
*/

#ifndef K2SO_WEBPAGE_H
#define K2SO_WEBPAGE_H

#include <Arduino.h>

//========================================
// WEB INTERFACE CONSTANTS
//========================================

// Page titles and branding
#define WEB_TITLE               "K-2SO Controller"
#define WEB_SUBTITLE            "Professional Droid Interface"
#define WEB_VERSION             "v1.3.0"

// Default values for dynamic content
#define DEFAULT_WEB_BRIGHTNESS  150
#define DEFAULT_WEB_VOLUME      15

// JavaScript update intervals (milliseconds)
#define STATUS_UPDATE_INTERVAL  2000    // Status updates every 2 seconds
#define SERVO_UPDATE_RATE       100     // Servo updates every 100ms max

// Servo position limits for web interface
#define WEB_SERVO_MIN          0        // Minimum servo position
#define WEB_SERVO_MAX          180      // Maximum servo position
#define WEB_SERVO_CENTER       90       // Center servo position

// Color picker defaults
#define WEB_DEFAULT_RED        255
#define WEB_DEFAULT_GREEN      255  
#define WEB_DEFAULT_BLUE       255

//========================================
// WEB PAGE SECTIONS
//========================================

// Main sections of the web interface
enum WebPageSection {
  SECTION_HEADER,
  SECTION_STATUS,
  SECTION_SERVO_CONTROL,
  SECTION_LED_CONTROL,
  SECTION_AUDIO_CONTROL,
  SECTION_MODE_CONTROL,
  SECTION_FOOTER
};

//========================================
// FUNCTION DECLARATIONS
//========================================

// Main page generation
String getIndexPage();                                 // Get complete HTML page

// Page section generators
String getPageHeader();                                // Get HTML head section
String getPageCSS();                                   // Get CSS styles
String getPageJavaScript();                            // Get JavaScript code

// Control section generators
String getServoControlSection();                       // Get servo gamepad HTML
String getLEDControlSection();                         // Get LED control HTML
String getDetailLEDControlSection();                   // Get Detail LED control HTML
String getAudioControlSection();                       // Get audio control HTML
String getModeControlSection();                        // Get mode selection HTML
String getStatusSection();                             // Get status display HTML
String getSequenceControlSection();                    // Get sequence management HTML

// Dynamic content generators
String getCurrentStatusJSON();                         // Get status as JSON

// Utility functions for web content
String formatUptime(unsigned long seconds);            // Format uptime display
String formatMemory(size_t bytes);                     // Format memory display
String escapeHTML(String input);                       // Escape HTML characters

//========================================
// CONSTANTS FOR HTML ELEMENTS
//========================================

// CSS class names
#define CSS_CONTAINER          "container"
#define CSS_SERVO_GAMEPAD      "servo-gamepad"
#define CSS_COLOR_PICKER       "color-picker"  
#define CSS_SLIDER            "slider"
#define CSS_BUTTON            "button"
#define CSS_STATUS            "status"
#define CSS_HEADER            "header"
#define CSS_FOOTER            "footer"

// HTML element IDs
#define ID_EYE_PAN_SLIDER     "eyePanSlider"
#define ID_EYE_TILT_SLIDER    "eyeTiltSlider"
#define ID_HEAD_PAN_SLIDER    "headPanSlider"
#define ID_HEAD_TILT_SLIDER   "headTiltSlider"
#define ID_BRIGHTNESS_SLIDER  "brightnessSlider"
#define ID_VOLUME_SLIDER      "volumeSlider"
#define ID_STATUS_DISPLAY     "statusDisplay"
#define ID_MODE_SELECTOR      "modeSelector"

// JavaScript function names
#define JS_UPDATE_STATUS      "updateStatus"
#define JS_SET_SERVOS         "setServos"
#define JS_SET_COLOR          "setColor"
#define JS_SET_BRIGHTNESS     "setBrightness"
#define JS_SET_VOLUME         "setVolume"
#define JS_PLAY_SOUND         "playSound"
#define JS_CHANGE_MODE        "changeMode"

#endif // K2SO_WEBPAGE_H
