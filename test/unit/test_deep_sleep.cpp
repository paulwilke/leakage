/**
 * Unit Tests for DeepSleepController
 *
 * Tests deep sleep configuration, wake reason detection, and power management.
 */

#include <gtest/gtest.h>
#include "../../framework/include/deep_sleep.h"
#include <vector>

using namespace esphome_ui;

// ============================================================================
// Test Fixture
// ============================================================================

class DeepSleepControllerTest : public ::testing::Test {
protected:
    DeepSleepController* controller;

    void SetUp() override {
        controller = new DeepSleepController();
    }

    void TearDown() override {
        delete controller;
    }
};

// ============================================================================
// Initialization Tests
// ============================================================================

TEST_F(DeepSleepControllerTest, InitializeController) {
    EXPECT_TRUE(controller->init());
}

TEST_F(DeepSleepControllerTest, DefaultConfigDisabled) {
    controller->init();

    DeepSleepController::Config config = controller->getConfig();
    EXPECT_FALSE(config.enabled);
}

// ============================================================================
// Configuration Tests
// ============================================================================

TEST_F(DeepSleepControllerTest, ConfigureTimerWake) {
    controller->init();

    DeepSleepController::Config config;
    config.enabled = true;
    config.sleep_duration_seconds = 60;
    config.wake_on_timer = true;
    config.wake_on_gpio = false;

    EXPECT_TRUE(controller->configure(config));

    DeepSleepController::Config retrieved = controller->getConfig();
    EXPECT_TRUE(retrieved.enabled);
    EXPECT_EQ(retrieved.sleep_duration_seconds, 60);
    EXPECT_TRUE(retrieved.wake_on_timer);
    EXPECT_FALSE(retrieved.wake_on_gpio);
}

TEST_F(DeepSleepControllerTest, ConfigureGpioWake) {
    controller->init();

    DeepSleepController::Config config;
    config.enabled = true;
    config.wake_on_gpio = true;
    config.wake_gpio_pins.push_back(GPIO_NUM_0);
    config.wake_gpio_pins.push_back(GPIO_NUM_4);

    EXPECT_TRUE(controller->configure(config));

    DeepSleepController::Config retrieved = controller->getConfig();
    EXPECT_TRUE(retrieved.wake_on_gpio);
    EXPECT_EQ(retrieved.wake_gpio_pins.size(), 2);
    EXPECT_EQ(retrieved.wake_gpio_pins[0], GPIO_NUM_0);
    EXPECT_EQ(retrieved.wake_gpio_pins[1], GPIO_NUM_4);
}

TEST_F(DeepSleepControllerTest, ConfigureBothWakeSources) {
    controller->init();

    DeepSleepController::Config config;
    config.enabled = true;
    config.sleep_duration_seconds = 300;
    config.wake_on_timer = true;
    config.wake_on_gpio = true;
    config.wake_gpio_pins.push_back(GPIO_NUM_0);

    EXPECT_TRUE(controller->configure(config));

    DeepSleepController::Config retrieved = controller->getConfig();
    EXPECT_TRUE(retrieved.wake_on_timer);
    EXPECT_TRUE(retrieved.wake_on_gpio);
}

TEST_F(DeepSleepControllerTest, ConfigureZeroSleepDuration) {
    controller->init();

    DeepSleepController::Config config;
    config.enabled = true;
    config.sleep_duration_seconds = 0;
    config.wake_on_timer = true;

    // Should fail - cannot have zero duration with timer wake
    EXPECT_FALSE(controller->configure(config));
}

TEST_F(DeepSleepControllerTest, ConfigureNoWakeSources) {
    controller->init();

    DeepSleepController::Config config;
    config.enabled = true;
    config.wake_on_timer = false;
    config.wake_on_gpio = false;

    // Should fail - need at least one wake source
    EXPECT_FALSE(controller->configure(config));
}

TEST_F(DeepSleepControllerTest, ConfigureInvalidGpio) {
    controller->init();

    DeepSleepController::Config config;
    config.enabled = true;
    config.wake_on_gpio = true;
    config.wake_gpio_pins.push_back(GPIO_NUM_MAX);  // Invalid GPIO

    EXPECT_FALSE(controller->configure(config));
}

// ============================================================================
// Sleep Duration Tests
// ============================================================================

