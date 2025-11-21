#pragma once

#include "esphome.h"
#include <nvs_flash.h>
#include <nvs.h>
#include <string>
#include <cstring>

namespace esphome_ui {

/**
 * @brief Persistent configuration storage using ESP32 NVS
 *
 * ConfigStorage provides a type-safe interface for storing and retrieving
 * device configuration in the ESP32's Non-Volatile Storage (NVS) flash memory.
 *
 * Features:
 * - WiFi credentials
 * - Admin authentication
 * - NTP/timezone settings
 * - Device customization
 * - Sensor filter settings
 * - Factory reset capability
 *
 * Usage:
 * ConfigStorage storage;
 * storage.init();
 *
 * ConfigStorage::Config config;
 * storage.load(config);  // Load from NVS
 * // Modify config...
 * storage.save(config);  // Save to NVS
 *
 * Thread Safety: NOT thread-safe. Call from main task only.
 * Memory: Uses NVS namespace "ui_framework_cfg" (~2KB flash)
 */
class ConfigStorage {
private:
    nvs_handle_t m_nvs_handle;
    bool m_initialized;

    // NVS namespace and keys
    static constexpr const char* NVS_NAMESPACE = "ui_framework_cfg";
    static constexpr const char* KEY_WIFI_SSID = "wifi_ssid";
    static constexpr const char* KEY_WIFI_PASS = "wifi_pass";
    static constexpr const char* KEY_ADMIN_USER = "admin_user";
    static constexpr const char* KEY_ADMIN_PASS = "admin_pass";
    static constexpr const char* KEY_NTP_PRIMARY = "ntp_primary";
    static constexpr const char* KEY_NTP_SECONDARY = "ntp_secondary";
    static constexpr const char* KEY_TIMEZONE = "timezone";
    static constexpr const char* KEY_DEVICE_NAME = "device_name";
    static constexpr const char* KEY_OTA_PASS = "ota_pass";
    static constexpr const char* KEY_REFRESH_INTERVAL = "refresh_int";
    static constexpr const char* KEY_FILTER_DELAY_ON = "filter_on";
    static constexpr const char* KEY_FILTER_DELAY_OFF = "filter_off";

public:
    /**
     * @brief Configuration structure
     *
     * All strings are null-terminated C strings with fixed buffer sizes.
     * This ensures predictable memory usage and simplifies NVS storage.
     */
    struct Config {
        char wifi_ssid[64];              ///< WiFi network SSID
        char wifi_password[64];          ///< WiFi password
        char admin_username[32];         ///< Admin login username
        char admin_password[64];         ///< Admin login password
        char ntp_primary[64];            ///< Primary NTP server
        char ntp_secondary[64];          ///< Secondary NTP server
        char timezone[64];               ///< POSIX timezone string
        char device_name[32];            ///< User-friendly device name
        char ota_password[64];           ///< OTA update password
        uint16_t refresh_interval;       ///< Dashboard refresh interval (seconds)
        uint16_t filter_delay_on;        ///< Sensor delayed_on filter (ms)
        uint16_t filter_delay_off;       ///< Sensor delayed_off filter (ms)

        Config() {
            // Zero-initialize all fields
            memset(this, 0, sizeof(Config));
        }
    };

    ConfigStorage() : m_nvs_handle(0), m_initialized(false) {}

    /**
     * @brief Initialize NVS storage
     *
     * Must be called before any load/save operations. If NVS is corrupted
     * or has version mismatch, it will be erased and reinitialized.
     *
     * @return true if initialized successfully, false on error
     */
    bool init() {
        if (m_initialized) {
            ESP_LOGW("config", "Already initialized");
            return true;
        }

        esp_err_t err = nvs_flash_init();
        if (err == ESP_ERR_NVS_NO_FREE_PAGES || err == ESP_ERR_NVS_NEW_VERSION_FOUND) {
            ESP_LOGW("config", "NVS needs erase, erasing...");
            nvs_flash_erase();
            err = nvs_flash_init();
        }

        if (err != ESP_OK) {
            ESP_LOGE("config", "Failed to init NVS: %s", esp_err_to_name(err));
            return false;
        }

        err = nvs_open(NVS_NAMESPACE, NVS_READWRITE, &m_nvs_handle);
        if (err != ESP_OK) {
            ESP_LOGE("config", "Failed to open NVS namespace: %s", esp_err_to_name(err));
            return false;
        }

        m_initialized = true;
        ESP_LOGI("config", "ConfigStorage initialized");
        return true;
    }

