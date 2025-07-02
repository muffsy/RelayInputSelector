/*
 * BETA-version: UNTESTED!
* 
 * Features:
 * - WiFi AP setup with web interface
 * - OTA updates
 * - Web-based console
 * - Clean modular code structure
 * - Persistent settings storage
 * 
 * Hardware: ESP32 NodeMCU-32S with relay board
 * 
 * Author: Enhanced version for muffsy.com
 * License: BSD 3-Clause
 */

#include <WiFi.h>
#include <WebServer.h>
#include <ArduinoOTA.h>
#include <Preferences.h>
#include <IRremote.hpp>
#include <Versatile_RotaryEncoder.h>
#include <DNSServer.h>

// ================================
// PIN DEFINITIONS
// ================================
#define RELAY_INPUT_A    16
#define RELAY_INPUT_B    12
#define RELAY_INPUT_C    27
#define RELAY_INPUT_D    33
#define RELAY_MUTE       32
#define SSR_PIN          17

#define POWER_LED        2
#define MUTE_LED         4
#define INPUT_A_LED      13
#define INPUT_B_LED      14
#define INPUT_C_LED      15
#define INPUT_D_LED      18

#define ENCODER_CLK      35
#define ENCODER_DT       34
#define ENCODER_SW       5
#define IR_RECEIVE_PIN   36

// ================================
// USER CONFIGURABLE SETTINGS
// ================================
#define FIRMWARE_VERSION "2.0.0"
#define AP_SSID          "MuffsyInputSelector"
#define AP_PASSWORD      "muffsy123"

// IR Remote codes (replace with your remote's codes)
#define IR_POWER         0x45
#define IR_MUTE          0x47
#define IR_INPUT_1       0x12
#define IR_INPUT_2       0x24
#define IR_INPUT_3       0x94
#define IR_INPUT_4       0x8
#define IR_UP            0x18
#define IR_DOWN          0x52

// Timing settings
#define STARTUP_DELAY    2000    // Mute delay on startup (ms)
#define DEBOUNCE_DELAY   200     // Button debounce (ms)
#define LONG_PRESS_TIME  1000    // Long press threshold (ms)

// ================================
// GLOBAL VARIABLES
// ================================
WebServer server(80);
DNSServer dnsServer;
Preferences prefs;
Versatile_RotaryEncoder encoder(ENCODER_CLK, ENCODER_DT, ENCODER_SW);

// Web console buffer
#define CONSOLE_BUFFER_SIZE 50
String consoleBuffer[CONSOLE_BUFFER_SIZE];
int consoleIndex = 0;
bool consoleWrapped = false;

// System state
struct SystemState {
  bool powered = false;
  bool muted = false;
  int currentInput = 1;        // 1-4
  bool ssrEnabled = false;
  String wifiSSID = "";
  String wifiPassword = "";
  bool wifiConnected = false;
  bool apMode = true;
} state;

// Timing variables
unsigned long lastIRCommand = 0;
unsigned long lastEncoderAction = 0;
bool setupComplete = false;

// ================================
// FUNCTION DECLARATIONS
// ================================
void setupHardware();
void setupWiFi();
void setupWebServer();
void setupOTA();
void setupIR();
void setupEncoder();
void loadSettings();
void saveSettings();

void handleInputSelection(int input);
void handlePowerToggle();
void handleMuteToggle();
void updateLEDs();
void updateRelays();

void processIR();
void processEncoder();
void processWebRequests();

// Console functions
void addToConsole(String message);
void serialPrintln(String message);

// Web interface handlers
void handleRoot();
void handleWiFiSetup();
void handleWiFiConnect();
void handleStatus();
void handleControl();
void handleConsole();
void handleConsoleData();
void handleOTAUpload();
void handleNotFound();

// ================================
// MAIN FUNCTIONS
// ================================
void setup() {
  Serial.begin(115200);
  delay(1000);
  
  serialPrintln("\n" + String(FIRMWARE_VERSION));
  serialPrintln("Muffsy Relay Input Selector - Enhanced");
  serialPrintln("=====================================");
  
  setupHardware();
  loadSettings();
  setupWiFi();
  setupWebServer();
  setupOTA();
  setupIR();
  setupEncoder();
  
  // Startup sequence - device starts OFF and MUTED for safety
  serialPrintln("Startup delay: " + String(STARTUP_DELAY) + "ms (safety mute period)");
  delay(STARTUP_DELAY);
  
  // Device remains off and muted until user explicitly turns it on
  // This ensures no audio signals pass through during startup
  updateLEDs();
  updateRelays();
  
  setupComplete = true;
  serialPrintln("Setup complete - ready for operation");
}

