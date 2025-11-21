#pragma once

/**
 * ESP-IDF Compatibility Layer for Native Testing
 *
 * This file provides mock implementations of ESP-IDF functions and macros
 * so that framework code can compile and run on native platforms for unit testing.
 */

#ifdef UNIT_TEST

#include <cstdio>
#include <cstring>

// Logging macros - map to printf for native testing
#define ESP_LOGE(tag, format, ...) printf("[E][%s] " format "\n", tag, ##__VA_ARGS__)
#define ESP_LOGW(tag, format, ...) printf("[W][%s] " format "\n", tag, ##__VA_ARGS__)
#define ESP_LOGI(tag, format, ...) printf("[I][%s] " format "\n", tag, ##__VA_ARGS__)
#define ESP_LOGD(tag, format, ...) printf("[D][%s] " format "\n", tag, ##__VA_ARGS__)
#define ESP_LOGV(tag, format, ...) printf("[V][%s] " format "\n", tag, ##__VA_ARGS__)

// String utilities
#define ESP_OK 0
#define ESP_FAIL -1

// GPIO definitions (mock)
typedef int gpio_num_t;
#define GPIO_NUM_0 0
#define GPIO_NUM_4 4
#define GPIO_NUM_MAX 40

// Memory functions (mock)
inline uint32_t esp_get_free_heap_size() { return 200000; }
inline uint32_t esp_get_minimum_free_heap_size() { return 150000; }

// Time functions (mock)
inline unsigned long millis() {
    return 0; // Can be overridden in tests
}

inline void delay(unsigned long ms) {
    // No-op for tests
}

#endif // UNIT_TEST
