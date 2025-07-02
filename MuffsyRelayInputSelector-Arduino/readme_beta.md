# Muffsy Relay Input Selector - Enhanced Version (Beta)

🎵 **Version 2.0.0** - Now with WiFi control, web interface, and OTA updates!

## 🆕 What's New in Version 2.0

This enhanced version of the Muffsy Relay Input Selector adds modern connectivity features while maintaining all the original functionality you know and love.

### ✨ New Features

- **📱 WiFi Control** - Control your input selector from your phone, tablet, or computer
- **🌐 Web Interface** - Clean, responsive web interface for easy control
- **🔄 Over-The-Air Updates** - Update firmware wirelessly without cables
- **💻 Web Console** - View device logs and status remotely
- **🔧 Easy WiFi Setup** - Captive portal setup for connecting to your home network
- **💾 Enhanced Settings** - Automatic saving of all settings including WiFi credentials

### 🔒 Safety Improvements

- **Power-On Safety** - Device starts completely OFF and MUTED for maximum safety
- **Relay Protection** - All input relays turn off when muted or powered off
- **Startup Delay** - Configurable mute period during startup to protect amplifiers

---

## 🚀 Quick Start Guide

### 1. First Power-On

When you first power on your enhanced Muffsy Input Selector:

1. **Look for the WiFi network** `MuffsyInputSelector` on your phone/computer
2. **Connect using password**: `muffsy123`
3. **Open any website** - you'll be automatically redirected to the control interface
4. **Device starts OFF and MUTED** for safety - use Power button to turn on

### 2. Connect to Your Home WiFi

1. On the control interface, find the **WiFi Setup** section
2. **Enter your home WiFi** network name and password
3. **Click Connect** - the device will restart and connect to your network
4. **Find the IP address** in your router's connected devices, or check the Serial monitor

### 3. Bookmark the Control Interface

