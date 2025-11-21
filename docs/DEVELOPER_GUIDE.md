# ESPHome UI Framework - Developer Guide

This guide is for developers who want to extend the ESPHome UI Framework by creating custom components, modifying the framework, or integrating it into their projects.

## 📖 Table of Contents

1. [Introduction](#introduction)
2. [Architecture Deep Dive](#architecture-deep-dive)
3. [Creating Custom Sensors](#creating-custom-sensors)
4. [Creating Custom Actuators](#creating-custom-actuators)
5. [Modifying the Web UI](#modifying-the-web-ui)
6. [API Development](#api-development)
7. [Testing Your Code](#testing-your-code)
8. [Performance Optimization](#performance-optimization)
9. [Debugging Techniques](#debugging-techniques)
10. [Best Practices](#best-practices)

---

## Introduction

### Who This Guide Is For

This guide is for:
- **C++ developers** creating custom components
- **Web developers** customizing the UI
- **Framework contributors** adding new features
- **Advanced users** wanting deep customization

### Prerequisites

You should be familiar with:
- C++11 or later (lambda functions, smart pointers, move semantics)
- ESPHome and ESP-IDF framework basics
- YAML configuration syntax
- REST API design patterns
- JavaScript (ES6+) for UI customization

### Development Tools

**Recommended setup:**
```bash
# Code editor
VS Code with extensions:
  - C/C++
  - PlatformIO
  - ESPHome Helper

# Version control
git

# Testing
PlatformIO (for unit tests)
curl or Postman (for API testing)
Browser DevTools (for UI debugging)
```

---

## Architecture Deep Dive

### Component System

The framework uses a **component-based architecture** with pure virtual interfaces.

#### Interface Hierarchy

```
IComponent (base interface)
    ├── ISensor (read-only measurements)
    └── IActuator (controllable devices)
```

#### IComponent Interface

All components must implement:

```cpp
class IComponent {
public:
    virtual const char* getId() const = 0;
    virtual const char* getType() const = 0;
    virtual const char* getName() const = 0;
    virtual bool isVisible() const = 0;
    virtual bool requiresAuth() const = 0;
    virtual std::string toJson() const = 0;
};
```

**Design rationale:**
- **Pure virtual**: Maximum flexibility, no implementation lock-in
- **Const methods**: Thread-safe reads, clear intent
- **C strings**: Minimize memory allocations on ESP32
- **toJson()**: Self-serializing components, no external serializer needed

#### ISensor Interface

```cpp
class ISensor : public IComponent {
public:
    virtual float getValue() = 0;
    virtual const char* getUnit() const = 0;
    virtual bool isAvailable() = 0;

    // Optional Home Assistant integration
    virtual const char* getDeviceClass() const {
        return "measurement";
    }

    virtual uint8_t getAccuracy() const {
        return 2;  // decimal places
    }
};
```

**Key points:**
- `getValue()` non-const: May update cached value
- `isAvailable()` handles sensor failures gracefully
- Device class for Home Assistant compatibility
- Accuracy for display formatting

#### IActuator Interface

```cpp
class IActuator : public IComponent {
public:
    virtual bool setState(const std::string& json) = 0;
    virtual std::string getState() const = 0;
    virtual bool isAvailable() = 0;
};
```

**Key points:**
- JSON-based state for flexibility (complex states possible)
- Returns bool for error handling
- `getState()` for feedback/verification

### ComponentRegistry

Central registry using **pointer-based storage** for minimal overhead.

```cpp
class ComponentRegistry {
private:
    std::map<std::string, IComponent*> m_components;
    size_t m_sensor_count;
    size_t m_actuator_count;

public:
    bool registerComponent(IComponent* component);
    IComponent* get(const char* id) const;
    std::vector<ISensor*> getAllSensors() const;
    std::vector<IActuator*> getAllActuators() const;
    std::string toJson(bool visible_only = false) const;
};
```

**Design decisions:**
- **std::map**: O(1) lookups by ID
- **Pointers not ownership**: Components manage own lifetime
- **Type counters**: Fast statistics without iteration
- **Filtering**: Visible-only option for API

**Memory overhead:**
- ~40 bytes per component (map entry + counters)
- No copying of component data
- Components allocated once, never moved

### UIFramework Singleton

Main controller class:

```cpp
class UIFramework {
private:
    static UIFramework* s_instance;

    httpd_handle_t m_server;
    ComponentRegistry m_registry;
    AuthController m_auth;
    ConfigStorage m_config_storage;
    DeepSleepController m_deep_sleep;

public:
    static UIFramework* getInstance();
    bool setup();
    void registerHandlers();
    bool registerComponent(IComponent* component);

    ComponentRegistry& getRegistry() { return m_registry; }
    AuthController& getAuth() { return m_auth; }
    DeepSleepController& getDeepSleep() { return m_deep_sleep; }
};
```

**Singleton pattern rationale:**
- Global access from ESPHome lambdas
- Single source of truth
- Lifecycle management

### Data Flow

**Sensor reading flow:**
```
1. Hardware → Sensor::getValue()
2. Sensor caches value
3. Registry serializes on API request
4. HTTP handler returns JSON
5. Web UI parses and displays
```

**Actuator control flow:**
```
1. Web UI sends JSON
2. HTTP handler validates session
3. AuthController checks permissions
4. Registry looks up actuator
5. Actuator::setState() executes
6. Hardware updated
7. Response sent back
```

---

## Creating Custom Sensors

### Step-by-Step: Temperature Sensor

Let's create a complete custom temperature sensor from scratch.

#### Step 1: Define the Class

Create `custom_temp_sensor.h`:

```cpp
#pragma once

#include "../../framework/include/component.h"
#include <Arduino.h>

namespace esphome_ui {
namespace examples {

/**
 * @brief Custom temperature sensor implementation
 *
 * This example shows how to create a sensor that reads from
 * an analog pin and converts the voltage to temperature.
 */
class CustomTempSensor : public ISensor {
private:
    // Component identity
    std::string m_id;
    std::string m_name;

    // Hardware configuration
    uint8_t m_analog_pin;

    // Sensor state
    float m_temperature;
    bool m_available;

    // Calibration
    float m_voltage_offset;
    float m_temp_coefficient;

    /**
     * @brief Read raw voltage from ADC
     */
    float readVoltage() {
        int raw = analogRead(m_analog_pin);
        // ESP32 ADC: 12-bit, 0-3.3V
        return (raw / 4095.0f) * 3.3f;
    }

    /**
     * @brief Convert voltage to temperature
     *
     * For TMP36: Vout = (temp_C * 10mV) + 500mV
     * Temp_C = (Vout - 0.5) * 100
     */
    float voltageToTemperature(float voltage) {
        return (voltage - m_voltage_offset) * m_temp_coefficient;
    }

public:
    /**
     * @brief Constructor
     *
     * @param id Unique component ID
     * @param pin Analog pin number
     * @param name Display name
     */
    CustomTempSensor(const char* id, uint8_t pin, const char* name = nullptr)
        : m_id(id)
        , m_analog_pin(pin)
        , m_temperature(0.0f)
        , m_available(false)
        , m_voltage_offset(0.5f)      // TMP36: 500mV offset
        , m_temp_coefficient(100.0f)  // TMP36: 10mV per °C
    {
        if (name) {
            m_name = name;
        } else {
            m_name = "Temperature Sensor";
        }
    }

    /**
     * @brief Initialize the sensor
     *
     * @return true if initialization successful
     */
    bool setup() {
        ESP_LOGI("custom_temp", "Initializing sensor '%s' on pin %d",
                 m_id.c_str(), m_analog_pin);

        // Configure ADC
        pinMode(m_analog_pin, INPUT);
        analogSetAttenuation(ADC_11db);  // 0-3.3V range

        // Test read
        float voltage = readVoltage();

        if (voltage < 0.1f || voltage > 3.2f) {
            ESP_LOGE("custom_temp", "Sensor voltage out of range: %.2fV", voltage);
            m_available = false;
            return false;
        }

        m_temperature = voltageToTemperature(voltage);
        m_available = true;

        ESP_LOGI("custom_temp", "Sensor initialized: %.1f°C", m_temperature);
        return true;
    }

    /**
     * @brief Update sensor reading
     *
     * Call this periodically to update the cached value.
     * For better accuracy, average multiple readings.
     */
    void update() {
        if (!m_available) return;

        // Average 10 readings for noise reduction
        float sum = 0.0f;
        const int samples = 10;

        for (int i = 0; i < samples; i++) {
            sum += readVoltage();
            delay(1);  // Small delay between samples
        }

        float avg_voltage = sum / samples;
        m_temperature = voltageToTemperature(avg_voltage);

        ESP_LOGD("custom_temp", "Updated: %.1f°C (%.3fV)",
                 m_temperature, avg_voltage);
    }

    /**
     * @brief Set calibration parameters
     *
     * @param offset Voltage offset (V)
     * @param coefficient Temperature coefficient (°C/V)
     */
    void setCalibration(float offset, float coefficient) {
        m_voltage_offset = offset;
        m_temp_coefficient = coefficient;
        ESP_LOGI("custom_temp", "Calibration: offset=%.3fV, coeff=%.1f",
                 offset, coefficient);
    }

    // IComponent interface implementation
    const char* getId() const override {
        return m_id.c_str();
    }

    const char* getName() const override {
        return m_name.c_str();
    }

    const char* getType() const override {
        return "temperature";
    }

    bool isVisible() const override {
        return true;
    }

    bool requiresAuth() const override {
        return false;  // Sensors typically don't require auth
    }

    std::string toJson() const override {
        char buffer[256];
        snprintf(buffer, sizeof(buffer),
            "{"
            "\"id\":\"%s\","
            "\"name\":\"%s\","
            "\"type\":\"temperature\","
            "\"value\":%.1f,"
            "\"unit\":\"°C\","
            "\"available\":%s,"
            "\"visible\":true,"
            "\"requires_auth\":false"
            "}",
            m_id.c_str(),
            m_name.c_str(),
            m_temperature,
            m_available ? "true" : "false"
        );
        return std::string(buffer);
    }

    // ISensor interface implementation
    float getValue() override {
        // Optionally update on every getValue() call
        // update();

        return m_temperature;
    }

    const char* getUnit() const override {
        return "°C";
    }

    bool isAvailable() override {
        return m_available;
    }

    const char* getDeviceClass() const override {
        return "temperature";  // Home Assistant device class
    }

    uint8_t getAccuracy() const override {
        return 1;  // 1 decimal place
    }
};

} // namespace examples
} // namespace esphome_ui
```

#### Step 2: Register in ESPHome

Create `my_device.yaml`:

```yaml
esphome:
  name: my-device
  includes:
    - ../../framework/include/ui_framework.h
    - path/to/custom_temp_sensor.h

  on_boot:
    priority: -10
    then:
      - lambda: |-
          auto framework = esphome_ui::UIFramework::getInstance();

          // Create sensor instance
          static auto* temp_sensor = new esphome_ui::examples::CustomTempSensor(
            "custom_temp_1",  // ID
            34,               // GPIO34 (ADC1_CH6)
            "Custom Temperature"
          );

          // Optional: Custom calibration
          // temp_sensor->setCalibration(0.5, 100.0);

          // Initialize and register
          if (temp_sensor->setup()) {
            framework->registerComponent(temp_sensor);
            ESP_LOGI("main", "✅ Custom temperature sensor registered");
          } else {
            ESP_LOGE("main", "❌ Failed to initialize temperature sensor");
          }

# Update sensor every 5 seconds
interval:
  - interval: 5s
    then:
      - lambda: |-
          // Get sensor and update
          auto* sensor = static_cast<esphome_ui::examples::CustomTempSensor*>(
            esphome_ui::UIFramework::getInstance()->getRegistry().get("custom_temp_1")
          );

          if (sensor) {
            sensor->update();
          }
```

#### Step 3: Compile and Test

```bash
esphome compile my_device.yaml
esphome run my_device.yaml
```

**Verify:**
- Sensor appears in dashboard
- Value updates every 5 seconds
- Unit displayed correctly
- Available status accurate

### Advanced Sensor Features

#### Multi-measurement Sensor

Some sensors provide multiple values (e.g., BME280: temp, humidity, pressure).

**Strategy:**
Create separate sensor instances sharing the same hardware interface:

```cpp
class BME280Sensor : public ISensor {
private:
    enum MeasurementType { TEMPERATURE, HUMIDITY, PRESSURE };
    MeasurementType m_measurement_type;

    // Shared hardware instance (static)
    static BME280_Hardware* s_hardware;

public:
    BME280Sensor(const char* id, MeasurementType type, uint8_t addr)
        : m_measurement_type(type) {
        // All instances share same hardware
        if (!s_hardware) {
            s_hardware = new BME280_Hardware(addr);
        }
    }

    float getValue() override {
        switch (m_measurement_type) {
            case TEMPERATURE: return s_hardware->readTemperature();
            case HUMIDITY:    return s_hardware->readHumidity();
            case PRESSURE:    return s_hardware->readPressure();
        }
    }
};
```

**Register all measurements:**
```cpp
auto* temp = new BME280Sensor("bme_temp", TEMPERATURE, 0x76);
auto* hum  = new BME280Sensor("bme_hum", HUMIDITY, 0x76);
auto* pres = new BME280Sensor("bme_pres", PRESSURE, 0x76);

temp->setup();  // Initializes shared hardware once
framework->registerComponent(temp);
framework->registerComponent(hum);
framework->registerComponent(pres);
```

#### Error Handling

Always handle sensor failures gracefully:

```cpp
float getValue() override {
    try {
        float raw = readHardware();

        if (std::isnan(raw) || std::isinf(raw)) {
            ESP_LOGW("sensor", "Invalid reading");
            m_available = false;
            return m_last_good_value;  // Return cached value
        }

        m_last_good_value = raw;
        m_available = true;
        return raw;

    } catch (const std::exception& e) {
        ESP_LOGE("sensor", "Exception: %s", e.what());
        m_available = false;
        return 0.0f;
    }
}
```

#### Filtering and Smoothing

For noisy sensors, implement filtering:

```cpp
class MovingAverageFilter {
private:
    std::vector<float> m_buffer;
    size_t m_size;
    size_t m_index;

public:
    MovingAverageFilter(size_t size) : m_size(size), m_index(0) {
        m_buffer.resize(size, 0.0f);
    }

    float update(float value) {
        m_buffer[m_index] = value;
        m_index = (m_index + 1) % m_size;

        float sum = 0.0f;
        for (float v : m_buffer) sum += v;
        return sum / m_size;
    }
};

// Use in sensor:
class FilteredSensor : public ISensor {
private:
    MovingAverageFilter m_filter;

public:
    FilteredSensor() : m_filter(10) {}  // 10-sample average

    float getValue() override {
        float raw = readHardware();
        return m_filter.update(raw);
    }
};
```

---

## Creating Custom Actuators

### Step-by-Step: PWM Fan Controller

Let's create a fan controller with variable speed.

#### Step 1: Define the Class

Create `pwm_fan_controller.h`:

```cpp
#pragma once

#include "../../framework/include/component.h"
#include <Arduino.h>
#include <ArduinoJson.h>

namespace esphome_ui {
namespace examples {

/**
 * @brief PWM-controlled fan actuator
 *
 * Controls fan speed using PWM (0-100%).
 * Supports safety features like minimum speed and overheat protection.
 */
class PWMFanController : public IActuator {
private:
    std::string m_id;
    std::string m_name;

    // Hardware
    uint8_t m_pwm_pin;
    uint8_t m_pwm_channel;

    // State
    uint8_t m_speed_percent;  // 0-100
    bool m_running;
    bool m_available;

    // Safety
    uint8_t m_min_speed;  // Minimum speed to prevent stall
    uint32_t m_ramp_time_ms;  // Gradual speed changes

    /**
     * @brief Set PWM duty cycle
     */
    void setPWM(uint8_t percent) {
        // ESP32 PWM: 0-255 duty cycle
        uint8_t duty = map(percent, 0, 100, 0, 255);
        ledcWrite(m_pwm_channel, duty);
    }

    /**
     * @brief Ramp speed gradually to target
     */
    void rampToSpeed(uint8_t target_percent) {
        if (m_ramp_time_ms == 0) {
            // Immediate change
            m_speed_percent = target_percent;
            setPWM(target_percent);
            return;
        }

        // Gradual ramp
        int8_t direction = (target_percent > m_speed_percent) ? 1 : -1;
        uint32_t delay_ms = m_ramp_time_ms / abs(target_percent - m_speed_percent);

        while (m_speed_percent != target_percent) {
            m_speed_percent += direction;
            setPWM(m_speed_percent);
            delay(delay_ms);
        }
    }

public:
    PWMFanController(const char* id, uint8_t pin, const char* name = nullptr)
        : m_id(id)
        , m_pwm_pin(pin)
        , m_pwm_channel(0)
        , m_speed_percent(0)
        , m_running(false)
        , m_available(false)
        , m_min_speed(30)  // 30% minimum to prevent stall
        , m_ramp_time_ms(2000)  // 2 second ramp
    {
        m_name = name ? name : "PWM Fan";
    }

    /**
     * @brief Initialize PWM hardware
     */
    bool setup() {
        ESP_LOGI("pwm_fan", "Initializing fan '%s' on pin %d",
                 m_id.c_str(), m_pwm_pin);

        // Configure PWM
        // frequency: 25kHz, resolution: 8-bit
        ledcSetup(m_pwm_channel, 25000, 8);
        ledcAttachPin(m_pwm_pin, m_pwm_channel);

        // Start at 0%
        setPWM(0);

        m_available = true;
        ESP_LOGI("pwm_fan", "Fan initialized successfully");
        return true;
    }

    /**
     * @brief Set minimum safe speed
     */
    void setMinSpeed(uint8_t percent) {
        if (percent > 100) percent = 100;
        m_min_speed = percent;
        ESP_LOGI("pwm_fan", "Minimum speed set to %d%%", percent);
    }

    /**
     * @brief Set ramp time for smooth speed changes
     */
    void setRampTime(uint32_t ms) {
        m_ramp_time_ms = ms;
    }

    // IComponent interface
    const char* getId() const override { return m_id.c_str(); }
    const char* getName() const override { return m_name.c_str(); }
    const char* getType() const override { return "fan"; }
    bool isVisible() const override { return true; }
    bool requiresAuth() const override { return true; }

    std::string toJson() const override {
        char buffer[256];
        snprintf(buffer, sizeof(buffer),
            "{"
            "\"id\":\"%s\","
            "\"name\":\"%s\","
            "\"type\":\"fan\","
            "\"running\":%s,"
            "\"speed\":%d,"
            "\"available\":%s,"
            "\"visible\":true,"
            "\"requires_auth\":true"
            "}",
            m_id.c_str(),
            m_name.c_str(),
            m_running ? "true" : "false",
            m_speed_percent,
            m_available ? "true" : "false"
        );
        return std::string(buffer);
    }

    // IActuator interface
    bool setState(const std::string& json) override {
        if (!m_available) {
            ESP_LOGE("pwm_fan", "Fan not available");
            return false;
        }

        // Parse JSON
        StaticJsonDocument<256> doc;
        DeserializationError error = deserializeJson(doc, json);

        if (error) {
            ESP_LOGE("pwm_fan", "JSON parse error: %s", error.c_str());
            return false;
        }

        // Handle commands
        if (doc.containsKey("speed")) {
            int speed = doc["speed"];

            if (speed < 0 || speed > 100) {
                ESP_LOGW("pwm_fan", "Speed out of range: %d", speed);
                return false;
            }

            if (speed > 0 && speed < m_min_speed) {
                ESP_LOGW("pwm_fan", "Speed below minimum, using %d%%", m_min_speed);
                speed = m_min_speed;
            }

            rampToSpeed(speed);
            m_running = (speed > 0);

            ESP_LOGI("pwm_fan", "Fan speed set to %d%%", speed);
            return true;
        }

        if (doc.containsKey("state")) {
            const char* state = doc["state"];

            if (strcmp(state, "on") == 0) {
                rampToSpeed(100);  // Full speed
                m_running = true;
                return true;
            } else if (strcmp(state, "off") == 0) {
                rampToSpeed(0);
                m_running = false;
                return true;
            }
        }

        ESP_LOGW("pwm_fan", "Unknown command: %s", json.c_str());
        return false;
    }

    std::string getState() const override {
        char buffer[128];
        snprintf(buffer, sizeof(buffer),
            "{\"running\":%s,\"speed\":%d}",
            m_running ? "true" : "false",
            m_speed_percent
        );
        return std::string(buffer);
    }

    bool isAvailable() override {
        return m_available;
    }

    /**
     * @brief Get current speed
     */
    uint8_t getSpeed() const {
        return m_speed_percent;
    }

    /**
     * @brief Check if fan is running
     */
    bool isRunning() const {
        return m_running;
    }
};

} // namespace examples
} // namespace esphome_ui
```

#### Step 2: Register and Use

```yaml
esphome:
  includes:
    - ../../framework/include/ui_framework.h
    - path/to/pwm_fan_controller.h

  on_boot:
    then:
      - lambda: |-
          auto framework = esphome_ui::UIFramework::getInstance();

          static auto* fan = new esphome_ui::examples::PWMFanController(
            "pwm_fan_1",
            19,  // GPIO19
            "Cooling Fan"
          );

          // Configure safety features
          fan->setMinSpeed(30);       // 30% minimum
          fan->setRampTime(2000);     // 2s ramp

          if (fan->setup()) {
            framework->registerComponent(fan);
            ESP_LOGI("main", "✅ PWM fan registered");
          }
```

#### Step 3: Control via API

```bash
# Set speed to 75%
curl -X POST http://10.10.50.100/api/actuators/pwm_fan_1 \
  -H "Content-Type: application/json" \
  -H "Cookie: session_id=YOUR_SESSION" \
  -d '{"speed":75}'

# Turn on (100%)
curl -X POST http://10.10.50.100/api/actuators/pwm_fan_1 \
  -H "Content-Type: application/json" \
  -H "Cookie: session_id=YOUR_SESSION" \
  -d '{"state":"on"}'

# Turn off (0%)
curl -X POST http://10.10.50.100/api/actuators/pwm_fan_1 \
  -H "Content-Type: application/json" \
  -H "Cookie: session_id=YOUR_SESSION" \
  -d '{"state":"off"}'
```

### Complex Actuator Example: RGB LED Strip

```cpp
class RGBLEDController : public IActuator {
private:
    uint8_t m_r_pin, m_g_pin, m_b_pin;
    uint8_t m_r_val, m_g_val, m_b_val;

public:
    bool setState(const std::string& json) override {
        StaticJsonDocument<256> doc;
        deserializeJson(doc, json);

        if (doc.containsKey("rgb")) {
            // Hex color: {"rgb": "#FF0000"}
            const char* hex = doc["rgb"];
            parseHexColor(hex, m_r_val, m_g_val, m_b_val);
        } else {
            // Individual channels: {"r": 255, "g": 0, "b": 0}
            m_r_val = doc["r"] | m_r_val;
            m_g_val = doc["g"] | m_g_val;
            m_b_val = doc["b"] | m_b_val;
        }

        // Apply to hardware
        analogWrite(m_r_pin, m_r_val);
        analogWrite(m_g_pin, m_g_val);
        analogWrite(m_b_pin, m_b_val);

        return true;
    }

    std::string getState() const override {
        char buffer[128];
        snprintf(buffer, sizeof(buffer),
            "{\"r\":%d,\"g\":%d,\"b\":%d,\"hex\":\"#%02X%02X%02X\"}",
            m_r_val, m_g_val, m_b_val,
            m_r_val, m_g_val, m_b_val
        );
        return std::string(buffer);
    }
};
```

---

## Modifying the Web UI

The web UI is pure vanilla JavaScript - no build tools required!

### File Structure

```
web/
├── index.html          # Main HTML shell
├── css/
│   ├── themes.css      # Color themes (dark/light)
│   └── main.css        # Layout and components
├── js/
│   ├── api.js          # REST API client
│   ├── i18n.js         # Internationalization
│   ├── app.js          # Main application logic
│   └── charts.js       # Chart functionality
└── lang/
    ├── en.json         # English translations
    └── de.json         # German translations
```

### Adding a New Page

#### Step 1: Add Route Handler

Edit `web/js/app.js`:

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
        case '/about':
            this.showAbout();
            break;
        case '/custom':  // NEW PAGE
            this.showCustomPage();
            break;
        default:
            this.show404();
    }
}
```

#### Step 2: Implement Page Logic

```javascript
showCustomPage() {
    const content = document.getElementById('content');

    content.innerHTML = `
        <div class="page-header">
            <h1>${this.i18n.t('custom.title')}</h1>
        </div>

        <div class="card">
            <h2>${this.i18n.t('custom.subtitle')}</h2>
            <p>${this.i18n.t('custom.description')}</p>

            <button id="customButton" class="btn btn-primary">
                ${this.i18n.t('custom.button')}
            </button>
        </div>
    `;

    // Attach event listeners
    document.getElementById('customButton').addEventListener('click', () => {
        this.handleCustomAction();
    });
}

async handleCustomAction() {
    try {
        const result = await this.api.get('/api/custom/endpoint');
        console.log('Result:', result);
        // Update UI with result
    } catch (error) {
        console.error('Error:', error);
        this.showError(error.message);
    }
}
```

#### Step 3: Add Translations

Edit `web/lang/en.json`:

```json
{
  "custom": {
    "title": "Custom Page",
    "subtitle": "Your custom functionality",
    "description": "This is a custom page you added!",
    "button": "Do Something"
  }
}
```

Edit `web/lang/de.json`:

```json
{
  "custom": {
    "title": "Benutzerdefinierte Seite",
    "subtitle": "Ihre benutzerdefinierte Funktionalität",
    "description": "Dies ist eine benutzerdefinierte Seite, die Sie hinzugefügt haben!",
    "button": "Etwas tun"
  }
}
```

#### Step 4: Add Navigation Link

Edit `web/index.html`:

```html
<nav class="nav">
    <a href="#/dashboard">Dashboard</a>
    <a href="#/settings">Settings</a>
    <a href="#/custom">Custom</a>  <!-- NEW LINK -->
    <a href="#/about">About</a>
</nav>
```

### Customizing Styles

#### Changing Theme Colors

Edit `web/css/themes.css`:

```css
.theme-dark {
    /* Primary colors */
    --bg-primary: #0f172a;      /* Dark blue-gray */
    --bg-secondary: #1e293b;
    --text-primary: #f1f5f9;

    /* Accent colors */
    --accent-primary: #3b82f6;  /* Blue */
    --accent-secondary: #8b5cf6; /* Purple */

    /* Status colors */
    --status-success: #10b981;  /* Green */
    --status-warning: #f59e0b;  /* Orange */
    --status-error: #ef4444;    /* Red */
    --status-info: #3b82f6;     /* Blue */
}
```

Want a **green theme**? Change `--accent-primary` to a green:

```css
--accent-primary: #10b981;  /* Green instead of blue */
```

#### Creating a Custom Component Style

Edit `web/css/main.css`:

```css
/* Custom card style */
.card-custom {
    background: linear-gradient(135deg, var(--accent-primary), var(--accent-secondary));
    color: white;
    border: none;
    box-shadow: 0 10px 30px rgba(0, 0, 0, 0.3);
}

.card-custom h2 {
    color: white;
    text-shadow: 0 2px 4px rgba(0, 0, 0, 0.2);
}

.card-custom .value {
    font-size: 3rem;
    font-weight: bold;
    text-shadow: 0 2px 8px rgba(0, 0, 0, 0.3);
}
```

Use in JavaScript:

```javascript
content.innerHTML = `
    <div class="card card-custom">
        <h2>Temperature</h2>
        <div class="value">23.5°C</div>
    </div>
`;
```

### Adding Custom API Endpoints

#### Step 1: Add Handler in C++

Edit your YAML:

```yaml
esphome:
  on_boot:
    then:
      - lambda: |-
          // Register custom endpoint
          httpd_uri_t custom_uri = {
              .uri       = "/api/custom/data",
              .method    = HTTP_GET,
              .handler   = [](httpd_req_t *req) -> esp_err_t {
                  // Your custom logic
                  const char* json = "{\"custom\":\"data\",\"value\":42}";
                  httpd_resp_set_type(req, "application/json");
                  httpd_resp_send(req, json, strlen(json));
                  return ESP_OK;
              },
              .user_ctx  = nullptr
          };

          httpd_register_uri_handler(
              esphome_ui::UIFramework::getInstance()->getServer(),
              &custom_uri
          );
```

#### Step 2: Call from JavaScript

```javascript
async fetchCustomData() {
    const data = await this.api.get('/api/custom/data');
    console.log('Custom data:', data);
    return data;
}
```

---

## API Development

### REST API Endpoints

Current endpoints:

| Method | Endpoint | Auth Required | Description |
|--------|----------|---------------|-------------|
| GET | `/api/status` | No | Device status |
| GET | `/api/components` | No | All components |
| GET | `/api/sensors` | No | Sensor values |
| GET | `/api/actuators` | No | Actuator states |
| POST | `/api/auth/login` | No | Login |
| POST | `/api/auth/logout` | Yes | Logout |
| POST | `/api/actuators/:id` | Yes | Control actuator |

### Adding a New Endpoint

Want to add `/api/diagnostics` endpoint?

```cpp
// In on_boot lambda:
httpd_uri_t diagnostics_uri = {
    .uri       = "/api/diagnostics",
    .method    = HTTP_GET,
    .handler   = diagnosticsHandler,
    .user_ctx  = framework
};

httpd_register_uri_handler(server, &diagnostics_uri);

// Handler function:
esp_err_t diagnosticsHandler(httpd_req_t *req) {
    auto* framework = static_cast<UIFramework*>(req->user_ctx);

    // Gather diagnostics
    char buffer[1024];
    snprintf(buffer, sizeof(buffer),
        "{"
        "\"free_heap\":%u,"
        "\"uptime\":%u,"
        "\"wifi_rssi\":%d,"
        "\"component_count\":%zu,"
        "\"active_sessions\":%u"
        "}",
        esp_get_free_heap_size(),
        millis() / 1000,
        WiFi.RSSI(),
        framework->getRegistry().count(),
        framework->getAuth().getActiveSessionCount()
    );

    httpd_resp_set_type(req, "application/json");
    httpd_resp_send(req, buffer, strlen(buffer));

    return ESP_OK;
}
```

### Authentication Middleware

For protected endpoints:

```cpp
esp_err_t protectedHandler(httpd_req_t *req) {
    auto* framework = static_cast<UIFramework*>(req->user_ctx);

    // Extract session cookie
    char cookie[256] = {0};
    if (httpd_req_get_hdr_value_str(req, "Cookie", cookie, sizeof(cookie)) != ESP_OK) {
        httpd_resp_send_err(req, HTTPD_401_UNAUTHORIZED, "No session");
        return ESP_FAIL;
    }

    // Parse session_id
    char* session_id = strstr(cookie, "session_id=");
    if (!session_id) {
        httpd_resp_send_err(req, HTTPD_401_UNAUTHORIZED, "Invalid session");
        return ESP_FAIL;
    }
    session_id += 11;  // Skip "session_id="

    // Validate session
    if (!framework->getAuth().validateSession(session_id)) {
        httpd_resp_send_err(req, HTTPD_401_UNAUTHORIZED, "Session invalid");
        return ESP_FAIL;
    }

    // Session valid, proceed with protected logic
    // ...

    return ESP_OK;
}
```

---

## Testing Your Code

### Unit Testing

Framework uses Google Test. See `test/unit/` directory.

**Create a test:**

```cpp
// test/unit/test_my_sensor.cpp

#include <gtest/gtest.h>
#include "../../path/to/my_sensor.h"

class MySensorTest : public ::testing::Test {
protected:
    MySensor* sensor;

    void SetUp() override {
        sensor = new MySensor("test_sensor", 34, "Test");
    }

    void TearDown() override {
        delete sensor;
    }
};

TEST_F(MySensorTest, InitializesCorrectly) {
    EXPECT_TRUE(sensor->setup());
    EXPECT_TRUE(sensor->isAvailable());
}

TEST_F(MySensorTest, ReadsValueInRange) {
    sensor->setup();
    float value = sensor->getValue();

    EXPECT_GE(value, -40.0f);  // Minimum
    EXPECT_LE(value, 125.0f);  // Maximum
}

TEST_F(MySensorTest, HandlesInvalidPin) {
    MySensor* bad_sensor = new MySensor("bad", 255, "Bad");
    EXPECT_FALSE(bad_sensor->setup());
    delete bad_sensor;
}

int main(int argc, char **argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
```

**Run tests:**

```bash
cd test/unit
pio test -e native
```

### Integration Testing

See `test/integration/test_compilation.sh`:

```bash
cd test/integration
./test_compilation.sh
```

### Manual Testing

See `test/manual/testing-checklist.md` for comprehensive manual testing procedures.

---

## Performance Optimization

### Memory Management

**ESP32 constraints:**
- SRAM: ~320KB (shared between stack, heap, WiFi)
- Flash: 4MB typical (code + data + OTA)

**Optimization tips:**

1. **Use const char* instead of std::string** where possible:
```cpp
// Bad - heap allocation
std::string name = "Sensor Name";

// Good - in flash memory
const char* name = "Sensor Name";
```

2. **Minimize dynamic allocations:**
```cpp
// Bad - allocates every time
std::string toJson() const {
    std::string result = "{\"id\":\"";
    result += m_id;
    result += "\"}";
    return result;
}

// Good - stack buffer
std::string toJson() const {
    char buffer[256];
    snprintf(buffer, sizeof(buffer), "{\"id\":\"%s\"}", m_id.c_str());
    return std::string(buffer);
}
```

3. **Monitor heap usage:**
```cpp
ESP_LOGI("mem", "Free heap: %u bytes", esp_get_free_heap_size());
ESP_LOGI("mem", "Largest free block: %u bytes", heap_caps_get_largest_free_block(MALLOC_CAP_8BIT));
```

### CPU Optimization

1. **Avoid blocking operations:**
```cpp
// Bad - blocks for 1 second
delay(1000);

// Good - yield to other tasks
vTaskDelay(pdMS_TO_TICKS(1000));
```

2. **Use efficient data structures:**
```cpp
// Bad - O(n) lookup
std::vector<IComponent*> components;

// Good - O(1) lookup
std::map<std::string, IComponent*> components;
```

### Network Optimization

1. **Batch sensor readings:**
```cpp
// Bad - separate requests
GET /api/sensors/temp1
GET /api/sensors/temp2
GET /api/sensors/humidity

// Good - single request
GET /api/sensors
```

2. **Use compression (future enhancement):**
```cpp
httpd_resp_set_hdr(req, "Content-Encoding", "gzip");
```

---

## Debugging Techniques

### Serial Logging

**Log levels:**
```cpp
ESP_LOGE("tag", "Error: %s", error);     // Red
ESP_LOGW("tag", "Warning: %d", value);   // Yellow
ESP_LOGI("tag", "Info: %s", message);    // Default
ESP_LOGD("tag", "Debug: %f", float_val); // Verbose
ESP_LOGV("tag", "Verbose details");      // Very verbose
```

**Configure log levels in YAML:**
```yaml
logger:
  level: DEBUG
  logs:
    component: DEBUG
    auth: INFO
    wifi: WARN
```

### GDB Debugging (ESP32)

**Enable debugging:**
```yaml
esphome:
  platformio_options:
    build_flags:
      - -DDEBUG_ESP_PORT=Serial
      - -DDEBUG_ESP_CORE
```

**Connect GDB:**
```bash
pio debug
```

### Memory Debugging

**Stack overflow detection:**
```cpp
void myFunction() {
    // Check stack usage
    UBaseType_t stackLeft = uxTaskGetStackHighWaterMark(NULL);
    ESP_LOGI("debug", "Stack remaining: %u bytes", stackLeft * sizeof(StackType_t));
}
```

**Heap corruption detection:**
```cpp
if (!heap_caps_check_integrity_all(true)) {
    ESP_LOGE("mem", "Heap corruption detected!");
}
```

### Web UI Debugging

**Browser DevTools:**
```javascript
// Add debug logging
console.log('Sensor data:', data);
console.table(sensors);

// Breakpoints
debugger;

// Performance monitoring
console.time('fetchSensors');
await this.api.getSensors();
console.timeEnd('fetchSensors');
```

---

## Best Practices

### Code Organization

1. **One class per file**
2. **Clear include guards**
3. **Namespace organization**
4. **Comprehensive comments**

### Error Handling

Always handle errors gracefully:

```cpp
// Check return values
if (!sensor->setup()) {
    ESP_LOGE("main", "Sensor setup failed");
    // Don't register failed sensors
    return;
}

// Validate inputs
if (value < 0 || value > 100) {
    ESP_LOGW("sensor", "Value out of range: %.1f", value);
    return last_valid_value;
}

// Handle exceptions (if used)
try {
    risky_operation();
} catch (const std::exception& e) {
    ESP_LOGE("error", "Exception: %s", e.what());
}
```

### Documentation

**Comment thoroughly:**

```cpp
/**
 * @brief Convert raw ADC value to temperature
 *
 * Uses linear approximation: T = (ADC - offset) * scale
 *
 * @param adc_value Raw ADC reading (0-4095)
 * @return float Temperature in Celsius
 *
 * @note Assumes ADC configured for 0-3.3V range
 * @warning Not accurate below -10°C or above 50°C
 */
float adcToTemperature(uint16_t adc_value);
```

### Version Control

**Git best practices:**

```bash
# Feature branches
git checkout -b feature/my-sensor

# Descriptive commits
git commit -m "Add support for DHT22 temperature sensor"

# Keep commits atomic
git add -p  # Interactive staging
```

---

**Ready to extend the framework?** Check out the [LLM Extension Guide](LLM_EXTENSION_GUIDE.md) for AI-assisted development tips!

---

**Version:** 1.0.0-alpha
**Last Updated:** 2025-11-21
**License:** MIT
