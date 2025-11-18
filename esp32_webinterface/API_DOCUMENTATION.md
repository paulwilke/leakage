# ESP32 JSON Webservice API Documentation

Complete API documentation for the ESP32 sensor webservice endpoints.

## 📡 Overview

The ESP32 runs an ESPHome web server on **port 80** that provides RESTful JSON API endpoints for real-time sensor data access.

**Base URL**: `http://<ESP32-IP-ADDRESS>/`

Example: `http://192.168.1.100/`

## 🔌 API Endpoints

### 1. `/sensor` - Numeric Sensor Data

Returns all numeric sensor values (temperature, humidity, WiFi signal, uptime, etc.)

#### Request

```http
GET /sensor HTTP/1.1
Host: 192.168.1.100
```

#### Response

```json
[
  {
    "id": "humidity01-temperatur",
    "name": "Temperatur",
    "value": 23.4,
    "unit": "°C"
  },
  {
    "id": "humidity01-luftfeuchtigkeit",
    "name": "Luftfeuchtigkeit",
    "value": 45.2,
    "unit": "%"
  },
  {
    "id": "humidity01-wifi_signal",
    "name": "WiFi Signal",
    "value": -67.0,
    "unit": "dBm"
  },
  {
    "id": "humidity01-uptime",
    "name": "Uptime",
    "value": 3654.5,
    "unit": "s"
  }
]
```

#### Response Fields

| Field | Type | Description |
|-------|------|-------------|
| `id` | string | Unique identifier for the sensor (format: `device-sensor_name`) |
| `name` | string | Human-readable sensor name |
| `value` | number | Current sensor value |
| `unit` | string | Unit of measurement (°C, %, dBm, s, etc.) |

#### Example with curl

```bash
curl http://192.168.1.100/sensor | jq
```

#### Example with JavaScript

```javascript
fetch('http://192.168.1.100/sensor')
  .then(response => response.json())
  .then(data => {
    const temp = data.find(s => s.id.includes('temperatur'));
    console.log(`Temperature: ${temp.value}${temp.unit}`);
  });
```

---

### 2. `/text_sensor` - Text Sensor Data

Returns all text-based sensor values (IP address, MAC address, SSID, etc.)

#### Request

```http
GET /text_sensor HTTP/1.1
Host: 192.168.1.100
```

#### Response

```json
[
  {
    "id": "humidity01-ip_adresse",
    "name": "IP Adresse",
    "value": "192.168.1.100"
  },
  {
    "id": "humidity01-mac_adresse",
    "name": "MAC Adresse",
    "value": "AA:BB:CC:DD:EE:FF"
  },
  {
    "id": "humidity01-ssid",
    "name": "SSID",
    "value": "MyWiFiNetwork"
  }
]
```

#### Response Fields

| Field | Type | Description |
|-------|------|-------------|
| `id` | string | Unique identifier for the text sensor |
| `name` | string | Human-readable sensor name |
| `value` | string | Current text value |

#### Example with curl

```bash
curl http://192.168.1.100/text_sensor | jq
```

#### Example with Python

```python
import requests

response = requests.get('http://192.168.1.100/text_sensor')
data = response.json()

ip_sensor = next(s for s in data if 'ip' in s['id'])
print(f"IP Address: {ip_sensor['value']}")
```

---

### 3. `/binary_sensor` - Binary Sensor Data

Returns all binary sensor states (leak detection, connection status, etc.)

#### Request

```http
GET /binary_sensor HTTP/1.1
Host: 192.168.1.100
```

#### Response

```json
[
  {
    "id": "leak01-wasserleck_erkannt",
    "name": "Wasserleck erkannt",
    "value": false
  },
  {
    "id": "leak01-status",
    "name": "Status",
    "value": true
  }
]
```

#### Response Fields

| Field | Type | Description |
|-------|------|-------------|
| `id` | string | Unique identifier for the binary sensor |
| `name` | string | Human-readable sensor name |
| `value` | boolean | Current state (`true` or `false`) |

#### Value Interpretation

| Sensor Type | `true` means | `false` means |
|-------------|--------------|---------------|
| Leak Detection | Water detected (WET) | No water (DRY) |
| Status | Connected/Online | Disconnected/Offline |
| Motion | Motion detected | No motion |
| Door/Window | Open | Closed |

#### Example with curl

```bash
curl http://192.168.1.100/binary_sensor | jq
```

#### Example with Node.js