    /**
     * @brief Load configuration from NVS
     *
     * Loads all configuration values from NVS into the provided Config struct.
     * If a value doesn't exist in NVS, the default value is used.
     *
     * @param config Reference to Config struct to populate
     * @return true if loaded successfully (even if using defaults)
     */
    bool load(Config& config) {
        if (!m_initialized && !init()) {
            return false;
        }

        // Set defaults first
        resetToDefaults(config);

        size_t len;

        // Load WiFi settings
        len = sizeof(config.wifi_ssid);
        nvs_get_str(m_nvs_handle, KEY_WIFI_SSID, config.wifi_ssid, &len);

        len = sizeof(config.wifi_password);
        nvs_get_str(m_nvs_handle, KEY_WIFI_PASS, config.wifi_password, &len);

        // Load admin credentials
        len = sizeof(config.admin_username);
        nvs_get_str(m_nvs_handle, KEY_ADMIN_USER, config.admin_username, &len);

        len = sizeof(config.admin_password);
        nvs_get_str(m_nvs_handle, KEY_ADMIN_PASS, config.admin_password, &len);

        // Load NTP settings
        len = sizeof(config.ntp_primary);
        nvs_get_str(m_nvs_handle, KEY_NTP_PRIMARY, config.ntp_primary, &len);

        len = sizeof(config.ntp_secondary);
        nvs_get_str(m_nvs_handle, KEY_NTP_SECONDARY, config.ntp_secondary, &len);

        len = sizeof(config.timezone);
        nvs_get_str(m_nvs_handle, KEY_TIMEZONE, config.timezone, &len);

        // Load system settings
        len = sizeof(config.device_name);
        nvs_get_str(m_nvs_handle, KEY_DEVICE_NAME, config.device_name, &len);

        len = sizeof(config.ota_password);
        nvs_get_str(m_nvs_handle, KEY_OTA_PASS, config.ota_password, &len);

        // Load numeric settings
        nvs_get_u16(m_nvs_handle, KEY_REFRESH_INTERVAL, &config.refresh_interval);
        nvs_get_u16(m_nvs_handle, KEY_FILTER_DELAY_ON, &config.filter_delay_on);
        nvs_get_u16(m_nvs_handle, KEY_FILTER_DELAY_OFF, &config.filter_delay_off);

        ESP_LOGI("config", "Configuration loaded from NVS");
        return true;
    }