Once connected to your home WiFi:
- **Access via IP address**: `http://192.168.1.xxx` (your device's IP)
- **Bookmark this page** for easy access
- **Works on phones, tablets, and computers**

---

## 🎛️ Control Interface

### Main Control Page

![Web Interface](https://via.placeholder.com/600x400/007bff/ffffff?text=Web+Interface+Screenshot)

The web interface provides:

#### 📊 Status Display
- **Power Status**: ON/OFF
- **Current Input**: Input 1-4
- **Mute Status**: YES/NO
- **WiFi Status**: Connected/Not Connected

#### 🎚️ Control Buttons
- **Power**: Turn the input selector on/off
- **Mute**: Mute/unmute audio output
- **Input 1-4**: Direct input selection buttons

#### ⚙️ Additional Features
- **Console**: View live device logs and status messages
- **Firmware Update**: Upload new firmware files
- **WiFi Setup**: Change WiFi network (if in AP mode)

---

## 💻 Web Console

### Accessing the Console

1. **Click "Open Console"** on the main interface
2. **New window opens** with live console output
3. **Watch real-time activity** as you use the device

### Console Features

- **🕒 Timestamped logs** - See exactly when events occurred
- **🔄 Auto-refresh** - Updates every 2 seconds automatically
- **📜 Scrolling history** - View up to 50 recent log entries
- **🧹 Clear function** - Reset console display
- **📱 Mobile friendly** - Works great on phones

### What You'll See

```
[0:02:15] Muffsy Relay Input Selector - Enhanced v2.0.0
[0:02:15] Hardware initialized
[0:02:16] Connected to WiFi!
[0:02:16] IP address: 192.168.1.100
[0:02:16] Setup complete - ready for operation
[0:02:45] IR received: 0x24
[0:02:45] Input selected: 2
[0:02:52] Power: OFF
[0:02:58] Power: ON
```

---

## 🔄 Firmware Updates

### Two Ways to Update

#### 1. Web Interface Upload (Recommended)
1. **Get the new firmware** (.bin file) from the releases page
2. **Go to Firmware Update** section on web interface
3. **Choose file** and click "Upload Firmware"
4. **Device updates automatically** and restarts

#### 2. Arduino IDE OTA (For Developers)
1. **Open Arduino IDE**
2. **Go to Tools → Port**
3. **Select "muffsy-input-selector" network port**
4. **Upload normally** - no USB cable needed!

### Creating Firmware Files
If you're compiling your own firmware:
1. **Arduino IDE**: Sketch → Export Compiled Binary
2. **Look for** `YourSketch.ino.esp32.bin` file
3. **Upload this file** via web interface

---

## 🎮 Control Methods

Your input selector responds to **three control methods**:

### 1. 📱 Web Interface
- **Power, Mute, Input selection**
- **Real-time status updates**
- **Works from anywhere on your network**

### 2. 📺 IR Remote Control
**Same IR codes as original version:**
- **Power**: 0x45
- **Mute**: 0x47  
- **Input 1**: 0x12
- **Input 2**: 0x24
- **Input 3**: 0x94
- **Input 4**: 0x8
- **Up/Down**: 0x18/0x52

### 3. 🎛️ Rotary Encoder
- **Rotate**: Change input (1→2→3→4→1...)
- **Short Press**: Power on/off
- **Long Press**: Mute/unmute

---

## ⚙️ Settings & Configuration

### Automatically Saved Settings
- **Current input selection** (survives power cycles)
- **WiFi network credentials**
- **SSR (Solid State Relay) enable/disable**

### User Configurable Options
Edit these in the source code before compiling:

```cpp
// WiFi Access Point settings
#define AP_SSID          "MuffsyInputSelector"
#define AP_PASSWORD      "muffsy123"

// Timing settings
#define STARTUP_DELAY    2000    // Startup mute delay (ms)
#define DEBOUNCE_DELAY   200     // Button debounce (ms)

// IR Remote codes (replace with your remote)
#define IR_POWER         0x45
#define IR_INPUT_1       0x12
// ... etc
```

---

## 🔧 Technical Details

### Hardware Compatibility
- **✅ Same PCB** - Works with existing Muffsy Input Selector hardware
- **✅ Same pins** - No hardware changes required
- **✅ Same relays** - All original relay functionality preserved
- **✅ Same components** - ESP32, rotary encoder, IR receiver

### Pin Configuration
| Function | ESP32 Pin | Notes |
|----------|-----------|-------|
| Input A Relay | 16 | Same as original |
| Input B Relay | 12 | Same as original |
| Input C Relay | 27 | Same as original |
| Input D Relay | 33 | Same as original |
| Mute Relay | 32 | Same as original |
| SSR Control | 17 | Same as original |
| Power LED | 2 | Same as original |
| Encoder CLK | 35 | Same as original |
| Encoder DT | 34 | Same as original |
| Encoder SW | 5 | Same as original |
| IR Receiver | 36 | Same as original |

### Network Information
- **Access Point Mode**: `MuffsyInputSelector` / `muffsy123`
- **Station Mode**: Connects to your home WiFi
- **Web Server**: Port 80 (standard HTTP)
- **OTA**: Password `muffsy`, hostname `muffsy-input-selector`

---

## 🛠️ Installation

### Required Libraries
Install these libraries in Arduino IDE:

1. **IRremote** by shirriff
2. **Versatile_RotaryEncoder** by Rui Seixas

ESP32 core libraries (included automatically):
- WiFi, WebServer, ArduinoOTA, Preferences, DNSServer

### Compilation & Upload
1. **Select board**: NodeMCU-32S
2. **Select port**: Your ESP32's USB port
3. **Compile and upload** normally
4. **Device creates WiFi AP** on first boot

---

## 🚨 Safety Features

### Startup Safety Sequence
1. **All relays OFF** immediately on power-up
2. **Device starts POWERED OFF**
3. **Device starts MUTED**
4. **Startup delay period** (configurable, default 2 seconds)
5. **User must explicitly turn on** via any control method

### Operational Safety
- **Mute activates** when powering off
- **All input relays turn off** when muted
- **SSR control** follows power state
- **Settings auto-save** to prevent loss

---

## 🆘 Troubleshooting

### WiFi Connection Issues

**Problem**: Can't connect to home WiFi
**Solution**: 
1. Hold down reset button for 10+ seconds to factory reset WiFi settings
2. Device will restart in Access Point mode
3. Reconnect and re-enter WiFi credentials

**Problem**: Can't find device IP address
**Solution**:
1. Check your router's connected devices list
2. Look for "muffsy-input-selector" or similar
3. Use Serial monitor to see IP address during startup

### Web Interface Issues

**Problem**: Web interface won't load
**Solution**:
1. Check device is connected to WiFi (LED indicators)
2. Try refreshing browser page
3. Clear browser cache
4. Try different browser/device

**Problem**: Controls don't work
**Solution**:
1. Check device power status
2. Refresh web page
3. Check console for error messages

### Hardware Issues

**Problem**: IR remote not working
**Solution**:
1. Check console for "IR received" messages
2. Point remote directly at IR receiver
3. Replace remote batteries
4. Verify IR codes in source code match your remote

**Problem**: Rotary encoder not working
**Solution**:
1. Check encoder connections
2. Try different rotation speeds
3. Check console for encoder activity
4. Verify pin assignments

### OTA Update Issues

**Problem**: OTA update fails
**Solution**:
1. Ensure device and computer on same network
2. Check Arduino IDE has ESP32 OTA port available
3. Try web interface update instead
4. Restart device and try again

---

## 📞 Support & More Information

### Original Project
- **Website**: [muffsy.com](https://www.muffsy.com/muffsy-relay-input-selector-4)
- **GitHub**: [github.com/muffsy/RelayInputSelector](https://github.com/muffsy/RelayInputSelector)
- **Tindie Store**: [Available as kit](https://www.tindie.com/products/skrodahl/muffsy-relay-input-selector-kit/)

### Enhanced Version
- **This is a community enhancement** of the original open-source project
- **All new features** maintain compatibility with original hardware
- **Open source** under same BSD 3-Clause license

### Getting Help
1. **Check console** for error messages
2. **Try Serial monitor** for detailed debugging  
3. **Reset to factory defaults** if needed
4. **Community support** via original project channels

---

## 🎉 Enjoy Your Enhanced Input Selector!

Your Muffsy Relay Input Selector now has modern connectivity while maintaining the same high-quality audio switching you expect. Control it from anywhere in your home, update firmware wirelessly, and monitor its operation through the web console.

**Happy listening! 🎵**