```javascript
const axios = require('axios');

axios.get('http://192.168.1.100/binary_sensor')
  .then(response => {
    const leakSensor = response.data.find(s => s.id.includes('wasserleck'));
    if (leakSensor.value) {
      console.log('⚠️ LEAK DETECTED!');
    } else {
      console.log('✅ All clear');
    }
  });
```

---

## 🔄 Polling vs WebSocket

### Current Implementation: HTTP Polling

The webapps use **HTTP polling** to fetch data at regular intervals:

- **Humidity Sensor**: Every 5 seconds
- **Leak Sensor**: Every 3 seconds

#### Pros
- ✅ Simple to implement
- ✅ Works with all browsers
- ✅ No persistent connection needed

#### Cons
- ❌ Higher latency
- ❌ More network overhead
- ❌ Not real-time

### Future: WebSocket Support

ESPHome supports WebSocket connections for real-time updates. To enable:

1. Connect to: `ws://192.168.1.100/events`
2. Receive real-time sensor updates as they occur

**Note**: The current webapps don't implement WebSocket, but this can be added for true real-time monitoring.

---

## 📊 Data Types & Ranges

### Humidity Sensor (SHT4x)

| Sensor | Range | Accuracy | Update Interval |
|--------|-------|----------|-----------------|
| Temperature | -40°C to +125°C | ±0.2°C | 60 seconds |
| Humidity | 0% to 100% | ±1.8% RH | 60 seconds |
| WiFi Signal | -100 to 0 dBm | ±1 dBm | 60 seconds |
| Uptime | 0 to ∞ seconds | Exact | 60 seconds |

#### WiFi Signal Quality

| dBm Range | Quality | Description |
|-----------|---------|-------------|
| -30 to -50 | Excellent | Perfect signal |
| -50 to -60 | Very Good | Excellent connection |
| -60 to -70 | Good | Reliable connection |
| -70 to -80 | Fair | May have issues |
| -80 to -90 | Poor | Unstable connection |
| < -90 | Very Poor | Likely to disconnect |

### Leak Sensor

| Sensor | Type | States | Update Interval |
|--------|------|--------|-----------------|
| Leak Detection | Binary | DRY (`false`) / WET (`true`) | Real-time with 100ms delay |
| WiFi Signal | Numeric | -100 to 0 dBm | 60 seconds |
| Uptime | Numeric | 0 to ∞ seconds | 60 seconds |

---

## 🚀 Advanced Usage

### Filtering Specific Sensors

#### JavaScript
```javascript
// Get only temperature
fetch('/sensor')
  .then(r => r.json())
  .then(data => {
    const temp = data.find(s => s.id.includes('temperatur'));
    return temp;
  });
```

#### jq (Command Line)
```bash
# Get only WiFi signal
curl -s http://192.168.1.100/sensor | jq '.[] | select(.id | contains("wifi"))'

# Get temperature value only
curl -s http://192.168.1.100/sensor | jq '.[] | select(.id | contains("temperatur")) | .value'
```

### Monitoring Script Example

#### Bash Script
```bash
#!/bin/bash
ESP32_IP="192.168.1.100"

while true; do
  TEMP=$(curl -s http://$ESP32_IP/sensor | jq -r '.[] | select(.id | contains("temperatur")) | .value')
  HUMIDITY=$(curl -s http://$ESP32_IP/sensor | jq -r '.[] | select(.id | contains("luftfeuchtigkeit")) | .value')

  echo "$(date): Temp=${TEMP}°C, Humidity=${HUMIDITY}%"

  sleep 60
done
```

#### Python Script
```python
import requests
import time
from datetime import datetime

ESP32_IP = "192.168.1.100"

while True:
    try:
        # Fetch sensor data
        response = requests.get(f'http://{ESP32_IP}/sensor')
        data = response.json()

        # Extract values
        temp = next((s['value'] for s in data if 'temperatur' in s['id']), None)
        humidity = next((s['value'] for s in data if 'luftfeuchtigkeit' in s['id']), None)

        print(f"{datetime.now()}: Temp={temp}°C, Humidity={humidity}%")

    except Exception as e:
        print(f"Error: {e}")

    time.sleep(60)
```

### Alerting Example

