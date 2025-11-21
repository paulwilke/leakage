# ESPHome UI Framework - Examples

This directory contains working examples demonstrating how to use the UI Framework with various sensors and actuators.

## 📁 Directory Structure

```
examples/
├── bme280/          # BME280 environmental sensor (I2C)
├── ds18b20/         # DS18B20 temperature sensor (OneWire)
├── relay/           # Relay actuator (GPIO)
├── demo/            # Complete demo with all components
└── README.md        # This file
```

## 🚀 Quick Start

### 1. Complete Demo (Recommended)

The **demo/** directory contains a fully configured example with all components:

```bash
cd examples/demo
esphome compile demo.yaml
esphome run demo.yaml
```

**Hardware Requirements:**
- ESP32 board
- BME280 sensor (I2C, 0x76)
- DS18B20 sensor (OneWire, GPIO4)
- Relay module (GPIO23)
- 4.7kΩ resistor for DS18B20

**Access:**
- Web UI: `http://10.10.50.100/`
- Login: `admin` / `admin` (change immediately!)

### 2. Individual Examples

Each example can be used independently:

#### BME280 Environmental Sensor
```bash
cd examples/bme280
esphome compile bme280_example.yaml
```

**Features:**
- Temperature (°C)
- Relative humidity (%)
- Atmospheric pressure (hPa)

**Wiring:**
- SDA → GPIO21
- SCL → GPIO22
- VCC → 3.3V
- GND → GND

#### DS18B20 Temperature Sensor
```bash
cd examples/ds18b20
esphome compile ds18b20_example.yaml
```

**Features:**
- High-precision temperature measurement
- Multiple sensors on one bus
- Unique 64-bit addressing

**Wiring:**
- Data → GPIO4 (with 4.7kΩ pull-up to VCC)
- VCC → 3.3V or 5V
- GND → GND

#### Relay Actuator
```bash
cd examples/relay
esphome compile relay_example.yaml
```

**Features:**
- ON/OFF control
- Safety timeout (auto-off)
- State feedback
- Web UI control

**Wiring:**
- Signal → GPIO23
- VCC → 5V (for relay coil)
- GND → GND

## 📝 Configuration

### Creating secrets.yaml

All examples require a `secrets.yaml` file:

```yaml
wifi_ssid: "YourWiFiName"
wifi_password: "YourWiFiPassword"
api_key: "your_32_character_api_key"
ota_password: "your_ota_password"
```

**Generate API Key:**
```bash
python -c "import secrets; print(secrets.token_urlsafe(32))"
```

### Customizing Examples

1. **Change GPIO Pins:**
   Edit the `lambda` section in the YAML file:
   ```yaml
   static BME280Sensor* sensor = new BME280Sensor(
     "sensor_id",
     BME280Sensor::TEMPERATURE,
     0x76,  // I2C address
     "Sensor Name"
   );
   ```

2. **Adjust I2C Address:**
   BME280 sensors typically use `0x76` or `0x77`:
   ```cpp
   0x76  // Most common
   0x77  // Alternative address
   ```

3. **Change Device IP:**
   In the `wifi:` section:
   ```yaml
   manual_ip:
     static_ip: 10.10.50.100  # Change this
     gateway: 10.10.50.1
     subnet: 255.255.255.0
   ```

## 🔧 Adding Your Own Components

### Step 1: Create Component Class

Inherit from `ISensor` or `IActuator`:

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
        // Initialize hardware
        return true;
    }

    // Implement ISensor interface
    const char* getId() const override { return m_id.c_str(); }
    const char* getName() const override { return m_name.c_str(); }
    const char* getType() const override { return "custom"; }
    const char* getUnit() const override { return "unit"; }
    float getValue() override { return m_value; }
    bool isAvailable() override { return true; }
    bool isVisible() const override { return true; }
    bool requiresAuth() const override { return false; }
};

} // namespace examples
} // namespace esphome_ui
```

### Step 2: Register in ESPHome YAML

```yaml
esphome:
  includes:
    - ../../framework/include/ui_framework.h
    - path/to/my_sensor.h

  on_boot:
    priority: -10
    then:
      - lambda: |-
          auto framework = esphome_ui::UIFramework::getInstance();

          static auto* sensor = new esphome_ui::examples::MySensor(
            "my_sensor_id",
            "My Sensor Name"
          );

          if (sensor->setup()) {
            framework->registerComponent(sensor);
          }
```

### Step 3: Component Appears Automatically

Once registered, your component:
- ✅ Appears in web UI dashboard
- ✅ Available via REST API (`/api/sensors` or `/api/actuators`)
- ✅ Serialized to JSON automatically
- ✅ Updates every 5 seconds (configurable)

## 🧪 Testing

### 1. Check Compilation

```bash
esphome compile your_example.yaml
```

### 2. Monitor Logs

```bash
esphome logs your_example.yaml --device <IP_ADDRESS>
```

### 3. Test API Endpoints

```bash
# Get device status
curl http://10.10.50.100/api/status

# Get all components
curl http://10.10.50.100/api/components

# Get sensor values
curl http://10.10.50.100/api/sensors

# Control relay (requires authentication)
curl -X POST http://10.10.50.100/api/actuators/relay1 \
  -H "Content-Type: application/x-www-form-urlencoded" \
  -d "state={\"state\":\"on\"}"
```

### 4. Check Web UI

1. Open `http://10.10.50.100/` in browser
2. Verify sensors display values
3. Test language switching (EN ↔ DE)
4. Login and test actuator control

## 📊 Component Features Comparison

| Feature | BME280 | DS18B20 | Relay |
|---------|--------|---------|-------|
| **Type** | Sensor | Sensor | Actuator |
| **Interface** | I2C | OneWire | GPIO |
| **Measurements** | 3 (T/H/P) | 1 (T) | N/A |
| **Multi-device** | Yes (different addresses) | Yes (unique IDs) | N/A |
| **Resolution** | Fixed | 9-12 bit | N/A |
| **Auth Required** | No | No | Yes |
| **Safety Features** | N/A | N/A | Timeout |

## 🐛 Troubleshooting

### BME280 Not Found

**Problem:** `BME280 not found at 0x76`

**Solutions:**
1. Check I2C wiring (SDA, SCL)
2. Try alternative address `0x77`
3. Run I2C scan: Look for device in logs
4. Verify 3.3V power supply
5. Check pull-up resistors (often built-in)

### DS18B20 No Response

**Problem:** `No OneWire devices found`

**Solutions:**
1. Check wiring (Data, VCC, GND)
2. **Essential:** Add 4.7kΩ pull-up resistor (Data → VCC)
3. Try different GPIO pin
4. Test sensor with Arduino/Multimeter
5. Check for short circuits

### Relay Not Switching

**Problem:** Relay doesn't respond to commands

**Solutions:**
1. Check GPIO pin number in code
2. Verify relay module power supply (usually 5V)
3. Try `inverted: true` for active-low modules
4. Check relay LED indicator
5. Measure voltage on signal pin with multimeter
6. Test relay manually (connect signal to VCC/GND)

### Web UI Not Loading

**Problem:** Cannot access web interface

**Solutions:**
1. Check device IP: Look in serial logs
2. Ping device: `ping 10.10.50.100`
3. Verify WiFi connection in logs
4. Try fallback hotspot: "UI Framework Demo"
5. Check firewall settings
6. Access via: `http://IP/` (not `https://`)

### Compilation Errors

**Problem:** ESPHome fails to compile

**Solutions:**
1. Check ESPHome version: `esphome version`
2. Update: `pip install --upgrade esphome`
3. Verify include paths are correct
4. Check for typos in YAML
5. Clear build cache: `rm -rf .esphome/`
6. Check ESP-IDF framework selected

## 💡 Best Practices

### 1. Start Simple
- Begin with one sensor/actuator
- Test individually before combining
- Use the demo as reference

### 2. Hardware First
- Test sensors with Arduino sketches first
- Verify wiring with multimeter
- Check I2C/OneWire bus with logic analyzer (if available)

### 3. Logging
- Enable debug logs during development
- Monitor serial output
- Check for initialization errors

### 4. Safety
- **Always** change default password
- Set relay safety timeouts
- Test emergency shutoff
- Use appropriate wire gauge
- Follow electrical safety guidelines

### 5. Documentation
- Comment your customizations
- Keep wiring diagrams
- Document GPIO pin assignments
- Note sensor addresses

## 🔗 Related Documentation

- [Framework Architecture](../../docs/ARCHITECTURE.md)
- [Component Interface](../../framework/include/component.h)
- [UI Framework Class](../../framework/include/ui_framework.h)
- [Developer Guide](../../docs/DEVELOPER_GUIDE.md) *(coming soon)*
- [LLM Extension Guide](../../docs/LLM_EXTENSION_GUIDE.md) *(coming soon)*

## 📞 Support

**Issues:**
- Check logs first: `esphome logs <config>.yaml`
- Verify hardware connections
- Test components individually
- Review troubleshooting section above

**Common Questions:**
- Q: Can I use different GPIO pins?
  - A: Yes! Just change pin numbers in YAML

- Q: How many sensors can I add?
  - A: Limited by ESP32 memory (~10-20 typical)

- Q: Does it work with ESP8266?
  - A: Framework requires ESP32 (ESP-IDF)

- Q: Can I add custom sensors?
  - A: Yes! See "Adding Your Own Components" above

## 📄 License

MIT License - See main project LICENSE file

---

**Version:** 1.0.0-alpha
**Last Updated:** 2025-11-20
**Maintained by:** ESPHome UI Framework Team
