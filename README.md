# K-2SO Animatronic Sketch v1.0.4
**Advanced ESP32-S3 based animatronics controller for Star Wars K-2SO droid builds**

## 🤖 Project Overview

This animatronics controller brings your K-2SO droid to life with film-accurate behaviors, advanced servo control, dynamic lighting, high-quality audio, and multiple control interfaces. 
Designed for builders who demand professional results.

### Key Features

- **🎯 4-Servo Motion Control** - Eye pan/tilt + Head pan/tilt with smooth autonomous movement
- **💡 Dual NeoPixel Eyes** - 7-pixel eyes with advanced animations (fade, flicker, pulse, scanner)
- **🌐 WiFi Web Interface** - Complete control via modern responsive web UI with gamepad
- **📡 IR Remote Learning** - Program any IR remote with 21-button support
- **🔊 Professional Audio** - DFPlayer Mini + PAM8406 amplifier with folder organization
- **💾 EEPROM Configuration** - Persistent settings with profile management
- **⌨️ Advanced Serial CLI** - Complete command-line interface for configuration
- **✨ Detail LED System** - Random blinking effects for added realism
- **🚀 Boot Sequence** - Authentic startup animation and sound

## 🔧 Hardware Requirements

### Core Components
- **ESP32-S3-Zero** (or compatible ESP32-S3 board)
- **4x Standard Servos** (180° rotation, SG90 or similar)
- **2x NeoPixel LED Strips** (7 pixels each, WS2812B)
- **5V Power Supply** (adequate for servos - minimum 3A recommended)

### Audio System
- **DFPlayer Mini** - MP3 player module
- **PAM8406 Stereo Amplifier** - 5W per channel at 4Ω
- **MicroSD Card** - Class 10 recommended, FAT32 format
- **Audio Output Options:**
  - **Option 1**: PAM8406 amplifier → 2 screw terminals (stereo speaker output)
  - **Option 2**: DFPlayer line out → 3.5mm jack (direct line signal)
- **Speakers** - 4Ω or 8Ω compatible (for amplified output)

### Optional Components  
- **IR Receiver TSOP38238** - For remote control
- **Detail LEDs** - For random blinking effects

### Recommended Carrier Board
- **Droid Logic Motion Board v1.2** - Professional carrier board from Printed-Droid.com

## 📋 Pin Configuration

ESP32-S3-Zero Pin Assignments:

GP1  → I2C SDA (future expansion)
GP2  → I2C SCL (future expansion)
GP3  → Left Eye NeoPixel (7 pixels)
GP4  → Right Eye NeoPixel (7 pixels)
GP5  → Eye Pan Servo
GP6  → Eye Tilt Servo
GP7  → Head Pan Servo
GP8  → Head Tilt Servo
GP9  → IR Receiver (TSOP38238)
GP10 → Detail LED 1
GP11 → DFPlayer TX
GP12 → DFPlayer RX
GP13 → Detail LED 2

## 🔌 Power Requirements

- **Main Supply**: 5V/3A minimum
- **Servos**: 5V direct connection (1A total)
- **Audio**: PAM8406 amplifier (up to 2A at full volume)
- **LEDs**: 5V with current limiting (500mA)
- **ESP32-S3**: 3.3V internal regulation (200mA)

**⚠️ Safety Warning**: Ensure adequate current capacity and proper fusing for safety.

## 💿 SD Card Structure

Create the following folder structure on your microSD card (FAT32 format):


📁 /
├── 📁 01/              # Scanning mode sounds
│   ├── 001.mp3         # Ambient scanning sounds
│   ├── 002.mp3         # Servo movement sounds
│   └── ...
├── 📁 02/              # Alert mode sounds  
│   ├── 001.mp3         # Alert beeps/warnings
│   ├── 002.mp3         # Fast response sounds
│   └── ...
├── 📁 03/              # Boot sequence
│   └── 001.mp3         # System startup sound
└── 📁 04/              # Voice lines/responses
    ├── 001.mp3         # "I am K-2SO"
    ├── 002.mp3         # "Behavior"
    ├── 003.mp3         # "Fresh one"
    └── ...
