# ESP32 Wasserleck-Sensor mit Web-Dashboard

Ein vollständiges ESP32-basiertes Wasserleck-Erkennungssystem mit modernem Web-Dashboard und Konfigurationsinterface - alles direkt vom ESP32 ausgeliefert.

## Features

- 💧 **Wasserleck-Erkennung** mit GPIO-basiertem Sensor
- 📊 **Echtzeit-Dashboard** mit automatischer Aktualisierung
- ⚙️ **Web-basierte Konfiguration** für WiFi, NTP und System-Einstellungen
- 🔒 **Passwort-geschützte Admin-Seite**
- 📡 **JSON REST-API** für alle Sensordaten
- 💾 **Persistente Speicherung** aller Einstellungen in NVS
- 🔄 **OTA-Updates** für Firmware-Aktualisierungen
- 🌐 **Alles vom ESP32** - keine externe Webapp nötig

## Hardware-Anforderungen

- **ESP32** Board (z.B. NodeMCU-32S)
- **Wasserleck-Sensor** (2-Draht-Kabel, z.B. Shelly Leak Sensor Cable)
- **USB-Kabel** für initiales Flashen

### Hardware-Setup

```
ESP32 Pin  |  Leak Sensor Cable
-----------|-------------------
GND        |  Draht 1 (schwarz)
GPIO4      |  Draht 2 (rot)
```

**Funktionsweise:** Der ESP32 aktiviert einen internen Pull-up-Widerstand auf GPIO4. Bei trockenem Sensor ist der Pin HIGH. Wenn Wasser die beiden Drähte verbindet, wird der Pin LOW → Leck erkannt!

## Erste Schritte

### 1. Repository klonen

```bash
git clone https://github.com/YOUR_USERNAME/leakage.git
cd leakage
```

### 2. Python Virtual Environment erstellen

```bash
python3 -m venv venv
source venv/bin/activate  # Auf Windows: venv\Scripts\activate
```

### 3. Dependencies installieren

```bash
pip install -r requirements.txt
```

### 4. Secrets konfigurieren

```bash
cp secrets.yaml.example secrets.yaml
```

Bearbeite `secrets.yaml` und füge deine WiFi-Credentials ein:

```yaml
wifi_ssid: "DeinWiFiName"
wifi_password: "DeinWiFiPasswort"
api_key: "GENERIERTER_API_KEY"
ota_password: "GENERIERTES_OTA_PASSWORT"
```

**API Key generieren:**
```bash
source venv/bin/activate
python -c "import secrets; print(secrets.token_urlsafe(32))"
```

### 5. Hardware anschließen

- Verbinde den ESP32 per USB mit deinem Computer
- Schließe den Leak-Sensor an (GPIO4 + GND)

### 6. Firmware flashen

```bash
./flash_esp32_complete.sh
```

Wähle Option 1 (USB) für das erste Flash.

### 7. Dashboard aufrufen

Nach dem Neustart:
- Öffne http://10.10.50.91/ im Browser
- Standard-Login: **admin** / **admin**
- **Wichtig:** Ändere das Passwort sofort über Einstellungen → System!

## Entwicklungs-Workflow

### Feature entwickeln

1. **YAML bearbeiten:**
   ```bash
   nano leak_sensors/leak01.yaml
   ```

2. **Custom Components anpassen:**
   ```bash
   nano leak_sensors/custom_components/idf_webserver.h
   nano leak_sensors/custom_components/config_storage.h
   ```

3. **Syntax prüfen:**
   ```bash
   source venv/bin/activate
   esphome config leak_sensors/leak01.yaml
   ```

### Tests ausführen

**Alle Tests (Mock + Hardware):**
```bash
source venv/bin/activate
./run_tests.sh
```

**Nur Mock-Tests (ohne Hardware):**
```bash
pytest tests/test_api_endpoints.py -v -k "not Hardware"
```

**Nur Hardware-Tests:**
```bash
pytest tests/test_api_endpoints.py::TestLeakHardware -v
```

### Kompilieren

```bash
source venv/bin/activate
esphome compile leak_sensors/leak01.yaml
```

### Deployment

**Via USB:**
```bash
./flash_esp32_complete.sh
# Wähle Option 1
```

**Via OTA (drahtlos):**
```bash
./flash_esp32_complete.sh
# Wähle Option 2
```

