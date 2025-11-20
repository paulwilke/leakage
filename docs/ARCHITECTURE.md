# ESPHome UI Framework - Architecture

## Overview

The ESPHome UI Framework is a production-ready, minimalist UI framework for ESP32/ESPHome devices. It provides a complete solution for building professional IoT products with web-based interfaces.

## Design Principles

1. **Minimalism**: Every byte counts - no unnecessary features
2. **Transparency**: Code is self-documenting and easy to understand
3. **Modularity**: Components are self-contained with clear interfaces
4. **Testability**: All business logic is unit-testable
5. **Resource Efficiency**: Optimized for ESP32 constraints

## Project Structure

```
esp-ui-framework/
├── framework/                      # Core framework code
│   ├── include/                    # Public headers
│   │   ├── component.h            # Base component classes
│   │   ├── sensor.h               # Sensor interface
│   │   ├── actuator.h             # Actuator interface
│   │   ├── registry.h             # Component registry
│   │   ├── auth.h                 # Authentication controller
│   │   ├── deep_sleep.h           # Deep sleep controller
│   │   ├── config_storage.h       # NVS configuration (existing)
│   │   └── ui_framework.h         # Main framework class
│   │
│   └── src/                        # Implementation files
│       ├── component.cpp
│       ├── registry.cpp
│       ├── auth.cpp
│       ├── deep_sleep.cpp
│       ├── config_storage.cpp     # Migrated from custom_components
│       └── ui_framework.cpp       # Web server + API endpoints
│
├── examples/                       # Example implementations
│   ├── bme280/                    # BME280 sensor example
│   │   ├── bme280_sensor.h
│   │   ├── bme280_sensor.cpp
│   │   └── bme280.yaml            # ESPHome config
│   │
│   ├── ds18b20/                   # DS18B20 sensor example
│   │   ├── ds18b20_sensor.h
│   │   ├── ds18b20_sensor.cpp
│   │   └── ds18b20.yaml
│   │
│   ├── relay/                     # Relay actuator example
│   │   ├── relay_actuator.h
│   │   ├── relay_actuator.cpp
│   │   └── relay.yaml
│   │
│   └── demo/                      # Full demo with all features
│       ├── demo.yaml
│       └── README.md
│
├── web/                           # Frontend assets
│   ├── index.html                 # Main SPA shell
│   │
│   ├── css/                       # Stylesheets
│   │   ├── main.css              # Core styles
│   │   ├── themes.css            # Theme variables
│   │   └── components.css        # Component styles
│   │
│   ├── js/                        # JavaScript modules
│   │   ├── app.js                # Main application
│   │   ├── api.js                # API client
│   │   ├── components.js         # UI component system
│   │   ├── i18n.js               # Internationalization
│   │   ├── charts.js             # Lazy chart loader
│   │   └── utils.js              # Utility functions
│   │
│   ├── lang/                      # Translation files
│   │   ├── en.json               # English
│   │   ├── de.json               # German
│   │   └── README.md             # Translation guide
│   │
│   └── vendor/                    # Third-party libraries
│       ├── uplot.min.js          # Lazy-loaded chart library
│       └── uplot.min.css
│
├── test/                          # Test suites
│   ├── unit/                      # C++ unit tests
│   │   ├── test_component.cpp
│   │   ├── test_registry.cpp
│   │   ├── test_auth.cpp
│   │   ├── test_deep_sleep.cpp
│   │   ├── platformio.ini        # Test configuration
│   │   └── mocks/                # Mock objects
│   │       ├── mock_sensor.h
│   │       └── mock_actuator.h
│   │
│   ├── integration/               # Integration tests
│   │   ├── test_compilation.py
│   │   ├── test_api.py
│   │   ├── conftest.py
│   │   └── requirements.txt
│   │
│   ├── frontend/                  # Frontend tests
│   │   ├── test_runner.html
│   │   └── tests/
│   │       ├── test_api.js
│   │       ├── test_i18n.js
│   │       └── test_components.js
│   │
│   └── manual/                    # Manual test docs
│       ├── testing-checklist.md
│       └── test-scenarios.md
│
├── docs/                          # Documentation
│   ├── USER_GUIDE.md             # User documentation
│   ├── DEVELOPER_GUIDE.md        # Developer documentation
│   ├── LLM_EXTENSION_GUIDE.md    # LLM extension guide
│   ├── API.md                     # API documentation
│   ├── ARCHITECTURE.md           # This file
│   ├── CONTRIBUTING.md           # Contribution guidelines
│   │
│   └── adr/                       # Architecture Decision Records
│       ├── ADR-001-webserver-foundation.md
│       ├── ADR-002-authentication-method.md
│       ├── ADR-003-frontend-framework.md
│       ├── ADR-004-internationalization.md
│       └── ADR-005-testing-strategy.md
│
├── scripts/                       # Build and utility scripts
│   ├── compress_web.py           # Gzip web assets
│   ├── generate_docs.py          # Auto-generate API docs
│   ├── validate_translations.py  # Check translation completeness
│   └── benchmark.py              # Performance benchmarks
│
├── .github/                       # GitHub configuration
│   └── workflows/
│       ├── ci.yml                # CI/CD pipeline
│       └── release.yml           # Release automation
│
├── platformio.ini                 # PlatformIO configuration
├── library.json                   # PlatformIO library manifest
├── README.md                      # Project README
├── LICENSE                        # License file
└── .gitignore                     # Git ignore rules
```

