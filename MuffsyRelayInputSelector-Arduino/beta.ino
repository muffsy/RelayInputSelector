/*
 * BETA-version: UNTESTED!
 *
 * Muffsy Relay Input Selector - Enhanced Version
 * 
 * Features:
 * - WiFi AP setup with web interface
 * - OTA updates
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

// Web interface handlers
void handleRoot();
void handleWiFiSetup();
void handleWiFiConnect();
void handleStatus();
void handleControl();
void handleOTAUpload();
void handleNotFound();

// ================================
// MAIN FUNCTIONS
// ================================
void setup() {
  Serial.begin(115200);
  delay(1000);
  
  Serial.println("\n" + String(FIRMWARE_VERSION));
  Serial.println("Muffsy Relay Input Selector - Enhanced");
  Serial.println("=====================================");
  
  setupHardware();
  loadSettings();
  setupWiFi();
  setupWebServer();
  setupOTA();
  setupIR();
  setupEncoder();
  
  // Startup sequence - device starts OFF and MUTED for safety
  Serial.println("Startup delay: " + String(STARTUP_DELAY) + "ms (safety mute period)");
  delay(STARTUP_DELAY);
  
  // Device remains off and muted until user explicitly turns it on
  // This ensures no audio signals pass through during startup
  updateLEDs();
  updateRelays();
  
  setupComplete = true;
  Serial.println("Setup complete - ready for operation");
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
        Serial.println("WiFi disconnected, attempting reconnection...");
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
  Serial.println("Initializing hardware...");
  
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
  Serial.println("Hardware initialized");
}

void setupWiFi() {
  Serial.println("Setting up WiFi...");
  
  if (state.wifiSSID.length() > 0) {
    // Try to connect to saved network
    Serial.println("Attempting to connect to saved network: " + state.wifiSSID);
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
      Serial.println("\nConnected to WiFi!");
      Serial.println("IP address: " + WiFi.localIP().toString());
      return;
    }
  }
  
  // Start AP mode if no saved network or connection failed
  Serial.println("Starting Access Point mode...");
  WiFi.mode(WIFI_AP);
  WiFi.softAP(AP_SSID, AP_PASSWORD);
  
  // Setup captive portal
  dnsServer.start(53, "*", WiFi.softAPIP());
  
  state.apMode = true;
  state.wifiConnected = false;
  Serial.println("AP started");
  Serial.println("SSID: " + String(AP_SSID));
  Serial.println("IP: " + WiFi.softAPIP().toString());
}

void setupWebServer() {
  Serial.println("Setting up web server...");
  
  server.on("/", handleRoot);
  server.on("/wifi", handleWiFiSetup);
  server.on("/connect", HTTP_POST, handleWiFiConnect);
  server.on("/status", handleStatus);
  server.on("/control", HTTP_POST, handleControl);
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
  Serial.println("Web server started");
}

void setupOTA() {
  Serial.println("Setting up OTA...");
  
  ArduinoOTA.setHostname("muffsy-input-selector");
  ArduinoOTA.setPassword("muffsy");
  
  ArduinoOTA.onStart([]() {
    String type = (ArduinoOTA.getCommand() == U_FLASH) ? "sketch" : "filesystem";
    Serial.println("Start updating " + type);
  });
  
  ArduinoOTA.onEnd([]() {
    Serial.println("\nEnd");
  });
  
  ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
    Serial.printf("Progress: %u%%\r", (progress / (total / 100)));
  });
  
  ArduinoOTA.onError([](ota_error_t error) {
    Serial.printf("Error[%u]: ", error);
    if (error == OTA_AUTH_ERROR) Serial.println("Auth Failed");
    else if (error == OTA_BEGIN_ERROR) Serial.println("Begin Failed");
    else if (error == OTA_CONNECT_ERROR) Serial.println("Connect Failed");
    else if (error == OTA_RECEIVE_ERROR) Serial.println("Receive Failed");
    else if (error == OTA_END_ERROR) Serial.println("End Failed");
  });
  
  ArduinoOTA.begin();
  Serial.println("OTA ready");
}

void setupIR() {
  Serial.println("Setting up IR receiver...");
  IrReceiver.begin(IR_RECEIVE_PIN, ENABLE_LED_FEEDBACK);
  Serial.println("IR receiver ready");
}

void setupEncoder() {
  Serial.println("Setting up rotary encoder...");
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
  
  Serial.println("Encoder ready");
}

void loadSettings() {
  Serial.println("Loading settings...");
  prefs.begin("muffsy", false);
  
  state.currentInput = prefs.getInt("input", 1);
  state.ssrEnabled = prefs.getBool("ssr", false);
  state.wifiSSID = prefs.getString("wifi_ssid", "");
  state.wifiPassword = prefs.getString("wifi_pass", "");
  
  Serial.println("Settings loaded");
}

void saveSettings() {
  prefs.putInt("input", state.currentInput);
  prefs.putBool("ssr", state.ssrEnabled);
  prefs.putString("wifi_ssid", state.wifiSSID);
  prefs.putString("wifi_pass", state.wifiPassword);
}

// ================================
// CONTROL FUNCTIONS
// ================================
void handleInputSelection(int input) {
  if (input >= 1 && input <= 4 && state.powered) {
    state.currentInput = input;
    Serial.println("Input selected: " + String(input));
    updateRelays();
    updateLEDs();
    saveSettings();
  }
}

void handlePowerToggle() {
  state.powered = !state.powered;
  Serial.println("Power: " + String(state.powered ? "ON" : "OFF"));
  
  if (!state.powered) {
    state.muted = true; // Mute when powering off
  }
  
  updateRelays();
  updateLEDs();
}

void handleMuteToggle() {
  if (state.powered) {
    state.muted = !state.muted;
    Serial.println("Mute: " + String(state.muted ? "ON" : "OFF"));
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
      Serial.println("IR received: 0x" + String(command, HEX));
      
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
