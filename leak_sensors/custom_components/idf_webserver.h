#pragma once
#include "esphome.h"
#include "config_storage.h"
#include <esp_http_server.h>
#include <esp_wifi.h>
#include <string>
#include <cstring>
#include <map>
#include <random>

class IDFWebServer {
 private:
  httpd_handle_t server = nullptr;
  ConfigStorage config_storage;
  ConfigStorage::Config current_config;
  
  // Session management
  std::map<std::string, time_t> active_sessions;  // session_id -> expiry_time
  static constexpr uint32_t SESSION_TIMEOUT = 3600;  // 1 hour
  time_t last_session_cleanup = 0;
  static constexpr uint32_t SESSION_CLEANUP_INTERVAL = 300;  // Cleanup every 5 minutes

  // Generate random session ID
  std::string generate_session_id() {
    static std::random_device rd;
    static std::mt19937 gen(rd());
    static std::uniform_int_distribution<> dis(0, 15);
    const char* hex = "0123456789abcdef";
    std::string id;
    for (int i = 0; i < 32; i++) {
      id += hex[dis(gen)];
    }
    return id;
  }

  // Cleanup expired sessions (prevent memory leaks)
  void cleanup_expired_sessions() {
    time_t now = ::time(nullptr);

    // Only cleanup every SESSION_CLEANUP_INTERVAL seconds
    if (now - last_session_cleanup < SESSION_CLEANUP_INTERVAL) {
      return;
    }

    last_session_cleanup = now;

    // Remove expired sessions
    auto it = active_sessions.begin();
    while (it != active_sessions.end()) {
      if (now > it->second) {
        ESP_LOGI("session", "Cleaning up expired session: %s", it->first.c_str());
        it = active_sessions.erase(it);
      } else {
        ++it;
      }
    }

    ESP_LOGI("session", "Active sessions: %d", active_sessions.size());
  }

  // Check if session is valid
  bool is_session_valid(const char* cookie_header) {
    if (!cookie_header) return false;

    // Periodic cleanup of expired sessions
    cleanup_expired_sessions();

    // Parse session=xxx from cookie
    const char* session_start = strstr(cookie_header, "session=");
    if (!session_start) return false;

    session_start += 8;  // Skip "session="
    const char* session_end = strchr(session_start, ';');
    std::string session_id;
    if (session_end) {
      session_id = std::string(session_start, session_end - session_start);
    } else {
      session_id = std::string(session_start);
    }

    // Check if session exists and not expired
    auto it = active_sessions.find(session_id);
    if (it == active_sessions.end()) return false;

    time_t now = ::time(nullptr);  // Use global time() function
    if (now > it->second) {
      active_sessions.erase(it);
      return false;
    }

    return true;
  }

  // URL decode helper
  std::string url_decode(const char* str) {
    std::string result;
    char ch;
    int i, ii;
    for (i=0; i<strlen(str); i++) {
      if (int(str[i])==37) {
        sscanf(str+i+1, "%2x", &ii);
        ch = static_cast<char>(ii);
        result += ch;
        i = i+2;
      } else if (str[i] == '+') {
        result += ' ';
      } else {
        result += str[i];
      }
    }
    return result;
  }

  // Parse POST body helper
  std::map<std::string, std::string> parse_post_body(const char* body) {
    std::map<std::string, std::string> params;
    std::string str(body);
    size_t pos = 0;
    
    while (pos < str.length()) {
      size_t eq_pos = str.find('=', pos);
      if (eq_pos == std::string::npos) break;
      
      size_t amp_pos = str.find('&', eq_pos);
      if (amp_pos == std::string::npos) amp_pos = str.length();
      
      std::string key = str.substr(pos, eq_pos - pos);
      std::string value = str.substr(eq_pos + 1, amp_pos - eq_pos - 1);
      params[key] = url_decode(value.c_str());
      
      pos = amp_pos + 1;
    }
    
    return params;
  }

  // JSON Response Builders (existing ones)
  std::string buildBinarySensorJSON() {
    std::string json = "[";
    bool water_leak_state = id(water_leak).state;
    bool status_state = id(device_status_sensor).state;
    
    json += "{\"id\":\"leak01-wasserleck_erkannt\",\"name\":\"Wasserleck erkannt\",\"value\":";
    json += water_leak_state ? "true" : "false";
    json += "},";
    
    json += "{\"id\":\"leak01-status\",\"name\":\"Status\",\"value\":";
    json += status_state ? "true" : "false";
    json += "}";
    
    json += "]";
    return json;
  }

  std::string buildSensorJSON() {
    char buffer[512];
    std::string json = "[";
    
    if (id(wifi_signal_sensor).has_state()) {
      snprintf(buffer, sizeof(buffer), 
        "{\"id\":\"leak01-wifi_signal\",\"name\":\"WiFi Signal\",\"value\":%d,\"unit\":\"dBm\"},",
        (int)id(wifi_signal_sensor).state);
      json += buffer;
    }
    
    if (id(uptime_sensor).has_state()) {
      snprintf(buffer, sizeof(buffer),
        "{\"id\":\"leak01-uptime\",\"name\":\"Uptime\",\"value\":%d,\"unit\":\"s\"}",
        (int)id(uptime_sensor).state);
      json += buffer;
    }
    
    json += "]";
    return json;
  }

  std::string buildTextSensorJSON() {
    char buffer[512];
    std::string json = "[";
    bool first = true;
    
    if (id(ip_address).has_state()) {
      snprintf(buffer, sizeof(buffer),
        "{\"id\":\"leak01-ip_adresse\",\"name\":\"IP Adresse\",\"value\":\"%s\"}",
        id(ip_address).state.c_str());
      json += buffer;
      first = false;
    }
    
    if (id(ssid).has_state()) {
      if (!first) json += ",";
      snprintf(buffer, sizeof(buffer),
        "{\"id\":\"leak01-ssid\",\"name\":\"SSID\",\"value\":\"%s\"}",
        id(ssid).state.c_str());
      json += buffer;
      first = false;
    }
    
    if (id(mac_address).has_state()) {
      if (!first) json += ",";
      snprintf(buffer, sizeof(buffer),
        "{\"id\":\"leak01-mac_adresse\",\"name\":\"MAC Adresse\",\"value\":\"%s\"}",
        id(mac_address).state.c_str());
      json += buffer;
    }
    
    json += "]";
    return json;
  }

