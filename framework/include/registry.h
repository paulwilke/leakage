#pragma once

#include "component.h"
#include <map>
#include <vector>
#include <string>
#include <algorithm>

namespace esphome_ui {

/**
 * @brief Central registry for all UI components
 *
 * The ComponentRegistry maintains a list of all sensors and actuators that
 * should be exposed through the web interface. It provides:
 *
 * - Automatic discovery and registration
 * - Type-based filtering
 * - JSON serialization for API endpoints
 * - Lifecycle management
 *
 * This class is designed to be lightweight and efficient, with O(1) lookups
 * by ID and O(n) lookups by type (where n is typically small).
 *
 * Thread Safety: This class is NOT thread-safe. All operations should be
 * called from the same task (typically the main loop task).
 *
 * Memory: Uses std::map for ID-based lookups (small overhead) and stores
 * pointers only (no copying of component data).
 */
class ComponentRegistry {
private:
    // Storage: map for O(1) ID lookups
    std::map<std::string, IComponent*> m_components;

    // Statistics
    size_t m_sensor_count;
    size_t m_actuator_count;

    /**
     * @brief Update internal counters
     */
    void updateCounters() {
        m_sensor_count = 0;
        m_actuator_count = 0;

        for (const auto& pair : m_components) {
            if (dynamic_cast<ISensor*>(pair.second)) {
                m_sensor_count++;
            } else if (dynamic_cast<IActuator*>(pair.second)) {
                m_actuator_count++;
            }
        }
    }

public:
    ComponentRegistry() : m_sensor_count(0), m_actuator_count(0) {}

    /**
     * @brief Register a component with the registry
     *
     * The component must remain valid for the lifetime of the registry.
     * Typically, components are static or heap-allocated during setup().
     *
     * If a component with the same ID already exists, it will be replaced
     * and a warning will be logged.
     *
     * @param component Pointer to component (must not be nullptr)
     * @return true if registered successfully, false on error
     *
     * @note Component ownership remains with caller
     * @warning Component must outlive the registry
     */
    bool registerComponent(IComponent* component) {
        if (!component) {
            ESP_LOGE("registry", "Cannot register null component");
            return false;
        }

        const char* id = component->getId();
        if (!id || strlen(id) == 0) {
            ESP_LOGE("registry", "Component has invalid ID");
            return false;
        }

        // Check for duplicate
        if (m_components.find(id) != m_components.end()) {
            ESP_LOGW("registry", "Component '%s' already registered, replacing", id);
        }

        m_components[id] = component;
        updateCounters();

        ESP_LOGI("registry", "Registered component: %s (type: %s)",
                 id, component->getType());

        return true;
    }

    /**
     * @brief Remove a component from the registry
     *
     * @param id Component ID to remove
     * @return true if component was found and removed, false otherwise
     */
    bool removeComponent(const char* id) {
        if (!id) return false;

        auto it = m_components.find(id);
        if (it == m_components.end()) {
            return false;
        }

        ESP_LOGI("registry", "Removed component: %s", id);
        m_components.erase(it);
        updateCounters();

        return true;
    }

    /**
     * @brief Check if a component with given ID exists
     *
     * @param id Component ID
     * @return true if component exists, false otherwise
     */
    bool has(const char* id) const {
        if (!id) return false;
        return m_components.find(id) != m_components.end();
    }

    /**
     * @brief Get a component by ID
     *
     * @param id Component ID
     * @return Pointer to component, or nullptr if not found
     */
    IComponent* get(const char* id) const {
        if (!id) return nullptr;

        auto it = m_components.find(id);
        if (it == m_components.end()) {
            return nullptr;
        }

        return it->second;
    }

    /**
     * @brief Get all components of a specific type
     *
     * @param type Component type (e.g., "temperature", "relay")
     * @return Vector of matching components
     */
    std::vector<IComponent*> getByType(const char* type) const {
        std::vector<IComponent*> result;

        if (!type) return result;

        for (const auto& pair : m_components) {
            if (strcmp(pair.second->getType(), type) == 0) {
                result.push_back(pair.second);
            }
        }

        return result;
    }

    /**
     * @brief Get all sensors
     *
     * @return Vector of all registered sensors
     */
    std::vector<ISensor*> getAllSensors() const {
        std::vector<ISensor*> result;
        result.reserve(m_sensor_count);

        for (const auto& pair : m_components) {
            ISensor* sensor = dynamic_cast<ISensor*>(pair.second);
            if (sensor) {
                result.push_back(sensor);
            }
        }

        return result;
    }