void loop() {
  if (setupComplete) {
    processIR();
    processEncoder();
    processWebRequests();
    ArduinoOTA.handle();
    
    // Handle WiFi reconnection
    if (!state.apMode && WiFi.status() != WL_CONNECTED) {
      static unsigned long lastReconnectAttempt = 0;
      if (millis() - lastReconnectAttempt > 30000) { // Try every 30 seconds
        serialPrintln("WiFi disconnected, attempting reconnection...");
        WiFi.reconnect();
        lastReconnectAttempt = millis();
      }
    }
  }
  delay(10); // Small delay to prevent watchdog issues
}

// ================================
// SETUP FUNCTIONS
// ================================
void setupHardware() {
  serialPrintln("Initializing hardware...");
  
  // Initialize relay pins
  pinMode(RELAY_INPUT_A, OUTPUT);
  pinMode(RELAY_INPUT_B, OUTPUT);
  pinMode(RELAY_INPUT_C, OUTPUT);
  pinMode(RELAY_INPUT_D, OUTPUT);
  pinMode(RELAY_MUTE, OUTPUT);
  pinMode(SSR_PIN, OUTPUT);
  
  // Initialize LED pins
  pinMode(POWER_LED, OUTPUT);
  pinMode(MUTE_LED, OUTPUT);
  pinMode(INPUT_A_LED, OUTPUT);
  pinMode(INPUT_B_LED, OUTPUT);
  pinMode(INPUT_C_LED, OUTPUT);
  pinMode(INPUT_D_LED, OUTPUT);
  
  // Set initial states - SAFETY FIRST!
  digitalWrite(RELAY_MUTE, HIGH);  // Start muted (relay OFF = muted)
  digitalWrite(SSR_PIN, LOW);      // SSR off
  
  // Turn off all input relays immediately
  digitalWrite(RELAY_INPUT_A, LOW);
  digitalWrite(RELAY_INPUT_B, LOW);
  digitalWrite(RELAY_INPUT_C, LOW);
  digitalWrite(RELAY_INPUT_D, LOW);
  
  state.muted = true;              // Always start muted
  state.powered = false;           // Start powered off for safety
  
  updateLEDs();
  serialPrintln("Hardware initialized");
}

void setupWiFi() {
  serialPrintln("Setting up WiFi...");
  
  if (state.wifiSSID.length() > 0) {
    // Try to connect to saved network
    serialPrintln("Attempting to connect to saved network: " + state.wifiSSID);
    WiFi.begin(state.wifiSSID.c_str(), state.wifiPassword.c_str());
    
    int attempts = 0;
    while (WiFi.status() != WL_CONNECTED && attempts < 20) {
      delay(500);
      Serial.print(".");
      attempts++;
    }
    
    if (WiFi.status() == WL_CONNECTED) {
      state.wifiConnected = true;
      state.apMode = false;
      serialPrintln("\nConnected to WiFi!");
      serialPrintln("IP address: " + WiFi.localIP().toString());
      return;
    }
  }
  
  // Start AP mode if no saved network or connection failed
  serialPrintln("Starting Access Point mode...");
  WiFi.mode(WIFI_AP);
  WiFi.softAP(AP_SSID, AP_PASSWORD);
  
  // Setup captive portal
  dnsServer.start(53, "*", WiFi.softAPIP());
  
  state.apMode = true;
  state.wifiConnected = false;
  serialPrintln("AP started");
  serialPrintln("SSID: " + String(AP_SSID));
  serialPrintln("IP: " + WiFi.softAPIP().toString());
}

