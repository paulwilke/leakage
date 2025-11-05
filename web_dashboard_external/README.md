# ESPHome Sensor Dashboard - Externes Webinterface

Ein modernes, professionelles Web-Dashboard zur Überwachung und Visualisierung von ESPHome-Sensoren in Echtzeit.

> ⚠️ **WICHTIG:** Dieses Dashboard läuft **NICHT** auf dem ESP32, sondern auf einem **separaten Computer/Server** (PC, Laptop, Raspberry Pi, etc.) und verbindet sich zu den ESP32-Sensoren.
>
> **Suchst du ein Dashboard direkt auf dem ESP32?** → Siehe [esp32_webinterface](../esp32_webinterface/README.md)

![Dashboard Preview](assets/dashboard-preview.png)

## Unterschied zu ESP32-Interface

| Feature | Dieses (Externes Dashboard) | ESP32-Interface |
|---------|----------------------------|-----------------|
| Läuft auf | PC/Server/Raspberry Pi | Direkt auf ESP32 |
| Zeigt an | ALLE Sensoren | Einen Sensor |
| Charts | ✅ Ja (Chart.js) | ❌ Nein (zu groß) |
| Voraussetzung | Separater Computer | Nur ESP32 |
| Komplexität | Hoch | Einfach |

**Empfehlung:**
- Für **einzelne Sensoren** → [ESP32-Interface](../esp32_webinterface/README.md)
- Für **zentrale Überwachung vieler Sensoren** → Dieses externe Dashboard

## Features

### 📊 Echtzeit-Überwachung
- Live-Anzeige aller Sensor-Daten
- Automatische Aktualisierung alle 5 Sekunden
- Responsive Design für Desktop, Tablet und Mobile
- Dunkles, modernes UI-Design

### 📈 Datenvisualisierung
- Interaktive Temperatur- und Luftfeuchtigkeits-Charts
- Historische Daten über 24 Stunden
- Trend-Analysen und Durchschnittswerte
- Powered by Chart.js

### 🚨 Alarm-System
- Schwellenwert-basierte Alarme
- Browser-Benachrichtigungen
- Alarm-Historie mit Zeitstempel
- Audio-Benachrichtigungen

### 🔌 Flexible Integration
- ESPHome Webserver API
- Home Assistant API Support
- Demo-Modus für Entwicklung und Tests
- Konfigurierbare Sensor-Endpoints

### 💎 Professionelles Design
- Moderne Card-basierte UI
- Smooth Animationen und Transitions
- Font Awesome Icons
- Responsive Grid-Layout

---

## 🚀 Schnellstart

### 1. Voraussetzungen

- ESPHome-Sensoren mit aktiviertem Webserver
- Moderner Webbrowser (Chrome, Firefox, Safari, Edge)
- Optional: Webserver (Apache, Nginx) oder direkt über `file://`

### 2. Installation

#### Option A: Direkt im Browser öffnen
```bash
# Einfach die index.html im Browser öffnen
open web_dashboard/index.html
```

#### Option B: Mit lokalem Webserver
```bash
# Python HTTP Server
cd web_dashboard
python3 -m http.server 8080

# Dann im Browser öffnen:
# http://localhost:8080
```

#### Option C: Mit Node.js http-server
```bash
npm install -g http-server
cd web_dashboard
http-server -p 8080

# Dann im Browser öffnen:
# http://localhost:8080
```

### 3. Konfiguration

Bearbeite die Datei `js/config.js` und passe die Sensor-Konfiguration an:

```javascript
const DashboardConfig = {
    sensors: {
        humidity: [
            {
                id: 'humidity01',
                name: 'Humidity Sensor 01',
                host: '192.168.1.100',  // IP-Adresse deines Sensors
                port: 80,
            }
        ],
        leak: [
            {
                id: 'leak01',
                name: 'Leak Sensor 01',
                host: '192.168.1.101',  // IP-Adresse deines Sensors
                port: 80,
            }
        ]
    }
};
```

### 4. ESPHome-Sensoren vorbereiten

Stelle sicher, dass deine ESPHome-Sensoren den Webserver aktiviert haben:

```yaml
# In deiner ESPHome YAML-Datei:
web_server:
  port: 80
```

---

## 📋 Detaillierte Konfiguration

### Sensor-Konfiguration

#### ESPHome Direkt-Anbindung

```javascript
sensors: {
    humidity: [
        {
            id: 'humidity01',              // Eindeutige ID
            name: 'Wohnzimmer',            // Anzeigename
            host: '192.168.1.100',         // IP oder Hostname
            port: 80,                       // Port des ESPHome Webservers
        }
    ]
}
```

#### Home Assistant Integration

Wenn du Home Assistant verwendest, kannst du auch direkt dessen API nutzen:

```javascript
// Home Assistant aktivieren
homeAssistant: {
    enabled: true,
    url: 'http://homeassistant.local:8123',
    accessToken: 'YOUR_LONG_LIVED_ACCESS_TOKEN',
},

// Sensoren mit Home Assistant Entity
sensors: {
    humidity: [
        {
            id: 'humidity01',
            name: 'Wohnzimmer',
            homeAssistantEntity: 'sensor.wohnzimmer_temperature'
        }
    ]
}
```

**Long-lived Access Token erstellen:**
1. Home Assistant öffnen
2. Profil → Sicherheit
3. Ganz unten "Long-lived access token" erstellen
4. Token kopieren und in `config.js` einfügen

### Update-Intervalle anpassen

```javascript
updateIntervals: {
    sensors: 5000,      // Sensoren alle 5 Sekunden
    charts: 60000,      // Charts alle 60 Sekunden
    history: 300000,    // Historie alle 5 Minuten
}
```

### Alarm-Schwellenwerte

```javascript
alerts: {
    temperature: {
        min: 10,        // Alarm bei unter 10°C
        max: 35,        // Alarm bei über 35°C
    },
    humidity: {
        min: 30,        // Alarm bei unter 30%
        max: 70,        // Alarm bei über 70%
    },
    leakAlert: true,    // Alarm bei Wasserleck

    notifications: {
        browser: true,   // Browser-Benachrichtigungen
        sound: true,     // Sound-Benachrichtigungen
    }
}
```

### UI-Anpassungen

```javascript
ui: {
    language: 'de',              // Sprache: 'de' oder 'en'
    theme: 'dark',               // Theme: 'dark' oder 'light'
    showOfflineSensors: true,    // Offline-Sensoren anzeigen
    animateCharts: true,         // Chart-Animationen
    compactMode: false,          // Kompakter Modus
}
```

---

## 🧪 Demo-Modus

Für Tests ohne echte Sensoren kann der Demo-Modus aktiviert werden:

```javascript
// In js/config.js
const DemoMode = {
    enabled: true,  // Demo-Modus aktivieren
};
```

Der Demo-Modus generiert zufällige Sensor-Daten und simuliert das Verhalten echter Sensoren.

---

## 🔧 Erweiterte Nutzung

### CORS-Probleme lösen

Falls der Browser CORS-Fehler meldet, gibt es mehrere Lösungen:

#### Option 1: Browser mit deaktivierten CORS starten

**Chrome/Chromium:**
```bash
# macOS
open -na "Google Chrome" --args --disable-web-security --user-data-dir=/tmp/chrome_dev

# Linux
google-chrome --disable-web-security --user-data-dir=/tmp/chrome_dev

# Windows
chrome.exe --disable-web-security --user-data-dir=C:\temp\chrome_dev
```

**WICHTIG:** Nutze dies nur für Entwicklung/Tests, nicht für normales Surfen!

#### Option 2: Webserver als Proxy

Nutze einen Webserver wie Nginx als Reverse Proxy:

```nginx
# nginx.conf
location /api/humidity01 {
    proxy_pass http://192.168.1.100:80;
    add_header Access-Control-Allow-Origin *;
}
```

#### Option 3: ESPHome CORS aktivieren

ESPHome unterstützt CORS noch nicht nativ. Als Workaround kannst du einen ESP32-Webserver mit CORS-Support nutzen.

### Home Assistant Add-on (empfohlen)

Die einfachste Lösung ist, das Dashboard als Home Assistant Add-on oder über Nginx Proxy Manager zu hosten.

---

## 📱 Mobile-Nutzung

Das Dashboard ist vollständig responsive und funktioniert auf allen Geräten:

- **Desktop:** Volle Funktionalität mit Multi-Column-Layout
- **Tablet:** Angepasstes 2-Column-Layout
- **Mobile:** Single-Column-Layout mit optimierten Touch-Targets

### Als Home Screen App (iOS/Android)

1. Dashboard im Browser öffnen
2. "Zum Home-Bildschirm hinzufügen"
3. Dashboard startet dann wie eine native App

---

## 🎨 Anpassung & Styling

### Farben anpassen

Bearbeite `css/style.css` und passe die CSS-Variablen an:

```css
:root {
    --color-primary: #2563eb;        /* Primärfarbe */
    --color-secondary: #10b981;      /* Sekundärfarbe */
    --color-bg-primary: #0f172a;     /* Hintergrund */
    /* ... weitere Variablen */
}
```

### Light Theme

```css
/* Light Theme Variablen */
:root {
    --color-bg-primary: #ffffff;
    --color-bg-secondary: #f8fafc;
    --color-text-primary: #0f172a;
    /* ... */
}
```

---

## 🐛 Troubleshooting

### Dashboard zeigt keine Daten

1. **Sensor erreichbar?**
   ```bash
   ping 192.168.1.100
   curl http://192.168.1.100/text_sensor
   ```

2. **CORS-Fehler?**
   - Siehe Abschnitt "CORS-Probleme lösen"