#### Send Email on Leak Detection
```python
import requests
import smtplib
from email.mime.text import MIMEText

ESP32_IP = "192.168.1.100"
EMAIL_FROM = "sensor@example.com"
EMAIL_TO = "admin@example.com"
SMTP_SERVER = "smtp.gmail.com"

def check_leak():
    response = requests.get(f'http://{ESP32_IP}/binary_sensor')
    data = response.json()

    leak = next((s for s in data if 'wasserleck' in s['id']), None)

    if leak and leak['value']:
        send_alert("⚠️ Water leak detected!")
        return True
    return False

def send_alert(message):
    msg = MIMEText(message)
    msg['Subject'] = 'ESP32 Leak Alert'
    msg['From'] = EMAIL_FROM
    msg['To'] = EMAIL_TO

    with smtplib.SMTP(SMTP_SERVER, 587) as server:
        server.starttls()
        server.login(EMAIL_FROM, "your_password")
        server.send_message(msg)

if __name__ == '__main__':
    import time
    while True:
        check_leak()
        time.sleep(5)
```

---

## 🔐 Security Considerations

### Authentication

⚠️ **ESPHome web server does NOT have built-in authentication**

To secure your API:

1. **Network Isolation**: Keep ESP32 on separate VLAN/network
2. **Firewall Rules**: Restrict access by IP address
3. **Reverse Proxy**: Use nginx/Apache with basic auth:

```nginx
location /sensor {
    auth_basic "Restricted";
    auth_basic_user_file /etc/nginx/.htpasswd;
    proxy_pass http://192.168.1.100/sensor;
}
```

4. **VPN**: Access only via VPN tunnel
5. **Home Assistant**: Use HA as intermediary with authentication

### HTTPS

ESPHome doesn't support HTTPS directly. Options:

1. **Use only on local network** (recommended for IoT devices)
2. **Reverse proxy with SSL**:

```nginx
server {
    listen 443 ssl;
    server_name sensor.example.com;

    ssl_certificate /path/to/cert.pem;
    ssl_certificate_key /path/to/key.pem;

    location / {
        proxy_pass http://192.168.1.100/;
    }
}
```

---

## 🐛 Error Handling

### HTTP Status Codes

| Code | Meaning | Action |
|------|---------|--------|
| 200 | Success | Process data normally |
| 404 | Endpoint not found | Check URL and ESPHome config |
| 500 | Server error | Check ESP32 logs |
| 503 | Service unavailable | ESP32 may be rebooting |
| Timeout | No response | Check network connection |

### Common Errors

#### Connection Refused
```
Error: connect ECONNREFUSED 192.168.1.100:80
```
**Solution**:
- Verify ESP32 is powered on
- Check IP address is correct
- Ensure web_server is enabled in YAML

#### CORS Error (Browser)
```
Access to fetch at 'http://192.168.1.100/sensor' from origin 'file://'
has been blocked by CORS policy
```
**Solution**:
- Use a web server instead of opening HTML directly
- Or add CORS headers to ESP32 (requires custom component)

#### Empty Response
```json
[]
```
**Solution**:
- Wait for sensor initialization (first boot)
- Check sensor configuration in YAML
- View ESPHome logs for errors

---

## 📚 Integration Examples

### Home Assistant

```yaml
# configuration.yaml
sensor:
  - platform: rest
    resource: http://192.168.1.100/sensor
    name: ESP32 Temperature
    value_template: '{{ value_json | selectattr("id", "contains", "temperatur") | map(attribute="value") | first }}'
    unit_of_measurement: "°C"
    scan_interval: 60
```

### Node-RED

```json
[
    {
        "id": "http_request",
        "type": "http request",
        "url": "http://192.168.1.100/sensor",
        "method": "GET",
        "return": "obj"
    },
    {
        "id": "function",
        "type": "function",
        "func": "const temp = msg.payload.find(s => s.id.includes('temperatur'));\nmsg.payload = temp.value;\nreturn msg;"
    }
]
```

### Grafana

Use JSON API data source:
1. Install JSON API plugin
2. Configure endpoint: `http://192.168.1.100/sensor`
3. Parse JSON and create visualizations

---

## 📖 Additional Resources

- [ESPHome Documentation](https://esphome.io/)
- [ESPHome Web Server](https://esphome.io/components/web_server.html)
- [ESPHome API](https://esphome.io/components/api.html)
- [RESTful API Best Practices](https://restfulapi.net/)

---

## 🔄 Changelog

### v1.0.0 (2024)
- Initial API documentation
- Support for `/sensor`, `/text_sensor`, `/binary_sensor` endpoints
- Examples for multiple programming languages
- Security best practices

---

**API Version**: ESPHome 2024.x
**Last Updated**: 2024
**Maintained by**: ESP32 Sensor Project