void setupWebServer() {
  serialPrintln("Setting up web server...");
  
  server.on("/", handleRoot);
  server.on("/wifi", handleWiFiSetup);
  server.on("/connect", HTTP_POST, handleWiFiConnect);
  server.on("/status", handleStatus);
  server.on("/control", HTTP_POST, handleControl);
  server.on("/console", handleConsole);
  server.on("/console-data", handleConsoleData);
  server.on("/update", HTTP_GET, handleOTAUpload);
  server.on("/update", HTTP_POST, []() {
    server.send(200, "text/plain", (Update.hasError()) ? "FAIL" : "OK");
    ESP.restart();
  }, []() {
    HTTPUpload& upload = server.upload();
    if (upload.status == UPLOAD_FILE_START) {
      Serial.printf("Update: %s\n", upload.filename.c_str());
      if (!Update.begin(UPDATE_SIZE_UNKNOWN)) {
        Update.printError(Serial);
      }
    } else if (upload.status == UPLOAD_FILE_WRITE) {
      if (Update.write(upload.buf, upload.currentSize) != upload.currentSize) {
        Update.printError(Serial);
      }
    } else if (upload.status == UPLOAD_FILE_END) {
      if (Update.end(true)) {
        Serial.printf("Update Success: %u\nRebooting...\n", upload.totalSize);
      } else {
        Update.printError(Serial);
      }
    }
  });
  
  server.onNotFound(handleNotFound);
  server.begin();
  serialPrintln("Web server started");
}

void setupOTA() {
  serialPrintln("Setting up OTA...");
  
  ArduinoOTA.setHostname("muffsy-input-selector");
  ArduinoOTA.setPassword("muffsy");
  
  ArduinoOTA.onStart([]() {
    String type = (ArduinoOTA.getCommand() == U_FLASH) ? "sketch" : "filesystem";
    serialPrintln("Start updating " + type);
  });
  
  ArduinoOTA.onEnd([]() {
    serialPrintln("\nOTA Update completed");
  });
  
  ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
    Serial.printf("Progress: %u%%\r", (progress / (total / 100)));
  });
  
  ArduinoOTA.onError([](ota_error_t error) {
    String errorMsg = "Error[" + String(error) + "]: ";
    if (error == OTA_AUTH_ERROR) errorMsg += "Auth Failed";
    else if (error == OTA_BEGIN_ERROR) errorMsg += "Begin Failed";
    else if (error == OTA_CONNECT_ERROR) errorMsg += "Connect Failed";
    else if (error == OTA_RECEIVE_ERROR) errorMsg += "Receive Failed";
    else if (error == OTA_END_ERROR) errorMsg += "End Failed";
    serialPrintln(errorMsg);
  });
  
  ArduinoOTA.begin();
  serialPrintln("OTA ready");
}

void setupIR() {
  serialPrintln("Setting up IR receiver...");
  IrReceiver.begin(IR_RECEIVE_PIN, ENABLE_LED_FEEDBACK);
  serialPrintln("IR receiver ready");
}

void setupEncoder() {
  serialPrintln("Setting up rotary encoder...");
  encoder.setHandleRotate([](int8_t rotation) {
    if (millis() - lastEncoderAction > DEBOUNCE_DELAY) {
      int newInput = state.currentInput + rotation;
      if (newInput < 1) newInput = 4;
      if (newInput > 4) newInput = 1;
      handleInputSelection(newInput);
      lastEncoderAction = millis();
    }
  });
  
  encoder.setHandlePressRotate([](int8_t rotation) {
    // Optional: Handle press+rotate for volume or other functions
  });
  
  encoder.setHandlePress([]() {
    handlePowerToggle();
  });
  
  encoder.setHandleLongPress([]() {
    handleMuteToggle();
  });
  
  serialPrintln("Encoder ready");
}

void loadSettings() {
  serialPrintln("Loading settings...");
  prefs.begin("muffsy", false);
  
  state.currentInput = prefs.getInt("input", 1);
  state.ssrEnabled = prefs.getBool("ssr", false);
  state.wifiSSID = prefs.getString("wifi_ssid", "");
  state.wifiPassword = prefs.getString("wifi_pass", "");
  
  serialPrintln("Settings loaded");
}

void saveSettings() {
  prefs.putInt("input", state.currentInput);
  prefs.putBool("ssr", state.ssrEnabled);
  prefs.putString("wifi_ssid", state.wifiSSID);
  prefs.putString("wifi_pass", state.wifiPassword);
}

