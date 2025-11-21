#pragma once

#include "../../framework/include/component.h"
#include "esphome.h"
#include <OneWire.h>

namespace esphome_ui {
namespace examples {

/**
 * @brief DS18B20 OneWire Temperature Sensor
 *
 * Provides temperature readings from a Dallas DS18B20 digital temperature
 * sensor via OneWire protocol.
 *
 * Features:
 * - Temperature range: -55°C to +125°C
 * - Accuracy: ±0.5°C (-10°C to +85°C)
 * - 9-12 bit resolution (configurable)
 * - Unique 64-bit address (supports multiple sensors on one bus)
 * - Parasitic power mode support
 *
 * Hardware:
 * - OneWire communication (single data wire + GND)
 * - 4.7kΩ pull-up resistor required on data line
 * - 3.0V to 5.5V power supply
 *
 * Wiring:
 * - Red/VCC: 3.3V or 5V
 * - Black/GND: Ground
 * - Yellow/Data: GPIO pin (with 4.7kΩ pull-up to VCC)
 *
 * Usage:
 * auto sensor = new DS18B20Sensor("pool_temp", 4, "Pool Temperature");
 * sensor->setup();
 * UIFramework::getInstance()->registerComponent(sensor);
 */
class DS18B20Sensor : public ISensor {
private:
    std::string m_id;
    std::string m_name;
    uint8_t m_pin;
    bool m_initialized;
    bool m_visible;

    // OneWire instance
    OneWire* m_one_wire;

    // Sensor address (8 bytes)
    uint8_t m_address[8];
    bool m_address_found;

    // Cached temperature
    float m_temperature;
    uint32_t m_last_read;

    // Update interval (milliseconds)
    static constexpr uint32_t UPDATE_INTERVAL = 5000;  // 5 seconds

    // DS18B20 commands
    static constexpr uint8_t CMD_CONVERT_T = 0x44;
    static constexpr uint8_t CMD_READ_SCRATCHPAD = 0xBE;
    static constexpr uint8_t CMD_WRITE_SCRATCHPAD = 0x4E;
    static constexpr uint8_t CMD_COPY_SCRATCHPAD = 0x48;
    static constexpr uint8_t CMD_SKIP_ROM = 0xCC;
    static constexpr uint8_t CMD_MATCH_ROM = 0x55;
    static constexpr uint8_t CMD_SEARCH_ROM = 0xF0;

    // Resolution settings
    enum Resolution {
        RES_9_BIT = 0x1F,   // 93.75ms conversion time
        RES_10_BIT = 0x3F,  // 187.5ms
        RES_11_BIT = 0x5F,  // 375ms
        RES_12_BIT = 0x7F   // 750ms (default)
    };

    Resolution m_resolution;

    /**
     * @brief Search for DS18B20 sensor on the bus
     */
    bool searchSensor() {
        m_one_wire->reset_search();
        delay(250);

        if (!m_one_wire->search(m_address)) {
            ESP_LOGE("ds18b20", "No OneWire devices found on pin %d", m_pin);
            return false;
        }

        // Check CRC
        if (OneWire::crc8(m_address, 7) != m_address[7]) {
            ESP_LOGE("ds18b20", "CRC check failed for device address");
            return false;
        }

        // Check device family code (0x28 for DS18B20)
        if (m_address[0] != 0x28) {
            ESP_LOGW("ds18b20", "Device is not a DS18B20 (family code: 0x%02X)", m_address[0]);
            // Continue anyway - might be DS18S20 (0x10) or DS1822 (0x22)
        }

        // Log address
        char addr_str[24];
        snprintf(addr_str, sizeof(addr_str), "%02X:%02X:%02X:%02X:%02X:%02X:%02X:%02X",
                 m_address[0], m_address[1], m_address[2], m_address[3],
                 m_address[4], m_address[5], m_address[6], m_address[7]);
        ESP_LOGI("ds18b20", "Found sensor at address: %s", addr_str);

        m_address_found = true;
        return true;
    }

    /**
     * @brief Set sensor resolution
     */
    bool setResolution(Resolution resolution) {
        m_one_wire->reset();
        m_one_wire->select(m_address);
        m_one_wire->write(CMD_WRITE_SCRATCHPAD);
        m_one_wire->write(0);  // TH register
        m_one_wire->write(0);  // TL register
        m_one_wire->write(resolution);

        m_one_wire->reset();
        m_one_wire->select(m_address);
        m_one_wire->write(CMD_COPY_SCRATCHPAD);

        delay(10);
        m_resolution = resolution;

        return true;
    }

