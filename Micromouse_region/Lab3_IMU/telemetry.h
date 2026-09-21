// ═══════════════════════════════════════════════════════════════════════════
//  telemetry.h
//  WiFi Telemetry with Web Dashboard
//  
//  Layer: 6 (Communication)
//  Dependencies: config.h, WiFi.h, ESPAsyncWebServer
//  
//  Features:
//    - WiFi Access Point mode (ไม่ต้องมี router)
//    - WebSocket real-time bidirectional communication
//    - Embedded Web Dashboard with charts
//    - JSON data format
//    - Configurable update rate
//    - Enable/Disable at runtime
//  
//  Usage:
//    1. Connect to WiFi: "Micromouse_XXXX"
//    2. Open browser: http://192.168.4.1
//    3. View real-time data and send commands
//  
//  Required Libraries (install via Arduino Library Manager):
//    - ESPAsyncWebServer
//    - AsyncTCP
// ═══════════════════════════════════════════════════════════════════════════
#ifndef TELEMETRY_H
#define TELEMETRY_H

#include <Arduino.h>
#include "config.h"

#if TELEMETRY_WIFI_ENABLED

#include <WiFi.h>
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>

// ┌───────────────────────────────────────────────────────────────────────────
// │ SECTION 1: DATA TYPES
// └───────────────────────────────────────────────────────────────────────────

/**
 * @brief Telemetry data packet (ข้อมูลที่ส่งไป Dashboard)
 */
typedef struct {
    // Timestamp
    uint32_t timestamp_ms;
    
    // Encoder data
    int32_t encoder_left;
    int32_t encoder_right;
    float velocity_left;
    float velocity_right;
    float distance_left;
    float distance_right;
    
    // Motor data
    int16_t motor_pwm_left;
    int16_t motor_pwm_right;
    
    // IMU data (placeholder)
    float gyro_z;
    float accel_x;
    float accel_y;
    
    // Wall sensors (placeholder)
    uint16_t wall_front_left;
    uint16_t wall_front_right;
    uint16_t wall_left;
    uint16_t wall_right;
    
    // Battery
    uint16_t battery_mv;
    
    // System
    uint32_t loop_time_us;
    uint32_t free_heap;
    
    // Custom values (สำหรับ debug อะไรก็ได้)
    float custom1;
    float custom2;
    float custom3;
    float custom4;
} TelemetryData_t;

/**
 * @brief Command received from Dashboard
 */
typedef struct {
    char command[32];
    float value1;
    float value2;
    float value3;
    float value4;
} TelemetryCommand_t;

/**
 * @brief Callback function type for commands
 */
typedef void (*TelemetryCommandCallback_t)(TelemetryCommand_t cmd);

// ┌───────────────────────────────────────────────────────────────────────────
// │ SECTION 2: STATE VARIABLES (C++17 inline)
// └───────────────────────────────────────────────────────────────────────────

inline AsyncWebServer _telemetry_server(WEBSOCKET_PORT);
inline AsyncWebSocket _telemetry_ws("/ws");

inline TelemetryData_t _telemetry_data = {0};
inline TelemetryCommand_t _telemetry_last_cmd = {0};
inline TelemetryCommandCallback_t _telemetry_cmd_callback = nullptr;

inline bool _telemetry_enabled = false;
inline bool _telemetry_connected = false;
inline uint32_t _telemetry_last_send = 0;
inline uint32_t _telemetry_send_interval_ms = 1000 / TELEMETRY_UPDATE_RATE_HZ;
inline char _telemetry_ssid[32] = {0};
inline char _telemetry_json_buffer[TELEMETRY_BUFFER_SIZE];

// ┌───────────────────────────────────────────────────────────────────────────
// │ SECTION 3: WEB DASHBOARD HTML (Embedded)
// └───────────────────────────────────────────────────────────────────────────

