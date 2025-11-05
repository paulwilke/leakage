# ESP32 Webinterface - Professionelle Dashboards

Moderne, kompakte Dashboards die **direkt auf dem ESP32** laufen!

## 📊 Features

- ✅ **Ultra-kompakt** - Nur ~5KB pro Dashboard
- ✅ **Embedded-optimiert** - Kein externes CDN, alles inline
- ✅ **Modernes Design** - Dark Theme, responsive
- ✅ **Auto-Refresh** - Aktualisiert sich alle 5 Sekunden
- ✅ **Keine Abhängigkeiten** - Reines HTML/CSS/JavaScript
- ✅ **Echtzeit-Daten** - Nutzt ESPHome JSON API

## 📁 Verfügbare Dashboards

### 1. Humidity Dashboard (`humidity_dashboard.html`)
Für Temperatur- und Luftfeuchtigkeitssensoren (SHT4x, DHT22, BME280, etc.)

**Anzeigt:**
- 🌡️ Temperatur in °C
- 💧 Luftfeuchtigkeit in %
- 📶 WiFi Signal Stärke
- ⏱️ Uptime
- 🌐 IP-Adresse, MAC, SSID

### 2. Leak Dashboard (`leak_dashboard.html`)
Für Wasserleck-Sensoren

**Anzeigt:**
- 💧 Leak-Status (Trocken / ALARM)
- ⚠️ Visuelle Warnung bei Wasserleck
- 📶 WiFi Signal Stärke
- ⏱️ Uptime
- 🌐 IP-Adresse, MAC, SSID

---

## 🚀 Installation & Nutzung

Du hast **3 Optionen**, wie du die Dashboards nutzen kannst:

### Option 1: Standard ESPHome Interface (Einfachst)

**Keine Installation nötig!**

Einfach im Browser öffnen:
```
http://<esp32-ip>/
```

Das ist das Standard-ESPHome-Interface. Funktioniert sofort, ist aber basic.

---

### Option 2: Custom Dashboard lokal öffnen (Schnell)

1. **Dashboard-Datei herunterladen**
   ```bash
   # Für Humidity-Sensor
   open humidity_dashboard.html

   # Für Leak-Sensor
   open leak_dashboard.html
   ```

2. **Im Browser öffnen**
   - Doppelklick auf die HTML-Datei
   - ODER: Datei in Browser ziehen

3. **CORS-Problem?**
   Falls der Browser Fehler meldet (CORS), öffne die Datei über einen lokalen Webserver:

   ```bash
   # Python HTTP Server
   python3 -m http.server 8080

   # Dann öffnen:
   # http://localhost:8080/humidity_dashboard.html
   ```

**Vorteil:** Schnell, kein Upload nötig
**Nachteil:** CORS-Probleme möglich, muss lokal geöffnet werden

---

### Option 3: Dashboard auf ESP32 hochladen (Professionell) ⭐ EMPFOHLEN

Upload das Dashboard direkt auf den ESP32! Dann ist es unter der ESP32-IP erreichbar.

#### Schritt 1: LittleFS in ESPHome aktivieren

Füge zu deiner YAML-Datei hinzu:

```yaml
# In humidity01.yaml oder leak01.yaml

esp32:
  board: nodemcu-32s
  framework:
    type: arduino  # WICHTIG: Arduino Framework nutzen (nicht esp-idf)

# LittleFS Filesystem
esp32_improv:
  authorizer: none

# Webserver mit custom HTML
web_server:
  port: 80
  local: true  # Wichtig für custom HTML
```

#### Schritt 2: Filesystem-Ordner erstellen

```bash
# Im ESPHome-Projekt-Ordner
mkdir -p humidity_sensors/data
mkdir -p leak_sensors/data
```

#### Schritt 3: Dashboard kopieren

```bash
# Für Humidity-Sensor
cp esp32_webinterface/humidity_dashboard.html humidity_sensors/data/index.html

# Für Leak-Sensor
cp esp32_webinterface/leak_dashboard.html leak_sensors/data/index.html
```

#### Schritt 4: Filesystem hochladen

```bash
# Mit ESPHome CLI
esphome upload-filesystem humidity_sensors/humidity01.yaml

# Oder mit PlatformIO
pio run --target uploadfs
```

