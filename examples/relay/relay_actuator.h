#pragma once

#include "../../framework/include/component.h"
#include "esphome.h"
#include <ArduinoJson.h>

namespace esphome_ui {
namespace examples {

/**
 * @brief Relay Actuator Component
 *
 * Controls a relay module via GPIO pin. Supports:
 * - ON/OFF control
 * - State persistence (optional)
 * - Safety timeout (auto-off after duration)
 * - Inverted logic (active low relays)
 * - Pulse mode (momentary activation)
 *
 * Features:
 * - Simple GPIO control
 * - State feedback
 * - Safety features
 * - Configurable behavior
 *
 * Hardware:
 * - Relay module (1-channel, 2-channel, etc.)
 * - GPIO control pin
 * - Optional external power for relay coil
 *
 * Common relay modules:
 * - 5V relay boards (often active-low)
 * - Solid-state relays (SSR)
 * - Mechanical relays
 *
 * Wiring:
 * - VCC: 5V (for relay coil)
 * - GND: Ground
 * - IN/Signal: GPIO pin
 * - COM/NO/NC: Load connections
 *
 * Usage:
 * auto relay = new RelayActuator("garage_relay", 23, "Garage Door");
 * relay->setup();
 * relay->setSafetyTimeout(30000);  // 30 seconds auto-off
 * UIFramework::getInstance()->registerComponent(relay);
 */
class RelayActuator : public IActuator {
private:
    std::string m_id;
    std::string m_name;
    uint8_t m_pin;
    bool m_state;
    bool m_inverted;
    bool m_initialized;
    bool m_visible;

    // Safety timeout (milliseconds, 0 = disabled)
    uint32_t m_safety_timeout;
    uint32_t m_turn_on_time;

    // State persistence
    bool m_restore_state;
    bool m_persist_state;

    /**
     * @brief Update physical GPIO state
     */
    void updateGPIO() {
        if (!m_initialized) return;

        bool gpio_state = m_inverted ? !m_state : m_state;
        digitalWrite(m_pin, gpio_state ? HIGH : LOW);

        ESP_LOGI("relay", "Relay '%s' (GPIO%d) set to %s",
                 m_id.c_str(), m_pin, m_state ? "ON" : "OFF");
    }

    /**
     * @brief Check safety timeout
     */
    void checkSafetyTimeout() {
        if (!m_state || m_safety_timeout == 0) {
            return;
        }

        uint32_t elapsed = millis() - m_turn_on_time;
        if (elapsed >= m_safety_timeout) {
            ESP_LOGW("relay", "Safety timeout reached for '%s', turning OFF", m_id.c_str());
            turnOff();
        }
    }

public:
    /**
     * @brief Constructor
     *
     * @param id Unique component ID
     * @param pin GPIO pin number
     * @param name Human-readable name
     * @param inverted True for active-low relays (default: false)
     * @param restore_state Restore state after reboot (default: false)
     */
    RelayActuator(const char* id, uint8_t pin, const char* name = nullptr,
                  bool inverted = false, bool restore_state = false)
        : m_id(id)
        , m_pin(pin)
        , m_state(false)
        , m_inverted(inverted)
        , m_initialized(false)
        , m_visible(true)
        , m_safety_timeout(0)
        , m_turn_on_time(0)
        , m_restore_state(restore_state)
        , m_persist_state(false)
    {
        if (name) {
            m_name = name;
        } else {
            m_name = "Relay";
        }
    }

    /**
     * @brief Initialize the relay
     *
     * @return true if initialized successfully
     */
    bool setup() {
        ESP_LOGI("relay", "Initializing relay '%s' on GPIO%d (inverted: %s)",
                 m_id.c_str(), m_pin, m_inverted ? "yes" : "no");

        // Configure GPIO
        pinMode(m_pin, OUTPUT);

        // Set initial state (OFF by default, unless restoring)
        if (m_restore_state) {
            // TODO: Load state from NVS
            // For now, default to OFF
            m_state = false;
        }

        updateGPIO();
        m_initialized = true;

        ESP_LOGI("relay", "Relay '%s' initialized successfully (state: %s)",
                 m_id.c_str(), m_state ? "ON" : "OFF");

        return true;
    }

