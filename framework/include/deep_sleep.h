#pragma once

#include "esphome.h"
#include <esp_sleep.h>
#include <vector>
#include <cstdint>

namespace esphome_ui {

/**
 * @brief Wake reason enumeration
 */
enum class WakeReason {
    UNKNOWN,
    TIMER,
    GPIO,
    TOUCHPAD,
    ULP,
    RESET
};

/**
 * @brief Deep Sleep Controller for power management
 *
 * Controls ESP32 deep sleep functionality for battery-powered devices.
 * Deep sleep reduces power consumption to ~10µA by shutting down most
 * of the chip, keeping only the RTC and wake-up logic active.
 *
 * Features:
 * - Timer-based wake (sleep for N seconds)
 * - GPIO wake (wake on pin level change)
 * - Configurable wake sources
 * - State preservation across sleep/wake cycles
 * - Integration with ESPHome deep_sleep component
 *
 * Typical Usage:
 * 1. Configure wake sources
 * 2. Call enterSleep() when ready
 * 3. Device wakes after timer/GPIO trigger
 * 4. Check getWakeReason() to determine why we woke
 * 5. Do work, then sleep again
 *
 * Power Consumption:
 * - Active (WiFi): ~150-200mA
 * - Light sleep: ~1mA
 * - Deep sleep: ~10µA
 *
 * Battery Life Example (1000mAh battery):
 * - Always on: ~5-7 hours
 * - Sleep 5min, wake 1min: ~2-3 weeks
 * - Sleep 1hour, wake 1min: ~6+ months
 */
class DeepSleepController {
public:
    /**
     * @brief Configuration for deep sleep behavior
     */
    struct Config {
        bool enabled;                           ///< Enable deep sleep
        uint32_t sleep_duration_seconds;        ///< Sleep duration (0 = indefinite)
        std::vector<gpio_num_t> wake_gpio_pins; ///< GPIO pins that can wake device
        bool wake_on_timer;                     ///< Allow timer wake
        bool wake_on_gpio;                      ///< Allow GPIO wake
        gpio_int_type_t gpio_wake_level;        ///< LOW or HIGH level wake
        bool run_duration_enabled;              ///< Enable run duration limit
        uint32_t run_duration_seconds;          ///< Max run time before sleep

        Config()
            : enabled(false)
            , sleep_duration_seconds(300)  // 5 minutes default
            , wake_on_timer(true)
            , wake_on_gpio(false)
            , gpio_wake_level(GPIO_INTR_LOW_LEVEL)
            , run_duration_enabled(false)
            , run_duration_seconds(60)
        {}
    };

private:
    Config m_config;
    bool m_initialized;
    uint32_t m_wake_time;
    WakeReason m_wake_reason;
    uint32_t m_boot_time;

    /**
     * @brief Determine why the device woke up
     */
    WakeReason detectWakeReason() const {
        esp_sleep_wakeup_cause_t cause = esp_sleep_get_wakeup_cause();

        switch (cause) {
            case ESP_SLEEP_WAKEUP_TIMER:
                return WakeReason::TIMER;
            case ESP_SLEEP_WAKEUP_EXT0:
            case ESP_SLEEP_WAKEUP_EXT1:
                return WakeReason::GPIO;
            case ESP_SLEEP_WAKEUP_TOUCHPAD:
                return WakeReason::TOUCHPAD;
            case ESP_SLEEP_WAKEUP_ULP:
                return WakeReason::ULP;
            case ESP_SLEEP_WAKEUP_UNDEFINED:
            default:
                return WakeReason::RESET;
        }
    }

public:
    DeepSleepController()
        : m_initialized(false)
        , m_wake_time(0)
        , m_wake_reason(WakeReason::UNKNOWN)
        , m_boot_time(0)
    {}

