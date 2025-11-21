# ESPHome UI Framework - LLM Extension Guide

**For Large Language Models (AI Assistants) Extending This Framework**

This guide teaches LLMs (like Claude, GPT-4, etc.) how to effectively read, understand, and extend the ESPHome UI Framework. The framework was specifically designed with LLM-friendly patterns to enable AI-assisted development.

## 📖 Table of Contents

1. [Why This Guide Exists](#why-this-guide-exists)
2. [Framework Philosophy](#framework-philosophy)
3. [Reading the Codebase](#reading-the-codebase)
4. [Patterns to Recognize](#patterns-to-recognize)
5. [Common Extension Tasks](#common-extension-tasks)
6. [Workflow Examples](#workflow-examples)
7. [Testing Extensions](#testing-extensions)
8. [Pitfalls to Avoid](#pitfalls-to-avoid)
9. [Validation Checklist](#validation-checklist)

---

## Why This Guide Exists

### The LLM-Friendly Design

This framework was intentionally designed to be:

✅ **Transparent** - No magic, no hidden abstractions
✅ **Explicit** - Clear intent, no implicit behavior
✅ **Self-documenting** - Code explains itself
✅ **Pattern-consistent** - Same patterns everywhere
✅ **Modular** - Self-contained components
✅ **Testable** - Mock-friendly interfaces

**Goal:** An LLM should be able to read any part of the codebase and immediately understand:
- What it does
- Why it exists
- How to extend it
- What constraints apply

### Example of LLM-Friendly Code

**Bad (Magic):**
```cpp
// What does this do? Where's the implementation?
@sensor(id="temp", type="temperature")
class TempSensor;
```

**Good (Explicit):**
```cpp
/**
 * @brief Temperature sensor implementation
 *
 * Reads from BME280 I2C sensor at address 0x76.
 * Returns temperature in Celsius with 0.1°C accuracy.
 */
class BME280Sensor : public ISensor {
public:
    // Clear constructor shows all dependencies
    BME280Sensor(const char* id, uint8_t i2c_address, const char* name);

    // Explicit initialization
    bool setup() {
        // Every step documented
        if (!initI2C()) return false;
        if (!readCalibration()) return false;
        return true;
    }

    // Self-explanatory interface
    float getValue() override {
        return m_temperature;
    }
};
```

**Why this helps LLMs:**
- No magic macros to figure out
- Clear lifecycle (constructor → setup() → getValue())
- Explicit error handling
- Self-contained logic

---

## Framework Philosophy

### Core Principles for LLM Understanding

1. **Minimal Abstraction**
   - Only abstract when absolutely necessary
   - Each abstraction layer should be obvious
   - No "clever" code

2. **Explicit Over Implicit**
   ```cpp
   // Bad - implicit magic
   auto sensor = createSensor("temperature");

   // Good - explicit construction
   auto sensor = new BME280Sensor("id", TEMPERATURE, 0x76, "name");
   ```

3. **Self-Contained Components**
   - Each component is a complete unit
   - No hidden global state
   - Clear dependencies

4. **Consistent Patterns**
   - Same structure for all sensors
   - Same structure for all actuators
   - Predictable naming conventions

5. **Resource Awareness**
   - ESP32 has limited memory (~320KB RAM)
   - Avoid dynamic allocations in loops
   - Prefer stack over heap

---

## Reading the Codebase

### File Organization

```
esphome-ui-framework/
├── framework/              # Core framework
│   └── include/
│       ├── component.h     # START HERE - Base interfaces
│       ├── registry.h      # Component storage
│       ├── auth.h          # Authentication
│       ├── config_storage.h # Configuration
│       ├── deep_sleep.h    # Power management
│       └── ui_framework.h  # Main controller
│
├── examples/               # Reference implementations
│   ├── bme280/            # I2C sensor example
│   ├── ds18b20/           # OneWire sensor example
│   ├── relay/             # Actuator example
│   └── demo/              # Complete demo
│
├── web/                    # Frontend (vanilla JS)
│   ├── index.html
│   ├── css/
│   │   ├── themes.css     # Color themes
│   │   └── main.css       # Layout
│   └── js/
│       ├── api.js         # REST client
│       ├── i18n.js        # Translations
│       ├── app.js         # Main app
│       └── charts.js      # Chart support
│
├── docs/                   # Documentation
│   ├── ARCHITECTURE.md     # System design
│   ├── USER_GUIDE.md       # End-user guide
│   ├── DEVELOPER_GUIDE.md  # Developer reference
│   └── adr/               # Architecture decisions
│
└── test/                   # Testing
    ├── unit/              # Google Test
    ├── integration/       # Compilation tests
    └── manual/            # Hardware testing
```

### Reading Order for LLMs

**To understand the framework:**

1. **Start**: `framework/include/component.h`
   - Read IComponent interface
   - Read ISensor interface
   - Read IActuator interface
   - **Time: 5 minutes, ~300 lines**

2. **Second**: `framework/include/registry.h`
   - Understand component storage
   - See how components are looked up
   - Notice JSON serialization
   - **Time: 3 minutes, ~400 lines**

3. **Third**: `examples/relay/relay_actuator.h`
   - See complete actuator implementation
   - Notice how interface is implemented
   - Understand lifecycle (constructor → setup() → methods)
   - **Time: 5 minutes, ~320 lines**

4. **Fourth**: `framework/include/ui_framework.h`
   - See how everything connects
   - Understand HTTP server setup
   - Notice endpoint handlers
   - **Time: 7 minutes, ~450 lines**

**Total reading time: ~20 minutes**

After this, you understand 80% of the framework!

### Code Navigation Tips for LLMs

**Finding implementations:**
```bash
# Find all sensor implementations
grep -r "class.*: public ISensor" examples/

# Find all actuator implementations
grep -r "class.*: public IActuator" examples/

# Find all API endpoints
grep -r "httpd_uri_t.*=" examples/
```

**Understanding data flow:**
1. User clicks button → web/js/app.js
2. API call → web/js/api.js
3. HTTP request → framework/include/ui_framework.h (handler)
4. Authentication → framework/include/auth.h
5. Component lookup → framework/include/registry.h
6. Component action → examples/*/component.h

---

## Patterns to Recognize

### Pattern 1: Component Lifecycle

**Every component follows this pattern:**

```cpp
class MyComponent : public ISensor {
private:
    // State
    std::string m_id;
    float m_value;
    bool m_available;

public:
    // 1. Constructor - Initialize state
    MyComponent(const char* id, /* params */)
        : m_id(id)
        , m_value(0.0f)
        , m_available(false)
    {}

    // 2. Setup - Initialize hardware
    bool setup() {
        // Configure hardware
        // Test communication
        // Set m_available = true on success
        return m_available;
    }

    // 3. Methods - Implement interface
    const char* getId() const override { return m_id.c_str(); }
    float getValue() override { return m_value; }
    bool isAvailable() override { return m_available; }
    // ... etc
};
```

**When creating new components, follow this exact pattern.**

### Pattern 2: Error Handling

**Sensors handle errors by:**

```cpp
bool isAvailable() override {
    return m_available;  // False if hardware failed
}

float getValue() override {
    if (!m_available) {
        return m_last_good_value;  // Return cached value
    }

    float raw = readHardware();

    if (std::isnan(raw) || std::isinf(raw)) {
        ESP_LOGW("sensor", "Invalid reading");
        return m_last_good_value;
    }

    m_last_good_value = raw;
    return raw;
}
```

**Never throw exceptions** - ESP32 has limited stack.

### Pattern 3: JSON Serialization

**Every component serializes itself:**

```cpp
std::string toJson() const override {
    char buffer[256];
    snprintf(buffer, sizeof(buffer),
        "{"
        "\"id\":\"%s\","
        "\"name\":\"%s\","
        "\"type\":\"%s\","
        "\"value\":%.1f,"
        "\"unit\":\"%s\","
        "\"available\":%s"
        "}",
        getId(),
        getName(),
        getType(),
        getValue(),
        getUnit(),
        isAvailable() ? "true" : "false"
    );
    return std::string(buffer);
}
```

**Use stack buffer + snprintf** - avoids heap fragmentation.

### Pattern 4: Registration

**Components register with framework:**

```yaml
esphome:
  on_boot:
    then:
      - lambda: |-
          // Get framework singleton
          auto framework = esphome_ui::UIFramework::getInstance();

          // Create component (static = persists across lambdas)
          static auto* component = new MyComponent("id", params);

          // Initialize
          if (component->setup()) {
            // Register with framework
            framework->registerComponent(component);
            ESP_LOGI("main", "✅ Component registered");
          } else {
            ESP_LOGE("main", "❌ Component setup failed");
          }
```

**Always use `static` for components** - they must outlive the lambda.

### Pattern 5: Authentication

**Protected endpoints check session:**

```cpp
esp_err_t protectedHandler(httpd_req_t *req) {
    auto* framework = static_cast<UIFramework*>(req->user_ctx);

    // 1. Extract session cookie
    char cookie[256];
    httpd_req_get_hdr_value_str(req, "Cookie", cookie, sizeof(cookie));

    // 2. Parse session_id
    char* session_id = strstr(cookie, "session_id=");

    // 3. Validate
    if (!framework->getAuth().validateSession(session_id)) {
        httpd_resp_send_err(req, HTTPD_401_UNAUTHORIZED, "Unauthorized");
        return ESP_FAIL;
    }

    // 4. Proceed with protected logic
    // ...
}
```

**Public endpoints skip validation** (status, sensors, etc.).

---

## Common Extension Tasks

### Task 1: Add New Sensor Type

**User request:** "Add support for DHT22 temperature/humidity sensor"

**LLM reasoning process:**

1. **Identify base interface:** ISensor (from component.h:100)

2. **Find similar example:** BME280 (examples/bme280/) also does temp+humidity

3. **Understand DHT22:**
   - OneWire-like protocol (1-wire data pin)
   - Returns temperature and humidity
   - Requires pull-up resistor

4. **Design approach:**
   - Create DHT22Sensor class inheriting ISensor
   - One instance per measurement (like BME280)
   - Shared hardware access pattern

5. **Implementation:**

```cpp
#pragma once

#include "../../framework/include/component.h"
#include "DHT.h"  // DHT library

namespace esphome_ui {
namespace examples {

class DHT22Sensor : public ISensor {
public:
    enum MeasurementType { TEMPERATURE, HUMIDITY };

private:
    std::string m_id;
    std::string m_name;
    MeasurementType m_type;
    uint8_t m_pin;
    float m_value;
    bool m_available;

    // Shared DHT instance
    static DHT* s_dht;
    static uint8_t s_pin;

public:
    DHT22Sensor(const char* id, MeasurementType type,
                uint8_t pin, const char* name = nullptr)
        : m_id(id)
        , m_type(type)
        , m_pin(pin)
        , m_value(0.0f)
        , m_available(false)
    {
        if (name) {
            m_name = name;
        } else {
            m_name = (type == TEMPERATURE) ? "Temperature" : "Humidity";
        }

        // Initialize shared DHT instance
        if (!s_dht || s_pin != pin) {
            s_dht = new DHT(pin, DHT22);
            s_pin = pin;
        }
    }

    bool setup() override {
        ESP_LOGI("dht22", "Initializing DHT22 on pin %d", m_pin);

        s_dht->begin();
        delay(2000);  // DHT22 needs time to stabilize

        // Test read
        update();

        if (std::isnan(m_value)) {
            ESP_LOGE("dht22", "Failed to read from DHT22");
            return false;
        }

        m_available = true;
        return true;
    }

    void update() {
        if (m_type == TEMPERATURE) {
            m_value = s_dht->readTemperature();
        } else {
            m_value = s_dht->readHumidity();
        }

        if (std::isnan(m_value)) {
            m_available = false;
        } else {
            m_available = true;
        }
    }

    // ISensor interface
    const char* getId() const override { return m_id.c_str(); }
    const char* getName() const override { return m_name.c_str(); }

    const char* getType() const override {
        return (m_type == TEMPERATURE) ? "temperature" : "humidity";
    }

    const char* getUnit() const override {
        return (m_type == TEMPERATURE) ? "°C" : "%";
    }

    float getValue() override {
        update();
        return m_value;
    }

    bool isAvailable() override { return m_available; }
    bool isVisible() const override { return true; }
    bool requiresAuth() const override { return false; }

    const char* getDeviceClass() const override {
        return getType();
    }

    std::string toJson() const override {
        char buffer[256];
        snprintf(buffer, sizeof(buffer),
            "{\"id\":\"%s\",\"name\":\"%s\",\"type\":\"%s\","
            "\"value\":%.1f,\"unit\":\"%s\",\"available\":%s}",
            m_id.c_str(), m_name.c_str(), getType(),
            m_value, getUnit(),
            m_available ? "true" : "false"
        );
        return std::string(buffer);
    }
};

// Static members
DHT* DHT22Sensor::s_dht = nullptr;
uint8_t DHT22Sensor::s_pin = 0;

} // namespace examples
} // namespace esphome_ui
```

6. **Register in YAML:**

```yaml
esphome:
  includes:
    - ../../framework/include/ui_framework.h
    - path/to/dht22_sensor.h

  libraries:
    - "DHT sensor library"  # PlatformIO library

  on_boot:
    then:
      - lambda: |-
          auto framework = esphome_ui::UIFramework::getInstance();

          static auto* temp = new esphome_ui::examples::DHT22Sensor(
            "dht22_temp",
            DHT22Sensor::TEMPERATURE,
            4,  // GPIO4
            "Room Temperature"
          );

          static auto* humidity = new esphome_ui::examples::DHT22Sensor(
            "dht22_hum",
            DHT22Sensor::HUMIDITY,
            4,  // Same GPIO
            "Room Humidity"
          );

          if (temp->setup()) {
            framework->registerComponent(temp);
            framework->registerComponent(humidity);
            ESP_LOGI("main", "✅ DHT22 sensors registered");
          }
```

7. **Validation:**
   - Compile: `esphome compile device.yaml`
   - Upload and test
   - Verify dashboard shows sensors
   - Check value updates

**Total time: ~20 minutes**

### Task 2: Add New API Endpoint

**User request:** "Add /api/diagnostics endpoint showing system health"

**Implementation:**

```cpp
// In on_boot lambda after framework setup:

// Register diagnostics endpoint
httpd_uri_t diagnostics_uri = {
    .uri       = "/api/diagnostics",
    .method    = HTTP_GET,
    .handler   = [](httpd_req_t *req) -> esp_err_t {
        auto* framework = static_cast<esphome_ui::UIFramework*>(req->user_ctx);

        // Gather diagnostics
        uint32_t free_heap = esp_get_free_heap_size();
        uint32_t uptime_sec = millis() / 1000;
        int rssi = WiFi.RSSI();
        size_t component_count = framework->getRegistry().count();
        uint32_t active_sessions = framework->getAuth().getActiveSessionCount();

        // Build JSON response
        char buffer[512];
        snprintf(buffer, sizeof(buffer),
            "{"
            "\"status\":\"ok\","
            "\"uptime\":%u,"
            "\"free_heap\":%u,"
            "\"wifi_rssi\":%d,"
            "\"component_count\":%zu,"
            "\"active_sessions\":%u,"
            "\"ip\":\"%s\""
            "}",
            uptime_sec,
            free_heap,
            rssi,
            component_count,
            active_sessions,
            WiFi.localIP().toString().c_str()
        );

        httpd_resp_set_type(req, "application/json");
        httpd_resp_send(req, buffer, strlen(buffer));

        return ESP_OK;
    },
    .user_ctx  = framework
};

httpd_register_uri_handler(framework->getServer(), &diagnostics_uri);
ESP_LOGI("main", "✅ Diagnostics endpoint registered");
```

**Test:**
```bash
curl http://10.10.50.100/api/diagnostics
```

**Expected output:**
```json
{
  "status": "ok",
  "uptime": 12345,
  "free_heap": 180000,
  "wifi_rssi": -65,
  "component_count": 5,
  "active_sessions": 1,
  "ip": "10.10.50.100"
}
```

### Task 3: Add Custom Web UI Page

**User request:** "Add a 'System Info' page showing diagnostics"

**Implementation:**

1. **Add route handler** (web/js/app.js):

```javascript
async handleRoute() {
    const route = window.location.hash.slice(1) || '/dashboard';

    switch (route) {
        case '/dashboard':
            await this.showDashboard();
            break;
        case '/settings':
            await this.showSettings();
            break;
        case '/system-info':  // NEW
            await this.showSystemInfo();
            break;
        case '/about':
            this.showAbout();
            break;
    }
}
```

2. **Implement page logic:**

```javascript
async showSystemInfo() {
    const content = document.getElementById('content');

    // Fetch diagnostics
    let diagnostics = null;
    try {
        diagnostics = await this.api.get('/api/diagnostics');
    } catch (error) {
        this.showError('Failed to load diagnostics');
        return;
    }

    // Format uptime
    const hours = Math.floor(diagnostics.uptime / 3600);
    const minutes = Math.floor((diagnostics.uptime % 3600) / 60);
    const uptimeStr = `${hours}h ${minutes}m`;

    // Format memory
    const heapMB = (diagnostics.free_heap / 1024 / 1024).toFixed(2);

    // Render page
    content.innerHTML = `
        <div class="page-header">
            <h1>${this.i18n.t('system.title')}</h1>
        </div>

        <div class="card">
            <h2>${this.i18n.t('system.status')}</h2>
            <div class="info-grid">
                <div class="info-item">
                    <span class="label">${this.i18n.t('system.uptime')}</span>
                    <span class="value">${uptimeStr}</span>
                </div>
                <div class="info-item">
                    <span class="label">${this.i18n.t('system.free_heap')}</span>
                    <span class="value">${heapMB} MB</span>
                </div>
                <div class="info-item">
                    <span class="label">${this.i18n.t('system.wifi_rssi')}</span>
                    <span class="value">${diagnostics.wifi_rssi} dBm</span>
                </div>
                <div class="info-item">
                    <span class="label">${this.i18n.t('system.ip_address')}</span>
                    <span class="value">${diagnostics.ip}</span>
                </div>
                <div class="info-item">
                    <span class="label">${this.i18n.t('system.components')}</span>
                    <span class="value">${diagnostics.component_count}</span>
                </div>
                <div class="info-item">
                    <span class="label">${this.i18n.t('system.active_sessions')}</span>
                    <span class="value">${diagnostics.active_sessions}</span>
                </div>
            </div>
        </div>
    `;
}
```

3. **Add translations** (web/lang/en.json):

```json
{
  "system": {
    "title": "System Information",
    "status": "Status",
    "uptime": "Uptime",
    "free_heap": "Free Memory",
    "wifi_rssi": "WiFi Signal",
    "ip_address": "IP Address",
    "components": "Components",
    "active_sessions": "Active Sessions"
  }
}
```

4. **Add navigation link** (web/index.html):

```html
<nav class="nav">
    <a href="#/dashboard">Dashboard</a>
    <a href="#/system-info">System Info</a>
    <a href="#/settings">Settings</a>
    <a href="#/about">About</a>
</nav>
```

5. **Add CSS** (web/css/main.css):

```css
.info-grid {
    display: grid;
    grid-template-columns: repeat(auto-fit, minmax(250px, 1fr));
    gap: 1rem;
}

.info-item {
    display: flex;
    flex-direction: column;
    padding: 1rem;
    background: var(--bg-secondary);
    border-radius: 8px;
}

.info-item .label {
    font-size: 0.875rem;
    color: var(--text-secondary);
    margin-bottom: 0.5rem;
}

.info-item .value {
    font-size: 1.5rem;
    font-weight: bold;
    color: var(--text-primary);
}
```

**Done!** New page appears in navigation and shows live diagnostics.

---

## Workflow Examples

### Example 1: User Wants BH1750 Light Sensor

**User prompt:**
> "Add support for BH1750 ambient light sensor (I2C). It should show lux values in the dashboard."

**LLM reasoning:**

1. **Identify sensor type:** I2C sensor, similar to BME280
2. **Check existing I2C examples:** BME280 in examples/bme280/
3. **Understand BH1750:**
   - I2C address: 0x23 or 0x5C
   - Returns 16-bit lux value
   - Simple protocol: command → wait → read 2 bytes

4. **Create implementation:**

```cpp
// File: examples/bh1750/bh1750_sensor.h

#pragma once

#include "../../framework/include/component.h"
#include <Wire.h>

namespace esphome_ui {
namespace examples {

class BH1750Sensor : public ISensor {
private:
    std::string m_id;
    std::string m_name;
    uint8_t m_i2c_address;
    float m_lux;
    bool m_available;

    // BH1750 commands
    static constexpr uint8_t CMD_POWER_ON = 0x01;
    static constexpr uint8_t CMD_CONTINUOUS_HIGH_RES = 0x10;

    bool sendCommand(uint8_t cmd) {
        Wire.beginTransmission(m_i2c_address);
        Wire.write(cmd);
        return (Wire.endTransmission() == 0);
    }

    bool readLux() {
        if (Wire.requestFrom(m_i2c_address, (uint8_t)2) != 2) {
            return false;
        }

        uint16_t raw = (Wire.read() << 8) | Wire.read();
        m_lux = raw / 1.2f;  // Convert to lux
        return true;
    }

public:
    BH1750Sensor(const char* id, uint8_t address = 0x23, const char* name = nullptr)
        : m_id(id)
        , m_i2c_address(address)
        , m_lux(0.0f)
        , m_available(false)
    {
        m_name = name ? name : "Ambient Light";
    }

    bool setup() {
        ESP_LOGI("bh1750", "Initializing BH1750 at 0x%02X", m_i2c_address);

        // Power on
        if (!sendCommand(CMD_POWER_ON)) {
            ESP_LOGE("bh1750", "Failed to power on");
            return false;
        }

        delay(10);

        // Start continuous measurement
        if (!sendCommand(CMD_CONTINUOUS_HIGH_RES)) {
            ESP_LOGE("bh1750", "Failed to start measurement");
            return false;
        }

        delay(180);  // Wait for first measurement

        // Test read
        if (!readLux()) {
            ESP_LOGE("bh1750", "Failed to read lux value");
            return false;
        }

        m_available = true;
        ESP_LOGI("bh1750", "Initialized: %.1f lux", m_lux);
        return true;
    }

    void update() {
        if (!readLux()) {
            m_available = false;
        } else {
            m_available = true;
        }
    }

    // ISensor interface
    const char* getId() const override { return m_id.c_str(); }
    const char* getName() const override { return m_name.c_str(); }
    const char* getType() const override { return "illuminance"; }
    const char* getUnit() const override { return "lx"; }
    float getValue() override {
        update();
        return m_lux;
    }
    bool isAvailable() override { return m_available; }
    bool isVisible() const override { return true; }
    bool requiresAuth() const override { return false; }

    const char* getDeviceClass() const override {
        return "illuminance";
    }

    std::string toJson() const override {
        char buffer[256];
        snprintf(buffer, sizeof(buffer),
            "{\"id\":\"%s\",\"name\":\"%s\",\"type\":\"illuminance\","
            "\"value\":%.1f,\"unit\":\"lx\",\"available\":%s}",
            m_id.c_str(), m_name.c_str(), m_lux,
            m_available ? "true" : "false"
        );
        return std::string(buffer);
    }
};

} // namespace examples
} // namespace esphome_ui
```

5. **Create example YAML:**

```yaml
# File: examples/bh1750/bh1750_example.yaml

esphome:
  name: bh1750-example
  includes:
    - ../../framework/include/ui_framework.h
    - bh1750_sensor.h

  on_boot:
    priority: -10
    then:
      - lambda: |-
          auto framework = esphome_ui::UIFramework::getInstance();

          static auto* light_sensor = new esphome_ui::examples::BH1750Sensor(
            "bh1750_light",
            0x23,  // I2C address
            "Ambient Light"
          );

          if (light_sensor->setup()) {
            framework->registerComponent(light_sensor);
            ESP_LOGI("main", "✅ BH1750 sensor registered");
          }

# I2C configuration
i2c:
  sda: GPIO21
  scl: GPIO22
  scan: true
  frequency: 100kHz

# ... rest of config (wifi, esp32, etc.)
```

6. **Create documentation:**

```markdown
# File: examples/bh1750/README.md

# BH1750 Ambient Light Sensor Example

## Hardware Requirements
- BH1750 module (I2C)
- 4 wires for I2C connection

## Wiring
```
BH1750    ESP32
------    -----
VCC   →   3.3V
GND   →   GND
SDA   →   GPIO21
SCL   →   GPIO22
```

## Features
- Measures ambient light in lux
- I2C communication (address 0x23 or 0x5C)
- Auto-discovery in web UI
- Home Assistant compatible

## Usage
```bash
esphome compile bh1750_example.yaml
esphome run bh1750_example.yaml
```

Access web UI at http://10.10.50.100/
```

7. **Test:**
   - Compile
   - Upload
   - Verify in dashboard
   - Test value changes (cover sensor with hand)

**LLM completes task in ~15 minutes of actual work**

### Example 2: User Wants Email Notifications

**User prompt:**
> "When the relay turns on, send me an email notification."

**LLM reasoning:**

1. **Identify integration point:** Relay actuator state change
2. **Check ESPHome capabilities:** Has gmail SMTP support
3. **Design approach:**
   - Monitor relay state in interval
   - Detect ON transition
   - Send email via ESPHome action

**Implementation:**

```yaml
esphome:
  on_boot:
    then:
      - lambda: |-
          // Setup framework and relay as usual
          auto framework = esphome_ui::UIFramework::getInstance();
          static auto* relay = new esphome_ui::examples::RelayActuator(...);
          relay->setup();
          framework->registerComponent(relay);

          // Store last state globally
          static bool last_relay_state = false;

# Email configuration
mail:
  sender: "device@example.com"
  smtp_server: "smtp.gmail.com"
  smtp_port: 587
  username: !secret email_username
  password: !secret email_app_password
  recipients:
    - "user@example.com"

# Monitor relay state
interval:
  - interval: 1s
    then:
      - lambda: |-
          // Get relay from registry
          auto* relay = static_cast<esphome_ui::examples::RelayActuator*>(
            esphome_ui::UIFramework::getInstance()->getRegistry().get("relay1")
          );

          if (!relay) return;

          bool current_state = relay->isOn();

          // Detect OFF → ON transition
          if (current_state && !last_relay_state) {
            ESP_LOGI("notify", "Relay turned ON, sending email...");

            // Trigger email action
            id(mail_component).send({
              .subject = "Relay Notification",
              .body = "The relay has been turned ON."
            });
          }

          last_relay_state = current_state;
```

**Result:** Email sent automatically when relay turns on!

---

## Testing Extensions

### Unit Testing New Components

Create test file:

```cpp
// File: test/unit/test_bh1750.cpp

#include <gtest/gtest.h>
#include "../../examples/bh1750/bh1750_sensor.h"

class BH1750Test : public ::testing::Test {
protected:
    esphome_ui::examples::BH1750Sensor* sensor;

    void SetUp() override {
        sensor = new esphome_ui::examples::BH1750Sensor(
            "test_bh1750",
            0x23,
            "Test Light Sensor"
        );
    }

    void TearDown() override {
        delete sensor;
    }
};

TEST_F(BH1750Test, HasCorrectMetadata) {
    EXPECT_STREQ(sensor->getId(), "test_bh1750");
    EXPECT_STREQ(sensor->getName(), "Test Light Sensor");
    EXPECT_STREQ(sensor->getType(), "illuminance");
    EXPECT_STREQ(sensor->getUnit(), "lx");
}

TEST_F(BH1750Test, IsVisibleAndPublic) {
    EXPECT_TRUE(sensor->isVisible());
    EXPECT_FALSE(sensor->requiresAuth());
}

TEST_F(BH1750Test, SerializesToJson) {
    std::string json = sensor->toJson();

    EXPECT_NE(json.find("\"id\":\"test_bh1750\""), std::string::npos);
    EXPECT_NE(json.find("\"type\":\"illuminance\""), std::string::npos);
    EXPECT_NE(json.find("\"unit\":\"lx\""), std::string::npos);
}

TEST_F(BH1750Test, ValueInReasonableRange) {
    // Assuming setup succeeds in test environment
    if (sensor->setup()) {
        float lux = sensor->getValue();

        // Indoor light typically 100-1000 lux
        EXPECT_GE(lux, 0.0f);
        EXPECT_LE(lux, 100000.0f);  // Bright sunlight
    }
}

int main(int argc, char **argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
```

**Run tests:**
```bash
cd test/unit
pio test --filter test_bh1750
```

### Integration Testing

Add to `test/integration/test_compilation.sh`:

```bash
# Test BH1750 example compilation
./test_compilation.sh --example=bh1750
```

### Manual Testing Checklist

Add to `test/manual/testing-checklist.md`:

```markdown
### BH1750 Light Sensor

**Hardware Connection:**
- [ ] VCC → 3.3V
- [ ] GND → GND
- [ ] SDA → GPIO21
- [ ] SCL → GPIO22

**Functionality:**
- [ ] Sensor appears in dashboard
- [ ] Lux value updates every 5 seconds
- [ ] Value changes when covering sensor
- [ ] Value is realistic (indoor: 100-500 lux)

**API Testing:**
```bash
curl http://10.10.50.100/api/sensors | jq '.[] | select(.id=="bh1750_light")'
```

Expected:
```json
{
  "id": "bh1750_light",
  "name": "Ambient Light",
  "type": "illuminance",
  "value": 234.5,
  "unit": "lx",
  "available": true
}
```
```

---

## Pitfalls to Avoid

### Pitfall 1: Memory Leaks

**BAD:**
```cpp
std::string getData() {
    // New allocation every time!
    return std::string("data") + std::to_string(value);
}
```

**GOOD:**
```cpp
std::string getData() {
    char buffer[64];
    snprintf(buffer, sizeof(buffer), "data%d", value);
    return std::string(buffer);
}
```

### Pitfall 2: Blocking Operations

**BAD:**
```cpp
void update() {
    // Blocks entire ESP32!
    delay(5000);
    read_sensor();
}
```

**GOOD:**
```cpp
void update() {
    // Non-blocking
    if (millis() - last_update < 5000) return;
    read_sensor();
    last_update = millis();
}
```

### Pitfall 3: Missing Static Keyword

**BAD:**
```yaml
- lambda: |-
    // Component deleted when lambda exits!
    auto* sensor = new MySensor("id");
```

**GOOD:**
```yaml
- lambda: |-
    // Component persists
    static auto* sensor = new MySensor("id");
```

### Pitfall 4: Ignoring Errors

**BAD:**
```cpp
bool setup() {
    init_hardware();  // What if this fails?
    return true;
}
```

**GOOD:**
```cpp
bool setup() {
    if (!init_hardware()) {
        ESP_LOGE("sensor", "Hardware init failed");
        return false;
    }
    return true;
}
```

### Pitfall 5: Forgetting Null Checks

**BAD:**
```cpp
auto* component = registry.get("id");
component->doSomething();  // CRASH if nullptr!
```

**GOOD:**
```cpp
auto* component = registry.get("id");
if (component) {
    component->doSomething();
} else {
    ESP_LOGW("main", "Component 'id' not found");
}
```

---

## Validation Checklist

Before completing an extension task, verify:

**Code Quality:**
- [ ] Follows existing patterns (lifecycle, error handling, JSON)
- [ ] No memory leaks (use stack where possible)
- [ ] No blocking operations (delays, long loops)
- [ ] Proper error handling (null checks, return values)
- [ ] Static keyword for components in lambdas
- [ ] Comprehensive logging (ESP_LOGI/LOGW/LOGE)

**Functionality:**
- [ ] Compiles without errors: `esphome compile`
- [ ] Compiles without warnings
- [ ] Component appears in dashboard
- [ ] Values update correctly
- [ ] Error states handled (sensor disconnected, etc.)

**Documentation:**
- [ ] Code comments explain "why", not just "what"
- [ ] Public methods have docstrings
- [ ] Example YAML provided
- [ ] README with wiring diagram
- [ ] Troubleshooting tips included

**Testing:**
- [ ] Unit tests written (if applicable)
- [ ] Integration test added (compilation check)
- [ ] Manual test checklist item added
- [ ] All tests pass

**Integration:**
- [ ] Follows framework conventions
- [ ] Uses framework includes (not direct ESP-IDF)
- [ ] Registers with UIFramework correctly
- [ ] Works with existing dashboard
- [ ] Compatible with Home Assistant (device class, units)

**Performance:**
- [ ] RAM usage acceptable (check `esp_get_free_heap_size()`)
- [ ] No unnecessary allocations
- [ ] CPU usage reasonable
- [ ] Network traffic minimal

---

## Summary for LLMs

**When extending this framework:**

1. **Read first:** `component.h` → `registry.h` → example component
2. **Follow patterns:** Lifecycle, error handling, JSON serialization
3. **Use stack over heap:** Avoid `new` in loops, use stack buffers
4. **Check errors:** Never assume success, validate everything
5. **Log comprehensively:** Use ESP_LOGx for debugging
6. **Test thoroughly:** Unit + integration + manual testing
7. **Document well:** Comments, examples, troubleshooting

**The framework is designed to be LLM-friendly. If something seems unclear or magical, it's a bug in the documentation - not intentional!**

---

## For LLM Model Developers

**Training data considerations:**

This codebase demonstrates:
- Explicit over implicit patterns
- Self-documenting code structure
- Clear separation of concerns
- Consistent naming conventions
- Minimal abstraction layers
- Comprehensive inline documentation

**If training on this codebase, emphasize:**
- Pattern recognition (lifecycle, error handling)
- Resource awareness (memory constraints)
- Safety practices (null checks, error returns)
- Testing discipline (unit + integration)
- Documentation thoroughness

**This is a reference implementation of "LLM-friendly code".**

---

**Version:** 1.0.0-alpha
**Last Updated:** 2025-11-21
**License:** MIT
**For LLMs:** You are encouraged to learn from and extend this framework!
