/**
 * Unit Tests for ComponentRegistry
 *
 * Tests the component registration, lookup, and serialization functionality.
 */

#include <gtest/gtest.h>
#include "../../framework/include/component.h"
#include "../../framework/include/registry.h"
#include <string>
#include <cmath>

using namespace esphome_ui;

// ============================================================================
// Mock Components for Testing
// ============================================================================

class MockSensor : public ISensor {
private:
    std::string m_id;
    std::string m_name;
    std::string m_type;
    float m_value;
    bool m_available;
    bool m_visible;

public:
    MockSensor(const char* id, const char* type, float value = 0.0f)
        : m_id(id)
        , m_name(id)
        , m_type(type)
        , m_value(value)
        , m_available(true)
        , m_visible(true)
    {}

    const char* getId() const override { return m_id.c_str(); }
    const char* getName() const override { return m_name.c_str(); }
    const char* getType() const override { return m_type.c_str(); }
    const char* getUnit() const override { return "unit"; }
    float getValue() override { return m_value; }
    bool isAvailable() override { return m_available; }
    bool isVisible() const override { return m_visible; }
    bool requiresAuth() const override { return false; }

    void setValue(float v) { m_value = v; }
    void setAvailable(bool a) { m_available = a; }
    void setVisible(bool v) { m_visible = v; }
};

class MockActuator : public IActuator {
private:
    std::string m_id;
    std::string m_name;
    std::string m_state;
    bool m_available;

public:
    MockActuator(const char* id)
        : m_id(id)
        , m_name(id)
        , m_state("{\"state\":\"off\"}")
        , m_available(true)
    {}

    const char* getId() const override { return m_id.c_str(); }
    const char* getName() const override { return m_name.c_str(); }
    const char* getType() const override { return "actuator"; }
    bool isVisible() const override { return true; }
    bool requiresAuth() const override { return true; }

    bool setState(const std::string& json) override {
        m_state = json;
        return true;
    }

    std::string getState() const override { return m_state; }
    bool isAvailable() override { return m_available; }
};

// ============================================================================
// Test Fixture
// ============================================================================

class ComponentRegistryTest : public ::testing::Test {
protected:
    ComponentRegistry registry;
    MockSensor* sensor1;
    MockSensor* sensor2;
    MockActuator* actuator1;

    void SetUp() override {
        sensor1 = new MockSensor("temp1", "temperature", 23.5f);
        sensor2 = new MockSensor("humidity1", "humidity", 65.0f);
        actuator1 = new MockActuator("relay1");
    }

    void TearDown() override {
        delete sensor1;
        delete sensor2;
        delete actuator1;
    }
};

// ============================================================================
// Registration Tests
// ============================================================================

TEST_F(ComponentRegistryTest, RegisterSingleComponent) {
    EXPECT_TRUE(registry.registerComponent(sensor1));
    EXPECT_EQ(registry.count(), 1);
}

TEST_F(ComponentRegistryTest, RegisterMultipleComponents) {
    registry.registerComponent(sensor1);
    registry.registerComponent(sensor2);
    registry.registerComponent(actuator1);

    EXPECT_EQ(registry.count(), 3);
    EXPECT_EQ(registry.sensorCount(), 2);
    EXPECT_EQ(registry.actuatorCount(), 1);
}

TEST_F(ComponentRegistryTest, RegisterNullComponent) {
    EXPECT_FALSE(registry.registerComponent(nullptr));
    EXPECT_EQ(registry.count(), 0);
}

TEST_F(ComponentRegistryTest, RegisterDuplicateId) {
    registry.registerComponent(sensor1);

    // Registering same ID should replace
    MockSensor* duplicate = new MockSensor("temp1", "temperature", 99.0f);
    EXPECT_TRUE(registry.registerComponent(duplicate));
    EXPECT_EQ(registry.count(), 1);  // Still only 1

    delete duplicate;
}

// ============================================================================
// Lookup Tests
// ============================================================================

TEST_F(ComponentRegistryTest, HasComponent) {
    registry.registerComponent(sensor1);

    EXPECT_TRUE(registry.has("temp1"));
    EXPECT_FALSE(registry.has("nonexistent"));
}

TEST_F(ComponentRegistryTest, GetComponent) {
    registry.registerComponent(sensor1);

    IComponent* found = registry.get("temp1");
    ASSERT_NE(found, nullptr);
    EXPECT_STREQ(found->getId(), "temp1");
}

TEST_F(ComponentRegistryTest, GetNonexistentComponent) {
    IComponent* found = registry.get("nonexistent");
    EXPECT_EQ(found, nullptr);
}