  // WiFi Scan (Non-blocking approach with retry)
  std::string buildWiFiScanJSON() {
    char buffer[256];
    std::string json = "[";

    ESP_LOGI("wifi_scan", "Requesting WiFi scan results...");

    // Hole Scan-Ergebnisse direkt vom ESP-IDF
    // WICHTIG: Nicht delay() verwenden! Das blockiert den gesamten Server.
    // Stattdessen: Client triggert Scan, wartet client-seitig, ruft dann Ergebnisse ab
    uint16_t ap_count = 0;
    esp_wifi_scan_get_ap_num(&ap_count);
    ESP_LOGI("wifi_scan", "Found %d cached networks", ap_count);

    if (ap_count > 0) {
      // Begrenze auf max 20 APs
      if (ap_count > 20) ap_count = 20;

      wifi_ap_record_t ap_records[20];
      uint16_t actual_count = ap_count;

      esp_err_t err = esp_wifi_scan_get_ap_records(&actual_count, ap_records);

      if (err == ESP_OK) {
        for (int i = 0; i < actual_count; i++) {
          if (i > 0) json += ",";

          // Escape SSID für JSON (falls Sonderzeichen)
          std::string ssid_escaped;
          for (int j = 0; j < 33 && ap_records[i].ssid[j] != 0; j++) {
            char c = ap_records[i].ssid[j];
            if (c == '"' || c == '\\') {
              ssid_escaped += '\\';
            }
            ssid_escaped += c;
          }

          snprintf(buffer, sizeof(buffer),
            "{\"ssid\":\"%s\",\"rssi\":%d,\"secure\":%s}",
            ssid_escaped.c_str(),
            ap_records[i].rssi,
            (ap_records[i].authmode != WIFI_AUTH_OPEN) ? "true" : "false");
          json += buffer;
        }
      } else {
        ESP_LOGE("wifi_scan", "Failed to get AP records: %d", err);
      }
    } else {
      // Trigger neuen Scan für nächsten Request
      auto wifi_comp = wifi::global_wifi_component;
      if (wifi_comp != nullptr) {
        wifi_comp->start_scanning();
        ESP_LOGI("wifi_scan", "Triggered new scan for next request");
      }
    }

    json += "]";
    return json;
  }

  // HTTP Handlers

  static esp_err_t binary_sensor_handler(httpd_req_t *req) {
    IDFWebServer* instance = (IDFWebServer*)req->user_ctx;
    std::string json = instance->buildBinarySensorJSON();
    
    httpd_resp_set_type(req, "application/json");
    httpd_resp_set_hdr(req, "Access-Control-Allow-Origin", "*");
    httpd_resp_set_hdr(req, "Connection", "close");  // Schließe Verbindung nach Response
    httpd_resp_send(req, json.c_str(), json.length());
    return ESP_OK;
  }

  static esp_err_t sensor_handler(httpd_req_t *req) {
    IDFWebServer* instance = (IDFWebServer*)req->user_ctx;
    std::string json = instance->buildSensorJSON();
    
    httpd_resp_set_type(req, "application/json");
    httpd_resp_set_hdr(req, "Access-Control-Allow-Origin", "*");
    httpd_resp_set_hdr(req, "Connection", "close");
    httpd_resp_send(req, json.c_str(), json.length());
    return ESP_OK;
  }

  static esp_err_t text_sensor_handler(httpd_req_t *req) {
    IDFWebServer* instance = (IDFWebServer*)req->user_ctx;
    std::string json = instance->buildTextSensorJSON();
    
    httpd_resp_set_type(req, "application/json");
    httpd_resp_set_hdr(req, "Access-Control-Allow-Origin", "*");
    httpd_resp_set_hdr(req, "Connection", "close");
    httpd_resp_send(req, json.c_str(), json.length());
    return ESP_OK;
  }

