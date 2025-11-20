# ADR-001: Web Server Foundation

## Status
Accepted

## Context

For the ESPHome Professional UI Framework, we need to decide on the web server implementation. The framework must:

- Serve static HTML/CSS/JS files efficiently
- Handle RESTful API endpoints
- Support session-based authentication
- Run on ESP32 with limited memory (320KB RAM, 4MB Flash typical)
- Be maintainable and well-documented
- Support custom endpoints for component interactions

### Options Considered

#### Option A: Extend ESPHome's existing `web_server` component

**Pros:**
- Integrated with ESPHome ecosystem
- Users familiar with ESPHome already know it
- Easy YAML configuration
- Automatic sensor/entity discovery

**Cons:**
- Built on AsyncWebServer (Arduino framework)
- Limited customization for complex UIs
- Harder to implement custom authentication flows
- Mixed Arduino/ESP-IDF can cause conflicts
- Less control over memory management
- Difficult to implement large embedded HTML/JS

#### Option B: Build custom AsyncWebServer implementation using ESP-IDF

**Pros:**
- Native ESP-IDF support (no Arduino dependencies)
- Full control over HTTP handling
- Better memory management
- Excellent documentation from Espressif
- Supports large embedded resources
- Easy to implement custom authentication
- Better performance and stability for long-running devices
- Direct access to ESP32 features (NVS, WiFi, etc.)

**Cons:**
- More code to write initially
- Separate from ESPHome's native web_server
- Users need to understand custom component integration

## Decision

We will use **Option B: Custom ESP-IDF AsyncWebServer** implementation.

The existing codebase already demonstrates this approach successfully with:
- `esp_http_server.h` from ESP-IDF
- Custom handlers for all routes
- Embedded HTML/CSS/JS in header files
- Session management with cookies
- NVS integration for persistent config

## Rationale

1. **Memory Efficiency**: ESP-IDF's httpd allows fine-grained control over memory allocation, critical for ESP32

2. **Stability**: The current implementation shows excellent stability with:
   - Connection limits (max 3 concurrent)
   - Automatic LRU purge
   - Disabled keep-alive for predictable memory usage
   - Hardware watchdog integration

3. **Professional Features**: Easier to implement:
   - HTTP Digest Authentication
   - Custom MIME types
   - Large file serving
   - WebSocket support (future)
   - Fine-grained CORS control

4. **ESPHome Integration**: Can still integrate via:
   ```yaml
   esphome:
     includes:
       - framework/include/
   ```

5. **Proven Architecture**: The leak sensor implementation proves this works well in production

## Consequences

### Positive
- Complete control over UI/UX
- Better performance and stability
- Easier to implement framework features
- Clear separation of concerns
- Native ESP32 development experience

### Negative
- Cannot use ESPHome's built-in web_server component
- Need to manually integrate sensor data (acceptable trade-off)
- Slightly more complex initial setup for users

### Mitigation
- Provide clear documentation and examples
- Create helper classes for sensor integration
- Maintain compatibility with ESPHome sensor definitions
- Offer YAML templates for easy integration

## Implementation Notes

The framework will:

1. Use `esp_http_server.h` as HTTP foundation
2. Implement component registry for automatic sensor discovery
3. Provide clean API for registering custom endpoints
4. Include helper functions for ESPHome entity integration
5. Support both embedded and external file resources
6. Implement proper resource cleanup and memory management

## References

- ESP-IDF HTTP Server Documentation: https://docs.espressif.com/projects/esp-idf/en/latest/esp32/api-reference/protocols/esp_http_server.html
- ESPHome Custom Components: https://esphome.io/custom/custom_component.html
- Current implementation: `leak_sensors/custom_components/idf_webserver.h`

## Review Date

This decision should be reviewed if:
- ESPHome introduces major web_server improvements
- ESP-IDF httpd library has breaking changes
- Community feedback suggests alternative approaches
- Performance issues arise that could be solved differently

---

**Date:** 2025-11-20
**Author:** ESPHome UI Framework Team
**Reviewers:** N/A (Initial decision)