TEST_F(DeepSleepControllerTest, SetSleepDuration) {
    controller->init();

    EXPECT_TRUE(controller->setSleepDuration(120));

    DeepSleepController::Config config = controller->getConfig();
    EXPECT_EQ(config.sleep_duration_seconds, 120);
}

TEST_F(DeepSleepControllerTest, SetSleepDurationMinimum) {
    controller->init();

    // Minimum 1 second
    EXPECT_TRUE(controller->setSleepDuration(1));

    DeepSleepController::Config config = controller->getConfig();
    EXPECT_EQ(config.sleep_duration_seconds, 1);
}

TEST_F(DeepSleepControllerTest, SetSleepDurationMaximum) {
    controller->init();

    // Maximum depends on implementation, but should accept large values
    EXPECT_TRUE(controller->setSleepDuration(3600));  // 1 hour

    DeepSleepController::Config config = controller->getConfig();
    EXPECT_EQ(config.sleep_duration_seconds, 3600);
}

TEST_F(DeepSleepControllerTest, ConvertSecondsToMicroseconds) {
    controller->init();

    // Test internal conversion (60 seconds = 60,000,000 microseconds)
    uint64_t micros = controller->secondsToMicroseconds(60);
    EXPECT_EQ(micros, 60000000ULL);
}

TEST_F(DeepSleepControllerTest, ConvertSecondsOverflow) {
    controller->init();

    // Test overflow protection (uint64_t max is ~584,942 years in microseconds)
    uint64_t micros = controller->secondsToMicroseconds(UINT32_MAX);
    EXPECT_GT(micros, 0);
}

// ============================================================================
// Wake Reason Tests
// ============================================================================

TEST_F(DeepSleepControllerTest, GetWakeReasonAfterInit) {
    controller->init();

    DeepSleepController::WakeReason reason = controller->getWakeReason();

    // After init, should detect actual wake reason
    // In test environment, likely RESET
    EXPECT_NE(reason, DeepSleepController::WakeReason::UNKNOWN);
}

TEST_F(DeepSleepControllerTest, GetWakeReasonString) {
    controller->init();

    DeepSleepController::WakeReason reason = controller->getWakeReason();
    std::string reason_str = controller->getWakeReasonString();

    EXPECT_FALSE(reason_str.empty());

    // Check that string matches enum
    if (reason == DeepSleepController::WakeReason::TIMER) {
        EXPECT_EQ(reason_str, "timer");
    } else if (reason == DeepSleepController::WakeReason::GPIO) {
        EXPECT_EQ(reason_str, "gpio");
    } else if (reason == DeepSleepController::WakeReason::RESET) {
        EXPECT_EQ(reason_str, "reset");
    }
}

TEST_F(DeepSleepControllerTest, GetWakeGpioPin) {
    controller->init();

    // If not woken by GPIO, should return -1
    if (controller->getWakeReason() != DeepSleepController::WakeReason::GPIO) {
        EXPECT_EQ(controller->getWakeGpioPin(), -1);
    }
}

// ============================================================================
// Enable/Disable Tests
// ============================================================================

TEST_F(DeepSleepControllerTest, EnableDeepSleep) {
    controller->init();

    EXPECT_TRUE(controller->enable());
    EXPECT_TRUE(controller->isEnabled());
}

TEST_F(DeepSleepControllerTest, DisableDeepSleep) {
    controller->init();
    controller->enable();

    EXPECT_TRUE(controller->disable());
    EXPECT_FALSE(controller->isEnabled());
}

TEST_F(DeepSleepControllerTest, DisableBeforeConfiguration) {
    controller->init();

    // Should be able to disable even without configuration
    EXPECT_TRUE(controller->disable());
    EXPECT_FALSE(controller->isEnabled());
}

// ============================================================================
// State Management Tests
// ============================================================================

TEST_F(DeepSleepControllerTest, IsConfigured) {
    controller->init();

    // Not configured initially
    EXPECT_FALSE(controller->isConfigured());

    DeepSleepController::Config config;
    config.enabled = true;
    config.sleep_duration_seconds = 60;
    config.wake_on_timer = true;

    controller->configure(config);

    // Should be configured now
    EXPECT_TRUE(controller->isConfigured());
}

TEST_F(DeepSleepControllerTest, GetRunDuration) {
    controller->init();

    // Wait a bit
    delay(100);

    uint32_t duration = controller->getRunDuration();

    // Should be at least 100ms
    EXPECT_GE(duration, 100);
    EXPECT_LT(duration, 200);  // But not much more
}

