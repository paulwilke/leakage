# ADR-005: Testing Strategy

## Status
Accepted

## Context

The framework needs comprehensive testing to ensure:
- Code quality and reliability
- Regression prevention
- Confidence for LLM extensions
- Production readiness
- Long-term maintainability

Target: **>80% code coverage**

### Options Considered

#### Option A: Unit Tests Only (Google Test)

**Pros:**
- Fast execution
- Easy to write
- Mock dependencies
- Good coverage

**Cons:**
- Doesn't test real hardware
- May miss integration issues
- ESP32 behavior not validated

#### Option B: Hardware Tests Only

**Pros:**
- Tests real behavior
- Validates on actual device
- Catches hardware issues

**Cons:**
- Slow to run
- Requires hardware
- Brittle tests
- Hard to automate
- Difficult to mock

#### Option C: Layered Testing (Unit + Integration + Manual)

**Pros:**
- Best coverage
- Fast unit tests + real validation
- CI/CD friendly
- Catches all types of bugs

**Cons:**
- More work to set up
- Need multiple test suites
- More complex

## Decision

We will use a **layered testing approach**:

1. **Unit Tests** (Google Test) - Core C++ logic
2. **Integration Tests** (PlatformIO + pytest) - Full compilation
3. **Frontend Tests** (Browser-based) - UI functionality
4. **Manual Tests** (Checklist) - Hardware validation

## Test Architecture

### Layer 1: Unit Tests (C++)

**Tool**: Google Test (GTest)
**Target**: 80%+ coverage of C++ code
**Runs**: On every commit (CI)

```cpp
// test/unit/test_component_registry.cpp
#include <gtest/gtest.h>
#include "component_registry.h"

class MockSensor : public ISensor {
public:
    MockSensor(const char* id) : m_id(id), m_value(0) {}
    const char* getId() const override { return m_id.c_str(); }
    const char* getType() const override { return "mock"; }
    float getValue() override { return m_value; }
    const char* getUnit() const override { return "unit"; }
    bool isAvailable() override { return true; }
    void setValue(float v) { m_value = v; }
private:
    std::string m_id;
    float m_value;
};

TEST(ComponentRegistry, RegisterSensor) {
    ComponentRegistry registry;
    MockSensor sensor("test_sensor");

    registry.registerComponent(&sensor);

    EXPECT_EQ(registry.count(), 1);
    EXPECT_TRUE(registry.has("test_sensor"));
}

TEST(ComponentRegistry, GetByType) {
    ComponentRegistry registry;
    MockSensor sensor1("temp1");
    MockSensor sensor2("temp2");

    registry.registerComponent(&sensor1);
    registry.registerComponent(&sensor2);

    auto sensors = registry.getByType("mock");
    EXPECT_EQ(sensors.size(), 2);
}

TEST(ComponentRegistry, JsonSerialization) {
    ComponentRegistry registry;
    MockSensor sensor("test");
    sensor.setValue(23.5);

    registry.registerComponent(&sensor);

    std::string json = registry.toJson();
    EXPECT_NE(json.find("\"id\":\"test\""), std::string::npos);
    EXPECT_NE(json.find("\"value\":23.5"), std::string::npos);
}

TEST(ComponentRegistry, RemoveComponent) {
    ComponentRegistry registry;
    MockSensor sensor("test");

    registry.registerComponent(&sensor);
    EXPECT_EQ(registry.count(), 1);

    registry.removeComponent("test");
    EXPECT_EQ(registry.count(), 0);
}
```

**Coverage Target by Module:**

```
framework/component.cpp         90%
framework/registry.cpp          90%
framework/auth.cpp              85%
framework/deep_sleep.cpp        80%
framework/i18n_server.cpp       75%
examples/bme280/bme280.cpp      70%
examples/ds18b20/ds18b20.cpp    70%
examples/relay/relay.cpp        80%
```

### Layer 2: Integration Tests

**Tool**: pytest + ESPHome compile
**Target**: All examples compile successfully
**Runs**: On PR merge (CI)

