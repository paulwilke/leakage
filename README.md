# ESPHome Sensor Konfigurationen

Dieses Repository enthält ESPHome-Konfigurationen für verschiedene Sensoren.

## Repository-Struktur

```
.
├── humidity_sensors/       # Temperatur- und Luftfeuchtigkeitssensoren
│   └── humidity01.yaml    # SHT4x Sensor mit ESP32
├── leak_sensors/          # Wasserleck-Sensoren
│   └── leak01.yaml        # Leak Sensor Cable mit ESP32
└── secrets.yaml           # WiFi-Zugangsdaten (nicht im Git!)
```

## Erste Schritte

### 1. Secrets-Datei erstellen

```bash
cat > secrets.yaml << 'EOF'
wifi_ssid: "DeinWiFiName"
wifi_password: "DeinWiFiPasswort"
EOF
```

**WICHTIG:** Die `secrets.yaml` sollte in `.gitignore` stehen und nicht ins Repository committed werden!

### 2. ESPHome installieren

```bash
pip install esphome
```

### 3. API-Keys und Passwörter generieren

In den jeweiligen YAML-Dateien die Platzhalter "xxx" durch sichere Werte ersetzen:
- `api.encryption.key`: API Verschlüsselungsschlüssel
- `ota.password`: Passwort für OTA-Updates

Diese können mit ESPHome automatisch generiert werden.

---

## Humidity Sensor (humidity01.yaml)

### Hardware
- **Mikrocontroller**: ESP32 NodeMCU-32S
- **Sensor**: SHT4x (I2C)
- **Pins**:
  - SDA: GPIO21
  - SCL: GPIO22

### Features
- Temperaturmessung
- Luftfeuchtigkeitsmessung
- WiFi Signal-Überwachung
- Uptime-Tracking
- Home Assistant Integration
- OTA Updates
- Webserver
- Fallback Access Point

### Verwendung

```bash
# Konfiguration validieren
esphome config humidity_sensors/humidity01.yaml

# Erstes Mal flashen (per USB)
esphome run humidity_sensors/humidity01.yaml

# OTA Updates (nach dem ersten Flash)
esphome run humidity_sensors/humidity01.yaml
```

### Kalibrierung

Falls der Sensor leicht von Referenzwerten abweicht, können Offset-Filter aktiviert werden:

```yaml
temperature:
  name: "Temperatur"
  filters:
    - offset: -0.5  # z.B. -0.5°C Korrektur
```

---

## Leak Sensor (leak01.yaml)

### Hardware
- **Mikrocontroller**: ESP32 NodeMCU-32S (oder ESP8266)
- **Sensor**: Shelly Leak Sensor Cable (oder ähnlicher 2-Draht Leak Sensor)
- **Pin**: GPIO4 (kann angepasst werden)

### Anschluss

Der Leak Sensor Cable hat 2 Drähte:

```
ESP32 Pin     |  Leak Sensor Cable
--------------|-------------------
GND           |  Draht 1 (schwarz)
GPIO4         |  Draht 2 (rot)
```

**WICHTIG:**
- Keine externen Widerstände nötig - der ESP32 nutzt den internen Pullup!
- Im trockenen Zustand ist der Kontakt offen (Pin HIGH)
- Bei Nässe wird der Kontakt geschlossen (Pin LOW)
- Home Assistant zeigt dann "Wet" an

### Alternative Pins

Falls GPIO4 bereits belegt ist, kannst du jeden anderen GPIO nutzen:
- **ESP32**: GPIO2, GPIO4, GPIO5, GPIO12-19, GPIO21-23, GPIO25-27, GPIO32-39
- **ESP8266**: GPIO4 (D2), GPIO5 (D1), GPIO12 (D6), GPIO13 (D7), GPIO14 (D5)

**NICHT nutzen:** GPIO0, GPIO2, GPIO15 (Boot-Pins beim Start)

### Funktionsweise

1. Der ESP32 aktiviert den internen Pullup-Widerstand an GPIO4
2. Im trockenen Zustand: Kontakt offen → Pin bleibt HIGH
3. Bei Nässe: Wasser schließt den Kontakt → Pin wird LOW
4. ESPHome erkennt die Zustandsänderung und meldet sie an Home Assistant

### Features
- Wasserleck-Erkennung mit delay-Filter (gegen Fehlalarme)
- Logging bei Alarm
- WiFi Signal-Überwachung
- Home Assistant Integration mit Device Class "moisture"
- OTA Updates
- Webserver

### Verwendung

```bash
# Konfiguration validieren
esphome config leak_sensors/leak01.yaml

# Erstes Mal flashen (per USB)
esphome run leak_sensors/leak01.yaml

# OTA Updates
esphome run leak_sensors/leak01.yaml
```

### Testen

1. Logs anschauen: `esphome logs leak_sensors/leak01.yaml`
2. Die beiden Drähte des Sensors manuell verbinden
3. Im Log sollte "ALARM! Wasserleck erkannt!" erscheinen
4. Drähte trennen → "Sensor wieder trocken" sollte erscheinen

---

## Netzwerk-Konfiguration

Beide Sensoren nutzen DHCP by default. Falls du statische IPs brauchst, kannst du diese in den YAML-Dateien konfigurieren:

```yaml
wifi:
  ssid: !secret wifi_ssid
  password: !secret wifi_password

  manual_ip:
    static_ip: 192.168.1.100    # Deine gewünschte IP
    gateway: 192.168.1.1         # Dein Router
    subnet: 255.255.255.0
```

## Troubleshooting

### Humidity Sensor
1. **Sensor nicht gefunden**: I2C-Verkabelung prüfen (SDA/SCL)
2. **WiFi-Verbindung fehlschlägt**: Fallback-Hotspot "Humidity 1 Fallback Hotspot" nutzen
3. **Logs anschauen**: `esphome logs humidity_sensors/humidity01.yaml`

### Leak Sensor
1. **Sensor reagiert nicht**:
   - Verkabelung prüfen (GND und GPIO4)
   - Im Log nach I/O-Fehlern suchen
2. **Fehlalarme**:
   - `delayed_on` Filter erhöhen (z.B. auf 500ms)
   - Kabel auf Oxidation prüfen
3. **WiFi-Verbindung fehlschlägt**: Fallback-Hotspot "Leak Sensor 01 Fallback" nutzen
4. **Logs anschauen**: `esphome logs leak_sensors/leak01.yaml`

## Home Assistant Integration

Nach dem ersten Flash werden die Sensoren automatisch in Home Assistant erkannt (Auto-Discovery via ESPHome Integration).

1. Gehe zu **Einstellungen** → **Geräte & Dienste**
2. ESPHome sollte die neuen Sensoren anzeigen
3. Klicke auf **Konfigurieren** und gib den API-Key ein

## Weitere Informationen

- [ESPHome Dokumentation](https://esphome.io/)
- [Home Assistant ESPHome Integration](https://www.home-assistant.io/integrations/esphome/)
- [ESP32 GPIO Pinout](https://randomnerdtutorials.com/esp32-pinout-reference-gpios/)