## Component Architecture

### Core Classes

```
┌─────────────────────────────────────────────────────────────┐
│                        IComponent                            │
│  - getId(): string                                           │
│  - getType(): string                                         │
│  - isVisible(): bool                                         │
│  - toJson(): JsonObject                                      │
└─────────────────────────────────────────────────────────────┘
                            ▲
                            │
            ┌───────────────┴───────────────┐
            │                               │
┌───────────────────────┐       ┌───────────────────────┐
│      ISensor          │       │     IActuator         │
│ - getValue(): float   │       │ - setState(json)      │
│ - getUnit(): string   │       │ - getState(): json    │
│ - isAvailable(): bool │       │                       │
└───────────────────────┘       └───────────────────────┘
            ▲                               ▲
            │                               │
    ┌───────┴────────┐            ┌────────┴────────┐
    │                │            │                 │
┌────────┐    ┌──────────┐   ┌────────┐    ┌─────────┐
│BME280  │    │DS18B20   │   │Relay   │    │Switch   │
│Sensor  │    │Sensor    │   │Actuator│    │Actuator │
└────────┘    └──────────┘   └────────┘    └─────────┘
```

### Component Registry

The registry maintains a list of all registered components and provides:

- Automatic discovery
- Type-based filtering
- JSON serialization
- Lifecycle management

```cpp
class ComponentRegistry {
public:
    void registerComponent(IComponent* component);
    void removeComponent(const char* id);

    bool has(const char* id) const;
    IComponent* get(const char* id) const;
    std::vector<IComponent*> getByType(const char* type) const;
    std::vector<ISensor*> getAllSensors() const;
    std::vector<IActuator*> getAllActuators() const;

    size_t count() const;
    std::string toJson() const;
};
```

## Data Flow

### Startup Sequence

```
1. ESP32 Boot
   ↓
2. ESPHome Initialization
   ↓
3. UIFramework::setup()
   ├─ ConfigStorage::init()
   ├─ AuthController::init()
   ├─ DeepSleepController::init()
   ├─ ComponentRegistry::init()
   └─ WebServer::start()
   ↓
4. Component Registration
   ├─ BME280Sensor::setup() → registry.registerComponent()
   ├─ DS18B20Sensor::setup() → registry.registerComponent()
   └─ RelayActuator::setup() → registry.registerComponent()
   ↓
5. Ready - Web UI Accessible
```

### API Request Flow

```
Browser                ESP32                  Registry
   │                     │                        │
   │  GET /api/sensors   │                        │
   │────────────────────>│                        │
   │                     │  getAllSensors()       │
   │                     │───────────────────────>│
   │                     │  [sensor1, sensor2]    │
   │                     │<───────────────────────│
   │                     │                        │
   │                     │  sensor1.getValue()    │
   │                     │  sensor2.getValue()    │
   │                     │                        │
   │   JSON Response     │                        │
   │<────────────────────│                        │
   │                     │                        │
```

### Actuator Control Flow

```
Browser                ESP32                  Actuator
   │                     │                        │
   │ POST /api/actuators/relay1                   │
   │ {"state": "on"}     │                        │
   │────────────────────>│                        │
   │                     │  Check Auth            │
   │                     │  (Session Valid?)      │
   │                     │                        │
   │                     │  registry.get("relay1")│
   │                     │  actuator.setState()   │
   │                     │───────────────────────>│
   │                     │                        │
   │                     │       setState(on)     │
   │                     │       (GPIO HIGH)      │
   │                     │                        │
   │   JSON Success      │                        │
   │<────────────────────│                        │
   │                     │                        │
```

