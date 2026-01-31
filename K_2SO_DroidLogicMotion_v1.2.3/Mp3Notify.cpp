/*
================================================================================
// K-2SO Controller MP3 Notification Class Implementation
================================================================================
*/

#include "Mp3Notify.h"
#include "globals.h"   // Include for access to global objects like 'mp3'

void Mp3Notify::OnError(DFMiniMp3<HardwareSerial, Mp3Notify>& mp3, uint16_t errorCode) {
    Serial.print("DFPlayer Error: ");
    Serial.println(errorCode);
    isAudioReady = false;
}

void Mp3Notify::OnPlayFinished(DFMiniMp3<HardwareSerial, Mp3Notify>& mp3, DfMp3_PlaySources source, uint16_t track) {
    Serial.printf("Track %d finished from source %d\n", track, (int)source);

    if (isAudioReady && isAwake) {
        // Schedule next random sound with configured pause time
        unsigned long pauseMs = random(config.soundPauseMin, config.soundPauseMax + 1);
        nextPlayTime = millis() + pauseMs;
        isWaitingForNextTrack = true;

        Serial.printf("Next sound scheduled in %lu ms\n", pauseMs);
    }
}

void Mp3Notify::OnPlaySourceOnline(DFMiniMp3<HardwareSerial, Mp3Notify>& mp3, DfMp3_PlaySources source) {
    Serial.print("DFPlayer: Source online - ");
    printSourceName(source);
    Serial.println();
}

void Mp3Notify::OnPlaySourceInserted(DFMiniMp3<HardwareSerial, Mp3Notify>& mp3, DfMp3_PlaySources source) {
    Serial.print("DFPlayer: Source inserted - ");
    printSourceName(source);
    Serial.println();

    // SD card was inserted, audio might be ready now
    if (source == DfMp3_PlaySources_Sd) {
        delay(100); // Give it a moment to initialize
        if (mp3.getTotalTrackCount() > 0) {
            isAudioReady = true;
            Serial.println("Audio system ready");
        }
    }
}

void Mp3Notify::OnPlaySourceRemoved(DFMiniMp3<HardwareSerial, Mp3Notify>& mp3, DfMp3_PlaySources source) {
    Serial.print("DFPlayer: Source removed - ");
    printSourceName(source);
    Serial.println();

    // If SD card was removed, audio is no longer available
    if (source == DfMp3_PlaySources_Sd) {
        isAudioReady = false;
        isWaitingForNextTrack = false;
        Serial.println("Audio system offline");
    }
}

void Mp3Notify::OnCardOnline(DFMiniMp3<HardwareSerial, Mp3Notify>& mp3) {
    Serial.println("DFPlayer: SD card online");
    // Give the card a moment to be ready
    delay(200);

    // Check if we have tracks available
    uint16_t trackCount = mp3.getTotalTrackCount();
    if (trackCount > 0) {
        isAudioReady = true;
        Serial.printf("Audio system ready with %d tracks\n", trackCount);
    }
}

void Mp3Notify::OnCardInserted(DFMiniMp3<HardwareSerial, Mp3Notify>& mp3) {
    Serial.println("DFPlayer: SD card inserted");
}

void Mp3Notify::OnCardRemoved(DFMiniMp3<HardwareSerial, Mp3Notify>& mp3) {
    Serial.println("DFPlayer: SD card removed");
    isAudioReady = false;
    isWaitingForNextTrack = false;
}

void Mp3Notify::OnUsbOnline(DFMiniMp3<HardwareSerial, Mp3Notify>& mp3) {
    Serial.println("DFPlayer: USB online");
}

void Mp3Notify::OnUsbInserted(DFMiniMp3<HardwareSerial, Mp3Notify>& mp3) {
    Serial.println("DFPlayer: USB inserted");
}

void Mp3Notify::OnUsbRemoved(DFMiniMp3<HardwareSerial, Mp3Notify>& mp3) {
    Serial.println("DFPlayer: USB removed");
}

void Mp3Notify::printSourceName(DfMp3_PlaySources source) {
    switch (source) {
        case DfMp3_PlaySources_Sd:
            Serial.print("SD Card");
            break;
        case DfMp3_PlaySources_Usb:
            Serial.print("USB");
            break;
        case DfMp3_PlaySources_Flash:
            Serial.print("Flash");
            break;
        default:
            Serial.printf("Unknown(%d)", (int)source);
            break;
    }
}