inline const char TELEMETRY_HTML[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html>
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>Micromouse Telemetry</title>
    <style>
        * { box-sizing: border-box; margin: 0; padding: 0; }
        body { 
            font-family: -apple-system, BlinkMacSystemFont, 'Segoe UI', Roboto, sans-serif;
            background: #1a1a2e; color: #eee; padding: 10px;
        }
        .header { 
            text-align: center; padding: 10px; 
            background: linear-gradient(135deg, #667eea 0%, #764ba2 100%);
            border-radius: 10px; margin-bottom: 10px;
        }
        .header h1 { font-size: 1.5em; }
        .status { 
            display: inline-block; padding: 3px 10px; border-radius: 15px;
            font-size: 0.8em; margin-top: 5px;
        }
        .connected { background: #2ecc71; }
        .disconnected { background: #e74c3c; }
        
        .grid { display: grid; grid-template-columns: repeat(auto-fit, minmax(280px, 1fr)); gap: 10px; }
        .card {
            background: #16213e; border-radius: 10px; padding: 15px;
            border: 1px solid #0f3460;
        }
        .card h3 { 
            color: #667eea; font-size: 0.9em; margin-bottom: 10px;
            border-bottom: 1px solid #0f3460; padding-bottom: 5px;
        }
        .row { display: flex; justify-content: space-between; margin: 5px 0; }
        .label { color: #888; font-size: 0.85em; }
        .value { font-family: 'Courier New', monospace; font-weight: bold; color: #4ecca3; }
        .value.negative { color: #ff6b6b; }
        
        .bar-container { 
            background: #0f3460; border-radius: 5px; height: 20px; 
            margin: 5px 0; overflow: hidden; position: relative;
        }
        .bar { 
            height: 100%; transition: width 0.1s; border-radius: 5px;
            background: linear-gradient(90deg, #4ecca3, #667eea);
        }
        .bar.negative { background: linear-gradient(90deg, #ff6b6b, #ee5a24); }
        .bar-label {
            position: absolute; right: 5px; top: 50%; transform: translateY(-50%);
            font-size: 0.75em; color: #fff;
        }
        
        .controls { display: flex; flex-wrap: wrap; gap: 5px; margin-top: 10px; }
        .btn {
            flex: 1; min-width: 80px; padding: 10px; border: none; border-radius: 5px;
            font-weight: bold; cursor: pointer; transition: all 0.2s;
        }
        .btn:active { transform: scale(0.95); }
        .btn-green { background: #2ecc71; color: #fff; }
        .btn-red { background: #e74c3c; color: #fff; }
        .btn-blue { background: #3498db; color: #fff; }
        .btn-yellow { background: #f39c12; color: #fff; }
        .btn-purple { background: #9b59b6; color: #fff; }
        
        .slider-container { margin: 10px 0; }
        .slider-container label { font-size: 0.85em; color: #888; }
        .slider { width: 100%; margin: 5px 0; }
        
        canvas { width: 100%; height: 150px; background: #0f3460; border-radius: 5px; }
        
        .log { 
            background: #0a0a0a; border-radius: 5px; padding: 10px;
            font-family: monospace; font-size: 0.75em; height: 100px;
            overflow-y: auto; color: #4ecca3;
        }
        
        @media (max-width: 600px) {
            .grid { grid-template-columns: 1fr; }
        }
    </style>
</head>
<body>
    <div class="header">
        <h1>🐭 Micromouse Telemetry</h1>
        <span id="status" class="status disconnected">Disconnected</span>
    </div>
    
    <div class="grid">
        <!-- Encoder Card -->
        <div class="card">
            <h3>📊 Encoders</h3>
            <div class="row"><span class="label">Left Count</span><span id="encL" class="value">0</span></div>
            <div class="row"><span class="label">Right Count</span><span id="encR" class="value">0</span></div>
            <div class="row"><span class="label">Left Velocity</span><span id="velL" class="value">0 mm/s</span></div>
            <div class="row"><span class="label">Right Velocity</span><span id="velR" class="value">0 mm/s</span></div>
            <div class="row"><span class="label">Left Distance</span><span id="distL" class="value">0 mm</span></div>
            <div class="row"><span class="label">Right Distance</span><span id="distR" class="value">0 mm</span></div>
        </div>
        
        <!-- Motor Card -->
        <div class="card">
            <h3>⚡ Motors</h3>
            <div class="row"><span class="label">Left PWM</span><span id="pwmL" class="value">0</span></div>
            <div class="bar-container">
                <div id="barL" class="bar" style="width:50%"></div>
                <span id="barLabelL" class="bar-label">0%</span>
            </div>
            <div class="row"><span class="label">Right PWM</span><span id="pwmR" class="value">0</span></div>
            <div class="bar-container">
                <div id="barR" class="bar" style="width:50%"></div>
                <span id="barLabelR" class="bar-label">0%</span>
            </div>
            <div class="slider-container">
                <label>Test Speed: <span id="speedVal">0</span></label>
                <input type="range" id="speedSlider" class="slider" min="-100" max="100" value="0">
            </div>
        </div>
        
        <!-- Control Card -->
        <div class="card">
            <h3>🎮 Controls</h3>
            <div class="controls">
                <button class="btn btn-green" onclick="sendCmd('forward')">▲ Forward</button>
                <button class="btn btn-red" onclick="sendCmd('stop')">■ Stop</button>
            </div>
            <div class="controls">
                <button class="btn btn-blue" onclick="sendCmd('left')">◄ Left</button>
                <button class="btn btn-yellow" onclick="sendCmd('backward')">▼ Back</button>
                <button class="btn btn-blue" onclick="sendCmd('right')">► Right</button>
            </div>
            <div class="controls">
                <button class="btn btn-purple" onclick="sendCmd('reset')">↺ Reset</button>
                <button class="btn btn-purple" onclick="sendCmd('calibrate')">⚙ Calibrate</button>
            </div>
        </div>
        
        <!-- System Card -->
        <div class="card">
            <h3>💻 System</h3>
            <div class="row"><span class="label">Battery</span><span id="battery" class="value">0 mV</span></div>
            <div class="row"><span class="label">Loop Time</span><span id="loopTime" class="value">0 µs</span></div>
            <div class="row"><span class="label">Free Heap</span><span id="heap" class="value">0 KB</span></div>
            <div class="row"><span class="label">Update Rate</span><span id="updateRate" class="value">0 Hz</span></div>
        </div>
        
        <!-- Custom Values Card -->
        <div class="card">
            <h3>📈 Custom Values</h3>
            <div class="row"><span class="label">Custom 1</span><span id="custom1" class="value">0</span></div>
            <div class="row"><span class="label">Custom 2</span><span id="custom2" class="value">0</span></div>
            <div class="row"><span class="label">Custom 3</span><span id="custom3" class="value">0</span></div>
            <div class="row"><span class="label">Custom 4</span><span id="custom4" class="value">0</span></div>
        </div>
        
        <!-- Log Card -->
        <div class="card">
            <h3>📝 Log</h3>
            <div id="log" class="log"></div>
        </div>
    </div>
    
    <script>
        let ws;
        let lastUpdate = Date.now();
        let updateCount = 0;
        
        function connect() {
            ws = new WebSocket('ws://' + window.location.hostname + '/ws');
            
            ws.onopen = function() {
                document.getElementById('status').textContent = 'Connected';
                document.getElementById('status').className = 'status connected';
                log('WebSocket connected');
            };
            
            ws.onclose = function() {
                document.getElementById('status').textContent = 'Disconnected';
                document.getElementById('status').className = 'status disconnected';
                log('WebSocket disconnected, reconnecting...');
                setTimeout(connect, 2000);
            };
            
            ws.onmessage = function(event) {
                try {
                    const data = JSON.parse(event.data);
                    updateUI(data);
                    updateCount++;
                } catch (e) {
                    log('Parse error: ' + e);
                }
            };
            
            ws.onerror = function(error) {
                log('WebSocket error');
            };
        }
        
        function updateUI(d) {
            // Encoders
            setText('encL', d.eL || 0);
            setText('encR', d.eR || 0);
            setText('velL', (d.vL || 0).toFixed(1) + ' mm/s');
            setText('velR', (d.vR || 0).toFixed(1) + ' mm/s');
            setText('distL', (d.dL || 0).toFixed(1) + ' mm');
            setText('distR', (d.dR || 0).toFixed(1) + ' mm');
            
            // Motors
            setText('pwmL', d.pL || 0);
            setText('pwmR', d.pR || 0);
            updateBar('barL', 'barLabelL', d.pL || 0, 1023);
            updateBar('barR', 'barLabelR', d.pR || 0, 1023);
            
            // System
            setText('battery', (d.bat || 0) + ' mV');
            setText('loopTime', (d.lt || 0) + ' µs');
            setText('heap', ((d.hp || 0) / 1024).toFixed(1) + ' KB');
            
            // Custom
            setText('custom1', (d.c1 || 0).toFixed(2));
            setText('custom2', (d.c2 || 0).toFixed(2));
            setText('custom3', (d.c3 || 0).toFixed(2));
            setText('custom4', (d.c4 || 0).toFixed(2));
            
            // Update rate calculation
            const now = Date.now();
            if (now - lastUpdate >= 1000) {
                setText('updateRate', updateCount + ' Hz');
                updateCount = 0;
                lastUpdate = now;
            }
        }
        
        function setText(id, value) {
            const el = document.getElementById(id);
            if (el) el.textContent = value;
        }
        
        function updateBar(barId, labelId, value, max) {
            const bar = document.getElementById(barId);
            const label = document.getElementById(labelId);
            const percent = Math.min(Math.abs(value) / max * 100, 100);
            const isNeg = value < 0;
            
            bar.style.width = (50 + (isNeg ? -percent/2 : percent/2)) + '%';
            bar.style.marginLeft = isNeg ? (50 - percent/2) + '%' : '50%';
            bar.className = 'bar' + (isNeg ? ' negative' : '');
            label.textContent = (isNeg ? '-' : '') + percent.toFixed(0) + '%';
        }
        
        function sendCmd(cmd, v1, v2, v3, v4) {
            if (ws && ws.readyState === WebSocket.OPEN) {
                const msg = JSON.stringify({cmd: cmd, v1: v1||0, v2: v2||0, v3: v3||0, v4: v4||0});
                ws.send(msg);
                log('Sent: ' + cmd);
            }
        }
        
        function log(msg) {
            const logEl = document.getElementById('log');
            const time = new Date().toLocaleTimeString();
            logEl.innerHTML = '[' + time + '] ' + msg + '<br>' + logEl.innerHTML;
            if (logEl.children.length > 50) logEl.lastChild.remove();
        }
        
        // Slider control
        document.getElementById('speedSlider').addEventListener('input', function(e) {
            const val = parseInt(e.target.value);
            document.getElementById('speedVal').textContent = val;
            sendCmd('speed', val);
        });
        
        document.getElementById('speedSlider').addEventListener('change', function(e) {
            if (parseInt(e.target.value) === 0) {
                sendCmd('stop');
            }
        });
        
        // Connect on load
        connect();
    </script>
</body>
</html>
)rawliteral";

// ┌───────────────────────────────────────────────────────────────────────────
// │ SECTION 4: PRIVATE HELPER FUNCTIONS
// └───────────────────────────────────────────────────────────────────────────

/**
 * @brief Extract float value from JSON string
 * @note ต้องประกาศก่อน _telemetry_onWsEvent ที่เรียกใช้
 */
inline float _extractJsonValue(const String& json, const char* key) {
    String search = String("\"") + key + "\":";
    int start = json.indexOf(search);
    if (start < 0) return 0;
    start += search.length();
    int end = start;
    while (end < json.length() && (isdigit(json[end]) || json[end] == '.' || json[end] == '-')) {
        end++;
    }
    return json.substring(start, end).toFloat();
}

/**
 * @brief Handle WebSocket events
 */
inline void _telemetry_onWsEvent(AsyncWebSocket *server, AsyncWebSocketClient *client,
                                  AwsEventType type, void *arg, uint8_t *data, size_t len) {
    switch (type) {
        case WS_EVT_CONNECT:
            _telemetry_connected = true;
            #if DEBUG_SERIAL
            Serial.printf("[TELEMETRY] Client #%u connected from %s\n", 
                          client->id(), client->remoteIP().toString().c_str());
            #endif
            break;
            
        case WS_EVT_DISCONNECT:
            _telemetry_connected = (_telemetry_ws.count() > 0);
            #if DEBUG_SERIAL
            Serial.printf("[TELEMETRY] Client #%u disconnected\n", client->id());
            #endif
            break;
            
        case WS_EVT_DATA: {
            AwsFrameInfo *info = (AwsFrameInfo*)arg;
            if (info->final && info->index == 0 && info->len == len && info->opcode == WS_TEXT) {
                // Parse JSON command
                data[len] = 0;  // Null terminate
                
                // Simple JSON parsing (ไม่ใช้ library เพื่อประหยัด memory)
                String json = String((char*)data);
                
                // Extract command
                int cmdStart = json.indexOf("\"cmd\":\"") + 7;
                int cmdEnd = json.indexOf("\"", cmdStart);
                if (cmdStart > 6 && cmdEnd > cmdStart) {
                    String cmd = json.substring(cmdStart, cmdEnd);
                    cmd.toCharArray(_telemetry_last_cmd.command, 32);
                    
                    // Extract values
                    _telemetry_last_cmd.value1 = _extractJsonValue(json, "v1");
                    _telemetry_last_cmd.value2 = _extractJsonValue(json, "v2");
                    _telemetry_last_cmd.value3 = _extractJsonValue(json, "v3");
                    _telemetry_last_cmd.value4 = _extractJsonValue(json, "v4");
                    
                    #if DEBUG_SERIAL
                    Serial.printf("[TELEMETRY] CMD: %s (%.1f, %.1f, %.1f, %.1f)\n",
                                  _telemetry_last_cmd.command,
                                  _telemetry_last_cmd.value1,
                                  _telemetry_last_cmd.value2,
                                  _telemetry_last_cmd.value3,
                                  _telemetry_last_cmd.value4);
                    #endif
                    
                    // Call callback if registered
                    if (_telemetry_cmd_callback != nullptr) {
                        _telemetry_cmd_callback(_telemetry_last_cmd);
                    }
                }
            }
            break;
        }
        
        case WS_EVT_PONG:
        case WS_EVT_ERROR:
            break;
    }
}

/**
 * @brief Build JSON string from telemetry data
 */
inline void _telemetry_buildJson(void) {
    // Compact JSON format to minimize bandwidth
    snprintf(_telemetry_json_buffer, TELEMETRY_BUFFER_SIZE,
        "{\"t\":%lu,\"eL\":%ld,\"eR\":%ld,\"vL\":%.1f,\"vR\":%.1f,"
        "\"dL\":%.1f,\"dR\":%.1f,\"pL\":%d,\"pR\":%d,"
        "\"gz\":%.2f,\"ax\":%.2f,\"ay\":%.2f,"
        "\"wFL\":%u,\"wFR\":%u,\"wL\":%u,\"wR\":%u,"
        "\"bat\":%u,\"lt\":%lu,\"hp\":%lu,"
        "\"c1\":%.2f,\"c2\":%.2f,\"c3\":%.2f,\"c4\":%.2f}",
        _telemetry_data.timestamp_ms,
        _telemetry_data.encoder_left,
        _telemetry_data.encoder_right,
        _telemetry_data.velocity_left,
        _telemetry_data.velocity_right,
        _telemetry_data.distance_left,
        _telemetry_data.distance_right,
        _telemetry_data.motor_pwm_left,
        _telemetry_data.motor_pwm_right,
        _telemetry_data.gyro_z,
        _telemetry_data.accel_x,
        _telemetry_data.accel_y,
        _telemetry_data.wall_front_left,
        _telemetry_data.wall_front_right,
        _telemetry_data.wall_left,
        _telemetry_data.wall_right,
        _telemetry_data.battery_mv,
        _telemetry_data.loop_time_us,
        _telemetry_data.free_heap,
        _telemetry_data.custom1,
        _telemetry_data.custom2,
        _telemetry_data.custom3,
        _telemetry_data.custom4
    );
}

// ┌───────────────────────────────────────────────────────────────────────────
// │ SECTION 5: PUBLIC FUNCTIONS - Initialization
// └───────────────────────────────────────────────────────────────────────────

/**
 * @brief Initialize WiFi telemetry
 * @return true if successful
 * 
 * @note Creates WiFi Access Point and starts WebSocket server.
 *       Connect to WiFi and open http://192.168.4.1 in browser.
 */
inline bool telemetry_init(void) {
    #if DEBUG_SERIAL
    Serial.println("[TELEMETRY] Initializing...");
    #endif
    
    // Create unique SSID using MAC address
    uint8_t mac[6];
    WiFi.macAddress(mac);
    snprintf(_telemetry_ssid, sizeof(_telemetry_ssid), "%s_%02X%02X", 
             WIFI_AP_SSID, mac[4], mac[5]);
    
    // Start Access Point
    WiFi.mode(WIFI_AP);
    
    bool result;
    if (strlen(WIFI_AP_PASSWORD) >= 8) {
        result = WiFi.softAP(_telemetry_ssid, WIFI_AP_PASSWORD, 
                             WIFI_AP_CHANNEL, false, WIFI_AP_MAX_CONNECTIONS);
    } else {
        result = WiFi.softAP(_telemetry_ssid, NULL, 
                             WIFI_AP_CHANNEL, false, WIFI_AP_MAX_CONNECTIONS);
    }
    
    if (!result) {
        #if DEBUG_SERIAL
        Serial.println("[TELEMETRY] Failed to start AP!");
        #endif
        return false;
    }
    
    #if DEBUG_SERIAL
    Serial.println("[TELEMETRY] ════════════════════════════════════════");
    Serial.printf("[TELEMETRY] WiFi AP: %s\n", _telemetry_ssid);
    if (strlen(WIFI_AP_PASSWORD) >= 8) {
        Serial.printf("[TELEMETRY] Password: %s\n", WIFI_AP_PASSWORD);
    } else {
        Serial.println("[TELEMETRY] Password: (open)");
    }
    Serial.printf("[TELEMETRY] IP: %s\n", WiFi.softAPIP().toString().c_str());
    Serial.printf("[TELEMETRY] Dashboard: http://%s\n", WiFi.softAPIP().toString().c_str());
    Serial.println("[TELEMETRY] ════════════════════════════════════════");
    #endif
    
    // Setup WebSocket
    _telemetry_ws.onEvent(_telemetry_onWsEvent);
    _telemetry_server.addHandler(&_telemetry_ws);
    
    // Serve dashboard HTML
    _telemetry_server.on("/", HTTP_GET, [](AsyncWebServerRequest *request) {
        request->send(200, "text/html", TELEMETRY_HTML);
    });
    
    // Start server
    _telemetry_server.begin();
    _telemetry_enabled = true;
    
    #if DEBUG_SERIAL
    Serial.println("[TELEMETRY] Server started");
    #endif
    
    return true;
}

// ┌───────────────────────────────────────────────────────────────────────────
// │ SECTION 6: PUBLIC FUNCTIONS - Data Update
// └───────────────────────────────────────────────────────────────────────────

/**
 * @brief Update telemetry data and send to connected clients
 * 
 * @note Call this in main loop. Data is sent at TELEMETRY_UPDATE_RATE_HZ.
 */
inline void telemetry_update(void) {
    if (!_telemetry_enabled) return;
    
    // Cleanup disconnected clients
    _telemetry_ws.cleanupClients();
    
    // Check if it's time to send
    uint32_t now = millis();
    if (now - _telemetry_last_send < _telemetry_send_interval_ms) {
        return;
    }
    _telemetry_last_send = now;
    
    // Update system data
    _telemetry_data.timestamp_ms = now;
    _telemetry_data.free_heap = ESP.getFreeHeap();
    
    // Send to all connected clients
    if (_telemetry_ws.count() > 0) {
        _telemetry_buildJson();
        _telemetry_ws.textAll(_telemetry_json_buffer);
    }
}

/**
 * @brief Set encoder data
 */
inline void telemetry_set_encoder(int32_t left, int32_t right, 
                                   float vel_left, float vel_right,
                                   float dist_left, float dist_right) {
    _telemetry_data.encoder_left = left;
    _telemetry_data.encoder_right = right;
    _telemetry_data.velocity_left = vel_left;
    _telemetry_data.velocity_right = vel_right;
    _telemetry_data.distance_left = dist_left;
    _telemetry_data.distance_right = dist_right;
}

/**
 * @brief Set motor data
 */
inline void telemetry_set_motor(int16_t pwm_left, int16_t pwm_right) {
    _telemetry_data.motor_pwm_left = pwm_left;
    _telemetry_data.motor_pwm_right = pwm_right;
}

/**
 * @brief Set IMU data
 */
inline void telemetry_set_imu(float gyro_z, float accel_x, float accel_y) {
    _telemetry_data.gyro_z = gyro_z;
    _telemetry_data.accel_x = accel_x;
    _telemetry_data.accel_y = accel_y;
}

/**
 * @brief Set wall sensor data
 */
inline void telemetry_set_wall_sensors(uint16_t front_left, uint16_t front_right,
                                        uint16_t left, uint16_t right) {
    _telemetry_data.wall_front_left = front_left;
    _telemetry_data.wall_front_right = front_right;
    _telemetry_data.wall_left = left;
    _telemetry_data.wall_right = right;
}

/**
 * @brief Set battery voltage
 */
inline void telemetry_set_battery(uint16_t voltage_mv) {
    _telemetry_data.battery_mv = voltage_mv;
}

/**
 * @brief Set loop time
 */
inline void telemetry_set_loop_time(uint32_t time_us) {
    _telemetry_data.loop_time_us = time_us;
}

/**
 * @brief Set custom values (สำหรับ debug อะไรก็ได้)
 */
inline void telemetry_set_custom(float c1, float c2, float c3, float c4) {
    _telemetry_data.custom1 = c1;
    _telemetry_data.custom2 = c2;
    _telemetry_data.custom3 = c3;
    _telemetry_data.custom4 = c4;
}

// ┌───────────────────────────────────────────────────────────────────────────
// │ SECTION 7: PUBLIC FUNCTIONS - Command Handling
// └───────────────────────────────────────────────────────────────────────────

/**
 * @brief Register callback for commands from dashboard
 * @param callback Function to call when command received
 */
inline void telemetry_set_command_callback(TelemetryCommandCallback_t callback) {
    _telemetry_cmd_callback = callback;
}

/**
 * @brief Check if specific command was received
 * @param cmd Command name to check
 * @return true if this command was the last received
 */
inline bool telemetry_command_is(const char* cmd) {
    return strcmp(_telemetry_last_cmd.command, cmd) == 0;
}

/**
 * @brief Get last command
 */
inline TelemetryCommand_t telemetry_get_last_command(void) {
    return _telemetry_last_cmd;
}

/**
 * @brief Clear last command (call after handling)
 */
inline void telemetry_clear_command(void) {
    _telemetry_last_cmd.command[0] = '\0';
}

/**
 * @brief Check if command is pending (not yet cleared)
 */
inline bool telemetry_has_command(void) {
    return _telemetry_last_cmd.command[0] != '\0';
}

// ┌───────────────────────────────────────────────────────────────────────────
// │ SECTION 8: PUBLIC FUNCTIONS - Utility
// └───────────────────────────────────────────────────────────────────────────

/**
 * @brief Enable/Disable telemetry
 */
inline void telemetry_enable(bool enable) {
    _telemetry_enabled = enable;
}

/**
 * @brief Check if telemetry is enabled
 */
inline bool telemetry_is_enabled(void) {
    return _telemetry_enabled;
}

/**
 * @brief Check if any client is connected
 */
inline bool telemetry_is_connected(void) {
    return _telemetry_ws.count() > 0;
}

/**
 * @brief Get number of connected clients
 */
inline uint8_t telemetry_client_count(void) {
    return _telemetry_ws.count();
}

/**
 * @brief Set update rate
 * @param hz Update frequency in Hz (1-100)
 */
inline void telemetry_set_rate(uint8_t hz) {
    hz = constrain(hz, 1, 100);
    _telemetry_send_interval_ms = 1000 / hz;
}

/**
 * @brief Get WiFi SSID
 */
inline const char* telemetry_get_ssid(void) {
    return _telemetry_ssid;
}

/**
 * @brief Get IP address string
 */
inline String telemetry_get_ip(void) {
    return WiFi.softAPIP().toString();
}

/**
 * @brief Send log message to dashboard
 */
inline void telemetry_log(const char* message) {
    if (_telemetry_ws.count() > 0) {
        String json = "{\"log\":\"" + String(message) + "\"}";
        _telemetry_ws.textAll(json);
    }
}

/**
 * @brief Print connection info to Serial
 */
inline void telemetry_print_info(void) {
    #if DEBUG_SERIAL
    Serial.println();
    Serial.println("════════════════════════════════════════════════════════════");
    Serial.println("              TELEMETRY CONNECTION INFO");
    Serial.println("════════════════════════════════════════════════════════════");
    Serial.printf("  WiFi SSID:     %s\n", _telemetry_ssid);
    Serial.printf("  Password:      %s\n", strlen(WIFI_AP_PASSWORD) >= 8 ? WIFI_AP_PASSWORD : "(open)");
    Serial.printf("  IP Address:    %s\n", WiFi.softAPIP().toString().c_str());
    Serial.printf("  Dashboard:     http://%s\n", WiFi.softAPIP().toString().c_str());
    Serial.printf("  Update Rate:   %d Hz\n", 1000 / _telemetry_send_interval_ms);
    Serial.printf("  Clients:       %d\n", _telemetry_ws.count());
    Serial.println("════════════════════════════════════════════════════════════");
    Serial.println();
    #endif
}

#else // TELEMETRY_WIFI_ENABLED == 0

// ═══════════════════════════════════════════════════════════════════════════
// STUB FUNCTIONS (เมื่อปิด WiFi Telemetry)
// ═══════════════════════════════════════════════════════════════════════════

typedef struct { char command[32]; float value1, value2, value3, value4; } TelemetryCommand_t;
typedef void (*TelemetryCommandCallback_t)(TelemetryCommand_t cmd);

inline bool telemetry_init(void) { return false; }
inline void telemetry_update(void) {}
inline void telemetry_set_encoder(int32_t l, int32_t r, float vl, float vr, float dl, float dr) {}
inline void telemetry_set_motor(int16_t l, int16_t r) {}
inline void telemetry_set_imu(float gz, float ax, float ay) {}
inline void telemetry_set_wall_sensors(uint16_t fl, uint16_t fr, uint16_t l, uint16_t r) {}
inline void telemetry_set_battery(uint16_t v) {}
inline void telemetry_set_loop_time(uint32_t t) {}
inline void telemetry_set_custom(float c1, float c2, float c3, float c4) {}
inline void telemetry_set_command_callback(TelemetryCommandCallback_t cb) {}
inline bool telemetry_command_is(const char* cmd) { return false; }
inline TelemetryCommand_t telemetry_get_last_command(void) { return TelemetryCommand_t(); }
inline void telemetry_clear_command(void) {}
inline bool telemetry_has_command(void) { return false; }
inline void telemetry_enable(bool e) {}
inline bool telemetry_is_enabled(void) { return false; }
inline bool telemetry_is_connected(void) { return false; }
inline uint8_t telemetry_client_count(void) { return 0; }
inline void telemetry_set_rate(uint8_t hz) {}
inline const char* telemetry_get_ssid(void) { return ""; }
inline String telemetry_get_ip(void) { return ""; }
inline void telemetry_log(const char* msg) {}
inline void telemetry_print_info(void) {}

#endif // TELEMETRY_WIFI_ENABLED

#endif // TELEMETRY_H