    /**
     * @brief Set safety timeout
     *
     * If set, relay will automatically turn off after this duration.
     * Useful for garage doors, pumps, etc.
     *
     * @param timeout_ms Timeout in milliseconds (0 = disabled)
     */
    void setSafetyTimeout(uint32_t timeout_ms) {
        m_safety_timeout = timeout_ms;
        ESP_LOGI("relay", "Safety timeout set to %u ms for '%s'",
                 timeout_ms, m_id.c_str());
    }

    /**
     * @brief Turn relay ON
     */
    void turnOn() {
        if (m_state) {
            return;  // Already on
        }

        m_state = true;
        m_turn_on_time = millis();
        updateGPIO();

        if (m_persist_state) {
            // TODO: Save state to NVS
        }
    }

    /**
     * @brief Turn relay OFF
     */
    void turnOff() {
        if (!m_state) {
            return;  // Already off
        }

        m_state = false;
        updateGPIO();

        if (m_persist_state) {
            // TODO: Save state to NVS
        }
    }

    /**
     * @brief Toggle relay state
     */
    void toggle() {
        if (m_state) {
            turnOff();
        } else {
            turnOn();
        }
    }

    /**
     * @brief Pulse relay (turn on briefly then off)
     *
     * @param duration_ms Pulse duration in milliseconds
     */
    void pulse(uint32_t duration_ms = 500) {
        turnOn();
        delay(duration_ms);
        turnOff();
    }

    /**
     * @brief Update loop - call periodically to check safety timeout
     */
    void loop() {
        checkSafetyTimeout();
    }

    // IActuator interface implementation
    const char* getId() const override { return m_id.c_str(); }
    const char* getName() const override { return m_name.c_str(); }
    const char* getType() const override { return "relay"; }
    bool isVisible() const override { return m_visible; }
    bool requiresAuth() const override { return true; }  // Relays require auth

    bool setState(const std::string& json) override {
        if (!m_initialized) {
            ESP_LOGE("relay", "Cannot set state - not initialized");
            return false;
        }

        // Parse JSON: {"state": "on"} or {"state": "off"} or {"action": "toggle"}
        StaticJsonDocument<128> doc;
        DeserializationError error = deserializeJson(doc, json);

        if (error) {
            ESP_LOGE("relay", "Failed to parse JSON: %s", error.c_str());
            return false;
        }

        // Check for action command
        if (doc.containsKey("action")) {
            const char* action = doc["action"];

            if (strcmp(action, "toggle") == 0) {
                toggle();
                return true;
            } else if (strcmp(action, "pulse") == 0) {
                uint32_t duration = doc["duration"] | 500;
                pulse(duration);
                return true;
            }
        }

        // Check for state command
        if (doc.containsKey("state")) {
            const char* state = doc["state"];

            if (strcmp(state, "on") == 0 || strcmp(state, "ON") == 0 || strcmp(state, "true") == 0) {
                turnOn();
                return true;
            } else if (strcmp(state, "off") == 0 || strcmp(state, "OFF") == 0 || strcmp(state, "false") == 0) {
                turnOff();
                return true;
            }
        }

        ESP_LOGW("relay", "Unknown command in JSON: %s", json.c_str());
        return false;
    }

    std::string getState() const override {
        char buffer[128];

        uint32_t remaining_timeout = 0;
        if (m_state && m_safety_timeout > 0) {
            uint32_t elapsed = millis() - m_turn_on_time;
            if (elapsed < m_safety_timeout) {
                remaining_timeout = (m_safety_timeout - elapsed) / 1000;  // Convert to seconds
            }
        }

        snprintf(buffer, sizeof(buffer),
            "{\"state\":\"%s\",\"pin\":%d,\"safety_timeout\":%u,\"remaining\":%u}",
            m_state ? "on" : "off",
            m_pin,
            m_safety_timeout / 1000,  // Convert to seconds
            remaining_timeout
        );

        return std::string(buffer);
    }

    bool isAvailable() override {
        return m_initialized;
    }

    /**
     * @brief Get current relay state
     */
    bool isOn() const {
        return m_state;
    }

    /**
     * @brief Set whether to persist state across reboots
     */
    void setPersistState(bool persist) {
        m_persist_state = persist;
    }
};

} // namespace examples
} // namespace esphome_ui
