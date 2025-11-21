# ESPHome UI Framework - Manual Testing Checklist

This comprehensive checklist ensures all framework functionality works correctly on real hardware.

## Pre-Test Setup

### Hardware Requirements

- [ ] ESP32 development board (NodeMCU-32S or similar)
- [ ] USB cable for programming and power
- [ ] BME280 sensor (I2C, address 0x76 or 0x77)
- [ ] DS18B20 temperature sensor (OneWire)
- [ ] Relay module (1-channel)
- [ ] 4.7kΩ resistor (for DS18B20 pull-up)
- [ ] Breadboard and jumper wires
- [ ] Multimeter (optional but recommended)

### Software Requirements

- [ ] ESPHome installed (`pip install esphome`)
- [ ] Python 3.7 or higher
- [ ] Serial monitor access (`esphome logs` or Arduino IDE Serial Monitor)
- [ ] Modern web browser (Chrome, Firefox, Safari, Edge)
- [ ] curl or Postman (for API testing)

### Initial Configuration

- [ ] Create `secrets.yaml` with your WiFi credentials
- [ ] Generate API key: `python -c "import secrets; print(secrets.token_urlsafe(32))"`
- [ ] Note your network's IP range (e.g., 10.10.50.x)
- [ ] Ensure firewall allows HTTP traffic on port 80

---

## Section 1: Compilation Tests

### BME280 Example

- [ ] Navigate to `examples/bme280/`
- [ ] Run `esphome compile bme280_example.yaml`
- [ ] Compilation completes without errors
- [ ] No compiler warnings (or only minor ones)
- [ ] Binary size < 1MB
- [ ] RAM usage < 80%

**Expected Output:**
```
INFO Successfully compiled program.
RAM:   [====      ]  45.2% (used 148024 bytes from 327680 bytes)
Flash: [====      ]  42.1% (used 551234 bytes from 1310720 bytes)
```

### DS18B20 Example

- [ ] Navigate to `examples/ds18b20/`
- [ ] Run `esphome compile ds18b20_example.yaml`
- [ ] Compilation completes without errors
- [ ] No compiler warnings
- [ ] Binary size appropriate

### Relay Example

- [ ] Navigate to `examples/relay/`
- [ ] Run `esphome compile relay_example.yaml`
- [ ] Compilation completes without errors
- [ ] No compiler warnings
- [ ] Binary size appropriate

### Complete Demo

- [ ] Navigate to `examples/demo/`
- [ ] Run `esphome compile demo.yaml`
- [ ] Compilation completes without errors
- [ ] All includes resolved correctly
- [ ] No missing dependencies

---

## Section 2: Hardware Wiring

### BME280 Sensor (I2C)

- [ ] VCC → 3.3V (NOT 5V!)
- [ ] GND → GND
- [ ] SDA → GPIO21
- [ ] SCL → GPIO22
- [ ] Verify connections with multimeter (continuity test)

**Verification:**
- [ ] Power LED on BME280 module lights up
- [ ] No short circuits detected
- [ ] Voltage at VCC is 3.3V (±0.1V)

### DS18B20 Sensor (OneWire)

- [ ] VCC → 3.3V (or 5V, both work)
- [ ] GND → GND
- [ ] Data → GPIO4
- [ ] 4.7kΩ resistor between Data and VCC (CRITICAL!)

**Verification:**
- [ ] Resistor properly connected (measure ~4.7kΩ)
- [ ] Data line voltage at 3.3V when idle
- [ ] Sensor gets slightly warm when powered (normal)

### Relay Module

- [ ] VCC → 5V (relay coil power)
- [ ] GND → GND
- [ ] IN/Signal → GPIO23

**Verification:**
- [ ] Power LED on relay module lights up
- [ ] Relay does NOT click on power-up (should be OFF)
- [ ] Manual test: Connect IN to VCC, relay should click ON

---

## Section 3: Firmware Upload & Boot

### Upload Firmware

- [ ] Connect ESP32 via USB
- [ ] Run `esphome run demo.yaml`
- [ ] Select serial port
- [ ] Upload completes successfully
- [ ] Device reboots automatically

