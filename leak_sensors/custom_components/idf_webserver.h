#pragma once
#include "esphome.h"
#include <esp_http_server.h>
#include <string>

class IDFWebServer {
 private:
  httpd_handle_t server = nullptr;

  // JSON Response Builder
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
    std::string json = "[";
    
    // WiFi Signal
    if (id(wifi_signal_sensor).has_state()) {
      json += "{\"id\":\"leak01-wifi_signal\",\"name\":\"WiFi Signal\",\"value\":";
      json += std::to_string((int)id(wifi_signal_sensor).state);
      json += ",\"unit\":\"dBm\"},";
    }
    
    // Uptime
    if (id(uptime_sensor).has_state()) {
      json += "{\"id\":\"leak01-uptime\",\"name\":\"Uptime\",\"value\":";
      json += std::to_string((int)id(uptime_sensor).state);
      json += ",\"unit\":\"s\"}";
    }
    
    json += "]";
    return json;
  }

  std::string buildTextSensorJSON() {
    std::string json = "[";
    
    // IP Address
    if (id(ip_address).has_state()) {
      json += "{\"id\":\"leak01-ip_adresse\",\"name\":\"IP Adresse\",\"value\":\"";
      json += id(ip_address).state.c_str();
      json += "\"},";
    }
    
    // SSID
    if (id(ssid).has_state()) {
      json += "{\"id\":\"leak01-ssid\",\"name\":\"SSID\",\"value\":\"";
      json += id(ssid).state.c_str();
      json += "\"},";
    }
    
    // MAC Address
    if (id(mac_address).has_state()) {
      json += "{\"id\":\"leak01-mac_adresse\",\"name\":\"MAC Adresse\",\"value\":\"";
      json += id(mac_address).state.c_str();
      json += "\"}";
    }
    
    json += "]";
    return json;
  }

  // HTTP Request Handlers
  static esp_err_t binary_sensor_handler(httpd_req_t *req) {
    IDFWebServer* instance = (IDFWebServer*)req->user_ctx;
    std::string json = instance->buildBinarySensorJSON();
    
    httpd_resp_set_type(req, "application/json");
    httpd_resp_set_hdr(req, "Access-Control-Allow-Origin", "*");
    httpd_resp_send(req, json.c_str(), json.length());
    return ESP_OK;
  }

  static esp_err_t sensor_handler(httpd_req_t *req) {
    IDFWebServer* instance = (IDFWebServer*)req->user_ctx;
    std::string json = instance->buildSensorJSON();
    
    httpd_resp_set_type(req, "application/json");
    httpd_resp_set_hdr(req, "Access-Control-Allow-Origin", "*");
    httpd_resp_send(req, json.c_str(), json.length());
    return ESP_OK;
  }

  static esp_err_t text_sensor_handler(httpd_req_t *req) {
    IDFWebServer* instance = (IDFWebServer*)req->user_ctx;
    std::string json = instance->buildTextSensorJSON();
    
    httpd_resp_set_type(req, "application/json");
    httpd_resp_set_hdr(req, "Access-Control-Allow-Origin", "*");
    httpd_resp_send(req, json.c_str(), json.length());
    return ESP_OK;
  }

  static esp_err_t root_handler(httpd_req_t *req) {
    const char* html = R"(
<!DOCTYPE html>
<html>
<head>
<meta charset="UTF-8">
<title>Leak Sensor Dashboard</title>
<style>
body{background:linear-gradient(135deg,#0f172a,#1e293b);color:#f1f5f9;font-family:Arial,sans-serif;margin:0;padding:20px}
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
</style>
</head>
<body>
<div id="leak-status" class="leak-status dry">
<div id="leak-icon" class="leak-icon dry">✓</div>
<div id="leak-text" class="leak-text dry">TROCKEN</div>
</div>
<div class="sensor-grid">
<div class="sensor-card">
<div id="wifi-value" class="sensor-value">--</div>
<div class="sensor-label">WiFi Signal (dBm)</div>
</div>
<div class="sensor-card">
<div id="uptime-value" class="sensor-value">--</div>
<div class="sensor-label">Uptime</div>
</div>
</div>
<div class="info-card">
<div class="info-row"><span class="info-label">IP Adresse</span><span id="ip-value" class="info-value">--</span></div>
<div class="info-row"><span class="info-label">SSID</span><span id="ssid-value" class="info-value">--</span></div>
<div class="info-row"><span class="info-label">MAC Adresse</span><span id="mac-value" class="info-value">--</span></div>
</div>
<div id="update-time" class="update-time">Lade Daten...</div>
<script>
(function(){
const REFRESH_INTERVAL=3000;
async function updateDashboard(){
try{
const [binarySensors,sensors,textSensors]=await Promise.all([
fetch('/binary_sensor').then(r=>r.json()),
fetch('/sensor').then(r=>r.json()),
fetch('/text_sensor').then(r=>r.json())
]);
const leakSensor=binarySensors.find(s=>s.id.includes('wasserleck'));
if(leakSensor){
const isWet=leakSensor.value;
const statusEl=document.getElementById('leak-status');
const iconEl=document.getElementById('leak-icon');
const textEl=document.getElementById('leak-text');
if(isWet){
statusEl.className='leak-status wet';
iconEl.className='leak-icon wet';
iconEl.textContent='💧⚠️';
textEl.className='leak-text wet';
textEl.textContent='WASSERLECK!';
}else{
statusEl.className='leak-status dry';
iconEl.className='leak-icon dry';
iconEl.textContent='✓';
textEl.className='leak-text dry';
textEl.textContent='TROCKEN';
}
}
const wifiSensor=sensors.find(s=>s.id.includes('wifi'));
if(wifiSensor){
document.getElementById('wifi-value').textContent=wifiSensor.value.toFixed(0);
}
const uptimeSensor=sensors.find(s=>s.id.includes('uptime'));
if(uptimeSensor){
const hours=Math.floor(uptimeSensor.value/3600);
const minutes=Math.floor((uptimeSensor.value%3600)/60);
document.getElementById('uptime-value').textContent=hours+'h '+minutes+'m';
}
const ipSensor=textSensors.find(s=>s.id.includes('ip'));
if(ipSensor){
document.getElementById('ip-value').textContent=ipSensor.value;
}
const ssidSensor=textSensors.find(s=>s.id.includes('ssid'));
if(ssidSensor){
document.getElementById('ssid-value').textContent=ssidSensor.value;
}
const macSensor=textSensors.find(s=>s.id.includes('mac'));
if(macSensor){
document.getElementById('mac-value').textContent=macSensor.value;
}
const now=new Date();
document.getElementById('update-time').textContent='Letzte Aktualisierung: '+now.toLocaleTimeString();
}catch(e){
console.error('Fehler beim Laden der Daten:',e);
}
}
updateDashboard();
setInterval(updateDashboard,REFRESH_INTERVAL);
})();
</script>
</body>
</html>
)";
    
    httpd_resp_set_type(req, "text/html");
    httpd_resp_send(req, html, strlen(html));
    return ESP_OK;
  }

 public:
  void setup() {
    httpd_config_t config = HTTPD_DEFAULT_CONFIG();
    config.server_port = 80;
    config.max_uri_handlers = 10;
    config.stack_size = 8192;

    if (httpd_start(&server, &config) == ESP_OK) {
      ESP_LOGI("custom", "HTTP Server gestartet auf Port 80");

      // Register URI handlers
      httpd_uri_t root_uri = {
        .uri = "/",
        .method = HTTP_GET,
        .handler = root_handler,
        .user_ctx = this
      };
      httpd_register_uri_handler(server, &root_uri);

      httpd_uri_t binary_uri = {
        .uri = "/binary_sensor",
        .method = HTTP_GET,
        .handler = binary_sensor_handler,
        .user_ctx = this
      };
      httpd_register_uri_handler(server, &binary_uri);

      httpd_uri_t sensor_uri = {
        .uri = "/sensor",
        .method = HTTP_GET,
        .handler = sensor_handler,
        .user_ctx = this
      };
      httpd_register_uri_handler(server, &sensor_uri);

      httpd_uri_t text_uri = {
        .uri = "/text_sensor",
        .method = HTTP_GET,
        .handler = text_sensor_handler,
        .user_ctx = this
      };
      httpd_register_uri_handler(server, &text_uri);
    } else {
      ESP_LOGE("custom", "Fehler beim Starten des HTTP Servers");
    }
  }
};

