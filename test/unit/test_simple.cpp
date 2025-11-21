/**
 * Simplified Unit Tests for ESPHome UI Framework
 *
 * These tests verify basic compilation and structure of framework components.
 * Full integration testing happens via ESPHome compilation.
 */

#include <gtest/gtest.h>
#include "esp_mock.h"
#include "../../framework/include/component.h"
#include "../../framework/include/registry.h"
#include <string>

using namespace esphome_ui;

// ============================================================================
// Mock Sensor for Testing
// ============================================================================

class TestSensor : public ISensor {
private:
    std::string m_id;
    float m_value;

public:
    TestSensor(const char* id, float value)
        : m_id(id), m_value(value) {}

    const char* getId() const override { return m_id.c_str(); }
    const char* getName() const override { return "Test Sensor"; }
    const char* getType() const override { return "test"; }
    const char* getUnit() const override { return "unit"; }
    float getValue() override { return m_value; }
    bool isAvailable() override { return true; }
    bool isVisible() const override { return true; }
    bool requiresAuth() const override { return false; }

    std::string toJson() const override {
        return "{\"id\":\"" + m_id + "\",\"value\":" + std::to_string(m_value) + "}";
    }
};

// ============================================================================
// Basic Registry Tests
// ============================================================================

TEST(ComponentRegistryTest, CanCreateRegistry) {
    ComponentRegistry registry;
    EXPECT_EQ(registry.count(), 0);
}

TEST(ComponentRegistryTest, CanRegisterComponent) {
    ComponentRegistry registry;
    TestSensor sensor("test1", 42.0f);

    EXPECT_TRUE(registry.registerComponent(&sensor));
    EXPECT_EQ(registry.count(), 1);
}

TEST(ComponentRegistryTest, CanLookupComponent) {
    ComponentRegistry registry;
    TestSensor sensor("test1", 42.0f);

    registry.registerComponent(&sensor);

    IComponent* found = registry.get("test1");
    EXPECT_NE(found, nullptr);
    EXPECT_STREQ(found->getId(), "test1");
}

TEST(ComponentRegistryTest, LookupReturnsNullForMissing) {
    ComponentRegistry registry;

    IComponent* found = registry.get("nonexistent");
    EXPECT_EQ(found, nullptr);
}

TEST(ComponentRegistryTest, CanRegisterMultipleComponents) {
    ComponentRegistry registry;
    TestSensor sensor1("test1", 10.0f);
    TestSensor sensor2("test2", 20.0f);
    TestSensor sensor3("test3", 30.0f);

    registry.registerComponent(&sensor1);
    registry.registerComponent(&sensor2);
    registry.registerComponent(&sensor3);

    EXPECT_EQ(registry.count(), 3);
}

TEST(ComponentRegistryTest, CanRemoveComponent) {
    ComponentRegistry registry;
    TestSensor sensor("test1", 42.0f);

    registry.registerComponent(&sensor);
    EXPECT_EQ(registry.count(), 1);

    EXPECT_TRUE(registry.removeComponent("test1"));
    EXPECT_EQ(registry.count(), 0);
}

TEST(ComponentRegistryTest, RemoveNonexistentReturnsFalse) {
    ComponentRegistry registry;

    EXPECT_FALSE(registry.removeComponent("nonexistent"));
}

TEST(ComponentRegistryTest, CanGetAllSensors) {
    ComponentRegistry registry;
    TestSensor sensor1("test1", 10.0f);
    TestSensor sensor2("test2", 20.0f);

    registry.registerComponent(&sensor1);
    registry.registerComponent(&sensor2);

    std::vector<ISensor*> sensors = registry.getAllSensors();
    EXPECT_EQ(sensors.size(), 2);
}

TEST(ComponentRegistryTest, JsonSerializationWorks) {
    ComponentRegistry registry;
    TestSensor sensor("test1", 42.5f);

    registry.registerComponent(&sensor);

    std::string json = registry.toJson();

    // Basic validation - check if it contains expected data
    EXPECT_NE(json.find("test1"), std::string::npos);
    EXPECT_NE(json.find("42.5"), std::string::npos);
}

// ============================================================================
// Component Interface Tests
// ============================================================================

TEST(ComponentTest, SensorHasValidInterface) {
    TestSensor sensor("test", 42.0f);

    EXPECT_STREQ(sensor.getId(), "test");
    EXPECT_STREQ(sensor.getType(), "test");
    EXPECT_STREQ(sensor.getUnit(), "unit");
    EXPECT_FLOAT_EQ(sensor.getValue(), 42.0f);
    EXPECT_TRUE(sensor.isAvailable());
    EXPECT_TRUE(sensor.isVisible());
    EXPECT_FALSE(sensor.requiresAuth());
}

TEST(ComponentTest, SensorCanSerializeToJson) {
    TestSensor sensor("test1", 123.45f);

    std::string json = sensor.toJson();

    EXPECT_FALSE(json.empty());
    EXPECT_NE(json.find("test1"), std::string::npos);
}

// ============================================================================
// Main
// ============================================================================

int main(int argc, char **argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