TEST_F(ComponentRegistryTest, GetByType) {
    registry.registerComponent(sensor1);
    registry.registerComponent(sensor2);

    auto temps = registry.getByType("temperature");
    EXPECT_EQ(temps.size(), 1);
    EXPECT_STREQ(temps[0]->getId(), "temp1");

    auto humidity = registry.getByType("humidity");
    EXPECT_EQ(humidity.size(), 1);
}

TEST_F(ComponentRegistryTest, GetAllSensors) {
    registry.registerComponent(sensor1);
    registry.registerComponent(sensor2);
    registry.registerComponent(actuator1);

    auto sensors = registry.getAllSensors();
    EXPECT_EQ(sensors.size(), 2);
}

TEST_F(ComponentRegistryTest, GetAllActuators) {
    registry.registerComponent(sensor1);
    registry.registerComponent(actuator1);

    auto actuators = registry.getAllActuators();
    EXPECT_EQ(actuators.size(), 1);
}

// ============================================================================
// Visibility Tests
// ============================================================================

TEST_F(ComponentRegistryTest, GetVisibleComponents) {
    registry.registerComponent(sensor1);

    sensor2->setVisible(false);
    registry.registerComponent(sensor2);

    auto visible = registry.getVisible();
    EXPECT_EQ(visible.size(), 1);
    EXPECT_STREQ(visible[0]->getId(), "temp1");
}

// ============================================================================
// Removal Tests
// ============================================================================

TEST_F(ComponentRegistryTest, RemoveComponent) {
    registry.registerComponent(sensor1);
    EXPECT_EQ(registry.count(), 1);

    EXPECT_TRUE(registry.removeComponent("temp1"));
    EXPECT_EQ(registry.count(), 0);
}

TEST_F(ComponentRegistryTest, RemoveNonexistentComponent) {
    EXPECT_FALSE(registry.removeComponent("nonexistent"));
}

TEST_F(ComponentRegistryTest, ClearAll) {
    registry.registerComponent(sensor1);
    registry.registerComponent(sensor2);
    registry.registerComponent(actuator1);

    EXPECT_EQ(registry.count(), 3);

    registry.clear();
    EXPECT_EQ(registry.count(), 0);
}

// ============================================================================
// JSON Serialization Tests
// ============================================================================

TEST_F(ComponentRegistryTest, ToJsonEmpty) {
    std::string json = registry.toJson();
    EXPECT_EQ(json, "[]");
}

TEST_F(ComponentRegistryTest, ToJsonSingleComponent) {
    registry.registerComponent(sensor1);
    std::string json = registry.toJson();

    // Should contain the sensor ID
    EXPECT_NE(json.find("temp1"), std::string::npos);
    EXPECT_NE(json.find("temperature"), std::string::npos);
}

TEST_F(ComponentRegistryTest, ToJsonMultipleComponents) {
    registry.registerComponent(sensor1);
    registry.registerComponent(sensor2);

    std::string json = registry.toJson();

    EXPECT_NE(json.find("temp1"), std::string::npos);
    EXPECT_NE(json.find("humidity1"), std::string::npos);
}

TEST_F(ComponentRegistryTest, ToJsonVisibleOnly) {
    registry.registerComponent(sensor1);

    sensor2->setVisible(false);
    registry.registerComponent(sensor2);

    std::string json = registry.toJson(true);  // Visible only

    EXPECT_NE(json.find("temp1"), std::string::npos);
    EXPECT_EQ(json.find("humidity1"), std::string::npos);
}

TEST_F(ComponentRegistryTest, SensorsToJson) {
    registry.registerComponent(sensor1);
    registry.registerComponent(actuator1);

    std::string json = registry.sensorsToJson();

    EXPECT_NE(json.find("temp1"), std::string::npos);
    EXPECT_EQ(json.find("relay1"), std::string::npos);  // Actuator not included
}

TEST_F(ComponentRegistryTest, ActuatorsToJson) {
    registry.registerComponent(sensor1);
    registry.registerComponent(actuator1);

    std::string json = registry.actuatorsToJson();

    EXPECT_EQ(json.find("temp1"), std::string::npos);  // Sensor not included
    EXPECT_NE(json.find("relay1"), std::string::npos);
}

// ============================================================================
// Statistics Tests
// ============================================================================

TEST_F(ComponentRegistryTest, GetStats) {
    registry.registerComponent(sensor1);
    registry.registerComponent(sensor2);
    registry.registerComponent(actuator1);

    std::string stats = registry.getStats();

    // Should contain counts
    EXPECT_NE(stats.find("\"total\":3"), std::string::npos);
    EXPECT_NE(stats.find("\"sensors\":2"), std::string::npos);
    EXPECT_NE(stats.find("\"actuators\":1"), std::string::npos);
}

// ============================================================================
// Main
// ============================================================================

int main(int argc, char **argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