#### Schritt 5: Fertig!

Öffne im Browser:
```
http://<esp32-ip>/index.html
```

**Vorteil:** Professionell, direkt auf ESP32, keine CORS-Probleme
**Nachteil:** Etwas mehr Setup

---

## 🔧 Anpassung der Dashboards

Die HTML-Dateien sind bewusst einfach gehalten und können leicht angepasst werden.

### Farben ändern

Öffne die HTML-Datei und passe die CSS-Variablen an:

```css
/* Im <style> Block */
body{
  background:linear-gradient(135deg,#0f172a,#1e293b);  /* Hintergrund */
  color:#f1f5f9;  /* Textfarbe */
}
```

### Update-Intervall ändern

```javascript
/* Im <script> Block ganz unten */
setInterval(loadData,5000);  // 5000ms = 5 Sekunden
```

### Sensor-IDs anpassen

Falls deine Sensoren andere IDs haben, passe die Filter an:

```javascript
// Beispiel: Temperatur-Sensor findet
let temp=data.find(s=>s.id.includes('temperatur')||s.id.includes('temperature'));

// Anpassen auf deine Sensor-ID:
let temp=data.find(s=>s.id=='sensor-humidity01-temperatur');
```

---

## 📱 Mobile Nutzung

Die Dashboards sind **vollständig responsive** und funktionieren perfekt auf Smartphones!

### Als App speichern (iOS)

1. Dashboard im Safari öffnen
2. Teilen-Button → "Zum Home-Bildschirm"
3. Dashboard startet jetzt wie eine native App!

### Als App speichern (Android)

1. Dashboard im Chrome öffnen
2. Menü → "Zum Startbildschirm hinzufügen"
3. Dashboard startet jetzt wie eine native App!

---

## 🐛 Troubleshooting

### Dashboard zeigt keine Daten

**Problem:** Alle Werte zeigen `--`

**Lösung:**
1. Überprüfe ob der ESP32 erreichbar ist:
   ```bash
   ping <esp32-ip>
   ```

2. Teste die ESPHome JSON API direkt:
   ```bash
   curl http://<esp32-ip>/sensor
   curl http://<esp32-ip>/text_sensor
   curl http://<esp32-ip>/binary_sensor
   ```

3. Stelle sicher, dass `web_server` in der YAML aktiviert ist

---

### CORS-Fehler in Browser-Konsole

**Problem:** `Access to fetch at ... has been blocked by CORS policy`

**Lösung:**

**Option A:** Dashboard über lokalen Webserver öffnen
```bash
python3 -m http.server 8080
```

**Option B:** Dashboard auf ESP32 hochladen (siehe Option 3)

**Option C:** Browser mit deaktivierten CORS starten (nur für Tests!)
```bash
# Chrome/Chromium
google-chrome --disable-web-security --user-data-dir=/tmp/chrome_dev

# WICHTIG: Nur für Tests, nicht für normales Surfen!
```

---

### Sensor-Werte werden nicht gefunden

**Problem:** Dashboard lädt, aber einzelne Werte fehlen

**Lösung:**

1. **Sensor-IDs überprüfen:**

   Öffne im Browser:
   ```
   http://<esp32-ip>/sensor
   ```

   Schaue dir die `id` Felder an. Beispiel:
   ```json
   [
     {"id":"sensor-humidity01-temperatur","value":22.5},
     {"id":"sensor-humidity01-luftfeuchtigkeit","value":55}
   ]
   ```

2. **Dashboard anpassen:**

   Öffne die HTML-Datei und passe die Sensor-Suche an:
   ```javascript
   // Alte Version (sucht nach "temperatur" im Namen)
   let temp=data.find(s=>s.id.includes('temperatur'));

   // Neue Version (suche nach exakter ID)
   let temp=data.find(s=>s.id=='sensor-humidity01-temperatur');
   ```

---

### Dashboard funktioniert nicht auf ESP32

**Problem:** Nach Upload auf ESP32 funktioniert das Dashboard nicht

**Lösung:**

1. **Framework überprüfen:**

   Das ESP32 Framework muss `arduino` sein, nicht `esp-idf`:
   ```yaml
   esp32:
     framework:
       type: arduino  # NICHT esp-idf!
   ```

