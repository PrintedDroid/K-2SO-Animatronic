/*
================================================================================
// K-2SO Controller Web Interface Implementation - FIXED VERSION
// Complete HTML/CSS/JavaScript interface for droid control
// FIXED: Added missing includes for external variables and functions
================================================================================
*/

#include <Arduino.h>      // System zuerst
#include "webpage.h"      // DANN custom
#include "config.h"
#include "globals.h"
#include "handlers.h"

//========================================
// MAIN PAGE GENERATION
//========================================

String getIndexPage() {
  String html = "";
  
  // Build complete HTML page
  html += getPageHeader();
  html += "<body>\n";
  html += "  <div class='main-container'>\n";
  html += "    " + getStatusSection() + "\n";
  html += "    " + getServoControlSection() + "\n";
  html += "    " + getLEDControlSection() + "\n";
  html += "    " + getDetailLEDControlSection() + "\n";
  html += "    " + getAudioControlSection() + "\n";
  html += "    " + getModeControlSection() + "\n";
  html += "  </div>\n";
  html += getPageJavaScript();
  html += "</body>\n";
  html += "</html>\n";
  
  return html;
}

//========================================
// PAGE STRUCTURE SECTIONS
//========================================

String getPageHeader() {
  String header = R"rawliteral(
<!DOCTYPE html>
<html lang="en">
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>K-2SO Controller</title>
    <meta name="description" content="Professional K-2SO Droid Controller Interface">
    <meta name="author" content="Advanced Droid Systems">
    <style>
)rawliteral";
  
  header += getPageCSS();
  header += R"rawliteral(
    </style>
</head>
)rawliteral";
  
  return header;
}