// ================================
// CONSOLE FUNCTIONS
// ================================
void addToConsole(String message) {
  // Add timestamp
  unsigned long now = millis();
  unsigned long seconds = now / 1000;
  unsigned long minutes = seconds / 60;
  unsigned long hours = minutes / 60;
  
  String timestamp = String(hours % 24) + ":" + 
                    String((minutes % 60) < 10 ? "0" : "") + String(minutes % 60) + ":" +
                    String((seconds % 60) < 10 ? "0" : "") + String(seconds % 60);
  
  String logEntry = "[" + timestamp + "] " + message;
  
  consoleBuffer[consoleIndex] = logEntry;
  consoleIndex = (consoleIndex + 1) % CONSOLE_BUFFER_SIZE;
  
  if (consoleIndex == 0) {
    consoleWrapped = true;
  }
}

void serialPrintln(String message) {
  Serial.println(message);    // Still output to serial
  addToConsole(message);      // Also add to web console buffer
}

// ================================
// CONTROL FUNCTIONS
// ================================
void handleInputSelection(int input) {
  if (input >= 1 && input <= 4 && state.powered) {
    state.currentInput = input;
    serialPrintln("Input selected: " + String(input));
    updateRelays();
    updateLEDs();
    saveSettings();
  }
}

void handlePowerToggle() {
  state.powered = !state.powered;
  serialPrintln("Power: " + String(state.powered ? "ON" : "OFF"));
  
  if (!state.powered) {
    state.muted = true; // Mute when powering off
  }
  
  updateRelays();
  updateLEDs();
}

void handleMuteToggle() {
  if (state.powered) {
    state.muted = !state.muted;
    serialPrintln("Mute: " + String(state.muted ? "ON" : "OFF"));
    updateRelays();
    updateLEDs();
  }
}

void updateLEDs() {
  digitalWrite(POWER_LED, state.powered ? HIGH : LOW);
  digitalWrite(MUTE_LED, (state.powered && state.muted) ? HIGH : LOW);
  
  if (state.powered && !state.muted) {
    digitalWrite(INPUT_A_LED, (state.currentInput == 1) ? HIGH : LOW);
    digitalWrite(INPUT_B_LED, (state.currentInput == 2) ? HIGH : LOW);
    digitalWrite(INPUT_C_LED, (state.currentInput == 3) ? HIGH : LOW);
    digitalWrite(INPUT_D_LED, (state.currentInput == 4) ? HIGH : LOW);
  } else {
    digitalWrite(INPUT_A_LED, LOW);
    digitalWrite(INPUT_B_LED, LOW);
    digitalWrite(INPUT_C_LED, LOW);
    digitalWrite(INPUT_D_LED, LOW);
  }
}

void updateRelays() {
  // Mute relay (inverted logic - LOW = muted)
  digitalWrite(RELAY_MUTE, (state.powered && !state.muted) ? LOW : HIGH);
  
  // SSR control (if enabled)
  digitalWrite(SSR_PIN, (state.powered && state.ssrEnabled) ? HIGH : LOW);
  
  if (state.powered && !state.muted) {
    // Activate selected input relay
    digitalWrite(RELAY_INPUT_A, (state.currentInput == 1) ? HIGH : LOW);
    digitalWrite(RELAY_INPUT_B, (state.currentInput == 2) ? HIGH : LOW);
    digitalWrite(RELAY_INPUT_C, (state.currentInput == 3) ? HIGH : LOW);
    digitalWrite(RELAY_INPUT_D, (state.currentInput == 4) ? HIGH : LOW);
  } else {
    // Turn off all input relays
    digitalWrite(RELAY_INPUT_A, LOW);
    digitalWrite(RELAY_INPUT_B, LOW);
    digitalWrite(RELAY_INPUT_C, LOW);
    digitalWrite(RELAY_INPUT_D, LOW);
  }
}

