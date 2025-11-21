#pragma once

#include "../../framework/include/component.h"
#include "esphome.h"
#include <Wire.h>

namespace esphome_ui {
namespace examples {

/**
 * @brief BME280 Environmental Sensor
 *
 * Provides temperature, humidity, and pressure readings from a Bosch BME280
 * sensor via I2C communication.
 *
 * Features:
 * - Temperature in Celsius
 * - Relative humidity percentage
 * - Atmospheric pressure in hPa
 * - Automatic sensor detection
 * - Configurable I2C address (0x76 or 0x77)
 * - Error handling and availability checking
 *
 * Hardware:
 * - I2C communication (SDA/SCL)
 * - 3.3V or 5V power supply
 * - Pull-up resistors on I2C lines (often built-in)
 *
 * Usage:
 * auto temp = new BME280Sensor("bme_temp", BME280Sensor::TEMPERATURE, 0x76);
 * temp->setup();
 * UIFramework::getInstance()->registerComponent(temp);
 */
class BME280Sensor : public ISensor {
public:
    /**
     * @brief Measurement type
     */
    enum MeasurementType {
        TEMPERATURE,    ///< Temperature in Celsius
        HUMIDITY,       ///< Relative humidity in %
        PRESSURE        ///< Atmospheric pressure in hPa
    };

private:
    std::string m_id;
    std::string m_name;
    MeasurementType m_type;
    uint8_t m_i2c_address;
    bool m_initialized;
    bool m_visible;

    // Cached values
    float m_temperature;
    float m_humidity;
    float m_pressure;
    uint32_t m_last_read;

    // Update interval (milliseconds)
    static constexpr uint32_t UPDATE_INTERVAL = 5000;  // 5 seconds

    // BME280 registers
    static constexpr uint8_t BME280_REGISTER_CHIPID = 0xD0;
    static constexpr uint8_t BME280_REGISTER_CONTROL = 0xF4;
    static constexpr uint8_t BME280_REGISTER_CONFIG = 0xF5;
    static constexpr uint8_t BME280_REGISTER_CTRL_HUM = 0xF2;
    static constexpr uint8_t BME280_REGISTER_DATA = 0xF7;

    // Calibration data
    struct CalibrationData {
        uint16_t dig_T1;
        int16_t dig_T2, dig_T3;
        uint16_t dig_P1;
        int16_t dig_P2, dig_P3, dig_P4, dig_P5, dig_P6, dig_P7, dig_P8, dig_P9;
        uint8_t dig_H1, dig_H3;
        int16_t dig_H2, dig_H4, dig_H5;
        int8_t dig_H6;
    } m_calib;

    int32_t m_t_fine;  // Used in compensation calculations

    /**
     * @brief Read a byte from I2C register
     */
    uint8_t readRegister(uint8_t reg) {
        Wire.beginTransmission(m_i2c_address);
        Wire.write(reg);
        Wire.endTransmission();

        Wire.requestFrom(m_i2c_address, (uint8_t)1);
        return Wire.read();
    }

    /**
     * @brief Write a byte to I2C register
     */
    void writeRegister(uint8_t reg, uint8_t value) {
        Wire.beginTransmission(m_i2c_address);
        Wire.write(reg);
        Wire.write(value);
        Wire.endTransmission();
    }

    /**
     * @brief Read calibration data from sensor
     */
    bool readCalibrationData() {
        Wire.beginTransmission(m_i2c_address);
        Wire.write(0x88);
        Wire.endTransmission();
        Wire.requestFrom(m_i2c_address, (uint8_t)24);

        m_calib.dig_T1 = Wire.read() | (Wire.read() << 8);
        m_calib.dig_T2 = Wire.read() | (Wire.read() << 8);
        m_calib.dig_T3 = Wire.read() | (Wire.read() << 8);
        m_calib.dig_P1 = Wire.read() | (Wire.read() << 8);
        m_calib.dig_P2 = Wire.read() | (Wire.read() << 8);
        m_calib.dig_P3 = Wire.read() | (Wire.read() << 8);
        m_calib.dig_P4 = Wire.read() | (Wire.read() << 8);
        m_calib.dig_P5 = Wire.read() | (Wire.read() << 8);
        m_calib.dig_P6 = Wire.read() | (Wire.read() << 8);
        m_calib.dig_P7 = Wire.read() | (Wire.read() << 8);
        m_calib.dig_P8 = Wire.read() | (Wire.read() << 8);
        m_calib.dig_P9 = Wire.read() | (Wire.read() << 8);

        m_calib.dig_H1 = readRegister(0xA1);

        Wire.beginTransmission(m_i2c_address);
        Wire.write(0xE1);
        Wire.endTransmission();
        Wire.requestFrom(m_i2c_address, (uint8_t)7);

        m_calib.dig_H2 = Wire.read() | (Wire.read() << 8);
        m_calib.dig_H3 = Wire.read();

        uint8_t e4 = Wire.read();
        uint8_t e5 = Wire.read();
        uint8_t e6 = Wire.read();

        m_calib.dig_H4 = (e4 << 4) | (e5 & 0x0F);
        m_calib.dig_H5 = (e6 << 4) | (e5 >> 4);
        m_calib.dig_H6 = Wire.read();

        return true;
    }