  static esp_err_t root_handler(httpd_req_t *req) {
    IDFWebServer* instance = (IDFWebServer*)req->user_ctx;
    
    // Check if logged in
    char cookie[256] = {0};
    if (httpd_req_get_hdr_value_str(req, "Cookie", cookie, sizeof(cookie)) == ESP_OK) {
      if (instance->is_session_valid(cookie)) {
        // Show settings link if logged in
        const char* html = R"(<!DOCTYPE html><html><head><meta charset="UTF-8"><title>Leak Sensor Dashboard</title><style>
body{background:linear-gradient(135deg,#0f172a,#1e293b);color:#f1f5f9;font-family:Arial,sans-serif;margin:0;padding:20px}
.header{display:flex;justify-content:space-between;align-items:center;margin-bottom:20px}
.header h1{margin:0;font-size:24px}
.settings-btn{background:#3b82f6;color:#fff;padding:10px 20px;border:none;border-radius:5px;text-decoration:none;cursor:pointer}
.settings-btn:hover{background:#2563eb}
.leak-status{background:#334155;padding:40px;border-radius:15px;text-align:center;margin:20px 0;transition:all .3s}
.leak-status.dry{border:3px solid #10b981;background:rgba(16,185,129,0.1)}
.leak-status.wet{border:3px solid #ef4444;background:rgba(239,68,68,0.1);animation:pulse 2s infinite}
@keyframes pulse{0%,100%{opacity:1}50%{opacity:0.6}}
.leak-icon{font-size:80px;margin-bottom:15px}
.leak-icon.dry{color:#10b981}
.leak-icon.wet{color:#ef4444}
.leak-text{font-size:36px;font-weight:bold;margin:15px 0}
.leak-text.dry{color:#10b981}
.leak-text.wet{color:#ef4444}
.sensor-grid{display:grid;grid-template-columns:repeat(auto-fit,minmax(180px,1fr));gap:15px;margin:20px 0}
.sensor-card{background:#334155;padding:20px;border-radius:10px;text-align:center}
.sensor-value{font-size:32px;font-weight:bold;color:#60a5fa;margin:10px 0}
.sensor-label{font-size:13px;color:#94a3b8;text-transform:uppercase;letter-spacing:1px}
.info-card{background:#334155;padding:15px;border-radius:10px;margin:15px 0}
.info-row{display:flex;justify-content:space-between;padding:8px 0;border-bottom:1px solid #475569}
.info-row:last-child{border-bottom:none}
.info-label{color:#94a3b8;font-size:14px}
.info-value{color:#f1f5f9;font-weight:600}
.update-time{text-align:center;color:#64748b;font-size:12px;margin-top:15px}
</style></head><body>
<div class="header"><h1>Leak Sensor Dashboard</h1><a href="/config" class="settings-btn">⚙️ Einstellungen</a></div>
<div id="leak-status" class="leak-status dry"><div id="leak-icon" class="leak-icon dry">✓</div><div id="leak-text" class="leak-text dry">TROCKEN</div></div>
<div class="sensor-grid"><div class="sensor-card"><div id="wifi-value" class="sensor-value">--</div><div class="sensor-label">WiFi Signal (dBm)</div></div>
<div class="sensor-card"><div id="uptime-value" class="sensor-value">--</div><div class="sensor-label">Uptime</div></div></div>
<div class="info-card"><div class="info-row"><span class="info-label">IP Adresse</span><span id="ip-value" class="info-value">--</span></div>
<div class="info-row"><span class="info-label">SSID</span><span id="ssid-value" class="info-value">--</span></div>
<div class="info-row"><span class="info-label">MAC Adresse</span><span id="mac-value" class="info-value">--</span></div></div>
<div id="update-time" class="update-time">Lade Daten...</div>
<script>(function(){const REFRESH_INTERVAL=3000;async function updateDashboard(){try{const [binarySensors,sensors,textSensors]=await Promise.all([fetch('/binary_sensor').then(r=>r.json()),fetch('/sensor').then(r=>r.json()),fetch('/text_sensor').then(r=>r.json())]);const leakSensor=binarySensors.find(s=>s.id.includes('wasserleck'));if(leakSensor){const isWet=leakSensor.value;const statusEl=document.getElementById('leak-status');const iconEl=document.getElementById('leak-icon');const textEl=document.getElementById('leak-text');if(isWet){statusEl.className='leak-status wet';iconEl.className='leak-icon wet';iconEl.textContent='💧⚠️';textEl.className='leak-text wet';textEl.textContent='WASSERLECK!';}else{statusEl.className='leak-status dry';iconEl.className='leak-icon dry';iconEl.textContent='✓';textEl.className='leak-text dry';textEl.textContent='TROCKEN';}}const wifiSensor=sensors.find(s=>s.id.includes('wifi'));if(wifiSensor){document.getElementById('wifi-value').textContent=wifiSensor.value.toFixed(0);}const uptimeSensor=sensors.find(s=>s.id.includes('uptime'));if(uptimeSensor){const hours=Math.floor(uptimeSensor.value/3600);const minutes=Math.floor((uptimeSensor.value%3600)/60);document.getElementById('uptime-value').textContent=hours+'h '+minutes+'m';}const ipSensor=textSensors.find(s=>s.id.includes('ip'));if(ipSensor){document.getElementById('ip-value').textContent=ipSensor.value;}const ssidSensor=textSensors.find(s=>s.id.includes('ssid'));if(ssidSensor){document.getElementById('ssid-value').textContent=ssidSensor.value;}const macSensor=textSensors.find(s=>s.id.includes('mac'));if(macSensor){document.getElementById('mac-value').textContent=macSensor.value;}const now=new Date();document.getElementById('update-time').textContent='Letzte Aktualisierung: '+now.toLocaleTimeString();}catch(e){console.error('Fehler beim Laden der Daten:',e);}}updateDashboard();setInterval(updateDashboard,REFRESH_INTERVAL);})();</script></body></html>)";
        httpd_resp_set_type(req, "text/html");
        httpd_resp_send(req, html, strlen(html));
        return ESP_OK;
      }
    }
    
    // Not logged in - show dashboard without settings link
    const char* html = R"(<!DOCTYPE html><html><head><meta charset="UTF-8"><title>Leak Sensor Dashboard</title><style>
body{background:linear-gradient(135deg,#0f172a,#1e293b);color:#f1f5f9;font-family:Arial,sans-serif;margin:0;padding:20px}
.header{display:flex;justify-content:space-between;align-items:center;margin-bottom:20px}
.header h1{margin:0;font-size:24px}
.login-btn{background:#64748b;color:#fff;padding:10px 20px;border:none;border-radius:5px;text-decoration:none;cursor:pointer}
.login-btn:hover{background:#475569}
.leak-status{background:#334155;padding:40px;border-radius:15px;text-align:center;margin:20px 0;transition:all .3s}
.leak-status.dry{border:3px solid #10b981;background:rgba(16,185,129,0.1)}
.leak-status.wet{border:3px solid #ef4444;background:rgba(239,68,68,0.1);animation:pulse 2s infinite}
@keyframes pulse{0%,100%{opacity:1}50%{opacity:0.6}}
.leak-icon{font-size:80px;margin-bottom:15px}
.leak-icon.dry{color:#10b981}
.leak-icon.wet{color:#ef4444}
.leak-text{font-size:36px;font-weight:bold;margin:15px 0}
.leak-text.dry{color:#10b981}
.leak-text.wet{color:#ef4444}
.sensor-grid{display:grid;grid-template-columns:repeat(auto-fit,minmax(180px,1fr));gap:15px;margin:20px 0}
.sensor-card{background:#334155;padding:20px;border-radius:10px;text-align:center}
.sensor-value{font-size:32px;font-weight:bold;color:#60a5fa;margin:10px 0}
.sensor-label{font-size:13px;color:#94a3b8;text-transform:uppercase;letter-spacing:1px}
.info-card{background:#334155;padding:15px;border-radius:10px;margin:15px 0}
.info-row{display:flex;justify-content:space-between;padding:8px 0;border-bottom:1px solid #475569}
.info-row:last-child{border-bottom:none}
.info-label{color:#94a3b8;font-size:14px}
.info-value{color:#f1f5f9;font-weight:600}
.update-time{text-align:center;color:#64748b;font-size:12px;margin-top:15px}
</style></head><body>
<div class="header"><h1>Leak Sensor Dashboard</h1><a href="/login" class="login-btn">🔒 Login</a></div>
<div id="leak-status" class="leak-status dry"><div id="leak-icon" class="leak-icon dry">✓</div><div id="leak-text" class="leak-text dry">TROCKEN</div></div>
<div class="sensor-grid"><div class="sensor-card"><div id="wifi-value" class="sensor-value">--</div><div class="sensor-label">WiFi Signal (dBm)</div></div>
<div class="sensor-card"><div id="uptime-value" class="sensor-value">--</div><div class="sensor-label">Uptime</div></div></div>
<div class="info-card"><div class="info-row"><span class="info-label">IP Adresse</span><span id="ip-value" class="info-value">--</span></div>
<div class="info-row"><span class="info-label">SSID</span><span id="ssid-value" class="info-value">--</span></div>
<div class="info-row"><span class="info-label">MAC Adresse</span><span id="mac-value" class="info-value">--</span></div></div>
<div id="update-time" class="update-time">Lade Daten...</div>
<script>(function(){const REFRESH_INTERVAL=3000;async function updateDashboard(){try{const [binarySensors,sensors,textSensors]=await Promise.all([fetch('/binary_sensor').then(r=>r.json()),fetch('/sensor').then(r=>r.json()),fetch('/text_sensor').then(r=>r.json())]);const leakSensor=binarySensors.find(s=>s.id.includes('wasserleck'));if(leakSensor){const isWet=leakSensor.value;const statusEl=document.getElementById('leak-status');const iconEl=document.getElementById('leak-icon');const textEl=document.getElementById('leak-text');if(isWet){statusEl.className='leak-status wet';iconEl.className='leak-icon wet';iconEl.textContent='💧⚠️';textEl.className='leak-text wet';textEl.textContent='WASSERLECK!';}else{statusEl.className='leak-status dry';iconEl.className='leak-icon dry';iconEl.textContent='✓';textEl.className='leak-text dry';textEl.textContent='TROCKEN';}}const wifiSensor=sensors.find(s=>s.id.includes('wifi'));if(wifiSensor){document.getElementById('wifi-value').textContent=wifiSensor.value.toFixed(0);}const uptimeSensor=sensors.find(s=>s.id.includes('uptime'));if(uptimeSensor){const hours=Math.floor(uptimeSensor.value/3600);const minutes=Math.floor((uptimeSensor.value%3600)/60);document.getElementById('uptime-value').textContent=hours+'h '+minutes+'m';}const ipSensor=textSensors.find(s=>s.id.includes('ip'));if(ipSensor){document.getElementById('ip-value').textContent=ipSensor.value;}const ssidSensor=textSensors.find(s=>s.id.includes('ssid'));if(ssidSensor){document.getElementById('ssid-value').textContent=ssidSensor.value;}const macSensor=textSensors.find(s=>s.id.includes('mac'));if(macSensor){document.getElementById('mac-value').textContent=macSensor.value;}const now=new Date();document.getElementById('update-time').textContent='Letzte Aktualisierung: '+now.toLocaleTimeString();}catch(e){console.error('Fehler beim Laden der Daten:',e);}}updateDashboard();setInterval(updateDashboard,REFRESH_INTERVAL);})();</script></body></html>)";
    
    httpd_resp_set_type(req, "text/html");
    httpd_resp_send(req, html, strlen(html));
    return ESP_OK;
  }

  static esp_err_t login_get_handler(httpd_req_t *req) {
    const char* html = R"(<!DOCTYPE html><html><head><meta charset="UTF-8"><title>Login</title><style>
body{background:linear-gradient(135deg,#0f172a,#1e293b);color:#f1f5f9;font-family:Arial,sans-serif;margin:0;padding:20px;display:flex;justify-content:center;align-items:center;min-height:100vh}
.login-box{background:#334155;padding:40px;border-radius:15px;width:100%;max-width:400px}
.login-box h1{margin-top:0;text-align:center}
.form-group{margin-bottom:20px}
.form-group label{display:block;margin-bottom:8px;color:#94a3b8}
.form-group input{width:100%;padding:12px;border:1px solid #475569;border-radius:5px;background:#1e293b;color:#f1f5f9;font-size:16px;box-sizing:border-box}
.btn{width:100%;padding:12px;background:#3b82f6;color:#fff;border:none;border-radius:5px;font-size:16px;cursor:pointer}
.btn:hover{background:#2563eb}
.error{background:#ef4444;color:#fff;padding:12px;border-radius:5px;margin-bottom:20px;display:none}
</style></head><body>
<div class="login-box"><h1>🔒 Login</h1><div class="error" id="error"></div>
<form id="loginForm"><div class="form-group"><label>Benutzername</label><input type="text" name="username" required></div>
<div class="form-group"><label>Passwort</label><input type="password" name="password" required></div>
<button type="submit" class="btn">Anmelden</button></form></div>
<script>document.getElementById('loginForm').addEventListener('submit',async(e)=>{e.preventDefault();const form=e.target;const formData=new FormData(form);try{const response=await fetch('/api/login',{method:'POST',body:new URLSearchParams(formData)});const result=await response.json();if(result.success){window.location.href='/config';}else{document.getElementById('error').textContent=result.message||'Login fehlgeschlagen';document.getElementById('error').style.display='block';}}catch(err){document.getElementById('error').textContent='Verbindungsfehler';document.getElementById('error').style.display='block';}});</script></body></html>)";
    
    httpd_resp_set_type(req, "text/html");
    httpd_resp_send(req, html, strlen(html));
    return ESP_OK;
  }

  static esp_err_t login_post_handler(httpd_req_t *req) {
    IDFWebServer* instance = (IDFWebServer*)req->user_ctx;
    
    char content[256];
    int ret = httpd_req_recv(req, content, sizeof(content) - 1);
    if (ret <= 0) {
      httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "Bad Request");
      return ESP_FAIL;
    }
    content[ret] = '\0';
    
    auto params = instance->parse_post_body(content);
    std::string username = params["username"];
    std::string password = params["password"];
    
    // Check credentials
    if (username == instance->current_config.admin_username &&
        password == instance->current_config.admin_password) {
      // Create session
      std::string session_id = instance->generate_session_id();
      instance->active_sessions[session_id] = ::time(nullptr) + SESSION_TIMEOUT;
      
      // Set cookie
      std::string cookie = "session=" + session_id + "; Path=/; HttpOnly; Max-Age=3600";
      httpd_resp_set_hdr(req, "Set-Cookie", cookie.c_str());
      
      httpd_resp_set_type(req, "application/json");
      httpd_resp_send(req, "{\"success\":true}", HTTPD_RESP_USE_STRLEN);
      return ESP_OK;
    }
    
    httpd_resp_set_type(req, "application/json");
    httpd_resp_send(req, "{\"success\":false,\"message\":\"Ung\\u00fcltige Anmeldedaten\"}", HTTPD_RESP_USE_STRLEN);
    return ESP_OK;
  }

  static esp_err_t logout_handler(httpd_req_t *req) {
    // Clear session cookie
    httpd_resp_set_hdr(req, "Set-Cookie", "session=; Path=/; HttpOnly; Max-Age=0");
    httpd_resp_set_hdr(req, "Location", "/");
    httpd_resp_set_status(req, "302 Found");
    httpd_resp_send(req, nullptr, 0);
    return ESP_OK;
  }

  static esp_err_t config_menu_handler(httpd_req_t *req) {
    IDFWebServer* instance = (IDFWebServer*)req->user_ctx;
    
    // Check auth
    char cookie[256] = {0};
    if (httpd_req_get_hdr_value_str(req, "Cookie", cookie, sizeof(cookie)) != ESP_OK || 
        !instance->is_session_valid(cookie)) {
      httpd_resp_set_hdr(req, "Location", "/login");
      httpd_resp_set_status(req, "302 Found");
      httpd_resp_send(req, nullptr, 0);
      return ESP_OK;
    }
    
    const char* html = R"(<!DOCTYPE html><html><head><meta charset="UTF-8"><title>Einstellungen</title><style>
body{background:linear-gradient(135deg,#0f172a,#1e293b);color:#f1f5f9;font-family:Arial,sans-serif;margin:0;padding:20px}
.header{display:flex;justify-content:space-between;align-items:center;margin-bottom:30px}
.header h1{margin:0}
.logout-btn{background:#ef4444;color:#fff;padding:10px 20px;border:none;border-radius:5px;text-decoration:none;cursor:pointer}
.logout-btn:hover{background:#dc2626}
.menu-grid{display:grid;grid-template-columns:repeat(auto-fit,minmax(250px,1fr));gap:20px;margin-top:30px}
.menu-card{background:#334155;padding:30px;border-radius:15px;text-align:center;cursor:pointer;transition:all .3s;text-decoration:none;color:#f1f5f9}
.menu-card:hover{background:#3b82f6;transform:translateY(-5px)}
.menu-icon{font-size:48px;margin-bottom:15px}
.menu-title{font-size:20px;font-weight:bold;margin-bottom:10px}
.menu-desc{font-size:14px;color:#94a3b8}
</style></head><body>
<div class="header"><h1>⚙️ Einstellungen</h1><div><a href="/" style="margin-right:10px;background:#64748b;color:#fff;padding:10px 20px;border:none;border-radius:5px;text-decoration:none">🏠 Dashboard</a><a href="/api/logout" class="logout-btn">🚪 Logout</a></div></div>
<div class="menu-grid">
<a href="/config/wifi" class="menu-card"><div class="menu-icon">📡</div><div class="menu-title">WiFi</div><div class="menu-desc">Netzwerk konfigurieren</div></a>
<a href="/config/ntp" class="menu-card"><div class="menu-icon">🕐</div><div class="menu-title">NTP / Zeit</div><div class="menu-desc">Zeitserver einstellen</div></a>
<a href="/config/system" class="menu-card"><div class="menu-icon">🔧</div><div class="menu-title">System</div><div class="menu-desc">Gerät & Sensoren</div></a>
</div></body></html>)";
    
    httpd_resp_set_type(req, "text/html");
    httpd_resp_send(req, html, strlen(html));
    return ESP_OK;
  }

  // WiFi Config Page Handler wird im nächsten Teil fortgesetzt...
  static esp_err_t config_wifi_handler(httpd_req_t *req) {
    IDFWebServer* instance = (IDFWebServer*)req->user_ctx;
    
    // Check auth
    char cookie[256] = {0};
    if (httpd_req_get_hdr_value_str(req, "Cookie", cookie, sizeof(cookie)) != ESP_OK || 
        !instance->is_session_valid(cookie)) {
      httpd_resp_set_hdr(req, "Location", "/login");
      httpd_resp_set_status(req, "302 Found");
      httpd_resp_send(req, nullptr, 0);
      return ESP_OK;
    }
    
    // Lade aktuelle SSID
    std::string current_ssid = instance->current_config.wifi_ssid;
    
    std::string html = R"(<!DOCTYPE html><html><head><meta charset="UTF-8"><title>WiFi Konfiguration</title><style>
body{background:linear-gradient(135deg,#0f172a,#1e293b);color:#f1f5f9;font-family:Arial,sans-serif;margin:0;padding:20px}
.header{display:flex;justify-content:space-between;align-items:center;margin-bottom:30px}
.back-btn{background:#64748b;color:#fff;padding:10px 20px;border:none;border-radius:5px;text-decoration:none}
.back-btn:hover{background:#475569}
.container{max-width:800px;margin:0 auto}
.card{background:#334155;padding:30px;border-radius:15px;margin-bottom:20px}
.card h2{margin-top:0}
.form-group{margin-bottom:20px}
.form-group label{display:block;margin-bottom:8px;color:#94a3b8}
.form-group input,.form-group select{width:100%;padding:12px;border:1px solid #475569;border-radius:5px;background:#1e293b;color:#f1f5f9;font-size:16px;box-sizing:border-box}
.btn{padding:12px 24px;background:#3b82f6;color:#fff;border:none;border-radius:5px;font-size:16px;cursor:pointer}
.btn:hover{background:#2563eb}
.btn-scan{background:#10b981}
.btn-scan:hover{background:#059669}
.network-list{margin-top:20px}
.network-item{background:#1e293b;padding:15px;border-radius:5px;margin-bottom:10px;cursor:pointer;display:flex;justify-content:space-between;align-items:center}
.network-item:hover{background:#475569}
.network-item.selected{border:2px solid #3b82f6}
.alert{background:#3b82f6;color:#fff;padding:15px;border-radius:5px;margin-bottom:20px}
</style></head><body>
<div class="header"><h1>📡 WiFi Konfiguration</h1><a href="/config" class="back-btn">← Zurück</a></div>
<div class="container">
<div class="alert">⚠️ Nach Änderungen ist ein Neustart erforderlich!</div>
<div class="card">
<h2>Aktuelles Netzwerk</h2>
<p><strong>SSID:</strong> )";
    html += current_ssid.empty() ? "Nicht konfiguriert" : current_ssid;
    html += "</p><button class=\"btn btn-scan\" onclick=\"scanWiFi()\">Netzwerke scannen</button>";
    html += "<div id=\"networkList\" class=\"network-list\"></div></div>";
    html += "<div class=\"card\"><h2>Manuelle Konfiguration</h2><form id=\"wifiForm\">";
    html += "<div class=\"form-group\"><label>SSID</label><input type=\"text\" id=\"ssid\" name=\"ssid\" required></div>";
    html += "<div class=\"form-group\"><label>Passwort</label><input type=\"password\" id=\"password\" name=\"password\" required></div>";
    html += "<button type=\"submit\" class=\"btn\">Speichern & Neustart</button></form></div></div>";
    html += "<script>";
    html += "let selectedSSID=\"\";";
    html += "async function scanWiFi(){";
    html += "document.getElementById(\"networkList\").innerHTML=\"<p>Scanne...</p>\";";
    html += "try{";
    html += "const response=await fetch(\"/api/wifi/scan\");";
    html += "const networks=await response.json();";
    html += "let html=\"\";";
    html += "networks.forEach(net=>{";
    html += "html+=\"<div class=\\\"network-item\\\" onclick=\\\"selectNetwork(\\\"\"+net.ssid+\"\\\")\\\"><div><strong>\"+net.ssid+\"</strong><br><small>\"+net.rssi+\" dBm \"+(net.secure?\"Secure\":\"\")+\"</small></div></div>\";";
    html += "});";
    html += "document.getElementById(\"networkList\").innerHTML=html;";
    html += "}catch(e){alert(\"Scan fehlgeschlagen\");}";
    html += "}";
    html += "function selectNetwork(ssid){";
    html += "selectedSSID=ssid;";
    html += "document.getElementById(\"ssid\").value=ssid;";
    html += "document.querySelectorAll(\".network-item\").forEach(item=>{";
    html += "item.classList.remove(\"selected\");";
    html += "if(item.textContent.includes(ssid))item.classList.add(\"selected\");";
    html += "});";
    html += "}";
    html += "document.getElementById(\"wifiForm\").addEventListener(\"submit\",async(e)=>{";
    html += "e.preventDefault();";
    html += "const formData=new FormData(e.target);";
    html += "if(!confirm(\"ESP32 wird neu gestartet. Fortfahren?\"))return;";
    html += "try{";
    html += "await fetch(\"/api/wifi/save\",{method:\"POST\",body:new URLSearchParams(formData)});";
    html += "alert(\"Gespeichert! ESP32 wird neu gestartet...\");";
    html += "setTimeout(()=>window.location.href=\"/\",5000);";
    html += "}catch(err){alert(\"Fehler beim Speichern\");}";
    html += "});";
    html += "</script></body></html>";
    
    httpd_resp_set_type(req, "text/html");
    httpd_resp_send(req, html.c_str(), html.length());
    return ESP_OK;
  }

  // NTP Config Handler
  static esp_err_t config_ntp_handler(httpd_req_t *req) {
    IDFWebServer* instance = (IDFWebServer*)req->user_ctx;
    
    char cookie[256] = {0};
    if (httpd_req_get_hdr_value_str(req, "Cookie", cookie, sizeof(cookie)) != ESP_OK || 
        !instance->is_session_valid(cookie)) {
      httpd_resp_set_hdr(req, "Location", "/login");
      httpd_resp_set_status(req, "302 Found");
      httpd_resp_send(req, nullptr, 0);
      return ESP_OK;
    }
    
    std::string html = "<!DOCTYPE html><html><head><meta charset=\"UTF-8\"><title>NTP Konfiguration</title>";
    html += "<style>body{background:linear-gradient(135deg,#0f172a,#1e293b);color:#f1f5f9;font-family:Arial,sans-serif;margin:0;padding:20px}";
    html += ".header{display:flex;justify-content:space-between;align-items:center;margin-bottom:30px}";
    html += ".back-btn{background:#64748b;color:#fff;padding:10px 20px;border:none;border-radius:5px;text-decoration:none}";
    html += ".container{max-width:800px;margin:0 auto}.card{background:#334155;padding:30px;border-radius:15px;margin-bottom:20px}";
    html += ".form-group{margin-bottom:20px}.form-group label{display:block;margin-bottom:8px;color:#94a3b8}";
    html += ".form-group input,.form-group select{width:100%;padding:12px;border:1px solid #475569;border-radius:5px;background:#1e293b;color:#f1f5f9;font-size:16px;box-sizing:border-box}";
    html += ".btn{padding:12px 24px;background:#3b82f6;color:#fff;border:none;border-radius:5px;font-size:16px;cursor:pointer}</style></head><body>";
    html += "<div class=\"header\"><h1>🕐 NTP Konfiguration</h1><a href=\"/config\" class=\"back-btn\">← Zurück</a></div>";
    html += "<div class=\"container\"><div class=\"card\"><h2>Zeitserver</h2><form id=\"ntpForm\">";
    html += "<div class=\"form-group\"><label>Primärer NTP-Server</label><input type=\"text\" name=\"ntp_primary\" value=\"";
    html += instance->current_config.ntp_primary;
    html += "\" required></div><div class=\"form-group\"><label>Sekundärer NTP-Server</label><input type=\"text\" name=\"ntp_secondary\" value=\"";
    html += instance->current_config.ntp_secondary;
    html += "\" required></div><div class=\"form-group\"><label>Zeitzone</label><select name=\"timezone\">";
    html += "<option value=\"CET-1CEST,M3.5.0,M10.5.0/3\">Europa/Berlin (CET)</option>";
    html += "<option value=\"GMT0BST,M3.5.0/1,M10.5.0\">Europa/London (GMT)</option>";
    html += "<option value=\"EST5EDT,M3.2.0,M11.1.0\">Amerika/New York (EST)</option>";
    html += "<option value=\"PST8PDT,M3.2.0,M11.1.0\">Amerika/Los Angeles (PST)</option>";
    html += "<option value=\"JST-9\">Asien/Tokyo (JST)</option></select></div>";
    html += "<button type=\"submit\" class=\"btn\">💾 Speichern</button></form></div></div>";
    html += "<script>document.getElementById('ntpForm').addEventListener('submit',async(e)=>{e.preventDefault();";
    html += "const formData=new FormData(e.target);try{await fetch('/api/ntp/save',{method:'POST',body:new URLSearchParams(formData)});";
    html += "alert('Gespeichert!');window.location.href='/config';}catch(err){alert('Fehler');}});</script></body></html>";
    
    httpd_resp_set_type(req, "text/html");
    httpd_resp_send(req, html.c_str(), html.length());
    return ESP_OK;
  }

  // System Config Handler
  static esp_err_t config_system_handler(httpd_req_t *req) {
    IDFWebServer* instance = (IDFWebServer*)req->user_ctx;
    
    char cookie[256] = {0};
    if (httpd_req_get_hdr_value_str(req, "Cookie", cookie, sizeof(cookie)) != ESP_OK || 
        !instance->is_session_valid(cookie)) {
      httpd_resp_set_hdr(req, "Location", "/login");
      httpd_resp_set_status(req, "302 Found");
      httpd_resp_send(req, nullptr, 0);
      return ESP_OK;
    }
    
    std::string html = "<!DOCTYPE html><html><head><meta charset=\"UTF-8\"><title>System Einstellungen</title>";
    html += "<style>body{background:linear-gradient(135deg,#0f172a,#1e293b);color:#f1f5f9;font-family:Arial,sans-serif;margin:0;padding:20px}";
    html += ".header{display:flex;justify-content:space-between;align-items:center;margin-bottom:30px}";
    html += ".back-btn{background:#64748b;color:#fff;padding:10px 20px;border:none;border-radius:5px;text-decoration:none}";
    html += ".container{max-width:800px;margin:0 auto}.card{background:#334155;padding:30px;border-radius:15px;margin-bottom:20px}";
    html += ".form-group{margin-bottom:20px}.form-group label{display:block;margin-bottom:8px;color:#94a3b8}";
    html += ".form-group input{width:100%;padding:12px;border:1px solid #475569;border-radius:5px;background:#1e293b;color:#f1f5f9;font-size:16px;box-sizing:border-box}";
    html += ".btn{padding:12px 24px;background:#3b82f6;color:#fff;border:none;border-radius:5px;font-size:16px;cursor:pointer;margin-right:10px}";
    html += ".btn-danger{background:#ef4444}.btn-warning{background:#f59e0b}</style></head><body>";
    html += "<div class=\"header\"><h1>🔧 System Einstellungen</h1><a href=\"/config\" class=\"back-btn\">← Zurück</a></div>";
    html += "<div class=\"container\"><div class=\"card\"><h2>Geräteeinstellungen</h2><form id=\"deviceForm\">";
    html += "<div class=\"form-group\"><label>Gerätename</label><input type=\"text\" name=\"device_name\" value=\"";
    html += instance->current_config.device_name;
    html += "\" required></div><button type=\"submit\" class=\"btn\">💾 Speichern</button></form></div>";
    html += "<div class=\"card\"><h2>Passwörter ändern</h2><form id=\"passwordForm\">";
    html += "<div class=\"form-group\"><label>Admin-Passwort (aktuell)</label><input type=\"password\" name=\"current_password\" required></div>";
    html += "<div class=\"form-group\"><label>Neues Admin-Passwort</label><input type=\"password\" name=\"new_password\" required></div>";
    html += "<div class=\"form-group\"><label>Bestätigung</label><input type=\"password\" name=\"confirm_password\" required></div>";
    html += "<button type=\"submit\" class=\"btn\">🔒 Passwort ändern</button></form></div>";
    html += "<div class=\"card\"><h2>Sensor-Filter</h2><form id=\"filterForm\">";
    html += "<div class=\"form-group\"><label>Delayed ON (ms)</label><input type=\"number\" name=\"filter_on\" value=\"";
    html += std::to_string(instance->current_config.filter_delay_on);
    html += "\" min=\"0\" max=\"10000\" required></div>";
    html += "<div class=\"form-group\"><label>Delayed OFF (ms)</label><input type=\"number\" name=\"filter_off\" value=\"";
    html += std::to_string(instance->current_config.filter_delay_off);
    html += "\" min=\"0\" max=\"60000\" required></div>";
    html += "<button type=\"submit\" class=\"btn\">💾 Speichern</button></form></div>";
    html += "<div class=\"card\"><h2>System-Aktionen</h2>";
    html += "<button class=\"btn btn-warning\" onclick=\"if(confirm('ESP32 neu starten?'))fetch('/api/system/restart',{method:'POST'}).then(()=>alert('Neustart...'))\">🔄 Neustart</button>";
    html += "<button class=\"btn btn-danger\" onclick=\"if(confirm('Alle Einstellungen zurücksetzen?'))fetch('/api/system/factory_reset',{method:'POST'}).then(()=>alert('Reset durchgeführt'))\">⚠️ Factory Reset</button>";
    html += "</div></div>";
    html += "<script>";
    html += "document.getElementById('deviceForm').addEventListener('submit',async(e)=>{e.preventDefault();const formData=new FormData(e.target);";
    html += "try{await fetch('/api/system/save',{method:'POST',body:new URLSearchParams(formData)});alert('Gespeichert!');}catch(err){alert('Fehler');}});";
    html += "document.getElementById('passwordForm').addEventListener('submit',async(e)=>{e.preventDefault();const formData=new FormData(e.target);";
    html += "if(formData.get('new_password')!==formData.get('confirm_password')){alert('Passwörter stimmen nicht überein');return;}";
    html += "try{await fetch('/api/system/password',{method:'POST',body:new URLSearchParams(formData)});alert('Passwort geändert!');}catch(err){alert('Fehler');}});";
    html += "document.getElementById('filterForm').addEventListener('submit',async(e)=>{e.preventDefault();const formData=new FormData(e.target);";
    html += "try{await fetch('/api/system/filter',{method:'POST',body:new URLSearchParams(formData)});alert('Gespeichert!');}catch(err){alert('Fehler');}});";
    html += "</script></body></html>";
    
    httpd_resp_set_type(req, "text/html");
    httpd_resp_send(req, html.c_str(), html.length());
    return ESP_OK;
  }

  // API: WiFi Scan
  static esp_err_t api_wifi_scan_handler(httpd_req_t *req) {
    IDFWebServer* instance = (IDFWebServer*)req->user_ctx;
    std::string json = instance->buildWiFiScanJSON();
    
    httpd_resp_set_type(req, "application/json");
    httpd_resp_set_hdr(req, "Connection", "close");
    httpd_resp_send(req, json.c_str(), json.length());
    return ESP_OK;
  }

  // API: WiFi Save
  static esp_err_t api_wifi_save_handler(httpd_req_t *req) {
    IDFWebServer* instance = (IDFWebServer*)req->user_ctx;
    
    char content[512];
    int ret = httpd_req_recv(req, content, sizeof(content) - 1);
    if (ret <= 0) {
      httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "Bad Request");
      return ESP_FAIL;
    }
    content[ret] = '\0';
    
    auto params = instance->parse_post_body(content);
    
    // Save to NVS
    strncpy(instance->current_config.wifi_ssid, params["ssid"].c_str(), sizeof(instance->current_config.wifi_ssid) - 1);
    strncpy(instance->current_config.wifi_password, params["password"].c_str(), sizeof(instance->current_config.wifi_password) - 1);
    instance->config_storage.save(instance->current_config);
    
    ESP_LOGI("config", "WiFi config saved to NVS - SSID: %s", params["ssid"].c_str());
    
    httpd_resp_set_type(req, "application/json");
    httpd_resp_send(req, "{\"success\":true,\"message\":\"WiFi-Einstellungen gespeichert! ESP32 wird neu gestartet...\"}", HTTPD_RESP_USE_STRLEN);

    // Restart nach kurzem Delay (Response muss raus)
    delay(500);
    esp_restart();
    
    return ESP_OK;
  }

  // API: NTP Save
  static esp_err_t api_ntp_save_handler(httpd_req_t *req) {
    IDFWebServer* instance = (IDFWebServer*)req->user_ctx;
    
    char content[512];
    int ret = httpd_req_recv(req, content, sizeof(content) - 1);
    if (ret <= 0) {
      httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "Bad Request");
      return ESP_FAIL;
    }
    content[ret] = '\0';
    
    auto params = instance->parse_post_body(content);
    strncpy(instance->current_config.ntp_primary, params["ntp_primary"].c_str(), sizeof(instance->current_config.ntp_primary) - 1);
    strncpy(instance->current_config.ntp_secondary, params["ntp_secondary"].c_str(), sizeof(instance->current_config.ntp_secondary) - 1);
    strncpy(instance->current_config.timezone, params["timezone"].c_str(), sizeof(instance->current_config.timezone) - 1);
    
    instance->config_storage.save(instance->current_config);
    
    httpd_resp_set_type(req, "application/json");
    httpd_resp_send(req, "{\"success\":true}", HTTPD_RESP_USE_STRLEN);
    return ESP_OK;
  }

  // API: System Save
  static esp_err_t api_system_save_handler(httpd_req_t *req) {
    IDFWebServer* instance = (IDFWebServer*)req->user_ctx;
    
    char content[512];
    int ret = httpd_req_recv(req, content, sizeof(content) - 1);
    if (ret <= 0) {
      httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "Bad Request");
      return ESP_FAIL;
    }
    content[ret] = '\0';
    
    auto params = instance->parse_post_body(content);
    strncpy(instance->current_config.device_name, params["device_name"].c_str(), sizeof(instance->current_config.device_name) - 1);
    
    instance->config_storage.save(instance->current_config);
    
    httpd_resp_set_type(req, "application/json");
    httpd_resp_send(req, "{\"success\":true}", HTTPD_RESP_USE_STRLEN);
    return ESP_OK;
  }

  // API: Password Change
  static esp_err_t api_system_password_handler(httpd_req_t *req) {
    IDFWebServer* instance = (IDFWebServer*)req->user_ctx;
    
    char content[512];
    int ret = httpd_req_recv(req, content, sizeof(content) - 1);
    if (ret <= 0) {
      httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "Bad Request");
      return ESP_FAIL;
    }
    content[ret] = '\0';
    
    auto params = instance->parse_post_body(content);
    
    // Verify current password
    if (params["current_password"] != instance->current_config.admin_password) {
      httpd_resp_set_type(req, "application/json");
      httpd_resp_send(req, "{\"success\":false,\"message\":\"Falsches Passwort\"}", HTTPD_RESP_USE_STRLEN);
      return ESP_OK;
    }
    
    strncpy(instance->current_config.admin_password, params["new_password"].c_str(), sizeof(instance->current_config.admin_password) - 1);
    instance->config_storage.save(instance->current_config);
    
    httpd_resp_set_type(req, "application/json");
    httpd_resp_send(req, "{\"success\":true}", HTTPD_RESP_USE_STRLEN);
    return ESP_OK;
  }

  // API: Filter Save
  static esp_err_t api_system_filter_handler(httpd_req_t *req) {
    IDFWebServer* instance = (IDFWebServer*)req->user_ctx;
    
    char content[256];
    int ret = httpd_req_recv(req, content, sizeof(content) - 1);
    if (ret <= 0) {
      httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "Bad Request");
      return ESP_FAIL;
    }
    content[ret] = '\0';
    
    auto params = instance->parse_post_body(content);
    instance->current_config.filter_delay_on = std::stoi(params["filter_on"]);
    instance->current_config.filter_delay_off = std::stoi(params["filter_off"]);
    
    instance->config_storage.save(instance->current_config);
    
    httpd_resp_set_type(req, "application/json");
    httpd_resp_send(req, "{\"success\":true}", HTTPD_RESP_USE_STRLEN);
    return ESP_OK;
  }

  // API: Restart
  static esp_err_t api_system_restart_handler(httpd_req_t *req) {
    httpd_resp_set_type(req, "application/json");
    httpd_resp_send(req, "{\"success\":true}", HTTPD_RESP_USE_STRLEN);

    ESP_LOGI("config", "Restarting...");
    delay(500);  // Kurzer Delay für Response
    esp_restart();
    
    return ESP_OK;
  }

  // API: Factory Reset
  static esp_err_t api_system_factory_reset_handler(httpd_req_t *req) {
    IDFWebServer* instance = (IDFWebServer*)req->user_ctx;

    instance->config_storage.factory_reset();

    httpd_resp_set_type(req, "application/json");
    httpd_resp_send(req, "{\"success\":true}", HTTPD_RESP_USE_STRLEN);

    ESP_LOGI("config", "Factory reset, restarting...");
    delay(500);  // Kurzer Delay für Response
    esp_restart();
    
    return ESP_OK;
  }

 public:
  void setup() {
    // Init config storage
    config_storage.init();
    config_storage.load(current_config);
    
    // WiFi Override: Nutze NVS statt secrets.yaml wenn vorhanden
    if (strlen(current_config.wifi_ssid) > 0) {
      ESP_LOGI("config", "WiFi aus NVS gefunden: %s", current_config.wifi_ssid);
      ESP_LOGI("config", "Überschreibe ESPHome WiFi-Config mit NVS-Daten");
      
      // ESPHome WiFi neu konfigurieren
      auto wifi_comp = wifi::global_wifi_component;
      if (wifi_comp != nullptr) {
        wifi_comp->clear_sta();  // Lösche alte Config
        
        // Erstelle neuen WiFiAP
        wifi::WiFiAP ap;
        ap.set_ssid(current_config.wifi_ssid);
        ap.set_password(current_config.wifi_password);
        
        wifi_comp->add_sta(ap);
        ESP_LOGI("config", "WiFi-Config aus NVS geladen");
      }
    } else {
      ESP_LOGI("config", "Kein WiFi in NVS - nutze secrets.yaml");
    }
    
    httpd_config_t config = HTTPD_DEFAULT_CONFIG();
    config.server_port = 80;
    config.max_uri_handlers = 20;
    config.stack_size = 16384;  // 16KB stack for large HTML strings
    config.max_resp_headers = 8;
    config.recv_wait_timeout = 5;
    config.send_wait_timeout = 5;
    config.max_open_sockets = 3;  // Nur 3 gleichzeitige Verbindungen
    config.lru_purge_enable = true;  // Schließe alte Verbindungen automatisch
    config.backlog_conn = 1;  // Minimal backlog
    config.keep_alive_enable = false;  // Deaktiviere keep-alive
    config.close_fn = nullptr;  // Default close
    config.open_fn = nullptr;  // Default open

    if (httpd_start(&server, &config) == ESP_OK) {
      ESP_LOGI("custom", "HTTP Server gestartet auf Port 80");

      // Register all handlers
      httpd_uri_t uris[] = {
        {"/", HTTP_GET, root_handler, this},
        {"/binary_sensor", HTTP_GET, binary_sensor_handler, this},
        {"/sensor", HTTP_GET, sensor_handler, this},
        {"/text_sensor", HTTP_GET, text_sensor_handler, this},
        {"/login", HTTP_GET, login_get_handler, this},
        {"/api/login", HTTP_POST, login_post_handler, this},
        {"/api/logout", HTTP_GET, logout_handler, this},
        {"/config", HTTP_GET, config_menu_handler, this},
        {"/config/wifi", HTTP_GET, config_wifi_handler, this},
        {"/config/ntp", HTTP_GET, config_ntp_handler, this},
        {"/config/system", HTTP_GET, config_system_handler, this},
        {"/api/wifi/scan", HTTP_GET, api_wifi_scan_handler, this},
        {"/api/wifi/save", HTTP_POST, api_wifi_save_handler, this},
        {"/api/ntp/save", HTTP_POST, api_ntp_save_handler, this},
        {"/api/system/save", HTTP_POST, api_system_save_handler, this},
        {"/api/system/password", HTTP_POST, api_system_password_handler, this},
        {"/api/system/filter", HTTP_POST, api_system_filter_handler, this},
        {"/api/system/restart", HTTP_POST, api_system_restart_handler, this},
        {"/api/system/factory_reset", HTTP_POST, api_system_factory_reset_handler, this},
      };

      for (auto& uri : uris) {
        httpd_register_uri_handler(server, &uri);
      }
    } else {
      ESP_LOGE("custom", "Fehler beim Starten des HTTP Servers");
    }
  }
};