```

## 🚀 Installation

### 1. Arduino IDE Setup

1. Install ESP32 board support:
   - File → Preferences → Additional Board Manager URLs
   - Add: `https://raw.githubusercontent.com/espressif/arduino-esp32/gh-pages/package_esp32_index.json`
   - Tools → Board → Board Manager → Search "ESP32" → Install

2. **Board Configuration for Waveshare ESP32-S3 Zero:**
   - **Board**: "ESP32S3 Dev Module"
   - **USB CDC On Boot**: "Enabled" ⚠️ **CRITICAL - Must be enabled!**
   - **CPU Frequency**: "240MHz (WiFi)"
   - **Core Debug Level**: "None"
   - **USB DFU On Boot**: "Disabled"
   - **Erase All Flash Before Sketch Upload**: "Disabled"
   - **Events Run On**: "Core 1"
   - **Flash Mode**: "QIO 80MHz"
   - **Flash Size**: "8MB (64Mb)"
   - **JTAG Adapter**: "Disabled" 
   - **Arduino Runs On**: "Core 1"
   - **USB Firmware MSC On Boot**: "Disabled"
   - **Partition Scheme**: "8M with spiffs (3MB APP/1.5MB SPIFFS)"
   - **PSRAM**: "OPI PSRAM"
   - **Upload Mode**: "UART0 / Hardware CDC"
   - **Upload Speed**: "921600"
   - **USB Mode**: "Hardware CDC and JTAG"

3. **Important Upload Notes:**
   - Always enable "USB CDC On Boot" - without this, serial communication will fail
   - If upload fails, try holding BOOT button while connecting USB

### 2. Required Libraries

Install via Arduino Library Manager:


Required Libraries:
├── Adafruit NeoPixel (1.15.1+)
├── ESP32Servo (3.0.8+)
├── DFPlayer Mini Mp3 by Makuna (1.2.3+)
└── IRremote (4.4.2+)

Built-in (no installation needed):
├── WiFi
├── WebServer  
├── ESPmDNS
└── EEPROM


### 3. Project File Structure


K-2SO_DroidLogicMotion_v1.0.4/
├── K-2SO_DroidLogicMotion_v1.0.4.ino    # Main program file
├── config.h                              # Hardware configuration
├── globals.h                             # Global variable declarations
├── handlers.cpp/.h                       # Command processing
├── animations.cpp/.h                     # LED animation system
├── webpage.cpp/.h                        # Web interface
├── Mp3Notify.cpp/.h                      # Audio system callbacks
└── README.md                             # This file


### 4. Configuration Steps

#### Step 1: WiFi Setup
Edit `config.h`:

#define WIFI_SSID           "Your_WiFi_Network"
#define WIFI_PASSWORD       "Your_WiFi_Password"


#### Step 2: Upload Code
1. Connect ESP32-S3 via USB
2. Select correct COM port
3. Upload code (Ctrl+U)
4. Open Serial Monitor (115200 baud)

#### Step 3: Audio Setup
1. Format microSD card as FAT32
2. Create folder structure and copy MP3 files
3. Insert card into DFPlayer Mini
4. Verify audio system initialization in Serial Monitor

#### Step 4: IR Remote Programming
In Serial Monitor, type:

learn

Follow prompts to program your remote, or type:

default

To load standard NEC remote codes.

## 🎮 Control Interfaces

### 1. IR Remote Layout (17-button standard)

┌─────┬─────┬─────┬─────┐
│  1  │  2  │  3  │  UP │  
│Scan │Alert│Idle │ Eye │  
├─────┼─────┼─────┼─────┤
│  4  │  5  │  6  │DOWN │  
│RndS │RndA │Voice│ Eye │  
├─────┼─────┼─────┼─────┤
│  7  │  8  │  9  │LEFT │  
│     │     │     │ Eye │  
├─────┼─────┼─────┼─────┤
│  *  │  0  │  #  │RIGHT│  
│Col- │On/Of│Col+ │ Eye │  
└─────┴─────┴─────┴─────┘
         │ OK │  
         │Cntr│  
         └────┘