    /**
     * @brief Initialize the deep sleep controller
     *
     * Call this during setup() to detect wake reason and prepare for sleep.
     *
     * @return true if initialized successfully
     */
    bool init() {
        if (m_initialized) {
            ESP_LOGW("deep_sleep", "Already initialized");
            return true;
        }

        m_boot_time = millis();
        m_wake_reason = detectWakeReason();

        const char* reason_str = "UNKNOWN";
        switch (m_wake_reason) {
            case WakeReason::TIMER: reason_str = "TIMER"; break;
            case WakeReason::GPIO: reason_str = "GPIO"; break;
            case WakeReason::TOUCHPAD: reason_str = "TOUCHPAD"; break;
            case WakeReason::ULP: reason_str = "ULP"; break;
            case WakeReason::RESET: reason_str = "RESET/POWER_ON"; break;
            default: break;
        }

        ESP_LOGI("deep_sleep", "Wake reason: %s", reason_str);

        m_initialized = true;
        return true;
    }

    /**
     * @brief Configure deep sleep parameters
     *
     * @param config Configuration structure
     * @return true if configuration is valid and applied
     */
    bool configure(const Config& config) {
        if (!m_initialized) {
            ESP_LOGE("deep_sleep", "Not initialized");
            return false;
        }

        // Validate configuration
        if (config.wake_on_gpio && config.wake_gpio_pins.empty()) {
            ESP_LOGW("deep_sleep", "GPIO wake enabled but no pins configured");
        }

        if (!config.wake_on_timer && !config.wake_on_gpio) {
            ESP_LOGW("deep_sleep", "No wake sources enabled!");
        }

        m_config = config;

        ESP_LOGI("deep_sleep", "Configuration updated:");
        ESP_LOGI("deep_sleep", "  Enabled: %s", config.enabled ? "yes" : "no");
        ESP_LOGI("deep_sleep", "  Sleep duration: %u seconds", config.sleep_duration_seconds);
        ESP_LOGI("deep_sleep", "  Wake on timer: %s", config.wake_on_timer ? "yes" : "no");
        ESP_LOGI("deep_sleep", "  Wake on GPIO: %s", config.wake_on_gpio ? "yes" : "no");

        if (config.wake_on_gpio) {
            ESP_LOGI("deep_sleep", "  GPIO wake pins: %zu", config.wake_gpio_pins.size());
        }

        return true;
    }

    /**
     * @brief Enter deep sleep mode
     *
     * This function prepares the device for sleep and then enters deep sleep.
     * Execution does NOT return from this function - the device will reset
     * on wake-up and start from setup().
     *
     * Steps:
     * 1. Flush logs
     * 2. Close connections
     * 3. Save state (if needed)
     * 4. Configure wake sources
     * 5. Enter deep sleep
     *
     * @note This function does not return!
     */
    void enterSleep() {
        if (!m_config.enabled) {
            ESP_LOGW("deep_sleep", "Deep sleep not enabled");
            return;
        }

        ESP_LOGI("deep_sleep", "Preparing to enter deep sleep...");

        // Flush all logs
        esp_log_level_set("*", ESP_LOG_NONE);
        delay(100);

        // Configure wake sources
        if (m_config.wake_on_timer && m_config.sleep_duration_seconds > 0) {
            uint64_t sleep_time_us = m_config.sleep_duration_seconds * 1000000ULL;
            esp_sleep_enable_timer_wakeup(sleep_time_us);
            ESP_LOGI("deep_sleep", "Timer wake: %u seconds", m_config.sleep_duration_seconds);
        }

        if (m_config.wake_on_gpio && !m_config.wake_gpio_pins.empty()) {
            // ESP32 supports EXT0 (single pin) or EXT1 (multiple pins with mask)
            if (m_config.wake_gpio_pins.size() == 1) {
                // EXT0: Single pin, supports HIGH or LOW
                gpio_num_t pin = m_config.wake_gpio_pins[0];
                esp_sleep_ext0_wakeup_mode_t mode = (m_config.gpio_wake_level == GPIO_INTR_HIGH_LEVEL)
                    ? ESP_EXT0_WAKEUP_LEVEL_HIGH
                    : ESP_EXT0_WAKEUP_LEVEL_LOW;

                esp_sleep_enable_ext0_wakeup(pin, mode);
                ESP_LOGI("deep_sleep", "EXT0 wake on GPIO%d", pin);
            } else {
                // EXT1: Multiple pins with bitmask
                uint64_t pin_mask = 0;
                for (gpio_num_t pin : m_config.wake_gpio_pins) {
                    pin_mask |= (1ULL << pin);
                }

                esp_sleep_ext1_wakeup_mode_t mode = (m_config.gpio_wake_level == GPIO_INTR_HIGH_LEVEL)
                    ? ESP_EXT1_WAKEUP_ANY_HIGH
                    : ESP_EXT1_WAKEUP_ALL_LOW;

                esp_sleep_enable_ext1_wakeup(pin_mask, mode);
                ESP_LOGI("deep_sleep", "EXT1 wake on %zu pins", m_config.wake_gpio_pins.size());
            }
        }

        // Enter deep sleep (DOES NOT RETURN)
        ESP_LOGI("deep_sleep", "Entering deep sleep NOW");
        delay(100);
        esp_deep_sleep_start();
    }

