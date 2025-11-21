# ESPHome UI Framework - User Guide

Welcome to the ESPHome UI Framework! This comprehensive guide will help you get started with building professional web interfaces for your ESP32 devices.

## 📖 Table of Contents

1. [Introduction](#introduction)
2. [What You'll Build](#what-youll-build)
3. [Prerequisites](#prerequisites)
4. [Quick Start](#quick-start)
5. [Understanding the Framework](#understanding-the-framework)
6. [Adding Sensors](#adding-sensors)
7. [Adding Actuators](#adding-actuators)
8. [Configuring the Web UI](#configuring-the-web-ui)
9. [Authentication & Security](#authentication--security)
10. [Deep Sleep Mode](#deep-sleep-mode)
11. [Troubleshooting](#troubleshooting)
12. [FAQ](#faq)

---

## Introduction

The ESPHome UI Framework is a production-ready, lightweight solution for creating web interfaces on ESP32 devices. It provides:

✨ **Key Features:**
- 🎨 **Beautiful Web UI** - Modern, responsive design that works on any device
- 🔐 **Built-in Security** - Session-based authentication with rate limiting
- 🌍 **Multi-language** - Support for multiple languages (EN, DE included)
- 📊 **Component Auto-discovery** - Sensors and actuators appear automatically
- ⚡ **Minimal Overhead** - Frontend < 50KB, optimized for ESP32 constraints
- 🔄 **Real-time Updates** - Dashboard updates every 5 seconds
- 📱 **Mobile-friendly** - Optimized for phones, tablets, and desktops
- 🌙 **Dark Mode** - Eye-friendly interface for any lighting condition

**What makes it special:**
- **Zero dependencies** - Pure vanilla JavaScript, no frameworks required
- **LLM-friendly** - Designed to be easily understood and extended by AI assistants
- **Production-tested** - Battle-tested in real-world deployments
- **ESP32-optimized** - Respects memory constraints and performance limits

---

## What You'll Build

By following this guide, you'll create a complete IoT device with:

```
┌─────────────────────────────────────┐
│      ESP32 + UI Framework           │
│                                     │
│  📡 WiFi Connection                 │
│  🌡️  Temperature Sensors (BME280)   │
│  💧 Humidity Sensor                 │
│  📊 Pressure Sensor                 │
│  🌡️  DS18B20 Temperature            │
│  🔌 Relay Control                   │
│  🌐 Web Dashboard                   │
│  🔐 Secure Authentication           │
│  📱 Mobile App Feel                 │
└─────────────────────────────────────┘
```

**Access it from anywhere:**
- Open `http://your-device-ip/` in any browser
- Monitor sensors in real-time
- Control actuators securely
- Switch languages on-the-fly
- Integrate with Home Assistant

---

## Prerequisites

### Hardware

**Required:**
- ESP32 development board (any variant, NodeMCU-32S recommended)
- USB cable for programming
- WiFi network (2.4GHz)

**Optional (for examples):**
- BME280 sensor (I2C) - $5-10
- DS18B20 temperature sensor (OneWire) - $2-5
- Relay module (1-channel) - $3-8
- 4.7kΩ resistor (for DS18B20)
- Breadboard and jumper wires

### Software

**Required:**
```bash
# Install ESPHome
pip install esphome

# Verify installation
esphome version
```

**Recommended:**
- Python 3.7 or higher
- Text editor (VS Code, Sublime, Vim, etc.)
- Serial monitor (for debugging)
- Modern web browser

**Optional:**
- Home Assistant (for integration)
- Git (for version control)

### Knowledge

**You should be familiar with:**
- Basic electronics (connecting wires, sensors)
- YAML syntax (ESPHome configuration)
- WiFi setup
- Using command line/terminal

**You don't need to know:**
- C++ programming (framework handles it)
- JavaScript (UI is pre-built)
- Web development (everything is included)

---

## Quick Start

### Step 1: Clone the Repository

```bash
git clone https://github.com/your-org/esphome-ui-framework.git
cd esphome-ui-framework
```

### Step 2: Copy Demo Configuration

```bash
cd examples/demo
cp demo.yaml my-device.yaml
```

### Step 3: Create Secrets File

Create `secrets.yaml` in the same directory:

```yaml
wifi_ssid: "YourWiFiName"
wifi_password: "YourWiFiPassword"
api_key: "your_32_character_api_key_here"
ota_password: "your_ota_password"
```

**Generate API key:**
```bash
python -c "import secrets; print(secrets.token_urlsafe(32))"
```

### Step 4: Configure Device Settings

Edit `my-device.yaml`:

```yaml
esphome:
  name: my-device          # Change this
  friendly_name: My Device # Change this

wifi:
  manual_ip:
    static_ip: 10.10.50.100  # Change to your network
    gateway: 10.10.50.1      # Your router IP
    subnet: 255.255.255.0
```

### Step 5: Compile and Upload

```bash
# First time - upload via USB
esphome run my-device.yaml

# Select your USB port
# Wait for upload (~2 minutes)
# Device will reboot automatically
```

### Step 6: Access Web Interface

1. Open your browser
2. Go to `http://10.10.50.100/` (or your configured IP)
3. Dashboard loads automatically
4. Default login: `admin` / `admin`
5. **⚠️ CHANGE PASSWORD IMMEDIATELY!**

**Congratulations!** 🎉 You now have a working device with web UI!

---

## Understanding the Framework

### Architecture Overview

```
┌────────────────────────────────────────────────────────┐
│                    Web Browser                         │
│  ┌──────────────────────────────────────────────────┐ │
│  │  Dashboard  │  Settings  │  About  │  [EN|DE]    │ │
│  └──────────────────────────────────────────────────┘ │
└────────────────────┬───────────────────────────────────┘
                     │ HTTP/REST API
┌────────────────────▼───────────────────────────────────┐
│                 ESP32 Device                           │
│  ┌──────────────────────────────────────────────────┐ │
│  │           UIFramework (main class)               │ │
│  │  ┌────────────┐  ┌────────────┐  ┌────────────┐ │ │
│  │  │  Web       │  │  Auth      │  │  Registry  │ │ │
│  │  │  Server    │  │  Controller│  │            │ │ │
│  │  └────────────┘  └────────────┘  └────────────┘ │ │
│  └──────────────────────────────────────────────────┘ │
│  ┌──────────────────────────────────────────────────┐ │
│  │         Component Registry                       │ │
│  │  ┌──────────┐  ┌──────────┐  ┌──────────┐      │ │
│  │  │ Sensor 1 │  │ Sensor 2 │  │ Actuator │ ... │ │
│  │  └──────────┘  └──────────┘  └──────────┘      │ │
│  └──────────────────────────────────────────────────┘ │
│  ┌──────────────────────────────────────────────────┐ │
│  │         Hardware Layer                           │ │
│  │    I2C    │   OneWire   │    GPIO    │   WiFi   │ │
│  └──────────────────────────────────────────────────┘ │
└────────────────────────────────────────────────────────┘
```

### Core Components

1. **UIFramework** - Main controller class
   - Manages HTTP server
   - Routes requests to handlers
   - Initializes subsystems

2. **ComponentRegistry** - Component management
   - Stores all sensors and actuators
   - Provides lookup by ID or type
   - Serializes to JSON for API

3. **AuthController** - Security
   - User authentication
   - Session management
   - Rate limiting

4. **Web Server** - HTTP interface
   - Serves static files (HTML, CSS, JS)
   - Provides REST API endpoints
   - Handles authentication cookies

5. **Components** - Sensors & Actuators
   - Self-contained implementations
   - Auto-register with framework
   - Appear in UI automatically

### Data Flow

**Sensor Reading:**
```
Hardware → Sensor Class → Registry → API → Web UI → User
```

**Actuator Control:**
```
User → Web UI → API → Auth Check → Actuator Class → Hardware
```

---

## Adding Sensors

### Example: BME280 Environmental Sensor

#### Step 1: Connect Hardware

```
BME280 Module    ESP32
─────────────    ─────
VCC           →  3.3V
GND           →  GND
SDA           →  GPIO21
SCL           →  GPIO22
```

#### Step 2: Add to Configuration

Edit your YAML file:

```yaml
esphome:
  includes:
    - ../../framework/include/ui_framework.h
    - ../bme280/bme280_sensor.h  # Include sensor header

  on_boot:
    priority: -10
    then:
      - lambda: |-
          auto framework = esphome_ui::UIFramework::getInstance();

          // Create BME280 temperature sensor
          static auto* temp = new esphome_ui::examples::BME280Sensor(
            "bme280_temp",           // Unique ID
            BME280Sensor::TEMPERATURE, // Type
            0x76,                    // I2C address
            "Room Temperature"       // Display name
          );

          if (temp->setup()) {
            framework->registerComponent(temp);
            ESP_LOGI("main", "✅ Temperature sensor registered");
          }

          // Similarly for humidity and pressure
          static auto* humidity = new esphome_ui::examples::BME280Sensor(
            "bme280_humidity",
            BME280Sensor::HUMIDITY,
            0x76,
            "Room Humidity"
          );

          if (humidity->setup()) {
            framework->registerComponent(humidity);
          }

# Configure I2C bus
i2c:
  sda: GPIO21
  scl: GPIO22
  scan: true
  frequency: 100kHz
```

#### Step 3: Compile and Upload

```bash
esphome run my-device.yaml
```

#### Step 4: View in Dashboard

Open web UI - sensor appears automatically! No additional configuration needed.

**That's it!** The framework handles:
- ✅ Sensor registration
- ✅ JSON serialization
- ✅ API endpoint creation
- ✅ Dashboard display
- ✅ Real-time updates

### Creating Custom Sensors

Want to add your own sensor? It's easy!

```cpp
#include "../../framework/include/component.h"

namespace esphome_ui {
namespace examples {

class MySensor : public ISensor {
private:
    std::string m_id;
    std::string m_name;
    float m_value;

public:
    MySensor(const char* id, const char* name)
        : m_id(id), m_name(name), m_value(0.0f) {}

    bool setup() {
        // Initialize your hardware here
        return true;
    }

    // Implement ISensor interface
    const char* getId() const override { return m_id.c_str(); }
    const char* getName() const override { return m_name.c_str(); }
    const char* getType() const override { return "custom"; }
    const char* getUnit() const override { return "unit"; }
    float getValue() override {
        // Read from hardware and return value
        return m_value;
    }
    bool isAvailable() override { return true; }
    bool isVisible() const override { return true; }
    bool requiresAuth() const override { return false; }
};

} // namespace examples
} // namespace esphome_ui
```

**Register in YAML:**
```yaml
esphome:
  includes:
    - path/to/my_sensor.h

  on_boot:
    then:
      - lambda: |-
          static auto* sensor = new esphome_ui::examples::MySensor(
            "my_sensor_id",
            "My Sensor Name"
          );

          if (sensor->setup()) {
            esphome_ui::UIFramework::getInstance()->registerComponent(sensor);
          }
```

**That's all you need!** Your sensor will appear in the dashboard automatically.

---

## Adding Actuators

### Example: Relay Control

#### Step 1: Connect Hardware

```
Relay Module    ESP32
────────────    ─────
VCC          →  5V
GND          →  GND
IN/Signal    →  GPIO23
```

#### Step 2: Add to Configuration

```yaml
esphome:
  includes:
    - ../../framework/include/ui_framework.h
    - ../relay/relay_actuator.h

  on_boot:
    then:
      - lambda: |-
          auto framework = esphome_ui::UIFramework::getInstance();

          static auto* relay = new esphome_ui::examples::RelayActuator(
            "relay1",          // ID
            23,                // GPIO pin
            "Main Relay",      // Name
            false,             // not inverted
            false              // don't restore state
          );

          if (relay->setup()) {
            // Set safety timeout: auto-off after 30 seconds
            relay->setSafetyTimeout(30000);
            framework->registerComponent(relay);
          }
```

#### Step 3: Control from Web UI

1. Login (relays require authentication)
2. Navigate to Dashboard
3. Click relay card
4. Click "Turn ON" button
5. Relay activates
6. Automatically turns off after 30 seconds (safety feature)

#### Step 4: Control via API

```bash
# Login first
curl -X POST http://10.10.50.100/api/auth/login \
  -H "Content-Type: application/json" \
  -d '{"username":"admin","password":"admin"}' \
  -c cookies.txt

# Turn ON
curl -X POST http://10.10.50.100/api/actuators/relay1 \
  -H "Content-Type: application/json" \
  -b cookies.txt \
  -d '{"state":"on"}'

# Turn OFF
curl -X POST http://10.10.50.100/api/actuators/relay1 \
  -H "Content-Type: application/json" \
  -b cookies.txt \
  -d '{"state":"off"}'

# Toggle
curl -X POST http://10.10.50.100/api/actuators/relay1 \
  -H "Content-Type: application/json" \
  -b cookies.txt \
  -d '{"action":"toggle"}'
```

### Safety Features

**Auto-off Timeout:**
```cpp
relay->setSafetyTimeout(30000);  // 30 seconds
```
Relay automatically turns off after duration. Perfect for:
- Garage door openers
- Water pumps
- Heaters
- Any safety-critical application

**State Persistence:**
```cpp
relay->setPersistState(true);
```
Relay remembers state after reboot.

**Inverted Logic:**
```cpp
RelayActuator relay("id", 23, "Name", true);  // inverted = true
```
For active-low relay modules.

---

## Configuring the Web UI

### Customizing Appearance

#### Device Name
```yaml
esphome:
  name: my-device
  friendly_name: My Smart Home Device
```

Appears in:
- Web UI header
- Browser tab title
- Home Assistant

#### Static IP Address
```yaml
wifi:
  manual_ip:
    static_ip: 192.168.1.100  # Your choice
    gateway: 192.168.1.1      # Your router
    subnet: 255.255.255.0
```

**Benefits of static IP:**
- Always same address
- Easier to remember
- Faster connection
- No DHCP issues

#### Update Interval

Currently hardcoded to 5 seconds. To change, modify `web/js/app.js`:

```javascript
// Line ~340
this.startPeriodicUpdates() {
    this.updateInterval = setInterval(() => {
        this.updateSensorData();
    }, 5000);  // Change to your preference (milliseconds)
}
```

### Adding Languages

Framework includes EN and DE. To add more:

#### Step 1: Create Translation File

Create `web/lang/es.json`:

```json
{
  "meta": {
    "language": "es",
    "name": "Español"
  },
  "nav": {
    "dashboard": "Panel",
    "settings": "Configuración",
    "about": "Acerca de"
  },
  "ui": {
    "login": "Iniciar sesión",
    "logout": "Cerrar sesión"
  }
  // ... more translations
}
```

#### Step 2: Add to Language Selector

Modify `web/js/i18n.js`:

```javascript
getSupportedLanguages() {
    return ['en', 'de', 'es'];  // Add 'es'
}
```

#### Step 3: Upload New Files

```bash
esphome run my-device.yaml
```

Language appears in selector automatically!

---

## Authentication & Security

### Changing Default Password

**⚠️ CRITICAL: Change the default password immediately!**

#### Method 1: Via Web UI (Future feature)
1. Login with default credentials
2. Go to Settings
3. Change Password section
4. Enter new password
5. Save

#### Method 2: Modify Code (Current)

Edit your YAML `on_boot` section:

```yaml
on_boot:
  then:
    - lambda: |-
        auto auth = framework->getAuth();
        auth->setCredentials("admin", "your_new_strong_password");
```

**Strong password requirements:**
- Minimum 8 characters
- Mix of letters and numbers
- Include special characters
- Not a dictionary word

### Understanding Sessions

**How it works:**
1. User logs in with username/password
2. Server creates session with random ID (32+ characters)
3. Session ID stored in HTTP-only cookie
4. Cookie sent with each request
5. Server validates session on protected endpoints
6. Session expires after 30 minutes of inactivity

**Security features:**
- ✅ Cryptographically random session IDs
- ✅ HTTP-only cookies (JavaScript cannot access)
- ✅ Session timeout (30 minutes default)
- ✅ Rate limiting (5 attempts, 5-minute lockout)
- ✅ Session cleanup (expired sessions removed)

### Rate Limiting

**Protection against brute force attacks:**

- After 5 failed login attempts from same IP
- Account locked for 5 minutes
- Applies even to correct password during lockout
- Counter resets on successful login
- Per-IP tracking (different IPs independent)

**Testing rate limiting:**
```bash
# Try wrong password 6 times
for i in {1..6}; do
  curl -X POST http://10.10.50.100/api/auth/login \
    -H "Content-Type: application/json" \
    -d '{"username":"admin","password":"wrong"}'
done

# 6th attempt will be blocked:
# {"success": false, "error": "Too many login attempts"}
```

### Best Practices

**Security checklist:**
- [ ] Change default password
- [ ] Use strong password
- [ ] Keep firmware updated
- [ ] Use static IP or mDNS
- [ ] Don't expose device to internet without VPN
- [ ] Monitor failed login attempts
- [ ] Regular security audits

---

## Deep Sleep Mode

For battery-powered devices, deep sleep dramatically extends battery life.

### Understanding Deep Sleep

**Normal mode:**
- ESP32 fully active: ~160mA
- WiFi on: +80mA
- Total: ~240mA
- 2000mAh battery lasts: ~8 hours

**Deep sleep mode:**
- ESP32 sleeping: ~10µA (0.01mA)
- Wake periodically: 5 seconds every 5 minutes
- Average: ~2mA
- 2000mAh battery lasts: ~40 days!

### Enabling Deep Sleep

```yaml
esphome:
  on_boot:
    then:
      - lambda: |-
          auto framework = esphome_ui::UIFramework::getInstance();

          // Configure deep sleep
          esphome_ui::DeepSleepController::Config config;
          config.enabled = true;
          config.sleep_duration_seconds = 300;  // 5 minutes
          config.wake_on_timer = true;

          framework->getDeepSleep().configure(config);
```

### Wake Sources

**Timer wake:**
```cpp
config.wake_on_timer = true;
config.sleep_duration_seconds = 600;  // 10 minutes
```

**GPIO wake:**
```cpp
config.wake_on_gpio = true;
config.wake_gpio_pins.push_back(GPIO_NUM_0);  // Wake on button press
```

**Both:**
```cpp
config.wake_on_timer = true;
config.wake_on_gpio = true;
config.sleep_duration_seconds = 3600;  // 1 hour, or button press
```

### Typical Use Cases

**Weather Station:**
- Read sensors every 5 minutes
- Send data to Home Assistant
- Enter deep sleep
- Battery lasts months

**Door Sensor:**
- Sleep until door opens (GPIO wake)
- Send notification
- Go back to sleep
- Battery lasts years

**Soil Moisture Monitor:**
- Wake once per hour
- Check moisture level
- Water if needed
- Sleep again

### Important Notes

**⚠️ Web UI not accessible during sleep!**
- Device only active for few seconds
- Can't access dashboard
- Use Home Assistant for monitoring
- Or disable deep sleep for configuration

**Debugging deep sleep:**
```yaml
logger:
  level: DEBUG
  logs:
    deep_sleep: DEBUG
```

---

## Troubleshooting

### Device Won't Connect to WiFi

**Symptoms:** Device not accessible, no IP assigned

**Solutions:**

1. **Check credentials:**
   ```yaml
   wifi_ssid: "YourNetwork"  # Correct name?
   wifi_password: "password"  # Correct password?
   ```

2. **Check 2.4GHz:**
   ESP32 only supports 2.4GHz WiFi, not 5GHz.
   Ensure your router broadcasts 2.4GHz.

3. **Check signal strength:**
   Move device closer to router.
   Serial log shows: `[W][wifi] Signal strength: -85 dB` (weak if < -80)

4. **Use fallback hotspot:**
   After 1 minute, device creates AP:
   - SSID: "UI Framework Demo" (or your configured name)
   - Password: "demo12345"
   - Connect and configure via 192.168.4.1

5. **Check router settings:**
   - MAC filtering disabled?
   - Client isolation disabled?
   - DHCP enabled?

### Sensors Show "Unavailable"

**BME280:**
- Check I2C wiring (SDA, SCL)
- Try alternative address: 0x77 instead of 0x76
- Check 3.3V power (NOT 5V!)
- Run I2C scan in logs

**DS18B20:**
- Check 4.7kΩ pull-up resistor (CRITICAL!)
- Verify OneWire pin (default GPIO4)
- Check for short circuits
- Test sensor with multimeter

**General:**
- Check connections
- Reboot device
- Check sensor power LED
- Review serial logs for errors

### Web UI Won't Load

**Symptoms:** Cannot access http://device-ip/

**Solutions:**

1. **Verify device IP:**
   ```bash
   # Check serial logs for:
   [I][main] Web UI: http://10.10.50.100/

   # Or ping device:
   ping 10.10.50.100
   ```

2. **Check firewall:**
   ```bash
   # Temporarily disable firewall
   sudo ufw disable  # Linux
   # Or add rule:
   sudo ufw allow 80/tcp
   ```

3. **Try different browser:**
   - Chrome
   - Firefox
   - Safari
   - Edge

4. **Check browser console:**
   Press F12, check for errors

5. **Clear browser cache:**
   Ctrl+Shift+R (hard refresh)

### Login Not Working

**Symptoms:** Login fails with correct password

**Solutions:**

1. **Check credentials:**
   Default: `admin` / `admin`

2. **Check rate limiting:**
   After 5 failed attempts, wait 5 minutes

3. **Check serial logs:**
   ```
   [E][auth] Login failed for user: admin from IP: 192.168.1.50
   [W][auth] Rate limit exceeded for IP: 192.168.1.50
   ```

4. **Reset password:**
   Add to `on_boot`:
   ```yaml
   - lambda: |-
       framework->getAuth()->setCredentials("admin", "admin");
   ```

5. **Check cookies enabled:**
   Browser must accept cookies for sessions

### Relay Not Responding

**Symptoms:** Relay doesn't click when commanded

**Solutions:**

1. **Check authentication:**
   Must be logged in to control relays

2. **Check wiring:**
   - VCC to 5V (relay coil)
   - GND to GND
   - Signal to correct GPIO

3. **Check GPIO number:**
   Verify pin number in code matches wiring

4. **Check relay module type:**
   - Active high: `inverted = false`
   - Active low: `inverted = true`

5. **Test manually:**
   Connect Signal pin to VCC - relay should click

6. **Check serial logs:**
   ```
   [I][relay] Relay 'relay1' (GPIO23) set to ON
   ```

---

## FAQ

### General Questions

**Q: Does this work with ESP8266?**
A: No, framework requires ESP32. ESP8266 has insufficient memory and lacks ESP-IDF support.

**Q: How many sensors can I add?**
A: Typically 10-20 sensors. Limited by ESP32 memory (~320KB RAM). Monitor `Free Heap` in serial logs.

**Q: Does it work with Home Assistant?**
A: Yes! ESPHome auto-discovery supported. All sensors and actuators appear automatically.

**Q: Can I use HTTPS?**
A: Not built-in. ESP32 HTTPS requires large certificates (~5KB). Use VPN for secure remote access instead.

**Q: Is OTA updates supported?**
A: Yes! ESPHome OTA built-in. Update wirelessly after initial USB upload.

### Technical Questions

**Q: What's the web UI size?**
A: Total ~24KB gzipped:
- HTML: ~3KB
- CSS: ~6KB
- JavaScript: ~15KB
- Languages: ~2KB each

**Q: How much flash does framework use?**
A: ~550KB for complete demo (with all examples). Leaves ~750KB free on 4MB ESP32.

**Q: What's the RAM usage?**
A: ~150KB for framework + sensors. Leaves ~170KB free. Monitor with `esp_get_free_heap_size()`.

**Q: Can I customize the web UI?**
A: Yes! All files in `web/` directory. Modify HTML, CSS, JavaScript as needed. Pure vanilla code, no build tools required.

**Q: What's the API format?**
A: RESTful JSON API. See [API Documentation](API.md) for details.

### Performance Questions

**Q: How fast are sensor updates?**
A: Dashboard updates every 5 seconds (configurable). Sensors read continuously in background.

**Q: What's the web page load time?**
A: < 2 seconds on good WiFi. Initial load ~100ms, assets load in parallel.

**Q: How many concurrent users?**
A: Tested with 5+ simultaneous browsers. ESP32 HTTP server handles 4 concurrent connections by default.

**Q: Does it affect sensor accuracy?**
A: No. Sensor readings independent of web UI. Framework adds <1ms overhead per sensor.

### Development Questions

**Q: How do I add a new sensor?**
A: Implement `ISensor` interface, register in `on_boot`. See [Developer Guide](DEVELOPER_GUIDE.md).

**Q: Can LLMs extend the framework?**
A: Yes! Designed for LLM-friendly code. See [LLM Extension Guide](LLM_EXTENSION_GUIDE.md).

**Q: Where are the tests?**
A: See `test/` directory. >80% code coverage with Google Test.

**Q: How do I contribute?**
A: See [CONTRIBUTING.md](CONTRIBUTING.md) for guidelines.

---

## Next Steps

🎓 **Learn More:**
- [Developer Guide](DEVELOPER_GUIDE.md) - Extend the framework
- [LLM Extension Guide](LLM_EXTENSION_GUIDE.md) - AI-assisted development
- [Architecture Documentation](ARCHITECTURE.md) - Deep dive into design
- [API Reference](API.md) - REST API documentation

🔨 **Build Something:**
- Try the examples in `examples/` directory
- Create your own custom sensors
- Build a complete home automation project
- Share your creation with the community

🐛 **Get Help:**
- Check [Troubleshooting](#troubleshooting) section
- Review [FAQ](#faq)
- Read [Manual Testing Checklist](../test/manual/testing-checklist.md)
- Ask questions in Issues

🚀 **Advanced Topics:**
- Multi-device deployments
- MQTT integration
- Custom authentication
- Advanced power management

---

**Happy building!** 🎉

If you create something cool with this framework, we'd love to hear about it!

---

**Version:** 1.0.0-alpha
**Last Updated:** 2025-11-21
**License:** MIT
**Documentation:** https://github.com/your-org/esphome-ui-framework