    /**
     * @brief Read raw sensor data and calculate compensated values
     */
    bool readSensorData() {
        // Trigger measurement
        writeRegister(BME280_REGISTER_CONTROL, 0xB7);  // Temp oversampling x2, Pressure oversampling x16, Normal mode
        delay(100);  // Wait for measurement

        // Read raw data
        Wire.beginTransmission(m_i2c_address);
        Wire.write(BME280_REGISTER_DATA);
        Wire.endTransmission();
        Wire.requestFrom(m_i2c_address, (uint8_t)8);

        uint32_t pressure_raw = (Wire.read() << 12) | (Wire.read() << 4) | (Wire.read() >> 4);
        uint32_t temperature_raw = (Wire.read() << 12) | (Wire.read() << 4) | (Wire.read() >> 4);
        uint32_t humidity_raw = (Wire.read() << 8) | Wire.read();

        // Compensate temperature
        int32_t var1 = ((((temperature_raw >> 3) - ((int32_t)m_calib.dig_T1 << 1))) * ((int32_t)m_calib.dig_T2)) >> 11;
        int32_t var2 = (((((temperature_raw >> 4) - ((int32_t)m_calib.dig_T1)) * ((temperature_raw >> 4) - ((int32_t)m_calib.dig_T1))) >> 12) * ((int32_t)m_calib.dig_T3)) >> 14;
        m_t_fine = var1 + var2;
        m_temperature = ((m_t_fine * 5 + 128) >> 8) / 100.0f;

        // Compensate pressure
        int64_t var1_p = ((int64_t)m_t_fine) - 128000;
        int64_t var2_p = var1_p * var1_p * (int64_t)m_calib.dig_P6;
        var2_p = var2_p + ((var1_p * (int64_t)m_calib.dig_P5) << 17);
        var2_p = var2_p + (((int64_t)m_calib.dig_P4) << 35);
        var1_p = ((var1_p * var1_p * (int64_t)m_calib.dig_P3) >> 8) + ((var1_p * (int64_t)m_calib.dig_P2) << 12);
        var1_p = (((((int64_t)1) << 47) + var1_p)) * ((int64_t)m_calib.dig_P1) >> 33;

        if (var1_p != 0) {
            int64_t p = 1048576 - pressure_raw;
            p = (((p << 31) - var2_p) * 3125) / var1_p;
            var1_p = (((int64_t)m_calib.dig_P9) * (p >> 13) * (p >> 13)) >> 25;
            var2_p = (((int64_t)m_calib.dig_P8) * p) >> 19;
            p = ((p + var1_p + var2_p) >> 8) + (((int64_t)m_calib.dig_P7) << 4);
            m_pressure = p / 25600.0f;
        } else {
            m_pressure = NAN;
        }

        // Compensate humidity
        int32_t v_x1_u32r = (m_t_fine - ((int32_t)76800));
        v_x1_u32r = (((((humidity_raw << 14) - (((int32_t)m_calib.dig_H4) << 20) - (((int32_t)m_calib.dig_H5) * v_x1_u32r)) +
                       ((int32_t)16384)) >> 15) * (((((((v_x1_u32r * ((int32_t)m_calib.dig_H6)) >> 10) * (((v_x1_u32r *
                       ((int32_t)m_calib.dig_H3)) >> 11) + ((int32_t)32768))) >> 10) + ((int32_t)2097152)) *
                       ((int32_t)m_calib.dig_H2) + 8192) >> 14));
        v_x1_u32r = (v_x1_u32r - (((((v_x1_u32r >> 15) * (v_x1_u32r >> 15)) >> 7) * ((int32_t)m_calib.dig_H1)) >> 4));
        v_x1_u32r = (v_x1_u32r < 0 ? 0 : v_x1_u32r);
        v_x1_u32r = (v_x1_u32r > 419430400 ? 419430400 : v_x1_u32r);
        m_humidity = (v_x1_u32r >> 12) / 1024.0f;

        m_last_read = millis();
        return true;
    }

public:
    /**
     * @brief Constructor
     *
     * @param id Unique component ID
     * @param type Measurement type (temperature, humidity, or pressure)
     * @param i2c_address I2C address (0x76 or 0x77)
     * @param name Human-readable name (auto-generated if empty)
     */
    BME280Sensor(const char* id, MeasurementType type, uint8_t i2c_address = 0x76, const char* name = nullptr)
        : m_id(id)
        , m_type(type)
        , m_i2c_address(i2c_address)
        , m_initialized(false)
        , m_visible(true)
        , m_temperature(NAN)
        , m_humidity(NAN)
        , m_pressure(NAN)
        , m_last_read(0)
        , m_t_fine(0)
    {
        if (name) {
            m_name = name;
        } else {
            // Auto-generate name based on type
            switch (type) {
                case TEMPERATURE: m_name = "Temperature"; break;
                case HUMIDITY: m_name = "Humidity"; break;
                case PRESSURE: m_name = "Pressure"; break;
            }
        }
    }