**Oder manuell:**
```bash
source venv/bin/activate
esphome run leak_sensors/leak01.yaml --device 10.10.50.91
```

### Logs anzeigen

```bash
source venv/bin/activate
esphome logs leak_sensors/leak01.yaml --device 10.10.50.91
```

## Verwendung

### Dashboard

**URL:** http://10.10.50.91/

Zeigt in Echtzeit:
- Leak-Status (TROCKEN ✓ / WASSERLECK ⚠️)
- WiFi-Signal-Stärke
- Uptime
- IP-Adresse, MAC, SSID

**Auto-Refresh:** Alle 3 Sekunden (konfigurierbar in System-Einstellungen)

### Konfigurationsinterface

**URL:** http://10.10.50.91/login

**Standard-Login:** admin / admin

#### WiFi-Konfiguration
- Netzwerk-Scan durchführen
- SSID und Passwort ändern
- Nach Änderung: Automatischer Neustart

#### NTP/Zeit-Konfiguration
- Primärer und sekundärer NTP-Server
- Zeitzone auswählen
- Standard: pool.ntp.org, time.google.com

#### System-Einstellungen
- **Gerätename** ändern
- **Admin-Passwort** ändern
- **OTA-Passwort** ändern
- **Sensor-Filter** anpassen (delayed_on/delayed_off)
- **Dashboard-Refresh-Intervall** einstellen
- **Neustart** durchführen
- **Factory-Reset** (setzt alle Einstellungen zurück)

### REST-API

Alle Endpunkte sind öffentlich zugänglich (kein Auth erforderlich):

**Binärsensoren (Leak-Status):**
```bash
curl http://10.10.50.91/binary_sensor
```

Beispiel-Response:
```json
[
  {
    "id": "leak01-wasserleck_erkannt",
    "name": "Wasserleck erkannt",
    "value": false
  },
  {
    "id": "leak01-status",
    "name": "Status",
    "value": true
  }
]
```

**Sensoren (WiFi, Uptime):**
```bash
curl http://10.10.50.91/sensor
```

**Text-Sensoren (IP, MAC, SSID):**
```bash
curl http://10.10.50.91/text_sensor
```

## Testing

### Test-Arten

#### Mock-Tests
- Testen API-Struktur und Datentypen
- Benötigen **keine** Hardware
- Schnell und immer verfügbar
- Ideal für CI/CD

```bash
pytest tests/test_api_endpoints.py -v -k "not Hardware"
```

#### Hardware-Tests
- Testen echte API-Responses vom ESP32
- Benötigen ESP32 auf **10.10.50.91**
- Validieren echte Sensordaten

```bash
pytest tests/test_api_endpoints.py::TestLeakHardware -v
```

### Test-Struktur

```
tests/
├── conftest.py              # pytest Fixtures & Config
├── test_api_endpoints.py    # API Tests (Mock + Hardware)
└── README.md                # Test-Dokumentation
```

### Eigene Tests hinzufügen

1. Test-Funktion in `tests/test_api_endpoints.py` erstellen
2. Mock-Tests mit `@pytest.fixture` Daten
3. Hardware-Tests in `TestLeakHardware` Klasse
4. Tests ausführen mit `pytest -v`

## Troubleshooting

### ESP32 nicht erreichbar

**Problem:** `curl: (7) Failed to connect`

**Lösungen:**
1. Prüfe IP-Adresse im Router
2. Prüfe WiFi-Verbindung: `ping 10.10.50.91`
3. Logs prüfen: `esphome logs leak_sensors/leak01.yaml --device 10.10.50.91`
4. Bei WiFi-Problemen: Fallback-Hotspot "Leak Sensor 01 Fallback" (PW: fallback123)

### Kompilierungsfehler

**Problem:** `Failed config` oder C++ Fehler

**Lösungen:**
1. Syntax prüfen: `esphome config leak_sensors/leak01.yaml`
2. Dependencies updaten: `pip install --upgrade esphome`
3. Build-Cache löschen: `rm -rf leak_sensors/.esphome/build/`

### Login funktioniert nicht

**Problem:** "Ungültige Anmeldedaten"

**Lösungen:**
1. Standard-Login: admin / admin
2. Factory-Reset via Serial-Konsole
3. NVS manuell löschen und neu flashen

### Sensor erkennt Leck nicht

**Problem:** Keine Alarm-Meldung bei Wasser