    /**
     * @brief Check if it's time to sleep based on run duration
     *
     * If run_duration is configured, this checks if we've been running
     * longer than the limit.
     *
     * @return true if run duration exceeded, false otherwise
     */
    bool shouldSleep() const {
        if (!m_config.enabled || !m_config.run_duration_enabled) {
            return false;
        }

        uint32_t elapsed_ms = millis() - m_boot_time;
        uint32_t limit_ms = m_config.run_duration_seconds * 1000;

        return elapsed_ms >= limit_ms;
    }

    /**
     * @brief Get time until next sleep (if run duration enabled)
     *
     * @return Seconds remaining, or 0 if not applicable
     */
    uint32_t getTimeUntilSleep() const {
        if (!m_config.enabled || !m_config.run_duration_enabled) {
            return 0;
        }

        uint32_t elapsed_ms = millis() - m_boot_time;
        uint32_t limit_ms = m_config.run_duration_seconds * 1000;

        if (elapsed_ms >= limit_ms) {
            return 0;
        }

        return (limit_ms - elapsed_ms) / 1000;
    }

    /**
     * @brief Get the reason for the last wake-up
     *
     * @return WakeReason enumeration
     */
    WakeReason getWakeReason() const {
        return m_wake_reason;
    }

    /**
     * @brief Get current configuration
     *
     * @return Copy of configuration
     */
    Config getConfig() const {
        return m_config;
    }

    /**
     * @brief Check if deep sleep is enabled
     *
     * @return true if enabled
     */
    bool isEnabled() const {
        return m_config.enabled;
    }

    /**
     * @brief Serialize configuration to JSON
     *
     * @return JSON string with configuration
     */
    std::string toJson() const {
        char buffer[512];

        const char* wake_reason_str = "unknown";
        switch (m_wake_reason) {
            case WakeReason::TIMER: wake_reason_str = "timer"; break;
            case WakeReason::GPIO: wake_reason_str = "gpio"; break;
            case WakeReason::TOUCHPAD: wake_reason_str = "touchpad"; break;
            case WakeReason::ULP: wake_reason_str = "ulp"; break;
            case WakeReason::RESET: wake_reason_str = "reset"; break;
            default: break;
        }

        snprintf(buffer, sizeof(buffer),
            "{\"enabled\":%s,"
            "\"sleep_duration\":%u,"
            "\"wake_reason\":\"%s\","
            "\"time_until_sleep\":%u,"
            "\"should_sleep\":%s}",
            m_config.enabled ? "true" : "false",
            m_config.sleep_duration_seconds,
            wake_reason_str,
            getTimeUntilSleep(),
            shouldSleep() ? "true" : "false"
        );

        return std::string(buffer);
    }
};

} // namespace esphome_ui