    /**
     * @brief Read temperature from sensor
     */
    bool readTemperature() {
        if (!m_address_found) {
            return false;
        }

        // Start temperature conversion
        m_one_wire->reset();
        m_one_wire->select(m_address);
        m_one_wire->write(CMD_CONVERT_T);

        // Wait for conversion (depends on resolution)
        uint16_t conversion_time;
        switch (m_resolution) {
            case RES_9_BIT: conversion_time = 100; break;
            case RES_10_BIT: conversion_time = 200; break;
            case RES_11_BIT: conversion_time = 400; break;
            case RES_12_BIT: conversion_time = 800; break;
            default: conversion_time = 800;
        }
        delay(conversion_time);

        // Read scratchpad
        m_one_wire->reset();
        m_one_wire->select(m_address);
        m_one_wire->write(CMD_READ_SCRATCHPAD);

        uint8_t data[9];
        for (uint8_t i = 0; i < 9; i++) {
            data[i] = m_one_wire->read();
        }

        // Check CRC
        if (OneWire::crc8(data, 8) != data[8]) {
            ESP_LOGE("ds18b20", "CRC check failed on temperature reading");
            return false;
        }

        // Convert to temperature
        int16_t raw_temp = (data[1] << 8) | data[0];

        // Calculate temperature based on resolution
        uint8_t cfg = (data[4] & 0x60);
        if (cfg == 0x00) raw_temp = raw_temp & ~7;      // 9-bit
        else if (cfg == 0x20) raw_temp = raw_temp & ~3;  // 10-bit
        else if (cfg == 0x40) raw_temp = raw_temp & ~1;  // 11-bit
        // 12-bit: no mask needed

        m_temperature = (float)raw_temp / 16.0f;
        m_last_read = millis();

        return true;
    }

public:
    /**
     * @brief Constructor
     *
     * @param id Unique component ID
     * @param pin GPIO pin for OneWire data line
     * @param name Human-readable name
     * @param resolution Sensor resolution (9-12 bits)
     */
    DS18B20Sensor(const char* id, uint8_t pin, const char* name = nullptr, Resolution resolution = RES_12_BIT)
        : m_id(id)
        , m_pin(pin)
        , m_initialized(false)
        , m_visible(true)
        , m_one_wire(nullptr)
        , m_address_found(false)
        , m_temperature(NAN)
        , m_last_read(0)
        , m_resolution(resolution)
    {
        if (name) {
            m_name = name;
        } else {
            m_name = "Temperature";
        }

        memset(m_address, 0, sizeof(m_address));
    }

    /**
     * @brief Initialize the sensor
     *
     * @return true if sensor detected and initialized
     */
    bool setup() {
        ESP_LOGI("ds18b20", "Initializing DS18B20 sensor on GPIO%d", m_pin);

        // Create OneWire instance
        m_one_wire = new OneWire(m_pin);

        if (!m_one_wire) {
            ESP_LOGE("ds18b20", "Failed to create OneWire instance");
            return false;
        }

        // Search for sensor
        if (!searchSensor()) {
            ESP_LOGE("ds18b20", "No DS18B20 sensor found");
            return false;
        }

        // Set resolution
        if (!setResolution(m_resolution)) {
            ESP_LOGW("ds18b20", "Failed to set resolution");
        }

        // Do initial reading
        if (readTemperature()) {
            ESP_LOGI("ds18b20", "Initial temperature: %.2f°C", m_temperature);
            m_initialized = true;
        } else {
            ESP_LOGW("ds18b20", "Failed initial temperature reading");
            m_initialized = true;  // Still mark as initialized, will retry later
        }

        return true;
    }

    // ISensor interface implementation
    const char* getId() const override { return m_id.c_str(); }
    const char* getName() const override { return m_name.c_str(); }
    const char* getType() const override { return "temperature"; }
    bool isVisible() const override { return m_visible; }
    bool requiresAuth() const override { return false; }

    const char* getUnit() const override { return "°C"; }

    const char* getDeviceClass() const override {
        return "temperature";
    }

    float getValue() override {
        if (!m_initialized) {
            return NAN;
        }

        // Update if needed
        if (millis() - m_last_read > UPDATE_INTERVAL) {
            readTemperature();
        }

        return m_temperature;
    }

    bool isAvailable() override {
        return m_initialized && m_address_found && !std::isnan(m_temperature);
    }

    uint8_t getAccuracy() const override {
        return 2;  // 2 decimal places
    }

    /**
     * @brief Force immediate sensor reading
     */
    void update() {
        if (m_initialized) {
            readTemperature();
        }
    }

    /**
     * @brief Get sensor address as string
     */
    std::string getAddressString() const {
        if (!m_address_found) {
            return "Not found";
        }

        char addr_str[24];
        snprintf(addr_str, sizeof(addr_str), "%02X:%02X:%02X:%02X:%02X:%02X:%02X:%02X",
                 m_address[0], m_address[1], m_address[2], m_address[3],
                 m_address[4], m_address[5], m_address[6], m_address[7]);
        return std::string(addr_str);
    }

    ~DS18B20Sensor() {
        if (m_one_wire) {
            delete m_one_wire;
        }
    }
};

} // namespace examples
} // namespace esphome_ui