    /**
     * @brief Initialize the sensor
     *
     * Call this during setup() phase.
     *
     * @return true if sensor detected and initialized
     */
    bool setup() {
        ESP_LOGI("bme280", "Initializing BME280 sensor at 0x%02X", m_i2c_address);

        // Initialize I2C if not already done
        Wire.begin();

        // Check chip ID
        uint8_t chip_id = readRegister(BME280_REGISTER_CHIPID);
        if (chip_id != 0x60) {
            ESP_LOGE("bme280", "BME280 not found at 0x%02X (chip_id=0x%02X)", m_i2c_address, chip_id);
            return false;
        }

        ESP_LOGI("bme280", "BME280 detected, chip_id=0x%02X", chip_id);

        // Reset sensor
        writeRegister(0xE0, 0xB6);
        delay(10);

        // Read calibration data
        if (!readCalibrationData()) {
            ESP_LOGE("bme280", "Failed to read calibration data");
            return false;
        }

        // Configure sensor
        writeRegister(BME280_REGISTER_CTRL_HUM, 0x05);  // Humidity oversampling x16
        writeRegister(BME280_REGISTER_CONFIG, 0xA0);    // Standby time 1s, filter off
        writeRegister(BME280_REGISTER_CONTROL, 0xB7);  // Temp x2, Pressure x16, Normal mode

        m_initialized = true;
        ESP_LOGI("bme280", "BME280 initialized successfully");

        return true;
    }

    // ISensor interface implementation
    const char* getId() const override { return m_id.c_str(); }
    const char* getName() const override { return m_name.c_str(); }
    bool isVisible() const override { return m_visible; }
    bool requiresAuth() const override { return false; }

    const char* getType() const override {
        switch (m_type) {
            case TEMPERATURE: return "temperature";
            case HUMIDITY: return "humidity";
            case PRESSURE: return "pressure";
            default: return "sensor";
        }
    }

    const char* getUnit() const override {
        switch (m_type) {
            case TEMPERATURE: return "°C";
            case HUMIDITY: return "%";
            case PRESSURE: return "hPa";
            default: return "";
        }
    }

    const char* getDeviceClass() const override {
        switch (m_type) {
            case TEMPERATURE: return "temperature";
            case HUMIDITY: return "humidity";
            case PRESSURE: return "pressure";
            default: return nullptr;
        }
    }

    float getValue() override {
        if (!m_initialized) {
            return NAN;
        }

        // Update if needed
        if (millis() - m_last_read > UPDATE_INTERVAL) {
            readSensorData();
        }

        switch (m_type) {
            case TEMPERATURE: return m_temperature;
            case HUMIDITY: return m_humidity;
            case PRESSURE: return m_pressure;
            default: return NAN;
        }
    }

    bool isAvailable() override {
        return m_initialized && !std::isnan(getValue());
    }

    uint8_t getAccuracy() const override {
        return (m_type == PRESSURE) ? 1 : 1;  // 1 decimal place for all
    }

    /**
     * @brief Force immediate sensor reading
     */
    void update() {
        if (m_initialized) {
            readSensorData();
        }
    }
};

} // namespace examples
} // namespace esphome_ui
