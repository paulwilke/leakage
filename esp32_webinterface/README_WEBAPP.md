# ESP32 Professional Webserver Frontend

Professional web applications with JSON API integration for ESP32 sensors.

## 🚀 Features

### Humidity Sensor Webapp (`humidity_webapp.html`)
- ✅ **Real-time monitoring** of temperature and humidity
- 📊 **Interactive charts** with historical data (Chart.js)
- 📱 **Responsive design** - works on desktop, tablet, and mobile
- 🔄 **Auto-refresh** with configurable intervals
- 💾 **Data export** to JSON format
- 📡 **WiFi signal monitoring** with quality indicators
- ⏱️ **Uptime tracking** and system information
- 🎨 **Modern dark theme** with smooth animations

### Leak Sensor Webapp (`leak_webapp.html`)
- 🚨 **Large visual leak indicator** with animations
- 📜 **Event logging** with persistent storage (localStorage)
- 🔔 **Browser notifications** for leak detection
- 📊 **Event counter** and timeline
- 💧 **Binary state monitoring** (wet/dry)
- 🔄 **Auto-refresh** with status indicators
- 💾 **Export capability** for events and data
- 🧪 **Test alarm function** for system validation

## 📡 JSON API Endpoints

Both webapps consume the standard ESPHome JSON API:

### `/sensor` - Numeric Sensor Data
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
    "value": 3600.0,
    "unit": "s"
  }
]
```

### `/text_sensor` - Text Sensor Data
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
    "value": "MyWiFi"
  }
]
```

### `/binary_sensor` - Binary Sensor Data (Leak Sensor)
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

## 🔧 Installation & Usage

### Option 1: Standalone HTML Files (Easiest)

1. **Open the HTML files directly** in your browser:
   ```bash
   # Navigate to the directory
   cd esp32_webinterface/

   # Open in browser (Linux)
   xdg-open humidity_webapp.html
   xdg-open leak_webapp.html

   # Or macOS
   open humidity_webapp.html
   open leak_webapp.html
   ```

2. The webapps will automatically connect to the ESP32 on the **same network** using relative URLs (`/sensor`, `/text_sensor`, `/binary_sensor`)

3. **Note**: If you open the HTML file directly (`file://` protocol), you'll need to configure CORS on the ESP32 or use a local web server (see Option 2)

### Option 2: Local Web Server (Recommended for Development)

Serve the files using Python's built-in HTTP server:

```bash
cd esp32_webinterface/

# Python 3
python3 -m http.server 8000

# Python 2
python -m SimpleHTTPServer 8000
```

Then open in your browser:
- Humidity: http://localhost:8000/humidity_webapp.html
- Leak: http://localhost:8000/leak_webapp.html

### Option 3: Deploy to ESP32 Filesystem (Advanced)

You can embed the HTML files directly in the ESP32 firmware using ESPHome's `web_server` component with local files.

**Note**: This requires reducing file size due to ESP32 flash limitations. Consider minifying the HTML/CSS/JavaScript first.

1. **Add to your ESPHome YAML** (e.g., `humidity01.yaml`):

```yaml
web_server:
  port: 80
  local: true  # Serve custom files from ESP32 filesystem
  # Point to custom index page
  # (requires building with custom web server component)
```

2. **Alternative: Use web_server with include_internal**:

```yaml
web_server:
  port: 80
  include_internal: true
  # The default ESPHome web interface will be available
  # Your custom webapp can be accessed separately
```

### Option 4: External Web Server

Host the webapps on any web server (Apache, Nginx, Caddy, etc.) and configure them to proxy requests to the ESP32.

**Example nginx configuration**:

```nginx
server {
    listen 80;
    server_name sensor-dashboard.local;

    location / {
        root /path/to/esp32_webinterface;
        index humidity_webapp.html;
    }

    # Proxy API requests to ESP32
    location /sensor {
        proxy_pass http://192.168.1.100/sensor;
    }

    location /text_sensor {
        proxy_pass http://192.168.1.100/text_sensor;
    }

    location /binary_sensor {
        proxy_pass http://192.168.1.100/binary_sensor;
    }
}
```

## 🎯 Usage Guide

### Humidity Webapp

1. **Open the webapp** using one of the methods above
2. The dashboard will automatically connect and display:
   - Current temperature and humidity
   - WiFi signal strength
   - Historical chart (last 20 data points)
   - System information
3. **Controls**:
   - 🔄 **Aktualisieren**: Manual refresh
   - ⏸️ **Auto-Refresh pausieren**: Pause/resume automatic updates (default: 5s interval)
   - 💾 **Daten exportieren**: Export all data to JSON file

### Leak Sensor Webapp

1. **Open the webapp** using one of the methods above
2. The dashboard displays:
   - Large leak status indicator (green = dry, red = wet)
   - Event log with timestamps
   - WiFi and uptime metrics