**Expected Serial Output:**
```
[I][main:048] ===========================================
[I][main:049]   ESPHome UI Framework Demo Starting
[I][main:050] ===========================================
[I][main:064] Setting up BME280 sensors...
[I][main:091] ✅ BME280 sensors registered
[I][main:099] Setting up DS18B20 sensor...
[I][main:109] ✅ DS18B20 sensor registered (addr: 28FF...)
[I][main:118] Setting up relay actuator...
[I][main:132] ✅ Relay actuator registered (GPIO23, 30s timeout)
[I][main:145] Web UI: http://10.10.50.100/
[I][main:146] Default login: admin / admin
```

### Boot Sequence

- [ ] Device connects to WiFi
- [ ] Static IP assigned: 10.10.50.100 (or your configured IP)
- [ ] Framework initialization completes
- [ ] All sensors detected and registered
- [ ] Relay actuator initialized
- [ ] Web server starts
- [ ] No error messages in logs

### WiFi Connection

- [ ] Device connects within 30 seconds
- [ ] WiFi signal strength shown (RSSI)
- [ ] IP address displayed in logs
- [ ] Ping test: `ping 10.10.50.100` responds
- [ ] Device appears in router's DHCP table

---

## Section 4: Sensor Functionality

### BME280 Environmental Sensor

**Temperature:**
- [ ] Temperature value displayed in logs every 5 seconds
- [ ] Value is realistic (15-30°C typical room temp)
- [ ] Changes when you warm sensor with hand
- [ ] Accuracy seems reasonable (compare with thermometer)

**Humidity:**
- [ ] Humidity value displayed (30-70% typical)
- [ ] Changes when you breathe on sensor
- [ ] Value makes sense for environment

**Pressure:**
- [ ] Pressure displayed (990-1040 hPa typical)
- [ ] Value is stable
- [ ] Matches local weather report (±5 hPa)

### DS18B20 Temperature Sensor

- [ ] Sensor address detected and displayed (28FFxxxxxx)
- [ ] Temperature reading displayed
- [ ] Value is realistic
- [ ] Changes when you warm sensor with hand
- [ ] CRC check passes (no errors in logs)
- [ ] Reading updates every 5 seconds

**Accuracy Test:**
- [ ] Compare with BME280 temperature (should be within ±2°C)
- [ ] Compare with digital thermometer

### Relay Actuator

**Initial State:**
- [ ] Relay starts in OFF state
- [ ] Relay LED is OFF (if module has one)
- [ ] No clicking sound on boot

---

## Section 5: Web UI - Basic Access

### Page Loading

- [ ] Open browser to `http://10.10.50.100/`
- [ ] Page loads within 2 seconds
- [ ] No 404 errors
- [ ] No console errors (check browser DevTools)
- [ ] Loading spinner shows briefly

### UI Layout

- [ ] Header displays "ESPHome UI Framework"
- [ ] Navigation menu visible: Dashboard, Settings, About
- [ ] Language selector visible (EN/DE)
- [ ] Logout button visible (if logged in)
- [ ] Footer shows version and status

### Responsive Design

**Desktop (1920x1080):**
- [ ] Layout looks good
- [ ] Cards in grid layout
- [ ] No horizontal scrolling needed

**Tablet (768x1024):**
- [ ] Layout adapts to smaller screen
- [ ] Cards stack appropriately
- [ ] Text remains readable

**Mobile (375x667):**
- [ ] Single column layout
- [ ] Navigation menu collapses (if implemented)
- [ ] Touch targets large enough (>44px)
- [ ] No content cut off

---

## Section 6: Web UI - Dashboard

### Sensor Display

**BME280 Sensors:**
- [ ] "Indoor Temperature" card visible
- [ ] Value displayed with unit (°C)
- [ ] Value updates every 5 seconds
- [ ] "Indoor Humidity" card visible
- [ ] Value displayed with unit (%)
- [ ] "Atmospheric Pressure" card visible
- [ ] Value displayed with unit (hPa)

**DS18B20 Sensor:**
- [ ] "Water Temperature" card visible
- [ ] Value displayed with unit (°C)
- [ ] Updates every 5 seconds

