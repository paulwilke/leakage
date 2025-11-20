#pragma once

#include <string>
#include <cmath>

namespace esphome_ui {

/**
 * @brief Base interface for all components in the UI framework
 *
 * This interface defines the minimal contract that all components must fulfill
 * to be registered and displayed in the web UI. Components can be sensors,
 * actuators, or any other device that needs to be exposed through the API.
 *
 * Design Principles:
 * - Pure virtual interface for maximum flexibility
 * - Minimal memory footprint
 * - Clear separation of concerns
 * - Easy to mock for testing
 */
class IComponent {
public:
    virtual ~IComponent() = default;

    /**
     * @brief Get the unique identifier for this component
     *
     * The ID must be unique within the device and should follow naming
     * conventions: lowercase, alphanumeric, underscores allowed.
     *
     * Example: "temp_sensor_1", "relay_garage", "leak_detector"
     *
     * @return const char* Null-terminated C string with component ID
     */
    virtual const char* getId() const = 0;

    /**
     * @brief Get the component type identifier
     *
     * Used for filtering and categorization in the UI. Common types:
     * - "temperature"
     * - "humidity"
     * - "binary"
     * - "relay"
     * - "switch"
     *
     * @return const char* Component type string
     */
    virtual const char* getType() const = 0;

    /**
     * @brief Get the human-readable name for this component
     *
     * This name is displayed in the UI. It should be descriptive and
     * can be localized using the i18n system.
     *
     * Example: "Living Room Temperature", "Garage Door"
     *
     * @return const char* Display name
     */
    virtual const char* getName() const = 0;

    /**
     * @brief Check if this component should be visible in the UI
     *
     * Allows components to be hidden from the web interface while still
     * being registered (for internal use, debugging, etc.).
     *
     * @return true if visible, false otherwise
     */
    virtual bool isVisible() const = 0;

    /**
     * @brief Check if this component requires authentication to access
     *
     * Some components (like actuators) should only be accessible after login.
     * Sensors can typically be read without authentication.
     *
     * @return true if authentication required, false otherwise
     */
    virtual bool requiresAuth() const = 0;

    /**
     * @brief Serialize component data to JSON format
     *
     * Returns a JSON string representing the current state of the component.
     * Format should be: {"id":"xxx","type":"xxx","name":"xxx",...}
     *
     * @return std::string JSON representation
     */
    virtual std::string toJson() const = 0;
};

/**
 * @brief Interface for sensor components
 *
 * Sensors are read-only components that provide measurements. They can report
 * numeric values (temperature, humidity, etc.) or binary states (on/off, wet/dry).
 */
class ISensor : public IComponent {
public:
    /**
     * @brief Get the current sensor value
     *
     * Returns NaN if sensor is unavailable or has no valid reading.
     * Use std::isnan() to check for invalid readings.
     *
     * @return float Current measurement value, or NaN if unavailable
     * @note This method may block for up to 100ms during sensor communication
     */
    virtual float getValue() = 0;

    /**
     * @brief Get the unit of measurement
     *
     * Returns a string representing the unit (e.g., "°C", "°F", "%", "hPa").
     * Return empty string for unitless measurements.
     *
     * @return const char* Unit string
     */
    virtual const char* getUnit() const = 0;

    /**
     * @brief Check if sensor is currently available
     *
     * A sensor might be unavailable due to hardware errors, disconnection,
     * or power-saving modes.
     *
     * @return true if sensor is operational, false otherwise
     */
    virtual bool isAvailable() = 0;

    /**
     * @brief Get the sensor's device class for Home Assistant compatibility
     *
     * Device classes help Home Assistant choose appropriate icons and units.
     * Examples: "temperature", "humidity", "pressure", "battery", "signal_strength"
     *
     * @return const char* Device class string, or nullptr if not applicable
     */
    virtual const char* getDeviceClass() const {
        return nullptr;
    }

    /**
     * @brief Get the state class for Home Assistant statistics
     *
     * Indicates how the sensor value should be treated for statistics.
     * - "measurement": Instantaneous value (temperature, humidity)
     * - "total": Cumulative value that can only increase (energy usage)
     * - "total_increasing": Same as total but explicitly increasing
     *
     * @return const char* State class, or nullptr for default
     */
    virtual const char* getStateClass() const {
        return "measurement"; // Default to measurement
    }

    /**
     * @brief Get the number of decimal places to display
     *
     * @return uint8_t Number of decimals (0-3 recommended)
     */
    virtual uint8_t getAccuracy() const {
        return 1; // Default: one decimal place
    }

    // Sensors are typically visible and don't require auth
    bool requiresAuth() const override {
        return false;
    }

    std::string toJson() const override {
        char buffer[256];
        float value = const_cast<ISensor*>(this)->getValue();
        bool available = const_cast<ISensor*>(this)->isAvailable();

        snprintf(buffer, sizeof(buffer),
            "{\"id\":\"%s\",\"type\":\"%s\",\"name\":\"%s\","
            "\"value\":%s,\"unit\":\"%s\",\"available\":%s}",
            getId(),
            getType(),
            getName(),
            std::isnan(value) ? "null" : std::to_string(value).c_str(),
            getUnit(),
            available ? "true" : "false"
        );

        return std::string(buffer);
    }
};

/**
 * @brief Interface for actuator components
 *
 * Actuators are components that can be controlled (e.g., relays, switches, servos).
 * They require authentication to prevent unauthorized access.
 */
class IActuator : public IComponent {
public:
    /**
     * @brief Set the actuator state from JSON
     *
     * The JSON format depends on the actuator type:
     * - Binary: {"state": "on"} or {"state": "off"}
     * - Numeric: {"value": 75} (e.g., for dimmer, servo position)
     * - Complex: {"position": 50, "tilt": 30} (e.g., for blinds)
     *
     * @param json JSON string containing the desired state
     * @return true if state was set successfully, false on error
     */
    virtual bool setState(const std::string& json) = 0;

    /**
     * @brief Get the current actuator state as JSON
     *
     * @return std::string Current state in JSON format
     */
    virtual std::string getState() const = 0;

    /**
     * @brief Check if actuator is currently available/controllable
     *
     * @return true if actuator can be controlled, false otherwise
     */
    virtual bool isAvailable() = 0;

    // Actuators require authentication by default
    bool requiresAuth() const override {
        return true;
    }

    std::string toJson() const override {
        char buffer[256];
        std::string state = getState();
        bool available = const_cast<IActuator*>(this)->isAvailable();

        snprintf(buffer, sizeof(buffer),
            "{\"id\":\"%s\",\"type\":\"%s\",\"name\":\"%s\","
            "\"state\":%s,\"available\":%s}",
            getId(),
            getType(),
            getName(),
            state.c_str(),
            available ? "true" : "false"
        );

        return std::string(buffer);
    }
};

} // namespace esphome_ui