    /**
     * @brief Get all actuators
     *
     * @return Vector of all registered actuators
     */
    std::vector<IActuator*> getAllActuators() const {
        std::vector<IActuator*> result;
        result.reserve(m_actuator_count);

        for (const auto& pair : m_components) {
            IActuator* actuator = dynamic_cast<IActuator*>(pair.second);
            if (actuator) {
                result.push_back(actuator);
            }
        }

        return result;
    }

    /**
     * @brief Get all visible components
     *
     * @return Vector of components where isVisible() returns true
     */
    std::vector<IComponent*> getVisible() const {
        std::vector<IComponent*> result;

        for (const auto& pair : m_components) {
            if (pair.second->isVisible()) {
                result.push_back(pair.second);
            }
        }

        return result;
    }

    /**
     * @brief Get total number of registered components
     *
     * @return Total component count
     */
    size_t count() const {
        return m_components.size();
    }

    /**
     * @brief Get number of registered sensors
     *
     * @return Sensor count
     */
    size_t sensorCount() const {
        return m_sensor_count;
    }

    /**
     * @brief Get number of registered actuators
     *
     * @return Actuator count
     */
    size_t actuatorCount() const {
        return m_actuator_count;
    }

    /**
     * @brief Clear all registered components
     *
     * Does not delete the components themselves, only removes them from
     * the registry.
     */
    void clear() {
        ESP_LOGI("registry", "Clearing all components");
        m_components.clear();
        m_sensor_count = 0;
        m_actuator_count = 0;
    }

    /**
     * @brief Serialize all components to JSON
     *
     * Returns a JSON array of all registered components:
     * [
     *   {"id":"temp1","type":"temperature",...},
     *   {"id":"relay1","type":"relay",...}
     * ]
     *
     * @param visible_only If true, only include visible components
     * @return JSON string
     */
    std::string toJson(bool visible_only = false) const {
        std::string json = "[";
        bool first = true;

        for (const auto& pair : m_components) {
            IComponent* component = pair.second;

            // Skip invisible components if requested
            if (visible_only && !component->isVisible()) {
                continue;
            }

            if (!first) {
                json += ",";
            }
            first = false;

            json += component->toJson();
        }

        json += "]";
        return json;
    }

    /**
     * @brief Serialize only sensors to JSON
     *
     * @return JSON array of sensors
     */
    std::string sensorsToJson() const {
        std::string json = "[";
        bool first = true;

        for (const auto& pair : m_components) {
            ISensor* sensor = dynamic_cast<ISensor*>(pair.second);
            if (sensor && sensor->isVisible()) {
                if (!first) {
                    json += ",";
                }
                first = false;
                json += sensor->toJson();
            }
        }

        json += "]";
        return json;
    }

    /**
     * @brief Serialize only actuators to JSON
     *
     * @return JSON array of actuators
     */
    std::string actuatorsToJson() const {
        std::string json = "[";
        bool first = true;

        for (const auto& pair : m_components) {
            IActuator* actuator = dynamic_cast<IActuator*>(pair.second);
            if (actuator && actuator->isVisible()) {
                if (!first) {
                    json += ",";
                }
                first = false;
                json += actuator->toJson();
            }
        }

        json += "]";
        return json;
    }

    /**
     * @brief Get summary statistics
     *
     * Returns a JSON object with registry statistics:
     * {
     *   "total": 10,
     *   "sensors": 7,
     *   "actuators": 3,
     *   "visible": 9
     * }
     *
     * @return JSON string with statistics
     */
    std::string getStats() const {
        size_t visible_count = 0;
        for (const auto& pair : m_components) {
            if (pair.second->isVisible()) {
                visible_count++;
            }
        }

        char buffer[256];
        snprintf(buffer, sizeof(buffer),
            "{\"total\":%zu,\"sensors\":%zu,\"actuators\":%zu,\"visible\":%zu}",
            m_components.size(),
            m_sensor_count,
            m_actuator_count,
            visible_count
        );

        return std::string(buffer);
    }

    /**
     * @brief Log all registered components (for debugging)
     */
    void logComponents() const {
        ESP_LOGI("registry", "=== Registered Components ===");
        ESP_LOGI("registry", "Total: %zu (Sensors: %zu, Actuators: %zu)",
                 m_components.size(), m_sensor_count, m_actuator_count);

        for (const auto& pair : m_components) {
            IComponent* comp = pair.second;
            const char* role = "component";

            if (dynamic_cast<ISensor*>(comp)) {
                role = "sensor";
            } else if (dynamic_cast<IActuator*>(comp)) {
                role = "actuator";
            }

            ESP_LOGI("registry", "  [%s] %s (%s) - %s",
                     role,
                     comp->getId(),
                     comp->getType(),
                     comp->isVisible() ? "visible" : "hidden");
        }
    }
};

} // namespace esphome_ui