3. **Demo-Modus aktivieren zum Testen:**
   ```javascript
   const DemoMode = { enabled: true };
   ```

### Charts werden nicht angezeigt

1. **Chart.js geladen?**
   - Überprüfe die Browser-Konsole auf Fehler
   - Stelle sicher, dass Internet-Verbindung besteht (CDN)

2. **Alternative: Lokale Chart.js-Version**
   ```bash
   npm install chart.js
   # Dann in index.html den CDN-Link ersetzen
   ```

### Browser-Benachrichtigungen funktionieren nicht

1. **Permission erteilen:**
   - Browser fragt beim ersten Laden um Erlaubnis
   - Falls verweigert: Browser-Einstellungen → Benachrichtigungen

2. **HTTPS erforderlich:**
   - Browser-Benachrichtigungen funktionieren nur über HTTPS
   - Ausnahme: `localhost`

### Sensoren werden als "Offline" angezeigt

1. **IP-Adresse korrekt?**
   - Überprüfe `js/config.js`

2. **ESPHome Webserver aktiviert?**
   ```yaml
   web_server:
     port: 80
   ```

3. **Firewall-Regeln?**
   - Stelle sicher, dass Port 80 nicht blockiert wird

---

## 📊 API-Dokumentation

### ESPHome Webserver API

ESPHome bietet folgende Endpoints:

- `http://<sensor-ip>/` - Haupt-Webinterface
- `http://<sensor-ip>/text_sensor` - JSON mit Text-Sensoren
- `http://<sensor-ip>/sensor` - JSON mit Sensordaten
- `http://<sensor-ip>/binary_sensor` - JSON mit Binärsensoren

### Home Assistant REST API

- `GET /api/states/<entity_id>` - Status abrufen
- `POST /api/services/<domain>/<service>` - Service aufrufen

**Beispiel:**
```javascript
fetch('http://homeassistant.local:8123/api/states/sensor.temperature', {
    headers: {
        'Authorization': 'Bearer YOUR_TOKEN',
        'Content-Type': 'application/json'
    }
})
```

---

## 🔒 Sicherheit

### Wichtige Hinweise

1. **Access Tokens nicht committen**
   - `.gitignore` für `config.js` erstellen
   - Oder separate `config.local.js` verwenden

2. **Dashboard nur im lokalen Netzwerk**
   - Nicht öffentlich zugänglich machen ohne Authentifizierung

3. **HTTPS verwenden**
   - Für Produktivumgebungen immer HTTPS nutzen
   - Besonders wichtig mit Home Assistant Integration

### Beispiel `.gitignore`

```
# Dashboard Config mit Secrets
web_dashboard/js/config.js
web_dashboard/js/config.local.js
```

---

## 🚀 Deployment

### Statisches Hosting

Das Dashboard ist eine reine Client-Side-Anwendung und kann auf jedem Webserver gehostet werden:

- **GitHub Pages**
- **Netlify**
- **Vercel**
- **Apache/Nginx**
- **Home Assistant (via www-Ordner)**

### Home Assistant Integration

```bash
# Dashboard in Home Assistant www-Ordner kopieren
cp -r web_dashboard /config/www/esphome-dashboard

# Dann erreichbar unter:
# http://homeassistant.local:8123/local/esphome-dashboard/
```

---

## 📦 Projektstruktur

```
web_dashboard/
├── index.html              # Haupt-HTML-Datei
├── css/
│   └── style.css          # Alle Styles
├── js/
│   ├── config.js          # Konfiguration
│   └── dashboard.js       # Haupt-JavaScript
├── assets/                # Bilder und Icons
└── README.md             # Diese Datei
```

---

## 🤝 Beiträge

Verbesserungen und Pull Requests sind willkommen!

### Entwicklung

```bash
# Repository klonen
git clone <repository-url>
cd leakage/web_dashboard

# Lokalen Server starten
python3 -m http.server 8080

# Im Browser öffnen
open http://localhost:8080
```

### Roadmap

- [ ] PWA Support (Offline-Funktionalität)
- [ ] Mehrsprachigkeit (EN, DE)
- [ ] Export von Daten (CSV, JSON)
- [ ] Erweiterte Statistiken
- [ ] Sensor-Gruppen
- [ ] Custom-Widgets

---

## 📄 Lizenz

MIT License - siehe LICENSE-Datei für Details

---

## 🔗 Nützliche Links

- [ESPHome Dokumentation](https://esphome.io/)
- [Home Assistant](https://www.home-assistant.io/)
- [Chart.js Dokumentation](https://www.chartjs.org/)
- [Font Awesome Icons](https://fontawesome.com/)

---

## 💡 Support

Bei Fragen oder Problemen:

1. README vollständig lesen
2. Troubleshooting-Abschnitt konsultieren
3. Issue auf GitHub erstellen (falls Repository öffentlich)

---

**Viel Erfolg mit deinem ESPHome Sensor Dashboard! 🎉**