2. **LittleFS aktiviert?**

   Stelle sicher, dass LittleFS in der YAML konfiguriert ist.

3. **Pfad korrekt?**

   Die Datei muss `data/index.html` heißen im Sensor-Ordner.

---

## 💡 Tipps & Tricks

### Mehrere Sensoren überwachen

Öffne einfach mehrere Browser-Tabs mit verschiedenen ESP32-IPs:

```
Tab 1: http://192.168.1.100/  (Humidity-Sensor 1)
Tab 2: http://192.168.1.101/  (Leak-Sensor 1)
Tab 3: http://192.168.1.102/  (Humidity-Sensor 2)
```

### Bookmark mit Namen

Speichere Bookmarks mit sprechenden Namen:
- "Wohnzimmer Temperatur"
- "Keller Leak-Sensor"
- "Badezimmer Luftfeuchtigkeit"

### Browser-Lesezeichen-Leiste

Füge alle Sensoren zur Lesezeichen-Leiste hinzu für schnellen Zugriff!

### Auto-Refresh deaktivieren

Falls du den Auto-Refresh nicht möchtest, entferne diese Zeile:

```javascript
// Diese Zeile entfernen oder auskommentieren:
setInterval(loadData,5000);
```

---

## 🔄 Updates & Wartung

### Dashboard aktualisieren

1. Neue HTML-Datei herunterladen
2. Falls auf ESP32:
   ```bash
   cp neue-version.html humidity_sensors/data/index.html
   esphome upload-filesystem humidity_sensors/humidity01.yaml
   ```

### Mehrere Versionen testen

Speichere verschiedene Versionen mit anderen Namen:
- `dashboard_v1.html`
- `dashboard_v2.html`
- `dashboard_custom.html`

---

## 📊 Vergleich der Optionen

| Feature | Standard ESPHome | Lokal öffnen | Auf ESP32 |
|---------|-----------------|--------------|-----------|
| Setup-Aufwand | ✅ Keiner | ✅ Minimal | ⚠️ Mittel |
| Design | ⚠️ Basic | ✅ Modern | ✅ Modern |
| CORS-Probleme | ✅ Keine | ⚠️ Möglich | ✅ Keine |
| Offline-Nutzung | ✅ Ja | ❌ Nein | ✅ Ja |
| Anpassbar | ❌ Nein | ✅ Ja | ✅ Ja |
| Mobile-App | ⚠️ Basic | ✅ Gut | ✅ Perfekt |

**Empfehlung:** Option 3 (Auf ESP32) für beste Erfahrung!

---

## 🎨 Weitere Anpassungen

### Light Theme

Ändere die Farben für ein helles Theme:

```css
body{
  background:linear-gradient(135deg,#ffffff,#f8fafc);
  color:#0f172a;
}
.card{
  background:#ffffff;
  border:1px solid #e2e8f0;
}
```

### Eigenes Logo

Füge oben im Dashboard ein Logo hinzu:

```html
<div class="container">
  <img src="logo.png" style="width:100px;margin:0 auto;display:block">
  <h1>🌡️ Mein Custom Sensor</h1>
```

### Zusätzliche Sensoren

Falls du mehr Sensoren hast, füge weitere Karten hinzu:

```html
<div class="sensor-item">
  <div class="sensor-label">Luftdruck</div>
  <div class="sensor-value" id="pressure">-- hPa</div>
</div>
```

```javascript
let pressure=data.find(s=>s.id.includes('pressure'));
if(pressure)document.getElementById('pressure').textContent=pressure.value.toFixed(0)+' hPa';
```

---

## 📚 Weitere Ressourcen

- [ESPHome Dokumentation](https://esphome.io/)
- [ESPHome Web Server Component](https://esphome.io/components/web_server.html)
- [ESP32 LittleFS](https://github.com/lorol/LITTLEFS)

---

## 🆘 Support

Bei Problemen:
1. README vollständig lesen
2. Troubleshooting-Abschnitt konsultieren
3. Browser-Konsole überprüfen (F12)
4. ESPHome Logs überprüfen

---

**Viel Erfolg mit deinem ESP32-Dashboard! 🎉**
