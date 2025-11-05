# ESPHome Sensor Konfigurationen

Dieses Repository enthält ESPHome-Konfigurationen für verschiedene Sensoren.

## Repository-Struktur

```
.
├── humidity_sensors/        # Temperatur- und Luftfeuchtigkeitssensoren
│   └── humidity01.yaml     # SHT4x Sensor mit ESP32
├── leak_sensors/           # Wasserleck-Sensoren
│   └── leak01.yaml         # Leak Sensor Cable mit ESP32
├── esp32_webinterface/     # 🖥️ Kompakte Dashboards FÜR ESP32
│   ├── humidity_dashboard.html  # Läuft AUF dem ESP32
│   ├── leak_dashboard.html      # Läuft AUF dem ESP32
│   └── README.md           # Detaillierte Anleitung
├── web_dashboard_external/ # 📊 Externes Dashboard (PC/Raspberry Pi)
│   ├── index.html          # Zentrale Überwachung ALLER Sensoren
│   ├── css/, js/           # Komplexes Dashboard mit Charts
│   └── README.md           # Dokumentation
└── secrets.yaml            # WiFi-Zugangsdaten (nicht im Git!)
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

---

## 🖥️ Web-Dashboards - Zwei Optionen!

Du hast die Wahl zwischen zwei professionellen Dashboard-Lösungen:

### Option 1: ESP32-Webinterface ⭐ EMPFOHLEN

**Kompakte Dashboards die DIREKT auf dem ESP32 laufen!**

```
📍 Verzeichnis: esp32_webinterface/
```

**Features:**
- ✅ Läuft **direkt auf dem ESP32** (kein extra Server nötig!)
- ✅ Ultra-kompakt (~5KB)
- ✅ Modernes Dark-Theme Design
- ✅ Auto-Refresh alle 5 Sekunden
- ✅ Mobile-optimiert
- ✅ Keine externen Abhängigkeiten

**Schnellstart:**

1. **Standard-Interface nutzen:**
   ```
   http://<esp32-ip>/
   ```
   ✅ Funktioniert sofort, kein Setup!

2. **Custom Dashboard hochladen:**
   ```bash
   # Dashboard lokal öffnen
   open esp32_webinterface/humidity_dashboard.html

   # Oder auf ESP32 hochladen (siehe README)
   ```

3. **Detaillierte Anleitung:**
   → [esp32_webinterface/README.md](esp32_webinterface/README.md)

**Ideal für:**
- ✅ Einzelne Sensoren
- ✅ Einfache Überwachung
- ✅ Kein extra Server verfügbar

---

### Option 2: Externes Dashboard (PC/Raspberry Pi)

**Zentrales Dashboard zur Überwachung ALLER Sensoren mit Charts!**

```
📍 Verzeichnis: web_dashboard_external/
```

**Features:**
- 📊 **Zentrale Überwachung** aller Sensoren
- 📈 **Interaktive Charts** mit 24h-Historie
- 🚨 **Alarm-System** mit Browser-Benachrichtigungen
- 🏠 **Home Assistant Integration**
- 📱 **Responsive Design** für alle Geräte

**Schnellstart:**

```bash
cd web_dashboard_external
python3 -m http.server 8080

# Im Browser: http://localhost:8080
```

**Konfiguration in `js/config.js`:**
```javascript
sensors: {
    humidity: [
        { id: 'humidity01', host: '192.168.1.100', port: 80 }
    ],
    leak: [
        { id: 'leak01', host: '192.168.1.101', port: 80 }
    ]
}
```

**Detaillierte Anleitung:**
→ [web_dashboard_external/README.md](web_dashboard_external/README.md)

**Ideal für:**
- ✅ Mehrere Sensoren zentral überwachen
- ✅ Charts und Historie wichtig
- ✅ PC/Raspberry Pi vorhanden

---

### Vergleich der Optionen

| Feature | ESP32-Interface | Externes Dashboard |
|---------|----------------|-------------------|
| **Läuft auf** | Direkt auf ESP32 | PC/Server/Raspberry Pi |
| **Setup** | ✅ Minimal | ⚠️ Etwas Aufwand |
| **Charts** | ❌ Nein | ✅ Ja (24h Historie) |
| **Mehrere Sensoren** | ❌ Nein (nur einer) | ✅ Ja (alle zentral) |
| **Größe** | ✅ ~5KB | ⚠️ ~500KB |
| **Extra Server nötig** | ✅ Nein | ❌ Ja |
| **Mobile** | ✅ Ja | ✅ Ja |

**Empfehlung:**
- Für die meisten Nutzer → **Option 1 (ESP32-Interface)**
- Für Power-User mit vielen Sensoren → **Option 2 (Externes Dashboard)**
- Oder beides kombinieren! 🎉

---

## Weitere Informationen

- [ESPHome Dokumentation](https://esphome.io/)
- [Home Assistant ESPHome Integration](https://www.home-assistant.io/integrations/esphome/)
- [ESP32 GPIO Pinout](https://randomnerdtutorials.com/esp32-pinout-reference-gpios/)