**Lösungen:**
1. Verkabelung prüfen (GPIO4 + GND)
2. Logs prüfen auf GPIO-Events
3. Sensor-Filter anpassen (delayed_on/delayed_off)
4. Pin-Zustand testen: Drähte kurzschließen sollte Log-Meldung erzeugen

### OTA-Update schlägt fehl

**Problem:** Upload timeout oder Verbindungsfehler

**Lösungen:**
1. ESP32 neu starten
2. Via USB flashen statt OTA
3. OTA-Passwort in `secrets.yaml` prüfen
4. Port 3232 im Firewall freigeben

## Projekt-Struktur

```
leakage/
├── leak_sensors/                  # ESP32 Firmware
│   ├── leak01.yaml               # Haupt-Konfiguration (ESPHome)
│   ├── secrets.yaml              # WiFi/Passwörter (nicht in Git!)
│   ├── original.yaml             # Backup der Original-Config
│   └── custom_components/        # Custom C++ Code
│       ├── config_storage.h      # NVS Persistenz
│       └── idf_webserver.h       # HTTP Server + UI
│
├── tests/                        # pytest Tests
│   ├── conftest.py              # Test-Konfiguration
│   ├── test_api_endpoints.py    # API Tests
│   └── README.md                # Test-Dokumentation
│
├── flash_esp32_complete.sh      # Deploy-Script (USB/OTA)
├── run_tests.sh                 # Test-Automation
├── requirements.txt             # Python Dependencies
├── secrets.yaml.example         # Template für Secrets
└── README.md                    # Diese Datei
```

## Architektur

### Komponenten

**ESPHome (YAML):**
- Basis-Konfiguration (WiFi, API, OTA)
- Sensor-Definitionen (GPIO, WiFi-Signal, Uptime)
- Integration von Custom Components

**ConfigStorage (C++):**
- Persistente Speicherung in ESP32 NVS
- Konfiguration: WiFi, Admin-Credentials, NTP, System

**IDFWebServer (C++):**
- HTTP Server auf Port 80
- Session-Management (Cookie-basiert)
- REST-API Endpoints
- Eingebettetes HTML/CSS/JS Dashboard
- Konfigurationsseiten

### Datenfluss

```
Browser → ESP32:80 → IDFWebServer
                    ↓
         [Session Check] → ConfigStorage (NVS)
                    ↓
         [JSON Builder] ← ESPHome Sensors
                    ↓
         Browser ← HTML/JSON Response
```

## Sicherheit

### Best Practices

1. **Admin-Passwort ändern** sofort nach erstem Login
2. **OTA-Passwort** in `secrets.yaml` sicher aufbewahren
3. **secrets.yaml** niemals committen (ist in `.gitignore`)
4. **Session-Timeout:** 1 Stunde (konfigurierbar)
5. **Nur HTTP:** Kein SSL (für Einfachheit - lokales Netzwerk)

### Empfohlene Maßnahmen

- ESP32 in eigenem VLAN isolieren
- Firewall-Regeln für Port 80/3232
- Starke Passwörter verwenden
- Regelmäßige Firmware-Updates

## Lizenz

MIT License - Siehe LICENSE Datei

## Beitragen

Pull Requests sind willkommen! Für größere Änderungen bitte zuerst ein Issue öffnen.

### Entwicklungs-Workflow für Contributors

1. Fork erstellen
2. Feature-Branch erstellen (`git checkout -b feature/AmazingFeature`)
3. Änderungen committen (`git commit -m 'Add some AmazingFeature'`)
4. Tests ausführen (`./run_tests.sh`)
5. Branch pushen (`git push origin feature/AmazingFeature`)
6. Pull Request öffnen

## Support

Bei Problemen oder Fragen:
1. README und Troubleshooting-Section prüfen
2. Logs analysieren (`esphome logs ...`)
3. Issue auf GitHub öffnen mit:
   - Problembeschreibung
   - ESPHome-Version
   - Relevante Logs
   - Hardware-Details

## Roadmap

- [ ] MQTT Integration
- [ ] Home Assistant Discovery
- [ ] Mehrere Sensoren unterstützen
- [ ] E-Mail/Push-Benachrichtigungen
- [ ] Historische Daten/Graphen
- [ ] Backup/Restore von Konfigurationen

---

**Version:** 2.0  
**Letztes Update:** 2025-01-18  
**ESPHome:** 2025.10.5+