#### Button Functions:
- **Movement**: UP/DOWN/LEFT/RIGHT (direct eye positioning), OK (center all)
- **Personality**: 1=Scanning(Ice Blue), 2=Alert(Red), 3=Idle(Amber)  
- **Audio**: 4=Random Scan Sound, 5=Random Alert Sound, 6=Random Voice
- **Visual**: *=Color Backward, #=Color Forward, 0=Eyes On/Off

### 2. Web Interface

Access via browser: `http://[ESP32_IP_ADDRESS]` or `http://k2so.local`

**Features:**
- **Servo Gamepad** - 9-button directional control with position display
- **Eye Controls** - Color picker, animations, brightness slider
- **Audio Controls** - Volume control, sound playback buttons
- **Mode Selection** - Scanning/Alert/Idle personality modes
- **System Status** - Real-time monitoring (uptime, memory, statistics)

### 3. Serial Commands (115200 baud)

#### Basic Commands:

help          # Show complete command reference
status        # Display system information  
config        # Show current configuration
save          # Save settings to EEPROM
reset         # Restart system


#### Hardware Control:

# Servo Control
servo show                          # Display all servo settings
servo eye center 90 90              # Set eye center positions
servo eye limits 45 135 45 135      # Set eye movement ranges
servo head center 90 90             # Set head center
servo test all                      # Test all servos

# LED Control  
led color 255 0 0                   # Set red eyes (RGB values)
led brightness 200                  # Set brightness (0-255)
led mode flicker                    # Set animation mode

# Audio Control
sound volume 25                     # Set volume (0-30)
sound play 1                        # Play specific file
sound folder 4 1                    # Play folder 4, track 1


#### Mode Control:

mode scanning           # Slow, methodical observation
mode alert             # Fast, reactive responses  
mode idle              # Minimal movement, power saving


#### Timing Adjustment:

timing scan move 20 40              # Scan mode movement speed (ms)
timing scan wait 3000 6000          # Wait between movements (ms)
timing alert move 5 15              # Alert mode movement speed
timing sound 8000 20000             # Sound pause intervals


#### Profile Management:

profile save "MyK2SO"              # Save current configuration
profile load 0                     # Load saved profile
profile list                       # Show all profiles
profile delete 1                   # Delete profile


#### IR Remote Setup:

learn         # Program your IR remote (step-by-step)
scan          # IR code scanner mode
show          # Display programmed codes  
default       # Load standard remote codes
clear         # Clear all IR codes (requires confirmation)
ir on/off     # Enable/disable IR receiver


#### System Tools:

monitor       # Live system monitoring mode
test          # Run hardware test sequence
backup        # Export configuration as hex
restore       # Import configuration from hex


## 🎭 Personality Modes

### SCANNING Mode (Default)
- **Behavior**: Slow, methodical eye movements
- **Eye Color**: Ice Blue (RGB: 80, 150, 255)
- **Movement**: Deliberate servo positioning
- **Audio**: Ambient scanning sounds, servo effects
- **Timing**: 3-6 second pauses between movements

### ALERT Mode  
- **Behavior**: Fast, reactive movements
- **Eye Color**: Alert Red (RGB: 255, 0, 0)
- **Movement**: Quick response to stimuli
- **Audio**: Alert beeps, warnings, rapid responses
- **Timing**: 0.5-1.5 second pauses, sharp movements

### IDLE Mode
- **Behavior**: Minimal movement for power saving
- **Eye Color**: Dim Amber (RGB: 100, 60, 0)
- **Movement**: Occasional subtle positioning
- **Audio**: Silent operation
- **Timing**: Extended pauses, minimal servo activity

## 🔧 Advanced Configuration

### Servo Calibration

1. **Find Center Positions:**

   servo eye center 90 90     # Start with standard center
   servo head center 90 90    # Adjust as needed for your build

2. **Set Movement Ranges:**

   servo eye limits 45 135 30 150    # Pan: 45-135°, Tilt: 30-150°
   servo head limits 0 180 0 180     # Full range (adjust for clearance)

