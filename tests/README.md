# Hardware-Tests für ESPHome Sensoren

## Schnellstart

### 1. IP-Adresse des ESP32 in Tests konfigurieren

Die IP-Adressen sind bereits auf dein Gerät gesetzt (`10.10.50.91`).

Zum Anpassen öffne `tests/test_api_endpoints.py` und ändere:

```python
HUMIDITY_SENSOR_IP = "10.10.50.91"  # Deine ESP32 IP
LEAK_SENSOR_IP = "10.10.50.91"      # Oder andere IP für zweites Gerät
```

### 2. Prüfe ob das Gerät erreichbar ist

```bash
# Ping-Test
ping 10.10.50.91

# API-Test
curl http://10.10.50.91/sensor
```

**Expected Output von curl:**
```json
[
  {
    "id": "humidity01-temperatur",
    "name": "Temperatur", 
    "value": 23.4,
    "unit": "°C"
  },
  ...
]
```

### 3. Führe Hardware-Tests aus

```bash
# Aktiviere venv
source venv/bin/activate

# Nur Hardware-Tests
pytest tests/ -v -m hardware

# Oder alle Tests
./run_tests.sh --hardware
```

## Troubleshooting

### Gerät nicht erreichbar

**Problem:** `curl http://10.10.50.91/sensor` gibt Fehler

**Checklist:**
1. ✅ Ist das ESP32 eingeschaltet?
2. ✅ Ist es im gleichen Netzwerk?
3. ✅ Ping funktioniert: `ping 10.10.50.91`?
4. ✅ Ist der Webserver aktiviert? (sollte in YAML sein)
5. ✅ Firewall blockiert Port 80?

**Logs vom Gerät ansehen:**
```bash
esphome logs 10.10.50.91
```

Erwarte:
```
[I][app:102]: ESPHome version ...
[C][wifi:...]: WiFi Connected!
[C][web_server:...]: Web Server listening on port 80
```

### Tests werden übersprungen

Das ist normal! Ohne `--hardware` Flag oder `-m hardware` werden Hardware-Tests übersprungen.

```bash
# So werden sie ausgeführt:
pytest tests/ -v -m hardware
```

### Tests schlagen fehl

**Bei Connection Error:**
```
requests.exceptions.ConnectionError: ... Connection refused
```
→ Siehe "Gerät nicht erreichbar" oben

**Bei Timeout:**
```
requests.exceptions.Timeout
```
→ Erhöhe `API_TIMEOUT` in `test_api_endpoints.py`

## Welches Gerät wird getestet?

Aktuell beide Test-Klassen testen das gleiche Gerät (`10.10.50.91`):
- `TestHumidityHardware` - testet `/sensor`, `/text_sensor`, `/binary_sensor`
- `TestLeakHardware` - testet Leak-spezifische Endpoints

Wenn du ein **Humidity**-Gerät hast, laufen die `TestHumidityHardware` Tests.
Wenn du ein **Leak**-Gerät hast, funktionieren die Leak-Tests besser.

**Beide auf gleichem Gerät testen geht auch!** Die Tests skippen automatisch wenn Sensoren nicht vorhanden sind.

## Manuelle API-Tests

### Alle Endpoints testen

```bash
# Numerische Sensoren (Temperatur, etc.)
curl http://10.10.50.91/sensor | jq

# Text Sensoren (IP, MAC, SSID)
curl http://10.10.50.91/text_sensor | jq

# Binary Sensoren (Status, Leak)
curl http://10.10.50.91/binary_sensor | jq
```

### Mit Python

```python
import requests

# ESP32 IP
ip = "10.10.50.91"

# Alle Sensoren abrufen
response = requests.get(f"http://{ip}/sensor")
print(response.json())

# Temperatur extrahieren (wenn Humidity Sensor)
data = response.json()
temp_sensor = next((s for s in data if 'temperatur' in s['id'].lower()), None)
if temp_sensor:
    print(f"Temperatur: {temp_sensor['value']}{temp_sensor['unit']}")
```

## OTA-Update testen

Da dein Gerät auf Port 3232 für OTA lauscht:

```bash
# Mit ESPHome
esphome run humidity_sensors/humidity01.yaml

# Wähle "Over The Air (10.10.50.91)"
```

Das funktioniert nur wenn:
- ✅ ESP32 bereits geflasht wurde (mindestens einmal per USB)
- ✅ OTA in YAML konfiguriert ist
- ✅ Korrektes OTA-Passwort in `secrets.yaml`

## Weitere Hilfe

Siehe Haupt-Dokumentation: [TESTING.md](../TESTING.md)