```python
# test/integration/test_compilation.py
import pytest
import subprocess
import os

EXAMPLES = [
    'examples/bme280/bme280.yaml',
    'examples/ds18b20/ds18b20.yaml',
    'examples/relay/relay.yaml',
]

@pytest.mark.parametrize('example', EXAMPLES)
def test_example_compiles(example):
    """Test that example YAML compiles without errors"""
    result = subprocess.run(
        ['esphome', 'compile', example],
        capture_output=True,
        text=True
    )

    assert result.returncode == 0, f"Compilation failed:\n{result.stderr}"
    assert 'Successfully' in result.stdout
    assert 'error' not in result.stdout.lower()

def test_framework_size():
    """Verify framework fits in flash"""
    result = subprocess.run(
        ['esphome', 'compile', 'examples/bme280/bme280.yaml'],
        capture_output=True,
        text=True
    )

    # Extract binary size from output
    # ESP32 has 4MB flash, we should use <200KB for framework
    # This is a placeholder - actual parsing needed
    assert result.returncode == 0

def test_web_assets_size():
    """Verify web assets are under 50KB"""
    total_size = 0
    web_files = [
        'web/index.html',
        'web/css/main.css',
        'web/css/themes.css',
        'web/js/app.js',
        'web/js/api.js',
        'web/js/components.js',
        'web/js/i18n.js',
        'web/js/charts.js',
    ]

    for f in web_files:
        if os.path.exists(f):
            total_size += os.path.getsize(f)

    # Uncompressed size should be under 40KB
    assert total_size < 40 * 1024, f"Web assets too large: {total_size} bytes"
```

### Layer 3: Frontend Tests

**Tool**: Simple HTML test runner
**Target**: API client, i18n, component rendering
**Runs**: Manual during development, can be automated with Playwright later

```html
<!-- test/frontend/test_runner.html -->
<!DOCTYPE html>
<html>
<head>
    <meta charset="UTF-8">
    <title>Frontend Tests</title>
    <style>
        .pass { color: green; }
        .fail { color: red; }
    </style>
</head>
<body>
    <h1>Frontend Test Results</h1>
    <div id="results"></div>

    <script type="module">
        import { API } from '../../web/js/api.js';
        import { I18n } from '../../web/js/i18n.js';

        const results = [];

        function test(name, fn) {
            try {
                fn();
                results.push({ name, pass: true });
            } catch (error) {
                results.push({ name, pass: false, error: error.message });
            }
        }

        // API Tests
        test('API client constructs correct URLs', () => {
            const api = new API('http://localhost');
            if (api.buildUrl('/sensors') !== 'http://localhost/sensors') {
                throw new Error('URL construction failed');
            }
        });

        // i18n Tests
        test('i18n loads default language', async () => {
            const i18n = new I18n();
            await i18n.load('en');
            if (i18n.getCurrentLang() !== 'en') {
                throw new Error('Language not loaded');
            }
        });

        test('i18n translates nested keys', () => {
            const i18n = new I18n();
            i18n.translations = { ui: { dashboard: 'Dashboard' } };
            if (i18n.t('ui.dashboard') !== 'Dashboard') {
                throw new Error('Translation failed');
            }
        });

        // Render results
        const resultsEl = document.getElementById('results');
        results.forEach(r => {
            const div = document.createElement('div');
            div.className = r.pass ? 'pass' : 'fail';
            div.textContent = `${r.pass ? '✓' : '✗'} ${r.name}`;
            if (r.error) {
                div.textContent += ` - ${r.error}`;
            }
            resultsEl.appendChild(div);
        });
    </script>
</body>
</html>
```

### Layer 4: Manual Testing

**Tool**: Checklist in `docs/testing-checklist.md`
**Target**: Real hardware validation
**Runs**: Before major releases