3. **Features**:
   - **Browser notifications**: Enable in browser to get alerts
   - **Event persistence**: Events are saved in browser localStorage
   - **Event log**: View history of all leak events
   - 🗑️ **Clear log**: Remove all events
   - 🔔 **Alarm test**: Test the visual alarm system
4. **Controls**:
   - 🔄 **Aktualisieren**: Manual refresh
   - ⏸️ **Auto-Refresh pausieren**: Pause/resume (default: 3s interval)
   - 💾 **Daten exportieren**: Export events and data

## 🔐 Security Considerations

1. **No Authentication**: The default ESPHome web server has no authentication. To secure:
   - Use a VPN or firewall rules to restrict access
   - Deploy behind a reverse proxy with authentication
   - Keep the ESP32 on a separate IoT VLAN

2. **HTTPS**: ESPHome doesn't support HTTPS natively. For secure access:
   - Use a reverse proxy (nginx, Caddy) with SSL/TLS
   - Access only via local network

3. **CORS**: If hosting on a different domain, you may need to configure CORS headers on the ESP32 or use a proxy

## 🎨 Customization

### Change Refresh Intervals

Edit the JavaScript configuration in each HTML file:

```javascript
const CONFIG = {
    refreshInterval: 5000,  // Change to desired interval in milliseconds
    maxDataPoints: 20,      // Number of data points in chart
    // ...
};
```

### Modify Colors/Theme

Edit the CSS variables in the `<style>` section:

```css
:root {
    --primary: #3b82f6;      /* Primary blue color */
    --success: #10b981;      /* Success green */
    --danger: #ef4444;       /* Danger red */
    --bg-primary: #0f172a;   /* Dark background */
    /* ... */
}
```

### Add Custom Sensors

To display additional sensors, modify the JavaScript data fetching:

```javascript
// Example: Add a new sensor display
const pressure = sensorData.find(s => s.id.includes('pressure'));
if (pressure) {
    document.getElementById('pressureValue').textContent = pressure.value.toFixed(1);
}
```

## 📊 Browser Compatibility

- ✅ Chrome/Edge 90+
- ✅ Firefox 88+
- ✅ Safari 14+
- ✅ Mobile browsers (iOS Safari, Chrome Mobile)

**Requirements**:
- JavaScript enabled
- LocalStorage enabled (for leak sensor event log)
- Modern browser with ES6 support

## 🐛 Troubleshooting

### "Connection error" or "Offline" status

1. **Check ESP32 is online**: Ping the ESP32 IP address
2. **Verify network**: Ensure your computer and ESP32 are on the same network
3. **CORS issues**: If opening HTML from `file://`, use a local web server instead
4. **Firewall**: Check if firewall is blocking port 80

### Charts not displaying

1. **Check internet connection**: Chart.js is loaded from CDN
2. **Console errors**: Open browser DevTools (F12) and check for errors
3. **Try offline**: Download Chart.js locally if needed

### Data not updating

1. **Check auto-refresh**: Ensure it's not paused
2. **API endpoints**: Verify `/sensor`, `/text_sensor`, `/binary_sensor` return JSON
3. **Browser console**: Check for JavaScript errors

### Leak sensor notifications not working

1. **Enable notifications**: Click "Allow" when browser requests permission
2. **Browser support**: Some browsers (Safari) have limited notification support
3. **HTTPS required**: Some browsers only allow notifications over HTTPS

## 📝 API Response Examples

### Testing API Endpoints

You can test the API directly with curl:

```bash
# Test sensor endpoint
curl http://192.168.1.100/sensor

# Test text sensor endpoint
curl http://192.168.1.100/text_sensor

# Test binary sensor endpoint
curl http://192.168.1.100/binary_sensor
```

Or in your browser:
- http://192.168.1.100/sensor
- http://192.168.1.100/text_sensor
- http://192.168.1.100/binary_sensor

## 🆚 Comparison with Original Dashboards

| Feature | Original | New Webapp |
|---------|----------|------------|
| File Size | ~5KB (minified) | ~25-30KB (feature-rich) |
| Charts | ❌ | ✅ Chart.js integration |
| Event Logging | ❌ | ✅ Persistent storage |
| Data Export | ❌ | ✅ JSON export |
| Notifications | ❌ | ✅ Browser notifications |
| Customization | Limited | Extensive |
| Mobile UI | Basic | Fully responsive |
| Auto-refresh Control | ❌ | ✅ Pause/resume |

## 📚 Next Steps

1. **Configure static IPs** for your ESP32 sensors (recommended)
2. **Set up Home Assistant integration** for advanced automation
3. **Create alerts/automation** based on sensor data
4. **Deploy to production web server** for remote access
5. **Add authentication** if exposing to internet
6. **Customize theme** to match your preferences

## 🤝 Contributing

Feel free to modify and enhance these webapps for your specific needs!

## 📄 License

These webapps are part of your ESPHome sensor project. Use and modify freely.

---

**Powered by**: ESPHome • Chart.js • Vanilla JavaScript • Modern CSS
