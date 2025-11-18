#pragma once
#include "esphome.h"
#include <nvs_flash.h>
#include <nvs.h>
#include <string>
#include <cstring>

// ConfigStorage: Persistent configuration storage using ESP32 NVS
class ConfigStorage {
 private:
  nvs_handle_t nvs_handle;
  bool initialized = false;
  
  // Storage keys
  static constexpr const char* NVS_NAMESPACE = "leakage_cfg";
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
  // Configuration structure
  struct Config {
    char wifi_ssid[64];
    char wifi_password[64];
    char admin_username[32];
    char admin_password[64];
    char ntp_primary[64];
    char ntp_secondary[64];
    char timezone[64];
    char device_name[32];
    char ota_password[64];
    uint16_t refresh_interval;  // Dashboard refresh in seconds
    uint16_t filter_delay_on;   // Leak sensor delayed_on in ms
    uint16_t filter_delay_off;  // Leak sensor delayed_off in ms
  };

  ConfigStorage() {}

  // Initialize NVS
  bool init() {
    if (initialized) return true;
    
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

    err = nvs_open(NVS_NAMESPACE, NVS_READWRITE, &nvs_handle);
    if (err != ESP_OK) {
      ESP_LOGE("config", "Failed to open NVS namespace: %s", esp_err_to_name(err));
      return false;
    }

    initialized = true;
    ESP_LOGI("config", "ConfigStorage initialized");
    return true;
  }

  // Load configuration from NVS
  bool load(Config& config) {
    if (!initialized && !init()) return false;

    // Set defaults first
    reset_to_defaults(config);

    size_t len;
    esp_err_t err;

    // Load WiFi settings
    len = sizeof(config.wifi_ssid);
    nvs_get_str(nvs_handle, KEY_WIFI_SSID, config.wifi_ssid, &len);
    
    len = sizeof(config.wifi_password);
    nvs_get_str(nvs_handle, KEY_WIFI_PASS, config.wifi_password, &len);

    // Load admin credentials
    len = sizeof(config.admin_username);
    nvs_get_str(nvs_handle, KEY_ADMIN_USER, config.admin_username, &len);
    
    len = sizeof(config.admin_password);
    nvs_get_str(nvs_handle, KEY_ADMIN_PASS, config.admin_password, &len);

    // Load NTP settings
    len = sizeof(config.ntp_primary);
    nvs_get_str(nvs_handle, KEY_NTP_PRIMARY, config.ntp_primary, &len);
    
    len = sizeof(config.ntp_secondary);
    nvs_get_str(nvs_handle, KEY_NTP_SECONDARY, config.ntp_secondary, &len);
    
    len = sizeof(config.timezone);
    nvs_get_str(nvs_handle, KEY_TIMEZONE, config.timezone, &len);

    // Load system settings
    len = sizeof(config.device_name);
    nvs_get_str(nvs_handle, KEY_DEVICE_NAME, config.device_name, &len);
    
    len = sizeof(config.ota_password);
    nvs_get_str(nvs_handle, KEY_OTA_PASS, config.ota_password, &len);

    // Load numeric settings
    nvs_get_u16(nvs_handle, KEY_REFRESH_INTERVAL, &config.refresh_interval);
    nvs_get_u16(nvs_handle, KEY_FILTER_DELAY_ON, &config.filter_delay_on);
    nvs_get_u16(nvs_handle, KEY_FILTER_DELAY_OFF, &config.filter_delay_off);

    ESP_LOGI("config", "Configuration loaded");
    return true;
  }