// ================================
// PROCESSING FUNCTIONS
// ================================
void processIR() {
  if (IrReceiver.decode()) {
    if (millis() - lastIRCommand > DEBOUNCE_DELAY) {
      uint32_t command = IrReceiver.decodedIRData.command;
      serialPrintln("IR received: 0x" + String(command, HEX));
      
      switch (command) {
        case IR_POWER:
          handlePowerToggle();
          break;
        case IR_MUTE:
          handleMuteToggle();
          break;
        case IR_INPUT_1:
          handleInputSelection(1);
          break;
        case IR_INPUT_2:
          handleInputSelection(2);
          break;
        case IR_INPUT_3:
          handleInputSelection(3);
          break;
        case IR_INPUT_4:
          handleInputSelection(4);
          break;
        case IR_UP:
          {
            int newInput = state.currentInput + 1;
            if (newInput > 4) newInput = 1;
            handleInputSelection(newInput);
          }
          break;
        case IR_DOWN:
          {
            int newInput = state.currentInput - 1;
            if (newInput < 1) newInput = 4;
            handleInputSelection(newInput);
          }
          break;
      }
      lastIRCommand = millis();
    }
    IrReceiver.resume();
  }
}

void processEncoder() {
  encoder.ReadEncoder();
}

void processWebRequests() {
  if (state.apMode) {
    dnsServer.processNextRequest();
  }
  server.handleClient();
}