    /**
     * @brief Save configuration to NVS
     *
     * Writes all configuration values to NVS and commits changes.
     *
     * @param config Configuration to save
     * @return true if saved successfully, false on error
     */
    bool save(const Config& config) {
        if (!m_initialized && !init()) {
            return false;
        }

        esp_err_t err;

        // Save WiFi settings
        err = nvs_set_str(m_nvs_handle, KEY_WIFI_SSID, config.wifi_ssid);
        if (err != ESP_OK) ESP_LOGE("config", "Failed to save wifi_ssid");

        err = nvs_set_str(m_nvs_handle, KEY_WIFI_PASS, config.wifi_password);
        if (err != ESP_OK) ESP_LOGE("config", "Failed to save wifi_pass");

        // Save admin credentials
        err = nvs_set_str(m_nvs_handle, KEY_ADMIN_USER, config.admin_username);
        if (err != ESP_OK) ESP_LOGE("config", "Failed to save admin_user");

        err = nvs_set_str(m_nvs_handle, KEY_ADMIN_PASS, config.admin_password);
        if (err != ESP_OK) ESP_LOGE("config", "Failed to save admin_pass");

        // Save NTP settings
        err = nvs_set_str(m_nvs_handle, KEY_NTP_PRIMARY, config.ntp_primary);
        if (err != ESP_OK) ESP_LOGE("config", "Failed to save ntp_primary");

        err = nvs_set_str(m_nvs_handle, KEY_NTP_SECONDARY, config.ntp_secondary);
        if (err != ESP_OK) ESP_LOGE("config", "Failed to save ntp_secondary");

        err = nvs_set_str(m_nvs_handle, KEY_TIMEZONE, config.timezone);
        if (err != ESP_OK) ESP_LOGE("config", "Failed to save timezone");

        // Save system settings
        err = nvs_set_str(m_nvs_handle, KEY_DEVICE_NAME, config.device_name);
        if (err != ESP_OK) ESP_LOGE("config", "Failed to save device_name");

        err = nvs_set_str(m_nvs_handle, KEY_OTA_PASS, config.ota_password);
        if (err != ESP_OK) ESP_LOGE("config", "Failed to save ota_pass");

        // Save numeric settings
        err = nvs_set_u16(m_nvs_handle, KEY_REFRESH_INTERVAL, config.refresh_interval);
        if (err != ESP_OK) ESP_LOGE("config", "Failed to save refresh_interval");

        err = nvs_set_u16(m_nvs_handle, KEY_FILTER_DELAY_ON, config.filter_delay_on);
        if (err != ESP_OK) ESP_LOGE("config", "Failed to save filter_delay_on");

        err = nvs_set_u16(m_nvs_handle, KEY_FILTER_DELAY_OFF, config.filter_delay_off);
        if (err != ESP_OK) ESP_LOGE("config", "Failed to save filter_delay_off");

        // Commit changes
        err = nvs_commit(m_nvs_handle);
        if (err != ESP_OK) {
            ESP_LOGE("config", "Failed to commit NVS: %s", esp_err_to_name(err));
            return false;
        }

        ESP_LOGI("config", "Configuration saved to NVS");
        return true;
    }

    /**
     * @brief Reset configuration to default values
     *
     * Does not save to NVS, only updates the in-memory struct.
     *
     * @param config Configuration struct to reset
     */
    void resetToDefaults(Config& config) {
        strncpy(config.wifi_ssid, "", sizeof(config.wifi_ssid));
        strncpy(config.wifi_password, "", sizeof(config.wifi_password));
        strncpy(config.admin_username, "admin", sizeof(config.admin_username));
        strncpy(config.admin_password, "admin", sizeof(config.admin_password));
        strncpy(config.ntp_primary, "pool.ntp.org", sizeof(config.ntp_primary));
        strncpy(config.ntp_secondary, "time.google.com", sizeof(config.ntp_secondary));
        strncpy(config.timezone, "CET-1CEST,M3.5.0,M10.5.0/3", sizeof(config.timezone));
        strncpy(config.device_name, "esphome-device", sizeof(config.device_name));
        strncpy(config.ota_password, "", sizeof(config.ota_password));
        config.refresh_interval = 3;      // 3 seconds
        config.filter_delay_on = 100;     // 100ms
        config.filter_delay_off = 5000;   // 5s
    }

    /**
     * @brief Perform factory reset
     *
     * Erases ALL configuration from NVS and resets to defaults.
     * This operation cannot be undone!
     *
     * @return true if reset successful, false on error
     */
    bool factoryReset() {
        if (!m_initialized && !init()) {
            return false;
        }

        esp_err_t err = nvs_erase_all(m_nvs_handle);
        if (err != ESP_OK) {
            ESP_LOGE("config", "Failed to erase NVS: %s", esp_err_to_name(err));
            return false;
        }

        err = nvs_commit(m_nvs_handle);
        if (err != ESP_OK) {
            ESP_LOGE("config", "Failed to commit erase: %s", esp_err_to_name(err));
            return false;
        }

        ESP_LOGI("config", "Factory reset completed");
        return true;
    }

    /**
     * @brief Check if configuration has been customized
     *
     * Returns true if any settings differ from defaults.
     *
     * @return true if config has been modified from defaults
     */
    bool hasCustomConfig() const {
        if (!m_initialized) {
            return false;
        }

        Config test_config;
        const_cast<ConfigStorage*>(this)->load(test_config);

        // Check if admin password changed (most important indicator)
        return strcmp(test_config.admin_password, "admin") != 0;
    }

    ~ConfigStorage() {
        if (m_initialized) {
            nvs_close(m_nvs_handle);
        }
    }
};

} // namespace esphome_ui