TEST_F(DeepSleepControllerTest, ResetRunTime) {
    controller->init();

    delay(100);
    uint32_t duration1 = controller->getRunDuration();
    EXPECT_GE(duration1, 100);

    controller->resetRunTime();

    uint32_t duration2 = controller->getRunDuration();
    EXPECT_LT(duration2, 50);  // Should be near zero after reset
}

// ============================================================================
// Validation Tests
// ============================================================================

TEST_F(DeepSleepControllerTest, ValidateConfigAllOptionsValid) {
    controller->init();

    DeepSleepController::Config config;
    config.enabled = true;
    config.sleep_duration_seconds = 60;
    config.wake_on_timer = true;
    config.wake_on_gpio = true;
    config.wake_gpio_pins.push_back(GPIO_NUM_0);

    EXPECT_TRUE(controller->validateConfig(config));
}

TEST_F(DeepSleepControllerTest, ValidateConfigInvalidDuration) {
    controller->init();

    DeepSleepController::Config config;
    config.enabled = true;
    config.sleep_duration_seconds = 0;
    config.wake_on_timer = true;

    EXPECT_FALSE(controller->validateConfig(config));
}

TEST_F(DeepSleepControllerTest, ValidateConfigNoWakeSource) {
    controller->init();

    DeepSleepController::Config config;
    config.enabled = true;
    config.sleep_duration_seconds = 60;
    config.wake_on_timer = false;
    config.wake_on_gpio = false;

    EXPECT_FALSE(controller->validateConfig(config));
}

TEST_F(DeepSleepControllerTest, ValidateConfigGpioWithoutPins) {
    controller->init();

    DeepSleepController::Config config;
    config.enabled = true;
    config.wake_on_gpio = true;
    config.wake_gpio_pins.clear();  // No pins configured

    EXPECT_FALSE(controller->validateConfig(config));
}

TEST_F(DeepSleepControllerTest, ValidateConfigTooManyGpioPins) {
    controller->init();

    DeepSleepController::Config config;
    config.enabled = true;
    config.wake_on_gpio = true;

    // Add many GPIO pins (ESP32 RTC has limited wake pins)
    for (int i = 0; i < 20; i++) {
        config.wake_gpio_pins.push_back(static_cast<gpio_num_t>(i));
    }

    // Should fail - too many wake pins
    EXPECT_FALSE(controller->validateConfig(config));
}

// ============================================================================
// Statistics Tests
// ============================================================================

TEST_F(DeepSleepControllerTest, GetStats) {
    controller->init();

    DeepSleepController::Config config;
    config.enabled = true;
    config.sleep_duration_seconds = 120;
    config.wake_on_timer = true;
    controller->configure(config);

    std::string stats = controller->getStats();

    // Should contain enabled status
    EXPECT_NE(stats.find("\"enabled\":true"), std::string::npos);

    // Should contain sleep duration
    EXPECT_NE(stats.find("\"sleep_duration\":120"), std::string::npos);

    // Should contain wake reason
    EXPECT_NE(stats.find("\"wake_reason\""), std::string::npos);
}

TEST_F(DeepSleepControllerTest, GetStatsDisabled) {
    controller->init();

    std::string stats = controller->getStats();

    EXPECT_NE(stats.find("\"enabled\":false"), std::string::npos);
}

// ============================================================================
// JSON Serialization Tests
// ============================================================================

TEST_F(DeepSleepControllerTest, ToJsonFullConfig) {
    controller->init();

    DeepSleepController::Config config;
    config.enabled = true;
    config.sleep_duration_seconds = 300;
    config.wake_on_timer = true;
    config.wake_on_gpio = true;
    config.wake_gpio_pins.push_back(GPIO_NUM_0);
    config.wake_gpio_pins.push_back(GPIO_NUM_4);

    controller->configure(config);

    std::string json = controller->toJson();

    EXPECT_NE(json.find("\"enabled\":true"), std::string::npos);
    EXPECT_NE(json.find("\"sleep_duration\":300"), std::string::npos);
    EXPECT_NE(json.find("\"wake_on_timer\":true"), std::string::npos);
    EXPECT_NE(json.find("\"wake_on_gpio\":true"), std::string::npos);
    EXPECT_NE(json.find("wake_gpio_pins"), std::string::npos);
}

