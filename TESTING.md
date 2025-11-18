# Testing-Anleitung für ESPHome Sensoren

Dieses Dokument beschreibt alle Test-Verfahren für die ESPHome Sensor-Konfigurationen.

## 📋 Inhaltsverzeichnis

1. [Setup](#setup)
2. [Automatisierte Tests](#automatisierte-tests)
3. [Manuelle Tests](#manuelle-tests)
4. [Hardware-Tests](#hardware-tests)
5. [API-Tests](#api-tests)
6. [Troubleshooting](#troubleshooting)

---

## Setup

### 1. Virtual Environment einrichten

```bash
# Virtual Environment erstellen
python3 -m venv venv

# Aktivieren
source venv/bin/activate

# Dependencies installieren
pip install -r requirements.txt
```

### 2. Secrets konfigurieren

Erstelle eine `secrets.yaml` Datei im Repository-Root:

```yaml
wifi_ssid: "DEIN_WIFI_NAME"
wifi_password: "DEIN_WIFI_PASSWORT"
api_key: "DEIN_BASE64_API_KEY"
ota_password: "DEIN_OTA_PASSWORT"
```

**WICHTIG:** Diese Datei ist in `.gitignore` und wird NICHT committed!

---

## Automatisierte Tests

### Alle Tests ausführen (ohne Hardware)

```bash
./run_tests.sh
```

Führt aus:
- ✅ Syntax-Validierung (`esphome config`)
- ✅ Kompilierung (`esphome compile`)
- ✅ Unit-Tests mit Mock-Daten (`pytest`)

### Alle Tests inklusive Hardware

```bash
./run_tests.sh --hardware
```

Zusätzlich:
- ✅ Live API-Tests gegen echte ESP32-Geräte

### Nur Syntax-Validierung

```bash
# Humidity Sensor
esphome config humidity_sensors/humidity01.yaml

# Leak Sensor
esphome config leak_sensors/leak01.yaml
```

### Nur Kompilierung

```bash
# Humidity Sensor
esphome compile humidity_sensors/humidity01.yaml

# Leak Sensor
esphome compile leak_sensors/leak01.yaml
```

### Nur pytest Tests

```bash
# Nur Mock-Tests (ohne Hardware)
pytest tests/ -v -m "not hardware"

# Nur Hardware-Tests
pytest tests/ -v -m "hardware"

# Alle Tests
pytest tests/ -v
```

---

## Manuelle Tests

### 1. Konfiguration validieren

```bash
# Zeigt die gesamte verarbeitete Konfiguration an
esphome config humidity_sensors/humidity01.yaml

# Prüft auf Fehler
esphome config leak_sensors/leak01.yaml
```

**Expected Output:**
- Keine Fehler
- `INFO Configuration is valid!` am Ende

### 2. Kompilierung testen

```bash
# Kompiliert ohne zu flashen
esphome compile humidity_sensors/humidity01.yaml
```

**Expected Output:**
- Build-Prozess läuft durch
- `Successfully compiled program.` am Ende
- Binary-Dateien in `.esphome/build/`

### 3. Logs ansehen (ohne Flashen)

Nicht möglich ohne Hardware - siehe Hardware-Tests unten.

---

## Hardware-Tests

### Voraussetzungen

- ESP32 per USB verbunden ODER
- ESP32 im gleichen Netzwerk (für OTA)

### 1. Erstes Flashen (USB)

```bash
# Humidity Sensor
esphome run humidity_sensors/humidity01.yaml

# Leak Sensor
esphome run leak_sensors/leak01.yaml
```

**Schritte:**
1. ESPHome erkennt USB-Port automatisch
2. Firmware wird kompiliert
3. Firmware wird auf ESP32 geflasht
4. ESP32 startet neu
5. Logs werden angezeigt

**Expected Output:**
```
[I][app:102]: ESPHome version 2024.x.x compiled on ...
[C][wifi:...]: Setting up WiFi...
[C][wifi:...]: WiFi Connected!
[C][logger:...]: Logger initialized
```

### 2. OTA Updates (nach erstem Flash)

```bash
# ESP32 muss im Netzwerk sein
esphome run humidity_sensors/humidity01.yaml

# Wähle "Over The Air" wenn gefragt
```

### 3. Logs ansehen

```bash
# Live-Logs vom Gerät
esphome logs humidity_sensors/humidity01.yaml

# Oder über IP-Adresse
esphome logs 192.168.1.100
```

**Was zu prüfen:**
- ✅ WiFi Verbindung erfolgreich
- ✅ I2C Sensor gefunden (bei Humidity)
- ✅ GPIO Pin initialisiert (bei Leak)
- ✅ Webserver gestartet
- ✅ Keine ERROR-Meldungen

### 4. Funktionstest Humidity Sensor

**Hardware-Prüfung:**
1. Öffne Webserver: `http://<esp32-ip>/`
2. Prüfe Temperatur (sollte Raumtemperatur zeigen)
3. Prüfe Luftfeuchtigkeit (40-60% typisch)
4. Hauche auf Sensor → Luftfeuchtigkeit sollte steigen

**Logs prüfen:**
```bash
esphome logs humidity_sensors/humidity01.yaml
```

Erwarte:
```
[D][sensor:094]: 'Temperatur': Sending state 23.4 °C
[D][sensor:094]: 'Luftfeuchtigkeit': Sending state 45.2 %
```

### 5. Funktionstest Leak Sensor

**Hardware-Prüfung:**
1. Öffne Webserver: `http://<esp32-ip>/`
2. Status sollte "Clear" (trocken) zeigen
3. Verbinde die beiden Drähte des Sensors
4. Status sollte "Wet" (nass) anzeigen
5. Trenne Drähte wieder
6. Status sollte wieder "Clear" sein

**Logs prüfen:**
```bash
esphome logs leak_sensors/leak01.yaml
```

Bei Drähte-Verbinden:
```
[D][main:...]: ALARM! Wasserleck erkannt!
[D][binary_sensor:...]: 'Wasserleck erkannt': Sending state ON
```

Bei Drähte-Trennen:
```
[D][main:...]: Wasserleck behoben - Sensor wieder trocken
[D][binary_sensor:...]: 'Wasserleck erkannt': Sending state OFF
```

---

## API-Tests

Die ESP32-Geräte bieten JSON REST APIs für Sensordaten.

### Verfügbare Endpoints

| Endpoint | Beschreibung |
|----------|--------------|
| `/sensor` | Numerische Sensorwerte (Temperatur, Luftfeuchtigkeit, WiFi, Uptime) |
| `/text_sensor` | Text-Sensorwerte (IP, MAC, SSID) |
| `/binary_sensor` | Binäre Sensorwerte (Leak-Status, Online-Status) |

### Manuelle API-Tests mit curl

#### Humidity Sensor

```bash
# Alle numerischen Sensoren
curl http://192.168.1.100/sensor | jq

# Nur Temperatur
curl -s http://192.168.1.100/sensor | jq '.[] | select(.id | contains("temperatur"))'

# Text-Sensoren (IP, MAC, etc.)
curl http://192.168.1.100/text_sensor | jq

# Binär-Sensoren (Status)
curl http://192.168.1.100/binary_sensor | jq
```

**Expected Response (Beispiel):**
```json
[
  {
    "id": "humidity01-temperatur",
    "name": "Temperatur",
    "value": 23.4,
    "unit": "°C"
  },
  {
    "id": "humidity01-luftfeuchtigkeit",
    "name": "Luftfeuchtigkeit",
    "value": 45.2,
    "unit": "%"
  }
]
```

#### Leak Sensor

```bash
# Leak-Status prüfen
curl http://192.168.1.101/binary_sensor | jq

# Erwarte:
# [
#   {
#     "id": "leak01-wasserleck_erkannt",
#     "name": "Wasserleck erkannt",
#     "value": false  // oder true bei Alarm
#   },
#   {
#     "id": "leak01-status",
#     "name": "Status",
#     "value": true  // ESP32 ist online
#   }
# ]
```

### Automatisierte API-Tests mit pytest

```bash
# Prüfe API-Struktur mit Mock-Daten (keine Hardware)
pytest tests/test_api_endpoints.py::TestMockEndpoints -v

# Live-Tests gegen Hardware
pytest tests/test_api_endpoints.py::TestHumidityHardware -v -m hardware
pytest tests/test_api_endpoints.py::TestLeakHardware -v -m hardware
```

### API-Tests mit Python

```python
import requests

# Humidity Sensor abfragen
response = requests.get('http://192.168.1.100/sensor')
data = response.json()

# Temperatur extrahieren
temp = next(s['value'] for s in data if 'temperatur' in s['id'])
print(f"Temperatur: {temp}°C")

# Leak Sensor abfragen
response = requests.get('http://192.168.1.101/binary_sensor')
data = response.json()

# Leak-Status prüfen
leak = next((s for s in data if 'wasserleck' in s['id']), None)
if leak and leak['value']:
    print("⚠️ WASSERLECK ERKANNT!")
else:
    print("✅ Alles trocken")
```

---

## Troubleshooting

### Syntax-Fehler

**Problem:** `esphome config` zeigt Fehler

**Lösung:**
1. Prüfe YAML-Syntax (Einrückung!)
2. Prüfe ob `secrets.yaml` existiert
3. Prüfe ob alle Secret-Variablen definiert sind
4. Prüfe API-Key Format (muss Base64 sein)

### Kompilierungs-Fehler

**Problem:** `esphome compile` schlägt fehl

**Lösung:**
1. Lösche Build-Cache: `rm -rf .esphome/build/`
2. Prüfe Internet-Verbindung (Downloads benötigt)
3. Stelle sicher dass genug Speicher frei ist
4. Aktualisiere ESPHome: `pip install --upgrade esphome`

### Hardware nicht erreichbar

**Problem:** `esphome run` findet ESP32 nicht

**Lösung:**
1. USB-Kabel prüfen (Daten-Kabel, nicht nur Laden!)
2. USB-Port wechseln
3. ESP32 vom Strom trennen und neu verbinden
4. Driver installieren (CP2102 oder CH340)
5. Für OTA: IP-Adresse prüfen, Ping-Test

### Sensor nicht gefunden

**Problem:** I2C Sensor wird nicht erkannt (Humidity)

**Lösung:**
1. Verkabelung prüfen (SDA → GPIO21, SCL → GPIO22)
2. I2C-Scan in Logs prüfen
3. Sensor-Spannung prüfen (3.3V!)
4. Pull-up Widerstände prüfen (meist im Sensor integriert)

**Problem:** GPIO Sensor reagiert nicht (Leak)

**Lösung:**
1. Verkabelung prüfen (GND + GPIO4)
2. Anderen GPIO testen
3. Pullup-Konfiguration prüfen
4. Kabel auf Durchgang prüfen

### pytest Tests schlagen fehl

**Problem:** Mock-Tests schlagen fehl

**Lösung:**
1. Virtuelle Umgebung aktiviert? `source venv/bin/activate`
2. Dependencies installiert? `pip install -r requirements.txt`
3. Teste einzelne Tests: `pytest tests/test_api_endpoints.py::TestMockEndpoints::test_mock_sensor_structure -v`

**Problem:** Hardware-Tests werden übersprungen

**Lösung:**
- Das ist korrekt! Hardware-Tests benötigen `--hardware` Flag
- Oder: `pytest -m hardware`

**Problem:** Hardware-Tests schlagen fehl

**Lösung:**
1. Sind die ESP32-Geräte eingeschaltet?
2. Sind sie im gleichen Netzwerk?
3. Stimmen die IP-Adressen in `tests/test_api_endpoints.py`?
4. Ping-Test: `ping 192.168.1.100`
5. API-Test: `curl http://192.168.1.100/sensor`

### WiFi-Verbindung scheitert

**Problem:** ESP32 verbindet nicht mit WiFi

**Lösung:**
1. SSID und Passwort in `secrets.yaml` prüfen
2. WiFi-Band prüfen (ESP32 = nur 2.4 GHz!)
3. Fallback-Hotspot nutzen:
   - Humidity: "Humidity 1 Fallback Hotspot" / "bvJU6UGN6jIc"
   - Leak: "Leak Sensor 01 Fallback" / "fallback123"
4. Logs für Details prüfen

---

## CI/CD Integration

### GitHub Actions Beispiel

```yaml
name: ESPHome Tests

on: [push, pull_request]

jobs:
  test:
    runs-on: ubuntu-latest
    steps:
      - uses: actions/checkout@v3
      
      - name: Set up Python
        uses: actions/setup-python@v4
        with:
          python-version: '3.11'
      
      - name: Install dependencies
        run: |
          python -m pip install --upgrade pip
          pip install -r requirements.txt
      
      - name: Create secrets.yaml
        run: |
          echo 'wifi_ssid: "test"' > secrets.yaml
          echo 'wifi_password: "test"' >> secrets.yaml
          echo 'api_key: "dGVzdGtleQ=="' >> secrets.yaml
          echo 'ota_password: "test123"' >> secrets.yaml
      
      - name: Run tests
        run: ./run_tests.sh
```

---

## Weitere Ressourcen

- [ESPHome Dokumentation](https://esphome.io/)
- [pytest Dokumentation](https://docs.pytest.org/)
- [JSON API Dokumentation](esp32_webinterface/API_DOCUMENTATION.md)
- [Hauptprojekt README](README.md)

---

**Viel Erfolg beim Testen! 🚀**