```markdown
# Manual Testing Checklist

## Hardware Setup
- [ ] ESP32 boots successfully
- [ ] WiFi connects
- [ ] Serial output shows no errors
- [ ] LED indicators work (if any)

## Web UI (Desktop)
- [ ] Dashboard loads (Chrome, Firefox, Safari)
- [ ] Sensors display correct values
- [ ] Charts render when clicked
- [ ] Language switching works (DE ↔ EN)
- [ ] Responsive design adapts to window size

## Web UI (Mobile)
- [ ] Dashboard loads on mobile (iOS, Android)
- [ ] Touch interactions work
- [ ] Viewport scales correctly
- [ ] Performance is acceptable

## Authentication
- [ ] Login with correct credentials succeeds
- [ ] Login with wrong credentials fails
- [ ] Session persists across page reloads
- [ ] Session expires after timeout
- [ ] Logout works correctly

## Sensor Functionality
- [ ] BME280: Temperature, humidity, pressure display
- [ ] DS18B20: Temperature displays correctly
- [ ] Leak sensor: Dry state shows correctly
- [ ] Leak sensor: Wet state triggers alarm

## Actuator Control
- [ ] Relay can be turned ON via UI
- [ ] Relay can be turned OFF via UI
- [ ] Relay state persists across reboots (if configured)
- [ ] Relay safety timeout works (if configured)

## Deep Sleep
- [ ] Deep sleep can be configured
- [ ] Device enters deep sleep when triggered
- [ ] Device wakes on GPIO trigger
- [ ] Device wakes on timer
- [ ] State restores correctly after wake

## Configuration
- [ ] WiFi configuration can be changed
- [ ] NTP servers can be configured
- [ ] Admin password can be changed
- [ ] Device name can be changed
- [ ] Factory reset works

## API Endpoints
- [ ] GET /api/status returns valid JSON
- [ ] GET /api/components returns all components
- [ ] GET /api/sensors returns sensor values
- [ ] POST /api/actuators/{id} controls actuator
- [ ] Authentication protects admin endpoints

## Performance
- [ ] Dashboard loads in <2s on 3G
- [ ] API responses in <50ms
- [ ] Memory usage stable over 24h
- [ ] No memory leaks detected
- [ ] CPU usage acceptable

## Long-term Stability
- [ ] 24h uptime test passed
- [ ] 7-day uptime test passed (optional)
- [ ] No crashes or reboots
- [ ] Logs show no errors
```

## CI/CD Integration

```yaml
# .github/workflows/ci.yml
name: CI

on: [push, pull_request]

jobs:
  unit-tests:
    runs-on: ubuntu-latest
    steps:
      - uses: actions/checkout@v3

      - name: Install PlatformIO
        run: pip install platformio

      - name: Run unit tests
        run: |
          cd test/unit
          pio test

      - name: Check coverage
        run: |
          pio test --coverage
          # Fail if coverage < 80%
          coverage report --fail-under=80

  integration-tests:
    runs-on: ubuntu-latest
    steps:
      - uses: actions/checkout@v3

      - name: Install ESPHome
        run: pip install esphome

      - name: Compile examples
        run: |
          esphome compile examples/bme280/bme280.yaml
          esphome compile examples/ds18b20/ds18b20.yaml
          esphome compile examples/relay/relay.yaml

      - name: Check binary sizes
        run: python test/integration/check_sizes.py

  lint:
    runs-on: ubuntu-latest
    steps:
      - uses: actions/checkout@v3

      - name: Run clang-format
        run: |
          find framework -name '*.cpp' -o -name '*.h' | \
          xargs clang-format --dry-run --Werror

      - name: Run eslint
        run: npx eslint web/js/
```

## Coverage Reporting

```bash
# Generate coverage report
pio test --coverage

# View HTML report
genhtml coverage.info -o coverage_html
open coverage_html/index.html

# Upload to Codecov (CI)
bash <(curl -s https://codecov.io/bash)
```

## Test Organization

```
test/
├── unit/                    # C++ unit tests (GTest)
│   ├── test_component.cpp
│   ├── test_registry.cpp
│   ├── test_auth.cpp
│   ├── test_deep_sleep.cpp
│   └── mocks/              # Mock classes
│       ├── mock_sensor.h
│       └── mock_actuator.h
├── integration/            # Python integration tests
│   ├── test_compilation.py
│   ├── test_api.py
│   └── conftest.py
├── frontend/               # Browser-based tests
│   ├── test_runner.html
│   └── tests/
│       ├── test_api.js
│       ├── test_i18n.js
│       └── test_components.js
└── manual/                 # Manual test documentation
    ├── testing-checklist.md
    └── test-scenarios.md
```

## Consequences

### Positive
- High confidence in code quality
- Regression prevention
- Easy to extend safely
- CI/CD ready
- Production-ready validation

### Negative
- More work upfront
- Need to maintain tests
- Longer CI/CD time
- Requires discipline

### Mitigation
- Write tests alongside code (TDD)
- Keep unit tests fast (<1s total)
- Parallelize CI jobs
- Clear test documentation
- Make it easy to run locally

## References

- Google Test: https://github.com/google/googletest
- PlatformIO Testing: https://docs.platformio.org/en/latest/plus/unit-testing.html
- pytest: https://docs.pytest.org/

---

**Date:** 2025-11-20
**Author:** ESPHome UI Framework Team