**Status Indicators:**
- [ ] All sensors show "Available" status (green)
- [ ] Last update timestamp shown
- [ ] WiFi signal indicator visible

### Real-Time Updates

- [ ] Leave page open for 30 seconds
- [ ] Sensor values update automatically
- [ ] No page refresh required
- [ ] Connection status shows "Connected" (green)
- [ ] No JavaScript errors in console

### Relay Control

- [ ] "Main Relay" card visible
- [ ] Current state displayed: "OFF"
- [ ] "Turn ON" button visible
- [ ] Button is clickable (requires login)

---

## Section 7: Authentication

### Login Process

- [ ] Click "Settings" or relay control
- [ ] Login modal appears
- [ ] Form has username and password fields
- [ ] Default credentials: `admin` / `admin`
- [ ] Click "Login" button
- [ ] Login successful
- [ ] Modal closes
- [ ] "Logout" button now visible

### Session Management

- [ ] After login, navigate to Dashboard
- [ ] Session persists (don't need to login again)
- [ ] Reload page - still logged in
- [ ] Session cookie set (`session_id`)

### Logout

- [ ] Click "Logout" button
- [ ] Confirmation (if implemented)
- [ ] Redirect to Dashboard
- [ ] Login button visible again
- [ ] Cannot access Settings

### Rate Limiting

**Intentional Failed Login Test:**
- [ ] Logout if logged in
- [ ] Try to login with wrong password 6 times
- [ ] After 5 failures, account locked for 5 minutes
- [ ] Error message: "Too many login attempts"
- [ ] Even correct password doesn't work during lockout
- [ ] Wait 5 minutes, can login again

---

## Section 8: Relay Control

### Turn Relay ON

- [ ] Login if not already
- [ ] Navigate to Dashboard
- [ ] Click "Main Relay" card
- [ ] Click "Turn ON" button
- [ ] Relay clicks ON (audible click)
- [ ] LED on relay module turns ON
- [ ] UI updates to show "State: ON"
- [ ] Button changes to "Turn OFF"
- [ ] Serial log confirms: `Relay 'relay1' (GPIO23) set to ON`

### Turn Relay OFF

- [ ] Click "Turn OFF" button
- [ ] Relay clicks OFF
- [ ] LED turns OFF
- [ ] UI updates to show "State: OFF"
- [ ] Button changes to "Turn ON"
- [ ] Serial log confirms: `Relay 'relay1' (GPIO23) set to OFF`

### Safety Timeout

- [ ] Turn relay ON
- [ ] Note the time
- [ ] Wait exactly 30 seconds (timer visible in UI if implemented)
- [ ] Relay automatically turns OFF after 30 seconds
- [ ] UI updates automatically
- [ ] Serial log: `Safety timeout reached for 'relay1', turning OFF`

**Manual Timeout Reset Test:**
- [ ] Turn relay ON
- [ ] Wait 15 seconds
- [ ] Turn relay OFF manually
- [ ] Turn relay ON again
- [ ] Timer resets (another 30 seconds)

### Unauthorized Access

- [ ] Logout
- [ ] Try to control relay from Dashboard
- [ ] Login modal appears
- [ ] Cannot control relay without authentication

---

## Section 9: Internationalization (i18n)

### Language Switching

**Switch to German:**
- [ ] Click language selector (DE flag or dropdown)
- [ ] Select "Deutsch"
- [ ] UI text changes to German
  - [ ] "Dashboard" → "Übersicht"
  - [ ] "Settings" → "Einstellungen"
  - [ ] "Login" → "Anmelden"
  - [ ] "Logout" → "Abmelden"
- [ ] Sensor names remain in English (as configured)
- [ ] Units remain standard (°C, %, hPa)

**Switch back to English:**
- [ ] Select "English" from language selector
- [ ] UI text changes back to English
- [ ] All translations correct

**Persistence:**
- [ ] Reload page
- [ ] Language preference remembered (localStorage)
- [ ] No need to select language again

---

## Section 10: REST API

### API Endpoints - Public (No Auth)

**GET /api/status**
```bash
curl http://10.10.50.100/api/status
```
- [ ] Returns JSON response
- [ ] Contains `device_name`, `version`, `uptime`
- [ ] WiFi RSSI shown
- [ ] Free heap memory shown

**GET /api/components**
```bash
curl http://10.10.50.100/api/components
```
- [ ] Returns array of all components
- [ ] Contains 4 items (3 BME280, 1 DS18B20, 1 relay)
- [ ] Each has `id`, `name`, `type`

**GET /api/sensors**
```bash
curl http://10.10.50.100/api/sensors
```
- [ ] Returns array of sensors only (no actuators)
- [ ] Contains 4 sensors
- [ ] Each has `value`, `unit`, `available`
- [ ] Values are current (not cached)

**GET /api/actuators**
```bash
curl http://10.10.50.100/api/actuators
```
- [ ] Returns array of actuators only
- [ ] Contains 1 actuator (relay)
- [ ] Shows current state

### API Endpoints - Protected (Requires Auth)

**POST /api/auth/login**
```bash
curl -X POST http://10.10.50.100/api/auth/login \
  -H "Content-Type: application/json" \
  -d '{"username":"admin","password":"admin"}'
```
- [ ] Returns `{"success": true, "session_id": "..."}`
- [ ] Session cookie set
- [ ] Session ID is long (32+ characters)

**POST /api/auth/logout**
```bash
curl -X POST http://10.10.50.100/api/auth/logout \
  -H "Cookie: session_id=YOUR_SESSION_ID"
```
- [ ] Returns `{"success": true}`
- [ ] Session invalidated
- [ ] Cannot use session_id again

**POST /api/actuators/relay1**
```bash
# Turn ON
curl -X POST http://10.10.50.100/api/actuators/relay1 \
  -H "Content-Type: application/json" \
  -H "Cookie: session_id=YOUR_SESSION_ID" \
  -d '{"state":"on"}'

# Turn OFF
curl -X POST http://10.10.50.100/api/actuators/relay1 \
  -H "Content-Type: application/json" \
  -H "Cookie: session_id=YOUR_SESSION_ID" \
  -d '{"state":"off"}'

# Toggle
curl -X POST http://10.10.50.100/api/actuators/relay1 \
  -H "Content-Type: application/json" \
  -H "Cookie: session_id=YOUR_SESSION_ID" \
  -d '{"action":"toggle"}'
```
- [ ] Relay responds to commands
- [ ] State changes confirmed
- [ ] Returns `{"success": true, "state": "on"}`

**Unauthorized Test:**
```bash
curl -X POST http://10.10.50.100/api/actuators/relay1 \
  -H "Content-Type: application/json" \
  -d '{"state":"on"}'
```
- [ ] Returns 401 Unauthorized
- [ ] Relay does NOT activate
- [ ] Error message in response

---

## Section 11: Settings Page

### Access Settings

- [ ] Login if not already
- [ ] Click "Settings" in navigation
- [ ] Settings page loads
- [ ] Current configuration shown

### Change Password

- [ ] Find "Change Password" section
- [ ] Enter current password: `admin`
- [ ] Enter new password: `newpassword123`
- [ ] Confirm new password: `newpassword123`
- [ ] Click "Save"
- [ ] Success message appears
- [ ] Logout
- [ ] Try old password: `admin` - should FAIL
- [ ] Try new password: `newpassword123` - should SUCCEED

**Security Note:** Change password back to `admin` for consistency with tests.

### Device Configuration

- [ ] Device name displayed
- [ ] Firmware version shown
- [ ] Uptime displayed
- [ ] MAC address shown
- [ ] IP address shown

### Deep Sleep Configuration (if enabled)

- [ ] Deep sleep section visible
- [ ] Enable/disable toggle
- [ ] Sleep duration field (seconds)
- [ ] Wake sources: Timer, GPIO
- [ ] GPIO pin selector
- [ ] Save configuration button

---

## Section 12: About Page

- [ ] Click "About" in navigation
- [ ] Framework information displayed:
  - [ ] Version number
  - [ ] License (MIT)
  - [ ] Repository link
  - [ ] Credits/Authors
- [ ] Links are clickable
- [ ] Information is accurate

---

## Section 13: Error Handling

### Network Errors

**Simulate Network Loss:**
- [ ] Unplug ESP32 from power
- [ ] Observe web UI
- [ ] Connection status changes to "Disconnected" (red)
- [ ] Error message displayed
- [ ] Reconnect ESP32
- [ ] UI recovers automatically within 10 seconds
- [ ] Data resumes updating

### Sensor Failures

**Disconnect BME280:**
- [ ] Unplug BME280 VCC while device running
- [ ] Wait 10 seconds
- [ ] BME280 sensors show "Unavailable" status
- [ ] UI shows warning (yellow/orange)
- [ ] Other sensors continue working
- [ ] Reconnect BME280
- [ ] Reboot device
- [ ] Sensor works again

**Disconnect DS18B20:**
- [ ] Unplug DS18B20 data line
- [ ] DS18B20 shows "Unavailable"
- [ ] Error in serial log
- [ ] Reconnect
- [ ] Reboot device
- [ ] Sensor works again

### Browser Compatibility

Test in multiple browsers:

**Chrome/Chromium:**
- [ ] All features work
- [ ] No console errors
- [ ] UI renders correctly

**Firefox:**
- [ ] All features work
- [ ] No console errors
- [ ] UI renders correctly

**Safari (Mac/iOS):**
- [ ] All features work
- [ ] No console errors
- [ ] UI renders correctly

**Edge:**
- [ ] All features work
- [ ] No console errors
- [ ] UI renders correctly

---

## Section 14: Performance Tests

### Load Time

- [ ] Clear browser cache
- [ ] Load homepage
- [ ] Page loads in < 2 seconds (on good WiFi)
- [ ] Total page size < 100KB
- [ ] JavaScript loads and executes

### Memory Usage

**ESP32 Memory:**
- [ ] Monitor serial logs for 10 minutes
- [ ] No memory leaks reported
- [ ] Free heap stays stable (±5KB variation)
- [ ] No stack overflow warnings

**Browser Memory:**
- [ ] Open page
- [ ] Leave running for 1 hour
- [ ] Check browser task manager
- [ ] Memory usage stable (< 100MB)

### Update Frequency

- [ ] Sensor updates happen every 5 seconds (default)
- [ ] No excessive API calls
- [ ] Network traffic reasonable (< 1KB per update)

---

## Section 15: Stress Tests

### Rapid Relay Toggling

- [ ] Turn relay ON
- [ ] Immediately turn OFF
- [ ] Repeat 20 times quickly
- [ ] No crashes
- [ ] No missed commands
- [ ] Relay responds to all commands
- [ ] UI stays responsive

### Multiple Browser Sessions

- [ ] Open page in 3 different browsers simultaneously
- [ ] All show same data
- [ ] Control relay from Browser 1
- [ ] Browsers 2 and 3 update automatically
- [ ] Login works independently in each browser

### Long-Term Stability

- [ ] Leave device powered on for 24 hours
- [ ] Check every few hours
- [ ] No crashes
- [ ] No reboots
- [ ] Sensors continue reading
- [ ] Web UI remains accessible
- [ ] Memory usage stable

---

## Section 16: Deep Sleep (Optional)

**Note:** Only test if you have battery-powered setup and won't use web UI.

### Configure Deep Sleep

- [ ] Login to Settings
- [ ] Enable deep sleep
- [ ] Set duration: 60 seconds
- [ ] Enable timer wake
- [ ] Save configuration
- [ ] Device should enter deep sleep after run duration

### Wake from Sleep

- [ ] Wait for configured sleep duration
- [ ] Device wakes automatically
- [ ] Sensors read values
- [ ] Data transmitted (if Home Assistant connected)
- [ ] Device enters sleep again

### GPIO Wake

- [ ] Configure GPIO wake (e.g., GPIO0)
- [ ] Device enters sleep
- [ ] Press button connected to GPIO0
- [ ] Device wakes immediately
- [ ] Serial log shows: "Wake reason: GPIO"

---

## Section 17: Home Assistant Integration (Optional)

### Auto-Discovery

- [ ] Home Assistant running on network
- [ ] ESPHome integration installed
- [ ] Device auto-discovered
- [ ] Shows as "UI Framework Demo"
- [ ] All entities appear:
  - [ ] BME280 Temperature
  - [ ] BME280 Humidity
  - [ ] BME280 Pressure
  - [ ] DS18B20 Temperature
  - [ ] Relay switch

### Control from Home Assistant

- [ ] Toggle relay from HA dashboard
- [ ] State updates in real-time
- [ ] Sensor values update every 5 seconds
- [ ] Create automation using sensor data
- [ ] Automation triggers correctly

---

## Section 18: OTA Updates

### Over-The-Air Upload

- [ ] Modify demo.yaml (e.g., change device name)
- [ ] Run `esphome run demo.yaml`
- [ ] Select "Over The Air" option
- [ ] Select device IP
- [ ] Firmware uploads wirelessly
- [ ] Device reboots
- [ ] Changes take effect
- [ ] No USB cable needed

### OTA Password Protection

- [ ] Wrong password in secrets.yaml
- [ ] Try OTA update
- [ ] Update fails with authentication error
- [ ] Restore correct password
- [ ] Update succeeds

---

## Section 19: Security Validation

### Password Security

- [ ] Cannot login with empty password
- [ ] Cannot login with wrong password
- [ ] Session expires after timeout (30 min default)
- [ ] Rate limiting works (5 attempts, 5 min lockout)
- [ ] Session IDs are cryptographically random

### XSS Protection

**Basic XSS Test:**
- [ ] Try to inject script in username field: `<script>alert('XSS')</script>`
- [ ] Script does NOT execute
- [ ] Input is sanitized or escaped

### CSRF Protection (if implemented)

- [ ] Cross-origin requests blocked
- [ ] CSRF tokens validated

---

## Section 20: Final Validation

### Documentation Accuracy

- [ ] All examples match documentation
- [ ] Wiring diagrams are correct
- [ ] GPIO pin numbers accurate
- [ ] Troubleshooting tips helpful

### Success Criteria (from original requirements)

- [x] Framework compiles without errors
- [x] Unit tests implemented (>80% coverage target)
- [x] Web UI loads on mobile and desktop
- [x] 3+ example sensor implementations work
- [x] Deep sleep functionality exists
- [x] Authentication prevents unauthorized access
- [x] Multi-language switching works (DE/EN)
- [x] Charts system implemented (lazy loading)
- [ ] Complete documentation exists (Phase 4)
- [ ] LLM Extension Guide created (Phase 4)

---

## Test Results

**Tester Name:** ___________________________
**Date:** ___________________________
**Hardware Used:** ___________________________
**ESPHome Version:** ___________________________

### Summary

- Total tests executed: ______
- Tests passed: ______
- Tests failed: ______
- Tests skipped: ______

### Critical Issues Found

1. _____________________________________________________________
2. _____________________________________________________________
3. _____________________________________________________________

### Non-Critical Issues

1. _____________________________________________________________
2. _____________________________________________________________
3. _____________________________________________________________

### Recommendations

1. _____________________________________________________________
2. _____________________________________________________________
3. _____________________________________________________________

### Overall Assessment

- [ ] **PASS** - Ready for production use
- [ ] **PASS WITH MINOR ISSUES** - Ready with known limitations
- [ ] **FAIL** - Critical issues must be addressed

**Notes:**

_______________________________________________________________
_______________________________________________________________
_______________________________________________________________

---

## Appendix: Quick Reference

### Default Credentials
- Username: `admin`
- Password: `admin`

### Default IPs
- Device: `10.10.50.100`
- Gateway: `10.10.50.1`

### GPIO Pins
- BME280 I2C SDA: GPIO21
- BME280 I2C SCL: GPIO22
- DS18B20 Data: GPIO4
- Relay Control: GPIO23

### Serial Baud Rate
- 115200

### API Endpoints
- GET  `/api/status`
- GET  `/api/components`
- GET  `/api/sensors`
- GET  `/api/actuators`
- POST `/api/auth/login`
- POST `/api/auth/logout`
- POST `/api/actuators/:id`

---

**Version:** 1.0.0-alpha
**Last Updated:** 2025-11-21
**Maintained by:** ESPHome UI Framework Team