  // Save configuration to NVS
  bool save(const Config& config) {
    if (!initialized && !init()) return false;

    esp_err_t err;

    // Save WiFi settings
    err = nvs_set_str(nvs_handle, KEY_WIFI_SSID, config.wifi_ssid);
    if (err != ESP_OK) ESP_LOGE("config", "Failed to save wifi_ssid");
    
    err = nvs_set_str(nvs_handle, KEY_WIFI_PASS, config.wifi_password);
    if (err != ESP_OK) ESP_LOGE("config", "Failed to save wifi_pass");

    // Save admin credentials
    err = nvs_set_str(nvs_handle, KEY_ADMIN_USER, config.admin_username);
    if (err != ESP_OK) ESP_LOGE("config", "Failed to save admin_user");
    
    err = nvs_set_str(nvs_handle, KEY_ADMIN_PASS, config.admin_password);
    if (err != ESP_OK) ESP_LOGE("config", "Failed to save admin_pass");

    // Save NTP settings
    err = nvs_set_str(nvs_handle, KEY_NTP_PRIMARY, config.ntp_primary);
    if (err != ESP_OK) ESP_LOGE("config", "Failed to save ntp_primary");
    
    err = nvs_set_str(nvs_handle, KEY_NTP_SECONDARY, config.ntp_secondary);
    if (err != ESP_OK) ESP_LOGE("config", "Failed to save ntp_secondary");
    
    err = nvs_set_str(nvs_handle, KEY_TIMEZONE, config.timezone);
    if (err != ESP_OK) ESP_LOGE("config", "Failed to save timezone");

    // Save system settings
    err = nvs_set_str(nvs_handle, KEY_DEVICE_NAME, config.device_name);
    if (err != ESP_OK) ESP_LOGE("config", "Failed to save device_name");
    
    err = nvs_set_str(nvs_handle, KEY_OTA_PASS, config.ota_password);
    if (err != ESP_OK) ESP_LOGE("config", "Failed to save ota_pass");

    // Save numeric settings
    err = nvs_set_u16(nvs_handle, KEY_REFRESH_INTERVAL, config.refresh_interval);
    if (err != ESP_OK) ESP_LOGE("config", "Failed to save refresh_interval");
    
    err = nvs_set_u16(nvs_handle, KEY_FILTER_DELAY_ON, config.filter_delay_on);
    if (err != ESP_OK) ESP_LOGE("config", "Failed to save filter_delay_on");
    
    err = nvs_set_u16(nvs_handle, KEY_FILTER_DELAY_OFF, config.filter_delay_off);
    if (err != ESP_OK) ESP_LOGE("config", "Failed to save filter_delay_off");

    // Commit changes
    err = nvs_commit(nvs_handle);
    if (err != ESP_OK) {
      ESP_LOGE("config", "Failed to commit NVS: %s", esp_err_to_name(err));
      return false;
    }

    ESP_LOGI("config", "Configuration saved");
    return true;
  }

  // Reset to default values
  void reset_to_defaults(Config& config) {
    strncpy(config.wifi_ssid, "", sizeof(config.wifi_ssid));
    strncpy(config.wifi_password, "", sizeof(config.wifi_password));
    strncpy(config.admin_username, "admin", sizeof(config.admin_username));
    strncpy(config.admin_password, "admin", sizeof(config.admin_password));
    strncpy(config.ntp_primary, "pool.ntp.org", sizeof(config.ntp_primary));
    strncpy(config.ntp_secondary, "time.google.com", sizeof(config.ntp_secondary));
    strncpy(config.timezone, "CET-1CEST,M3.5.0,M10.5.0/3", sizeof(config.timezone));
    strncpy(config.device_name, "leak01", sizeof(config.device_name));
    strncpy(config.ota_password, "", sizeof(config.ota_password));
    config.refresh_interval = 3;      // 3 seconds
    config.filter_delay_on = 100;     // 100ms
    config.filter_delay_off = 5000;   // 5000ms = 5s
  }

  // Factory reset - erase all settings
  bool factory_reset() {
    if (!initialized && !init()) return false;

    esp_err_t err = nvs_erase_all(nvs_handle);
    if (err != ESP_OK) {
      ESP_LOGE("config", "Failed to erase NVS: %s", esp_err_to_name(err));
      return false;
    }

    err = nvs_commit(nvs_handle);
    if (err != ESP_OK) {
      ESP_LOGE("config", "Failed to commit erase: %s", esp_err_to_name(err));
      return false;
    }

    ESP_LOGI("config", "Factory reset completed");
    return true;
  }

  ~ConfigStorage() {
    if (initialized) {
      nvs_close(nvs_handle);
    }
  }
};