// ================================
// WEB INTERFACE HANDLERS
// ================================
void handleRoot() {
  String html = R"(
<!DOCTYPE html>
<html>
<head>
    <title>Muffsy Input Selector</title>
    <meta name="viewport" content="width=device-width, initial-scale=1">
    <style>
        body { font-family: Arial; margin: 20px; background: #f0f0f0; }
        .container { max-width: 600px; margin: 0 auto; background: white; padding: 20px; border-radius: 10px; }
        .status { background: #e7f3ff; padding: 15px; border-radius: 5px; margin-bottom: 20px; }
        .controls { display: grid; grid-template-columns: repeat(auto-fit, minmax(120px, 1fr)); gap: 10px; margin-bottom: 20px; }
        button { padding: 15px; border: none; border-radius: 5px; cursor: pointer; font-size: 16px; }
        .btn-primary { background: #007bff; color: white; }
        .btn-success { background: #28a745; color: white; }
        .btn-warning { background: #ffc107; color: black; }
        .btn-danger { background: #dc3545; color: white; }
        .btn-active { background: #17a2b8; color: white; }
        .section { margin-bottom: 30px; }
        h2 { color: #333; border-bottom: 2px solid #007bff; padding-bottom: 10px; }
        .wifi-setup { background: #fff3cd; padding: 15px; border-radius: 5px; margin-bottom: 20px; }
        input[type="text"], input[type="password"] { width: 100%; padding: 10px; margin: 5px 0; border: 1px solid #ddd; border-radius: 3px; }
    </style>
</head>
<body>
    <div class="container">
        <h1>🎵 Muffsy Input Selector</h1>
        <div class="status">
            <strong>Status:</strong> <span id="power">Loading...</span><br>
            <strong>Input:</strong> <span id="input">Loading...</span><br>
            <strong>Muted:</strong> <span id="mute">Loading...</span><br>
            <strong>WiFi:</strong> <span id="wifi">Loading...</span>
        </div>
        
        <div class="section">
            <h2>Controls</h2>
            <div class="controls">
                <button class="btn-primary" onclick="sendCommand('power')">Power</button>
                <button class="btn-warning" onclick="sendCommand('mute')">Mute</button>
                <button class="btn-success" onclick="sendCommand('input', 1)">Input 1</button>
                <button class="btn-success" onclick="sendCommand('input', 2)">Input 2</button>
                <button class="btn-success" onclick="sendCommand('input', 3)">Input 3</button>
                <button class="btn-success" onclick="sendCommand('input', 4)">Input 4</button>
            </div>
        </div>
)";

  if (state.apMode) {
    html += R"(
        <div class="section">
            <h2>WiFi Setup</h2>
            <div class="wifi-setup">
                <p>Connect to your WiFi network for remote access and OTA updates.</p>
                <form action="/connect" method="post">
                    <input type="text" name="ssid" placeholder="WiFi Network Name" required>
                    <input type="password" name="password" placeholder="WiFi Password">
                    <button type="submit" class="btn-primary">Connect</button>
                </form>
            </div>
        </div>
)";
  }

  html += R"(
        <div class="section">
            <h2>Console</h2>
            <p><a href="/console" target="_blank" class="btn-primary" style="text-decoration: none; display: inline-block; padding: 10px 15px; border-radius: 5px;">Open Console</a></p>
        </div>
        
        <div class="section">
            <h2>Firmware Update</h2>
            <form method="POST" action="/update" enctype="multipart/form-data">
                <input type="file" name="update" accept=".bin">
                <button type="submit" class="btn-danger">Upload Firmware</button>
            </form>
        </div>
    </div>

    <script>
        function updateStatus() {
            fetch('/status')
                .then(response => response.json())
                .then(data => {
                    document.getElementById('power').textContent = data.powered ? 'ON' : 'OFF';
                    document.getElementById('input').textContent = 'Input ' + data.currentInput;
                    document.getElementById('mute').textContent = data.muted ? 'YES' : 'NO';
                    document.getElementById('wifi').textContent = data.wifiConnected ? 'Connected' : 'Not Connected';
                });
        }
        
        function sendCommand(action, value = null) {
            const data = new FormData();
            data.append('action', action);
            if (value !== null) data.append('value', value);
            
            fetch('/control', {
                method: 'POST',
                body: data
            }).then(() => updateStatus());
        }
        
        updateStatus();
        setInterval(updateStatus, 2000);
    </script>
</body>
</html>
)";

  server.send(200, "text/html", html);
}

void handleWiFiSetup() {
  // Scan for networks and return JSON
  WiFi.scanNetworks();
  // Implementation for network scanning
  server.send(200, "application/json", "[]");
}

void handleWiFiConnect() {
  if (server.hasArg("ssid")) {
    state.wifiSSID = server.arg("ssid");
    state.wifiPassword = server.arg("password");
    
    saveSettings();
    
    server.send(200, "text/html", 
      "<html><body><h2>Connecting...</h2><p>Device will restart and connect to your network.</p></body></html>");
    
    delay(1000);
    ESP.restart();
  } else {
    server.send(400, "text/plain", "Missing SSID");
  }
}

void handleStatus() {
  String json = "{";
  json += "\"powered\":" + String(state.powered ? "true" : "false") + ",";
  json += "\"muted\":" + String(state.muted ? "true" : "false") + ",";
  json += "\"currentInput\":" + String(state.currentInput) + ",";
  json += "\"wifiConnected\":" + String(state.wifiConnected ? "true" : "false") + ",";
  json += "\"apMode\":" + String(state.apMode ? "true" : "false") + ",";
  json += "\"firmware\":\"" + String(FIRMWARE_VERSION) + "\"";
  json += "}";
  
  server.send(200, "application/json", json);
}

void handleControl() {
  if (server.hasArg("action")) {
    String action = server.arg("action");
    
    if (action == "power") {
      handlePowerToggle();
    } else if (action == "mute") {
      handleMuteToggle();
    } else if (action == "input" && server.hasArg("value")) {
      int input = server.arg("value").toInt();
      handleInputSelection(input);
    }
    
    server.send(200, "text/plain", "OK");
  } else {
    server.send(400, "text/plain", "Missing action");
  }
}

void handleConsole() {
  String html = R"(
<!DOCTYPE html>
<html>
<head>
    <title>Muffsy Console</title>
    <meta name="viewport" content="width=device-width, initial-scale=1">
    <style>
        body { 
            font-family: 'Courier New', monospace; 
            margin: 0; 
            padding: 20px; 
            background: #1e1e1e; 
            color: #00ff00;
        }
        .console-container { 
            max-width: 1000px; 
            margin: 0 auto; 
            background: #000; 
            border: 2px solid #333; 
            border-radius: 10px; 
            padding: 20px;
        }
        .console-header {
            color: #00ffff;
            text-align: center;
            margin-bottom: 20px;
            border-bottom: 1px solid #333;
            padding-bottom: 10px;
        }
        .console-output {
            height: 500px;
            overflow-y: auto;
            background: #000;
            border: 1px solid #333;
            padding: 10px;
            font-size: 14px;
            line-height: 1.4;
        }
        .console-line {
            margin-bottom: 2px;
            white-space: pre-wrap;
            word-wrap: break-word;
        }
        .timestamp {
            color: #888;
        }
        .controls {
            margin-top: 10px;
            text-align: center;
        }
        button {
            background: #333;
            color: #00ff00;
            border: 1px solid #666;
            padding: 10px 20px;
            margin: 0 10px;
            border-radius: 5px;
            cursor: pointer;
            font-family: inherit;
        }
        button:hover {
            background: #555;
        }
        .status-bar {
            margin-top: 10px;
            padding: 10px;
            background: #222;
            border-radius: 5px;
            color: #888;
            text-align: center;
        }
    </style>
</head>
<body>
    <div class="console-container">
        <div class="console-header">
            <h1>🎵 Muffsy Input Selector - Console</h1>
        </div>
        
        <div class="console-output" id="console">
            <div class="console-line">Loading console...</div>
        </div>
        
        <div class="controls">
            <button onclick="toggleAutoRefresh()">Auto Refresh: <span id="refresh-status">ON</span></button>
            <button onclick="clearConsole()">Clear</button>
            <button onclick="refreshConsole()">Refresh Now</button>
            <button onclick="window.close()">Close</button>
        </div>
        
        <div class="status-bar">
            Last updated: <span id="last-update">Never</span> | 
            Lines: <span id="line-count">0</span> |
            Auto-refresh: <span id="refresh-interval">2s</span>
        </div>
    </div>

    <script>
        let autoRefresh = true;
        let refreshTimer;
        
        function updateConsole() {
            fetch('/console-data')
                .then(response => response.json())
                .then(data => {
                    const console = document.getElementById('console');
                    console.innerHTML = '';
                    
                    data.lines.forEach(line => {
                        const div = document.createElement('div');
                        div.className = 'console-line';
                        div.textContent = line;
                        console.appendChild(div);
                    });
                    
                    // Auto-scroll to bottom
                    console.scrollTop = console.scrollHeight;
                    
                    // Update status
                    document.getElementById('last-update').textContent = new Date().toLocaleTimeString();
                    document.getElementById('line-count').textContent = data.lines.length;
                })
                .catch(error => {
                    console.error('Failed to fetch console data:', error);
                });
        }
        
        function toggleAutoRefresh() {
            autoRefresh = !autoRefresh;
            document.getElementById('refresh-status').textContent = autoRefresh ? 'ON' : 'OFF';
            
            if (autoRefresh) {
                startAutoRefresh();
            } else {
                clearInterval(refreshTimer);
            }
        }
        
        function startAutoRefresh() {
            refreshTimer = setInterval(updateConsole, 2000);
        }
        
        function clearConsole() {
            fetch('/console-data?clear=1')
                .then(() => updateConsole());
        }
        
        function refreshConsole() {
            updateConsole();
        }
        
        // Start auto-refresh
        updateConsole();
        startAutoRefresh();
    </script>
</body>
</html>
)";

  server.send(200, "text/html", html);
}

void handleConsoleData() {
  // Handle clear request
  if (server.hasArg("clear")) {
    consoleIndex = 0;
    consoleWrapped = false;
    for (int i = 0; i < CONSOLE_BUFFER_SIZE; i++) {
      consoleBuffer[i] = "";
    }
    serialPrintln("Console cleared via web interface");
  }
  
  // Build JSON response with console lines
  String json = "{\"lines\":[";
  
  int lineCount = 0;
  if (consoleWrapped) {
    // If wrapped, start from current index (oldest) and go around
    for (int i = 0; i < CONSOLE_BUFFER_SIZE; i++) {
      int idx = (consoleIndex + i) % CONSOLE_BUFFER_SIZE;
      if (consoleBuffer[idx].length() > 0) {
        if (lineCount > 0) json += ",";
        json += "\"" + consoleBuffer[idx] + "\"";
        lineCount++;
      }
    }
  } else {
    // If not wrapped, just go from 0 to current index
    for (int i = 0; i < consoleIndex; i++) {
      if (consoleBuffer[i].length() > 0) {
        if (lineCount > 0) json += ",";
        json += "\"" + consoleBuffer[i] + "\"";
        lineCount++;
      }
    }
  }
  
  json += "],\"count\":" + String(lineCount) + "}";
  
  server.send(200, "application/json", json);
}

void handleOTAUpload() {
  server.send(200, "text/html", 
    "<form method='POST' action='/update' enctype='multipart/form-data'>"
    "<input type='file' name='update'>"
    "<input type='submit' value='Update'>"
    "</form>");
}

void handleNotFound() {
  if (state.apMode) {
    // Captive portal redirect
    server.sendHeader("Location", "/", true);
    server.send(302, "text/plain", "");
  } else {
    server.send(404, "text/plain", "Not found");
  }
}