String getPageCSS() {
  return R"rawliteral(
    /* K-2SO Controller Styles */
    * {
        margin: 0;
        padding: 0;
        box-sizing: border-box;
    }
    
    body {
        font-family: 'Segoe UI', Tahoma, Geneva, Verdana, sans-serif;
        background: linear-gradient(135deg, #0a0a0a 0%, #1a1a2e 50%, #16213e 100%);
        color: #ffffff;
        min-height: 100vh;
        overflow-x: hidden;
    }
    
    .main-container {
        max-width: 1200px;
        margin: 0 auto;
        padding: 20px;
        display: grid;
        grid-template-columns: repeat(auto-fit, minmax(300px, 1fr));
        gap: 20px;
        align-items: start;
    }
    
    /* Card-based sections */
    .control-section {
        background: rgba(255, 255, 255, 0.1);
        backdrop-filter: blur(10px);
        border-radius: 15px;
        border: 1px solid rgba(255, 255, 255, 0.2);
        padding: 25px;
        box-shadow: 0 8px 32px rgba(0, 0, 0, 0.3);
        transition: transform 0.3s ease, box-shadow 0.3s ease;
    }
    
    .control-section:hover {
        transform: translateY(-5px);
        box-shadow: 0 12px 48px rgba(0, 0, 0, 0.4);
    }
    
    .section-title {
        font-size: 1.5rem;
        font-weight: 600;
        margin-bottom: 20px;
        color: #4da6ff;
        text-align: center;
        text-shadow: 0 2px 4px rgba(0, 0, 0, 0.5);
        position: relative;
    }
    
    .section-title::after {
        content: '';
        position: absolute;
        bottom: -8px;
        left: 50%;
        transform: translateX(-50%);
        width: 50px;
        height: 2px;
        background: linear-gradient(90deg, transparent, #4da6ff, transparent);
    }
    
    /* Status Section */
    .status-grid {
        display: grid;
        grid-template-columns: repeat(auto-fit, minmax(200px, 1fr));
        gap: 15px;
        margin-top: 15px;
    }
    
    .status-item {
        background: rgba(0, 0, 0, 0.3);
        padding: 15px;
        border-radius: 10px;
        text-align: center;
        border: 1px solid rgba(77, 166, 255, 0.3);
    }
    
    .status-label {
        font-size: 0.9rem;
        opacity: 0.8;
        margin-bottom: 5px;
    }
    
    .status-value {
        font-size: 1.4rem;
        font-weight: bold;
        color: #4da6ff;
    }
    
    /* Servo Control Gamepad */
    .servo-gamepad {
        display: grid;
        grid-template-columns: repeat(3, 1fr);
        gap: 8px;
        max-width: 200px;
        margin: 20px auto;
        padding: 15px;
        background: rgba(0, 0, 0, 0.4);
        border-radius: 15px;
        border: 2px solid rgba(77, 166, 255, 0.3);
    }
    
    .gamepad-btn {
        width: 50px;
        height: 50px;
        border: none;
        border-radius: 10px;
        background: linear-gradient(145deg, #2a2a2a, #1a1a1a);
        color: #ffffff;
        font-size: 18px;
        cursor: pointer;
        transition: all 0.2s ease;
        box-shadow: inset 0 2px 4px rgba(0, 0, 0, 0.3);
        display: flex;
        align-items: center;
        justify-content: center;
    }
    
    .gamepad-btn:hover {
        background: linear-gradient(145deg, #4da6ff, #3a8ae8);
        transform: translateY(-2px);
        box-shadow: 0 4px 8px rgba(77, 166, 255, 0.4);
    }
    
    .gamepad-btn:active {
        transform: translateY(0);
        box-shadow: inset 0 2px 4px rgba(0, 0, 0, 0.5);
    }
    
    /* Center button special styling */
    .gamepad-btn.center {
        background: linear-gradient(145deg, #ff6b6b, #e74c3c);
    }
    
    .gamepad-btn.center:hover {
        background: linear-gradient(145deg, #ff8787, #ff6b6b);
    }
    
    /* Servo Position Display */
    .servo-positions {
        display: grid;
        grid-template-columns: repeat(2, 1fr);
        gap: 10px;
        margin-top: 15px;
    }
    
    .position-display {
        background: rgba(0, 0, 0, 0.3);
        padding: 10px;
        border-radius: 8px;
        text-align: center;
        font-family: 'Courier New', monospace;
    }
    
    .position-label {
        font-size: 0.8rem;
        opacity: 0.7;
    }
    
    .position-value {
        font-size: 1.2rem;
        font-weight: bold;
        color: #4da6ff;
    }
    
    /* LED Controls */
    .led-controls {
        display: flex;
        flex-direction: column;
        gap: 20px;
    }
    
    .color-buttons {
        display: grid;
        grid-template-columns: repeat(5, 1fr);
        gap: 10px;
    }
    
    .color-btn {
        width: 100%;
        height: 45px;
        border: none;
        border-radius: 8px;
        cursor: pointer;
        transition: all 0.3s ease;
        font-weight: bold;
        text-shadow: 0 1px 2px rgba(0, 0, 0, 0.7);
        border: 2px solid rgba(255, 255, 255, 0.2);
    }
    
    .color-btn:hover {
        transform: scale(1.05);
        border-color: rgba(255, 255, 255, 0.5);
    }
    
    .color-btn.red { background: #ff4757; }
    .color-btn.green { background: #2ed573; }
    .color-btn.blue { background: #3742fa; }
    .color-btn.white { background: #f1f2f6; color: #333; }
    .color-btn.off { background: #2f3542; }
    
    .animation-buttons {
        display: grid;
        grid-template-columns: repeat(2, 1fr);
        gap: 10px;
        margin-top: 15px;
    }
    
    .animation-btn {
        padding: 12px 20px;
        border: none;
        border-radius: 8px;
        background: linear-gradient(145deg, #2a2a2a, #1a1a1a);
        color: #ffffff;
        font-weight: bold;
        cursor: pointer;
        transition: all 0.3s ease;
        border: 1px solid rgba(77, 166, 255, 0.3);
    }
    
    .animation-btn:hover {
        background: linear-gradient(145deg, #4da6ff, #3a8ae8);
        transform: translateY(-2px);
    }
    
    /* Sliders */
    .slider-container {
        margin: 15px 0;
    }
    
    .slider-label {
        display: block;
        margin-bottom: 8px;
        font-weight: 500;
        color: #4da6ff;
    }
    
    .slider {
        width: 100%;
        height: 8px;
        border-radius: 4px;
        background: rgba(255, 255, 255, 0.2);
        outline: none;
        cursor: pointer;
        -webkit-appearance: none;
    }
    
    .slider::-webkit-slider-thumb {
        -webkit-appearance: none;
        width: 20px;
        height: 20px;
        border-radius: 50%;
        background: linear-gradient(145deg, #4da6ff, #3a8ae8);
        cursor: pointer;
        box-shadow: 0 2px 6px rgba(0, 0, 0, 0.3);
    }
    
    .slider::-moz-range-thumb {
        width: 20px;
        height: 20px;
        border-radius: 50%;
        background: linear-gradient(145deg, #4da6ff, #3a8ae8);
        cursor: pointer;
        border: none;
        box-shadow: 0 2px 6px rgba(0, 0, 0, 0.3);
    }
    
    .slider-value {
        float: right;
        font-weight: bold;
        color: #4da6ff;
        font-family: 'Courier New', monospace;
    }
    
    /* Audio Controls */
    .sound-buttons {
        display: grid;
        grid-template-columns: repeat(2, 1fr);
        gap: 10px;
        max-height: 300px;
        overflow-y: auto;
    }
    
    .sound-btn {
        padding: 12px 15px;
        border: none;
        border-radius: 8px;
        background: linear-gradient(145deg, #2a2a2a, #1a1a1a);
        color: #ffffff;
        font-weight: 500;
        cursor: pointer;
        transition: all 0.3s ease;
        border: 1px solid rgba(77, 166, 255, 0.3);
        text-align: left;
    }
    
    .sound-btn:hover {
        background: linear-gradient(145deg, #4da6ff, #3a8ae8);
        transform: translateY(-1px);
    }
    
    /* Mode Selector */
    .mode-buttons {
        display: grid;
        grid-template-columns: repeat(3, 1fr);
        gap: 10px;
    }
    
    .mode-btn {
        padding: 15px 10px;
        border: none;
        border-radius: 8px;
        background: linear-gradient(145deg, #2a2a2a, #1a1a1a);
        color: #ffffff;
        font-weight: bold;
        cursor: pointer;
        transition: all 0.3s ease;
        border: 2px solid rgba(77, 166, 255, 0.3);
        text-transform: uppercase;
        font-size: 0.9rem;
    }
    
    .mode-btn:hover {
        border-color: rgba(77, 166, 255, 0.6);
        transform: translateY(-2px);
    }
    
    .mode-btn.active {
        background: linear-gradient(145deg, #4da6ff, #3a8ae8);
        border-color: #4da6ff;
        box-shadow: 0 4px 15px rgba(77, 166, 255, 0.4);
    }
    
    /* Mobile Responsive */
    @media (max-width: 768px) {
        .main-container {
            grid-template-columns: 1fr;
            padding: 15px;
            gap: 15px;
        }
        
        .control-section {
            padding: 20px;
        }
        
        .servo-gamepad {
            max-width: 180px;
        }
        
        .gamepad-btn {
            width: 45px;
            height: 45px;
            font-size: 16px;
        }
        
        .color-buttons {
            grid-template-columns: repeat(5, 1fr);
        }
        
        .sound-buttons {
            grid-template-columns: 1fr;
        }
    }
    
    /* Animations */
    @keyframes pulse {
        0% { box-shadow: 0 0 0 0 rgba(77, 166, 255, 0.7); }
        70% { box-shadow: 0 0 0 10px rgba(77, 166, 255, 0); }
        100% { box-shadow: 0 0 0 0 rgba(77, 166, 255, 0); }
    }
    
    .pulse-animation {
        animation: pulse 2s infinite;
    }
    
    /* Loading states */
    .loading {
        opacity: 0.6;
        pointer-events: none;
    }
    
    .loading::after {
        content: '';
        position: absolute;
        top: 50%;
        left: 50%;
        width: 20px;
        height: 20px;
        margin: -10px 0 0 -10px;
        border: 2px solid transparent;
        border-top: 2px solid #4da6ff;
        border-radius: 50%;
        animation: spin 1s linear infinite;
    }
    
    @keyframes spin {
        0% { transform: rotate(0deg); }
        100% { transform: rotate(360deg); }
    }
)rawliteral";
}

//========================================
// CONTROL SECTIONS
//========================================

String getStatusSection() {
  return R"rawliteral(
    <div class="control-section">
        <h2 class="section-title">System Status</h2>
        <div class="status-grid">
            <div class="status-item">
                <div class="status-label">Mode</div>
                <div class="status-value" id="currentMode">SCANNING</div>
            </div>
            <div class="status-item">
                <div class="status-label">Status</div>
                <div class="status-value" id="systemStatus">AWAKE</div>
            </div>
            <div class="status-item">
                <div class="status-label">Uptime</div>
                <div class="status-value" id="systemUptime">00:00:00</div>
            </div>
            <div class="status-item">
                <div class="status-label">Free RAM</div>
                <div class="status-value" id="freeMemory">0 KB</div>
            </div>
        </div>
    </div>
)rawliteral";
}

String getServoControlSection() {
  return R"rawliteral(
    <div class="control-section">
        <h2 class="section-title">Servo Control</h2>
        <div class="servo-gamepad">
            <button class="gamepad-btn" onclick="setServos(0, 180, 0, 180)">↖</button>
            <button class="gamepad-btn" onclick="setServos(90, 180, 90, 180)">↑</button>
            <button class="gamepad-btn" onclick="setServos(180, 180, 180, 180)">↗</button>
            <button class="gamepad-btn" onclick="setServos(0, 90, 0, 90)">←</button>
            <button class="gamepad-btn center" onclick="centerAllServos()">●</button>
            <button class="gamepad-btn" onclick="setServos(180, 90, 180, 90)">→</button>
            <button class="gamepad-btn" onclick="setServos(0, 0, 0, 0)">↙</button>
            <button class="gamepad-btn" onclick="setServos(90, 0, 90, 0)">↓</button>
            <button class="gamepad-btn" onclick="setServos(180, 0, 180, 0)">↘</button>
        </div>
        <div class="servo-positions">
            <div class="position-display">
                <div class="position-label">Eye Pan</div>
                <div class="position-value" id="eyePanPos">90°</div>
            </div>
            <div class="position-display">
                <div class="position-label">Eye Tilt</div>
                <div class="position-value" id="eyeTiltPos">90°</div>
            </div>
            <div class="position-display">
                <div class="position-label">Head Pan</div>
                <div class="position-value" id="headPanPos">90°</div>
            </div>
            <div class="position-display">
                <div class="position-label">Head Tilt</div>
                <div class="position-value" id="headTiltPos">90°</div>
            </div>
        </div>
    </div>
)rawliteral";
}

String getLEDControlSection() {
  return R"rawliteral(
    <div class="control-section">
        <h2 class="section-title">Eye Control</h2>
        <div class="led-controls">
            <div class="color-buttons">
                <button class="color-btn red" onclick="setColor('red')">RED</button>
                <button class="color-btn green" onclick="setColor('green')">GREEN</button>
                <button class="color-btn blue" onclick="setColor('blue')">BLUE</button>
                <button class="color-btn white" onclick="setColor('white')">WHITE</button>
                <button class="color-btn off" onclick="setColor('off')">OFF</button>
            </div>
            <div class="animation-buttons">
                <button class="animation-btn" onclick="setAnimation('flicker')">FLICKER</button>
                <button class="animation-btn" onclick="setAnimation('pulse')">PULSE</button>
            </div>
            <div class="slider-container">
                <label class="slider-label">Brightness <span class="slider-value" id="brightnessValue">150</span></label>
                <input type="range" class="slider" id="brightnessSlider" min="0" max="255" value="150" oninput="setBrightness(this.value)">
            </div>
        </div>
    </div>
)rawliteral";
}

String getAudioControlSection() {
  return R"rawliteral(
    <div class="control-section">
        <h2 class="section-title">Audio Control</h2>
        <div class="slider-container">
            <label class="slider-label">Volume <span class="slider-value" id="volumeValue">15</span></label>
            <input type="range" class="slider" id="volumeSlider" min="0" max="30" value="15" oninput="setVolume(this.value)">
        </div>
        <div class="sound-buttons">
            <button class="sound-btn" onclick="playSound(1)">🔊 I Am K-2SO</button>
            <button class="sound-btn" onclick="playSound(2)">🔊 Behavior</button>
            <button class="sound-btn" onclick="playSound(3)">🔊 Fresh One</button>
            <button class="sound-btn" onclick="playSound(4)">🔊 Clear of Hostiles</button>
            <button class="sound-btn" onclick="playSound(5)">🔊 Quiet</button>
            <button class="sound-btn" onclick="playSound(6)">🔊 Random Voice</button>
            <button class="sound-btn" onclick="playSound(7)">🔊 Alert Sound</button>
            <button class="sound-btn" onclick="playSound(8)">🔊 Boot Sound</button>
        </div>
    </div>
)rawliteral";
}

String getModeControlSection() {
  return R"rawliteral(
    <div class="control-section">
        <h2 class="section-title">Behavior Mode</h2>
        <div class="mode-buttons">
            <button class="mode-btn active" id="scanningMode" onclick="setMode('scanning')">SCANNING</button>
            <button class="mode-btn" id="alertMode" onclick="setMode('alert')">ALERT</button>
            <button class="mode-btn" id="idleMode" onclick="setMode('idle')">IDLE</button>
        </div>
    </div>
)rawliteral";
}

String getDetailLEDControlSection() {
  return R"rawliteral(
    <div class="control-section">
        <h2 class="section-title">Detail LED Control</h2>
        <div class="led-controls">
            <div class="slider-container">
                <label class="slider-label">LED Count <span class="slider-value" id="detailCountValue">5</span></label>
                <input type="range" class="slider" id="detailCountSlider" min="1" max="8" value="5" oninput="setDetailCount(this.value)">
            </div>
            <div class="slider-container">
                <label class="slider-label">Brightness <span class="slider-value" id="detailBrightnessValue">150</span></label>
                <input type="range" class="slider" id="detailBrightnessSlider" min="0" max="255" value="150" oninput="setDetailBrightness(this.value)">
            </div>
            <div class="animation-buttons">
                <button class="animation-btn" onclick="setDetailPattern('blink')">BLINK</button>
                <button class="animation-btn" onclick="setDetailPattern('fade')">FADE</button>
                <button class="animation-btn" onclick="setDetailPattern('chase')">CHASE</button>
                <button class="animation-btn" onclick="setDetailPattern('pulse')">PULSE</button>
                <button class="animation-btn" onclick="setDetailPattern('random')">RANDOM</button>
                <button class="animation-btn" onclick="setDetailEnabled('off')">OFF</button>
            </div>
        </div>
    </div>
)rawliteral";
}

//========================================
// JAVASCRIPT SECTION
//========================================

String getPageJavaScript() {
  return R"rawliteral(
<script>
    // Global variables
    let statusUpdateInterval;
    let isConnected = true;
    
    // Initialize page when loaded
    document.addEventListener('DOMContentLoaded', function() {
        console.log('K-2SO Controller initialized');
        startStatusUpdates();
        centerAllServos();
    });
    
    // Status update functions
    function startStatusUpdates() {
        updateStatus();
        statusUpdateInterval = setInterval(updateStatus, 2000);
    }
    
    function stopStatusUpdates() {
        if (statusUpdateInterval) {
            clearInterval(statusUpdateInterval);
        }
    }
    
    async function updateStatus() {
        try {
            const response = await fetch('/status');
            if (!response.ok) throw new Error('Network response was not ok');
            
            const data = await response.json();
            
            // Update status display
            document.getElementById('currentMode').textContent = data.mode;
            document.getElementById('systemStatus').textContent = data.awake ? 'AWAKE' : 'SLEEPING';
            document.getElementById('systemUptime').textContent = formatUptime(data.uptime);
            document.getElementById('freeMemory').textContent = formatMemory(data.freeMemory);
            
            // Update active mode button
            updateModeButtons(data.mode.toLowerCase());
            
            // Update connection status
            setConnectionStatus(true);
            
        } catch (error) {
            console.error('Status update failed:', error);
            setConnectionStatus(false);
        }
    }
    
    // Servo control functions
    async function setServos(eyePan, eyeTilt, headPan, headTilt) {
        try {
            showLoading('servo');
            const response = await fetch(`/setServos?eyePan=${eyePan}&eyeTilt=${eyeTilt}&headPan=${headPan}&headTilt=${headTilt}`);
            
            if (response.ok) {
                updateServoPositions(eyePan, eyeTilt, headPan, headTilt);
                showFeedback('Servos moved', 'success');
            } else {
                throw new Error('Servo command failed');
            }
        } catch (error) {
            showFeedback('Servo error: ' + error.message, 'error');
        } finally {
            hideLoading('servo');
        }
    }
    
    function centerAllServos() {
        setServos(90, 90, 90, 90);
    }
    
    function updateServoPositions(eyePan, eyeTilt, headPan, headTilt) {
        document.getElementById('eyePanPos').textContent = eyePan + '°';
        document.getElementById('eyeTiltPos').textContent = eyeTilt + '°';
        document.getElementById('headPanPos').textContent = headPan + '°';
        document.getElementById('headTiltPos').textContent = headTilt + '°';
    }
    
    // LED control functions
    async function setColor(color) {
        try {
            showLoading('led');
            const response = await fetch('/' + color);
            
            if (response.ok) {
                showFeedback('Eye color changed', 'success');
            } else {
                throw new Error('Color command failed');
            }
        } catch (error) {
            showFeedback('LED error: ' + error.message, 'error');
        } finally {
            hideLoading('led');
        }
    }
    
    async function setAnimation(animation) {
        try {
            showLoading('led');
            const response = await fetch('/' + animation);
            
            if (response.ok) {
                showFeedback('Animation started', 'success');
            } else {
                throw new Error('Animation command failed');
            }
        } catch (error) {
            showFeedback('Animation error: ' + error.message, 'error');
        } finally {
            hideLoading('led');
        }
    }
    
    async function setBrightness(value) {
        document.getElementById('brightnessValue').textContent = value;
        
        try {
            const response = await fetch(`/brightness?value=${value}`);
            if (!response.ok) throw new Error('Brightness command failed');
        } catch (error) {
            showFeedback('Brightness error: ' + error.message, 'error');
        }
    }
    
    // Audio control functions
    async function setVolume(value) {
        document.getElementById('volumeValue').textContent = value;
        
        try {
            const response = await fetch(`/volume?value=${value}`);
            if (!response.ok) throw new Error('Volume command failed');
        } catch (error) {
            showFeedback('Volume error: ' + error.message, 'error');
        }
    }
    
    async function playSound(fileNumber) {
        try {
            showLoading('audio');
            const response = await fetch(`/playSound?file=${fileNumber}`);
            
            if (response.ok) {
                showFeedback('Playing sound', 'success');
            } else {
                throw new Error('Sound playback failed');
            }
        } catch (error) {
            showFeedback('Audio error: ' + error.message, 'error');
        } finally {
            hideLoading('audio');
        }
    }
    
    // Mode control functions
    async function setMode(mode) {
        try {
            showLoading('mode');
            const response = await fetch(`/mode?mode=${mode}`);
            
            if (response.ok) {
                updateModeButtons(mode);
                showFeedback('Mode changed to ' + mode.toUpperCase(), 'success');
            } else {
                throw new Error('Mode change failed');
            }
        } catch (error) {
            showFeedback('Mode error: ' + error.message, 'error');
        } finally {
            hideLoading('mode');
        }
    }
    
    function updateModeButtons(activeMode) {
        // Remove active class from all buttons
        document.querySelectorAll('.mode-btn').forEach(btn => {
            btn.classList.remove('active');
        });
        
        // Add active class to selected button
        const activeButton = document.getElementById(activeMode + 'Mode');
        if (activeButton) {
            activeButton.classList.add('active');
        }
    }
    
    // Utility functions
    function formatUptime(seconds) {
        const hours = Math.floor(seconds / 3600);
        const minutes = Math.floor((seconds % 3600) / 60);
        const secs = seconds % 60;
        return `${hours.toString().padStart(2, '0')}:${minutes.toString().padStart(2, '0')}:${secs.toString().padStart(2, '0')}`;
    }
    
    function formatMemory(bytes) {
        if (bytes < 1024) return bytes + ' B';
        if (bytes < 1048576) return Math.round(bytes / 1024) + ' KB';
        return Math.round(bytes / 1048576) + ' MB';
    }
    
    function setConnectionStatus(connected) {
        if (connected !== isConnected) {
            isConnected = connected;
            const statusElements = document.querySelectorAll('.status-value');
            statusElements.forEach(el => {
                el.style.color = connected ? '#4da6ff' : '#ff4757';
            });
            
            if (!connected) {
                showFeedback('Connection lost - attempting to reconnect...', 'error');
            } else {
                showFeedback('Connected to K-2SO', 'success');
            }
        }
    }
    
    function showLoading(section) {
        // Visual feedback for loading state
        const elements = document.querySelectorAll('.control-section');
        elements.forEach(el => {
            if (el.textContent.toLowerCase().includes(section)) {
                el.classList.add('loading');
            }
        });
    }
    
    function hideLoading(section) {
        const elements = document.querySelectorAll('.control-section');
        elements.forEach(el => {
            el.classList.remove('loading');
        });
    }
    
    function showFeedback(message, type) {
        // Create temporary feedback element
        const feedback = document.createElement('div');
        feedback.textContent = message;
        feedback.style.cssText = `
            position: fixed;
            top: 20px;
            right: 20px;
            padding: 15px 25px;
            border-radius: 8px;
            color: white;
            font-weight: bold;
            z-index: 1000;
            animation: slideInRight 0.3s ease;
            background: ${type === 'success' ? '#2ed573' : '#ff4757'};
            box-shadow: 0 4px 12px rgba(0,0,0,0.3);
        `;
        
        document.body.appendChild(feedback);
        
        setTimeout(() => {
            feedback.style.animation = 'slideOutRight 0.3s ease';
            setTimeout(() => document.body.removeChild(feedback), 300);
        }, 3000);
    }
    
    // Add CSS animations for feedback
    const style = document.createElement('style');
    style.textContent = `
        @keyframes slideInRight {
            from { transform: translateX(100%); }
            to { transform: translateX(0); }
        }
        @keyframes slideOutRight {
            from { transform: translateX(0); }
            to { transform: translateX(100%); }
        }
    `;
    document.head.appendChild(style);
    
    // Keyboard shortcuts
    document.addEventListener('keydown', function(event) {
        // Only process if not in an input field
        if (event.target.tagName === 'INPUT') return;
        
        switch(event.key) {
            case 'c':
            case 'C':
                centerAllServos();
                break;
            case '1':
                setMode('scanning');
                break;
            case '2':
                setMode('alert');
                break;
            case '3':
                setMode('idle');
                break;
            case 'r':
            case 'R':
                setColor('red');
                break;
            case 'g':
            case 'G':
                setColor('green');
                break;
            case 'b':
            case 'B':
                setColor('blue');
                break;
            case 'w':
            case 'W':
                setColor('white');
                break;
            case 'ArrowUp':
                event.preventDefault();
                setServos(90, 180, 90, 180);
                break;
            case 'ArrowDown':
                event.preventDefault();
                setServos(90, 0, 90, 0);
                break;
            case 'ArrowLeft':
                event.preventDefault();
                setServos(0, 90, 0, 90);
                break;
            case 'ArrowRight':
                event.preventDefault();
                setServos(180, 90, 180, 90);
                break;
        }
    });
    
    // Touch/mobile support
    let touchStartX, touchStartY;
    
    document.addEventListener('touchstart', function(e) {
        touchStartX = e.touches[0].clientX;
        touchStartY = e.touches[0].clientY;
    });
    
    document.addEventListener('touchend', function(e) {
        if (!touchStartX || !touchStartY) return;
        
        const touchEndX = e.changedTouches[0].clientX;
        const touchEndY = e.changedTouches[0].clientY;
        
        const diffX = touchStartX - touchEndX;
        const diffY = touchStartY - touchEndY;
        
        const minSwipeDistance = 50;
        
        if (Math.abs(diffX) > Math.abs(diffY)) {
            // Horizontal swipe
            if (Math.abs(diffX) > minSwipeDistance) {
                if (diffX > 0) {
                    // Swipe left
                    setServos(0, 90, 0, 90);
                } else {
                    // Swipe right
                    setServos(180, 90, 180, 90);
                }
            }
        } else {
            // Vertical swipe
            if (Math.abs(diffY) > minSwipeDistance) {
                if (diffY > 0) {
                    // Swipe up
                    setServos(90, 180, 90, 180);
                } else {
                    // Swipe down
                    setServos(90, 0, 90, 0);
                }
            }
        }
        
        touchStartX = null;
        touchStartY = null;
    });
    
    // Detail LED control functions
    async function setDetailCount(value) {
        document.getElementById('detailCountValue').textContent = value;
        try {
            const response = await fetch(`/detailCount?value=${value}`);
            if (!response.ok) throw new Error('Detail count command failed');
        } catch (error) {
            showFeedback('Detail LED error: ' + error.message, 'error');
        }
    }

    async function setDetailBrightness(value) {
        document.getElementById('detailBrightnessValue').textContent = value;
        try {
            const response = await fetch(`/detailBrightness?value=${value}`);
            if (!response.ok) throw new Error('Detail brightness command failed');
        } catch (error) {
            showFeedback('Detail LED error: ' + error.message, 'error');
        }
    }

    async function setDetailPattern(pattern) {
        try {
            showLoading('detail');
            const response = await fetch(`/detailPattern?pattern=${pattern}`);
            if (response.ok) {
                showFeedback('Detail LED pattern: ' + pattern.toUpperCase(), 'success');
            } else {
                throw new Error('Pattern command failed');
            }
        } catch (error) {
            showFeedback('Detail LED error: ' + error.message, 'error');
        } finally {
            hideLoading('detail');
        }
    }

    async function setDetailEnabled(state) {
        try {
            const response = await fetch(`/detailEnabled?state=${state}`);
            if (response.ok) {
                showFeedback('Detail LEDs: ' + state.toUpperCase(), 'success');
            } else {
                throw new Error('Detail enabled command failed');
            }
        } catch (error) {
            showFeedback('Detail LED error: ' + error.message, 'error');
        }
    }

    console.log('K-2SO Controller ready. Use keyboard shortcuts: C=center, 1/2/3=modes, R/G/B/W=colors, arrows=movement');
</script>
)rawliteral";
}

//========================================
// UTILITY FUNCTIONS
//========================================

String getCurrentStatusJSON() {
  String json = "{";
  json += "\"mode\":\"" + getModeName(currentMode) + "\",";
  json += "\"awake\":" + String(isAwake ? "true" : "false") + ",";
  json += "\"uptime\":" + String((millis() - uptimeStart) / 1000) + ",";
  json += "\"freeMemory\":" + String(ESP.getFreeHeap()) + ",";
  json += "\"irCommands\":" + String(irCommandCount) + ",";
  json += "\"servoMovements\":" + String(servoMovements);
  json += "}";
  return json;
}

String formatUptime(unsigned long seconds) {
  unsigned long hours = seconds / 3600;
  unsigned long minutes = (seconds % 3600) / 60;
  unsigned long secs = seconds % 60;
  
  char buffer[16];
  sprintf(buffer, "%02lu:%02lu:%02lu", hours, minutes, secs);
  return String(buffer);
}

String formatMemory(size_t bytes) {
  if (bytes < 1024) {
    return String(bytes) + " B";
  } else if (bytes < 1048576) {
    return String(bytes / 1024) + " KB";
  } else {
    return String(bytes / 1048576) + " MB";
  }
}

String escapeHTML(String input) {
  input.replace("&", "&amp;");
  input.replace("<", "&lt;");
  input.replace(">", "&gt;");
  input.replace("\"", "&quot;");
  input.replace("'", "&#39;");
  return input;
}