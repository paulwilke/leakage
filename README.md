# ESPHome Humidity/Temperature Sensor

ESPHome-Konfiguration für einen ESP32-basierten Temperatur- und Luftfeuchtigkeitssensor mit SHT4x.

## Hardware

- **Mikrocontroller**: ESP32 NodeMCU-32S
- **Sensor**: SHT4x (I2C)
- **Pins**:
  - SDA: GPIO21
  - SCL: GPIO22

## Setup

1. **Secrets-Datei erstellen**:
   ```bash
   cp secrets.yaml.example secrets.yaml
   ```
   Dann die WiFi-Zugangsdaten in `secrets.yaml` eintragen.

2. **API-Key und OTA-Passwort generieren**:
   In der `humidity01.yaml` die Platzhalter "xxx" durch sichere Werte ersetzen.

3. **ESPHome installieren** (falls noch nicht geschehen):
   ```bash
   pip install esphome
   ```

4. **Konfiguration validieren**:
   ```bash
   esphome config humidity01.yaml
   ```

5. **Firmware flashen** (erstes Mal per USB):
   ```bash
   esphome run humidity01.yaml
   ```

6. **OTA Updates** (nach dem ersten Flash):
   ```bash
   esphome run humidity01.yaml --device 10.10.50.91
   ```

## Konfiguration

- **Statische IP**: 10.10.50.91
- **Gateway**: 10.10.50.1 (Mikrotik Router IoT VLAN)
- **Update-Intervall**: 60 Sekunden
- **Webserver**: http://10.10.50.91

## Features

✅ Temperaturmessung
✅ Luftfeuchtigkeitsmessung
✅ WiFi Signal-Überwachung
✅ Uptime-Tracking
✅ Home Assistant Integration
✅ OTA Updates
✅ Webserver
✅ Fallback Access Point

## Hinweise

- Die Konfiguration ist ausführlich kommentiert
- Deep Sleep ist deaktiviert (kann für Batteriebetrieb aktiviert werden)
- Filter für Median und Offset sind vorbereitet (auskommentiert)
- I2C-Scan ist aktiviert und zeigt beim Start erkannte Geräte im Log

## Kalibrierung

Falls der Sensor leicht von Referenzwerten abweicht, können Offset-Filter aktiviert werden:

```yaml
temperature:
  name: "Temperatur"
  filters:
    - offset: -0.5  # z.B. -0.5°C Korrektur
```

## Troubleshooting

1. **Sensor nicht gefunden**: I2C-Verkabelung prüfen (SDA/SCL)
2. **WiFi-Verbindung fehlschlägt**: Fallback-Hotspot "Humidity 1 Fallback Hotspot" nutzen
3. **Logs anschauen**: `esphome logs humidity01.yaml`