// ============================================================================
// Edge Cases
// ============================================================================

TEST_F(DeepSleepControllerTest, ConfigureBeforeInit) {
    // Don't call init()

    DeepSleepController::Config config;
    config.enabled = true;
    config.sleep_duration_seconds = 60;
    config.wake_on_timer = true;

    // Should fail - not initialized
    EXPECT_FALSE(controller->configure(config));
}

TEST_F(DeepSleepControllerTest, MultipleConfigure) {
    controller->init();

    DeepSleepController::Config config1;
    config1.enabled = true;
    config1.sleep_duration_seconds = 60;
    config1.wake_on_timer = true;

    EXPECT_TRUE(controller->configure(config1));

    // Reconfigure with different settings
    DeepSleepController::Config config2;
    config2.enabled = true;
    config2.sleep_duration_seconds = 120;
    config2.wake_on_timer = true;

    EXPECT_TRUE(controller->configure(config2));

    DeepSleepController::Config retrieved = controller->getConfig();
    EXPECT_EQ(retrieved.sleep_duration_seconds, 120);
}

TEST_F(DeepSleepControllerTest, VeryLongSleepDuration) {
    controller->init();

    // Test with very long duration (7 days)
    uint32_t seven_days = 7 * 24 * 3600;
    EXPECT_TRUE(controller->setSleepDuration(seven_days));

    DeepSleepController::Config config = controller->getConfig();
    EXPECT_EQ(config.sleep_duration_seconds, seven_days);
}

TEST_F(DeepSleepControllerTest, VeryShortSleepDuration) {
    controller->init();

    // 1 second should be minimum
    EXPECT_TRUE(controller->setSleepDuration(1));

    DeepSleepController::Config config = controller->getConfig();
    EXPECT_EQ(config.sleep_duration_seconds, 1);
}

TEST_F(DeepSleepControllerTest, DuplicateGpioPins) {
    controller->init();

    DeepSleepController::Config config;
    config.enabled = true;
    config.wake_on_gpio = true;
    config.wake_gpio_pins.push_back(GPIO_NUM_0);
    config.wake_gpio_pins.push_back(GPIO_NUM_0);  // Duplicate

    // Should handle duplicates gracefully (either accept or reject)
    bool result = controller->configure(config);

    if (result) {
        // If accepted, verify configuration
        DeepSleepController::Config retrieved = controller->getConfig();
        EXPECT_GE(retrieved.wake_gpio_pins.size(), 1);
    }
}

// ============================================================================
// Power Calculation Tests
// ============================================================================

TEST_F(DeepSleepControllerTest, CalculateBatteryLifeDays) {
    controller->init();

    // Example: 2000mAh battery, 50mA active, 10µA sleep, 5 min active per hour
    float days = controller->calculateBatteryLifeDays(
        2000,    // mAh capacity
        50,      // mA active current
        0.01,    // mA sleep current (10µA)
        5,       // minutes active per hour
        55       // minutes sleeping per hour
    );

    // Should be many days
    EXPECT_GT(days, 10);
    EXPECT_LT(days, 10000);  // Reasonable upper bound
}

TEST_F(DeepSleepControllerTest, CalculateBatteryLifeAlwaysActive) {
    controller->init();

    // Device always active, never sleeps
    float days = controller->calculateBatteryLifeDays(
        2000,   // mAh capacity
        50,     // mA active current
        0.01,   // mA sleep current (not used)
        60,     // minutes active per hour
        0       // minutes sleeping per hour
    );

    // 2000mAh / 50mA / 24h = ~1.67 days
    EXPECT_GT(days, 1.5);
    EXPECT_LT(days, 2.0);
}

TEST_F(DeepSleepControllerTest, CalculateBatteryLifeAlwaysSleeping) {
    controller->init();

    // Device always sleeping
    float days = controller->calculateBatteryLifeDays(
        2000,   // mAh capacity
        50,     // mA active current (not used)
        0.01,   // mA sleep current (10µA)
        0,      // minutes active per hour
        60      // minutes sleeping per hour
    );

    // 2000mAh / 0.01mA / 24h = ~8333 days (22+ years)
    EXPECT_GT(days, 1000);
}

// ============================================================================
// Main
// ============================================================================

int main(int argc, char **argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
