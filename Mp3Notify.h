/*
================================================================================
// K-2SO Controller MP3 Notification Class - FIXED ENGLISH VERSION
// COMPLETELY FIXED: No circular dependencies anymore
================================================================================
*/

#ifndef MP3_NOTIFY_H
#define MP3_NOTIFY_H

#include <Arduino.h>
#include <DFMiniMp3.h>  // Full include for template class

// Forward declaration for ConfigData (avoids including config.h)
struct ConfigData;

//========================================
// EXTERNAL VARIABLES (defined in main)
//========================================
extern bool isAudioReady;
extern bool isAwake;
extern bool isWaitingForNextTrack;
extern unsigned long nextPlayTime;
extern ConfigData config;

//========================================
// DFPLAYER NOTIFICATION CLASS
//========================================
class Mp3Notify {
public:
  static void OnError(DFMiniMp3<HardwareSerial, Mp3Notify>& mp3, uint16_t errorCode);
  static void OnPlayFinished(DFMiniMp3<HardwareSerial, Mp3Notify>& mp3, DfMp3_PlaySources source, uint16_t track);
  static void OnPlaySourceOnline(DFMiniMp3<HardwareSerial, Mp3Notify>& mp3, DfMp3_PlaySources source);
  static void OnPlaySourceInserted(DFMiniMp3<HardwareSerial, Mp3Notify>& mp3, DfMp3_PlaySources source);
  static void OnPlaySourceRemoved(DFMiniMp3<HardwareSerial, Mp3Notify>& mp3, DfMp3_PlaySources source);
  static void OnCardOnline(DFMiniMp3<HardwareSerial, Mp3Notify>& mp3);
  static void OnCardInserted(DFMiniMp3<HardwareSerial, Mp3Notify>& mp3);
  static void OnCardRemoved(DFMiniMp3<HardwareSerial, Mp3Notify>& mp3);
  static void OnUsbOnline(DFMiniMp3<HardwareSerial, Mp3Notify>& mp3);
  static void OnUsbInserted(DFMiniMp3<HardwareSerial, Mp3Notify>& mp3);
  static void OnUsbRemoved(DFMiniMp3<HardwareSerial, Mp3Notify>& mp3);

private:
  static void printSourceName(DfMp3_PlaySources source);
};

#endif // MP3_NOTIFY_H