3. **Test Movement:**

   servo test all             # Run complete servo test

### LED Customization


# Basic Colors
led color 255 255 255      # White
led color 255 0 0          # Red  
led color 0 255 0          # Green
led color 0 0 255          # Blue
led color 80 150 255       # K-2SO Ice Blue

# Animation Modes
led mode solid             # Static color
led mode flicker           # Random brightness variation
led mode pulse             # Smooth breathing effect

# Brightness Control
led brightness 255         # Maximum brightness
led brightness 150         # Medium brightness  
led brightness 50          # Dim
```

### Audio System Setup

1. **Volume Configuration:**

   sound volume 20           # Set volume (0-30)

2. **Test Playback:**

   sound play 1             # Test file playback
   sound folder 4 1         # Play specific folder/track

3. **Sound Timing:**

   timing sound 10000 30000  # 10-30 second pauses between sounds

## 🛠️ Troubleshooting

### Hardware Issues

#### LEDs Not Working
- ✅ Check power supply (5V, adequate current)
- ✅ Verify NeoPixel pin connections (GP3, GP4)
- ✅ Test with simple colors: `led color 255 0 0`
- ✅ Check if pixels are damaged (try different LED count)

#### Servos Not Moving  
- ✅ Verify 5V power supply (minimum 3A)
- ✅ Check servo pin connections (GP5-GP8)
- ✅ Test individual servos: `servo test all`
- ✅ Calibrate center positions if binding occurs

#### Audio Not Working
- ✅ Verify DFPlayer wiring (GP11→TX, GP12→RX)
- ✅ Check SD card format (FAT32) and file structure
- ✅ Ensure MP3 files are in correct folders (01/, 02/, etc.)
- ✅ Test volume: `sound volume 25`
- ✅ Check amplifier power and speaker connections

#### IR Remote Not Responding
- ✅ Verify IR receiver connection (GP9)
- ✅ Program remote: type `learn` in Serial Monitor
- ✅ Test scanner mode: type `scan`
- ✅ Check IR codes: type `show`

### Software Issues

#### Compilation Errors
- ✅ Verify ESP32 board package (3.3.1+)
- ✅ Install all required libraries
- ✅ Select "ESP32S3 Dev Module" as board
- ✅ Check for library version conflicts

#### WiFi Connection Problems
- ✅ Update credentials in config.h
- ✅ Check network availability and password
- ✅ Verify 2.4GHz network (ESP32 doesn't support 5GHz)
- ✅ Check router compatibility and firewall settings

#### Serial Communication Issues
- ✅ Set baud rate to 115200
- ✅ Select correct COM port
- ✅ Check USB cable quality
- ✅ Press EN button if ESP32 doesn't respond

### Performance Issues

#### Memory Problems

status                    # Check free RAM

- If RAM < 50KB, reduce audio buffer or LED effects

#### Servo Jitter

timing scan move 50 100   # Slow down movement speed
servo test all            # Check mechanical binding


#### Audio Dropouts
- Use Class 10 microSD card
- Check power supply stability
- Reduce audio bitrate if necessary

## 📊 System Monitoring

### Real-Time Monitoring Mode

monitor                   # Enter live monitoring mode


Displays:
- Current servo positions
- Memory usage
- IR command activity  
- Audio system status
- System uptime

### Status Information

status                    # Show complete system status


Provides:
- Operating mode and personality
- Hardware initialization status
- Network connection info
- Performance statistics
- Configuration summary

### Debug Output

Enable detailed debugging in Serial Monitor:

#define ENABLE_SERIAL_DEBUG true    // In config.h


## 💡 Status LED System

The K-2SO controller features an intelligent status LED system using a single WS2812 LED that provides instant visual feedback for all system activities and states.

### Status Indicators

#### System States
- **WiFi Connected**: Solid green - stable network connection
- **WiFi Disconnected**: Solid red - no network connection  
- **WiFi Connecting**: Fast yellow blinking - attempting connection
- **Boot Sequence**: Blue pulsing - system initialization in progress
- **Error State**: Fast red blinking - hardware or software error detected

#### Personality Modes
- **Scanning Mode**: Ice blue pulsing - methodical observation behavior
- **Alert Mode**: Red pulsing - fast reactive behavior
- **Idle Mode**: Amber pulsing - minimal movement, power saving

#### Activity Flashes (Brief 100ms Flashes)
- **IR Command**: White flash - remote control command received
- **Servo Movement**: Blue flash - servo motor activity
- **Audio Playback**: Green flash - sound system activity

#### Special Operating Modes
- **IR Learning**: Purple blinking - remote programming mode
- **Configuration**: Cyan pulsing - setup/monitor/test modes
- **Hardware Test**: Color cycling - diagnostic mode active

### Hardware Setup


## 🔐 Safety Considerations

### Electrical Safety
- **Power Supply**: Use regulated 5V supply with adequate current rating
- **Fusing**: Install appropriate fuses for servo and audio circuits
- **Isolation**: Ensure proper electrical isolation in droid housing
- **Heat Management**: Monitor ESP32 temperature during operation

### Mechanical Safety  
- **Servo Limits**: Set proper movement ranges to prevent binding
- **Emergency Stop**: Always accessible power disconnect
- **Secure Mounting**: Ensure all components are properly mounted
- **Cable Management**: Secure all wiring to prevent damage

### Operational Safety
- **EEPROM Wear**: Avoid excessive save operations (auto-leveling implemented)
- **Memory Management**: Monitor free RAM during operation
- **Update Safety**: Always backup configuration before updates

## 🎯 Performance Optimization

### Movement Smoothness

# Optimize servo timing for your build
timing scan move 20 40        # Faster movement
timing scan wait 2000 4000    # Shorter pauses


### Power Efficiency  

mode idle                     # Use idle mode when inactive
led brightness 100            # Reduce LED brightness
sound volume 15               # Lower audio volume


### Response Time

timing alert move 5 10        # Very fast alert responses
timing alert wait 200 800     # Quick reaction timing


## 🚀 Future Expansion

### Prepared Hardware Interfaces
- **I2C Bus** (GP1/GP2): Sensor expansion
- **Detail LEDs** (GP10/GP13): Additional effects
- **Spare Pins**: Custom sensor integration

### Planned Features
- PIR motion detection
- Proximity sensors
- Environmental sensors
- Advanced behavior programming
- Over-the-air updates

## 📞 Support

### Getting Help

1. **Check this README** for common solutions
2. **Review Serial Monitor** output for error messages
3. **Verify hardware connections** against pin diagram
4. **Test individual systems** using diagnostic commands

### Common Issues Quick Reference

| Problem | Quick Fix |
|---------|-----------|
| No web interface | Check WiFi credentials in config.h |
| No servo movement | Verify 5V power supply, check pin connections |
| No LED response | Test power supply, verify NeoPixel pins |
| No audio | Check SD card format, verify DFPlayer wiring |
| IR not working | Program remote with `learn` command |

### Diagnostic Commands


test                     # Complete hardware test
status                  # System health check  
config                  # Configuration verification
show                    # IR remote status


## 📜 License & Credits

### Project Credits
- **Hardware Design**: Printed-Droid.com
- **Software Development**: Printed-Droid.com

### Open Source Libraries
- **Adafruit NeoPixel**: LED control
- **ESP32Servo**: Servo motor control  
- **DFMiniMp3 (Makuna)**: Audio system
- **IRremote**: IR receiver support

### Disclaimer

⚠️ **IMPORTANT SAFETY NOTICE** ⚠️

This project involves electrical components, servo motors, and audio amplifiers. Users are responsible for:

- Proper electrical safety and insulation
- Adequate power supply sizing and protection  
- Safe mechanical assembly and operation
- Compliance with local electrical codes
- Testing all functions before final installation

**BUILD AT YOUR OWN RISK.** Ensure proper knowledge of electronics and safety practices. The authors assume no responsibility for damage, injury, or malfunction resulting from use of this design.


**May the Force be with your build!** 🌟

*For the latest updates and community support, visit: www.printed-droid.com*