## Web Architecture

### Single Page Application

The frontend is a simple SPA with:

- **index.html**: Application shell
- **Router**: Hash-based routing (#/dashboard, #/settings)
- **Components**: Vanilla JS component classes
- **State**: Simple observable pattern
- **API**: Fetch-based client

### Page Structure

```
┌────────────────────────────────────────────────────────┐
│  Header                                                 │
│  [Logo]  Device Name                    [EN/DE] [User] │
├────────────────────────────────────────────────────────┤
│  Navigation                                             │
│  [Dashboard] [Settings] [About]                         │
├────────────────────────────────────────────────────────┤
│                                                         │
│  Content Area (Router-driven)                           │
│                                                         │
│  #/dashboard  → Dashboard View                          │
│  #/settings   → Settings View                           │
│  #/about      → About View                              │
│                                                         │
├────────────────────────────────────────────────────────┤
│  Footer                                                 │
│  Status: Online | Uptime: 2d 4h | Memory: 234KB        │
└────────────────────────────────────────────────────────┘
```

### Component Hierarchy

```
App
├── Header
│   ├── Logo
│   ├── DeviceName
│   ├── LanguageSwitcher
│   └── UserMenu
│
├── Navigation
│   └── NavLink[]
│
├── Router
│   ├── DashboardView
│   │   ├── SensorGrid
│   │   │   └── SensorCard[]
│   │   ├── ActuatorGrid
│   │   │   └── ActuatorCard[]
│   │   └── StatusPanel
│   │
│   ├── SettingsView
│   │   ├── WiFiSettings
│   │   ├── SystemSettings
│   │   ├── DeepSleepSettings
│   │   └── AboutSection
│   │
│   └── ChartView (lazy loaded)
│       └── ChartComponent
│
└── Footer
    └── StatusBar
```

## API Endpoints

### Public Endpoints (No Auth Required)

```
GET  /                      # Web UI (HTML)
GET  /api/status            # Device status
GET  /api/components        # List all components
GET  /api/sensors           # All sensor values
GET  /api/sensors/{id}      # Specific sensor
GET  /lang/{lang}.json      # Translation file
```

### Protected Endpoints (Auth Required)

```
POST /api/auth/login        # Login
POST /api/auth/logout       # Logout
GET  /api/config            # Get configuration
POST /api/config            # Update configuration
POST /api/actuators/{id}    # Control actuator
GET  /api/sleep             # Deep sleep status
POST /api/sleep             # Configure deep sleep
POST /api/system/restart    # Restart device
POST /api/system/reset      # Factory reset
```

## Memory Management

### Flash Usage (Typical)

```
ESPHome Core:          ~500 KB
Framework:             ~200 KB
User Components:       ~100 KB
Web Assets (gzipped):   ~15 KB
-------------------------------------
Total:                 ~815 KB / 4MB (20%)
```

### RAM Usage (Runtime)

```
ESPHome:               ~80 KB
Framework:             ~50 KB
Component Registry:    ~20 KB
Web Server:            ~40 KB
Session Data:          ~10 KB
-------------------------------------
Total:                ~200 KB / 320KB (62%)
```

### Optimization Strategies

1. **Static Allocation**: Prefer stack and static storage
2. **String Pool**: Reuse string buffers
3. **Lazy Loading**: Load charts only when needed
4. **Compression**: Gzip all static assets
5. **Connection Limits**: Max 3 concurrent HTTP connections
6. **Session Cleanup**: Periodic cleanup of expired sessions

## Security Model

### Authentication Flow

```
1. User visits /login
2. Enters username/password
3. Server validates credentials
4. Server generates random session ID (128-bit entropy)
5. Server sets HTTP-only cookie: session={id}
6. Client stores cookie
7. Subsequent requests include cookie
8. Server validates session on each request
```

### Session Management

```cpp
struct Session {
    std::string id;           // 32-char hex string
    time_t created;           // Session creation time
    time_t expires;           // Expiration time (created + timeout)
    std::string username;     // Associated user
    std::string ip_address;   // Client IP for logging
};

class AuthController {
private:
    std::map<std::string, Session> active_sessions;

    // Rate limiting
    std::map<std::string, LoginAttempts> login_attempts;
};
```

### Security Measures

1. **Password Storage**: Plain text in NVS (acceptable for local IoT)
2. **Session IDs**: Cryptographically random (std::random_device)
3. **HTTP-only Cookies**: Not accessible via JavaScript
4. **Session Timeout**: Configurable (default 1 hour)
5. **Rate Limiting**: Max 5 login attempts per minute per IP
6. **Automatic Cleanup**: Expired sessions removed every 5 minutes
7. **CORS**: Configurable (default: same-origin only)

### User Recommendations

- Change default password immediately
- Use strong passwords (12+ characters)
- Network isolation (VLAN)
- Firewall rules
- Regular firmware updates

## Deep Sleep Architecture

### Sleep Modes

1. **Active Mode**: Normal operation
2. **Light Sleep**: CPU paused, WiFi off, quick wake
3. **Deep Sleep**: Everything off except RTC, GPIO wake, timer wake

### Deep Sleep Controller

```cpp
class DeepSleepController {
public:
    struct Config {
        bool enabled;
        uint32_t sleep_duration;           // Seconds
        std::vector<gpio_num_t> wake_pins; // GPIO wake sources
        bool wake_on_timer;
        bool preserve_wifi;
    };

    void configure(const Config& config);
    void enterSleep();
    void handleWakeup();
    uint32_t getNextWakeTime();
    WakeReason getWakeReason();
};
```

### Sleep/Wake Cycle

```
Active → Sleep Decision → Prepare → Deep Sleep → Wake → Resume
  ↑                                                        │
  └────────────────────────────────────────────────────────┘
```

## Internationalization

### Language File Structure

```json
{
  "meta": {
    "language": "de",
    "name": "Deutsch",
    "direction": "ltr"
  },
  "ui": { /* UI strings */ },
  "components": { /* Component names */ },
  "errors": { /* Error messages */ },
  "status": { /* Status messages */ }
}
```

### Translation Process

```
1. i18n.load('de')
   ↓
2. fetch('/lang/de.json')
   ↓
3. Parse JSON
   ↓
4. Store in memory
   ↓
5. i18n.t('ui.dashboard') → 'Dashboard'
```

## Extension Points

### Adding a New Sensor

1. Inherit from `ISensor`
2. Implement required methods
3. Register in `setup()`:
   ```cpp
   auto sensor = new MySensor();
   registry.registerComponent(sensor);
   ```
4. UI updates automatically

### Adding a New Actuator

1. Inherit from `IActuator`
2. Implement `setState()` and `getState()`
3. Register in setup
4. UI control available automatically

### Adding a New API Endpoint

```cpp
// In UIFramework::registerHandlers()
httpd_uri_t my_endpoint = {
    .uri = "/api/custom",
    .method = HTTP_GET,
    .handler = custom_handler,
    .user_ctx = this
};
httpd_register_uri_handler(server, &my_endpoint);
```

## Performance Considerations

### Response Time Targets

- API endpoints: <50ms average
- Page load: <2s on 3G connection
- Sensor updates: <100ms
- Actuator control: <200ms

### Optimization Techniques

1. **JSON Streaming**: Build JSON incrementally
2. **Buffer Reuse**: Static buffers for HTTP responses
3. **Lazy Loading**: Load charts only when needed
4. **Caching**: Browser caching for static assets
5. **Compression**: Gzip transfer encoding
6. **Connection Pooling**: Reuse TCP connections

## Monitoring & Debugging

### Logging Levels

```cpp
ESP_LOGD("tag", "Debug message");    // Development only
ESP_LOGI("tag", "Info message");     // Normal operation
ESP_LOGW("tag", "Warning message");  // Potential issues
ESP_LOGE("tag", "Error message");    // Errors
```

### Key Metrics

- Free heap memory
- WiFi signal strength
- Uptime
- Active sessions
- Request count
- Error count

### Debug Endpoints (Development Only)

```
GET /api/debug/memory     # Memory usage
GET /api/debug/sessions   # Active sessions
GET /api/debug/registry   # Registered components
```

## References

- ESP-IDF Documentation: https://docs.espressif.com/projects/esp-idf/
- ESPHome Documentation: https://esphome.io/
- Architecture Decision Records: docs/adr/

---

**Version**: 1.0.0
**Last Updated**: 2025-11-20
**Maintainer**: ESPHome UI Framework Team
