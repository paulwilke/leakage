/* ============================================================================
   ESPHome Sensor Dashboard - Configuration
   ============================================================================ */

// Dashboard-Konfiguration
const DashboardConfig = {
    // ========================================================================
    // Sensor-Konfiguration
    // ========================================================================
    // Hier werden alle ESPHome-Sensoren konfiguriert
    // Füge für jeden Sensor die IP-Adresse oder den Hostnamen hinzu
    // ========================================================================

    sensors: {
        // --------------------------------------------------------------------
        // Temperatur & Luftfeuchtigkeit Sensoren
        // --------------------------------------------------------------------
        humidity: [
            {
                id: 'humidity01',
                name: 'Humidity Sensor 01',
                host: '192.168.1.100',  // IP-Adresse des Sensors
                port: 80,
                // Alternativ: Home Assistant API verwenden
                // homeAssistantEntity: 'sensor.humidity01_temperature'
            },
            // Weitere Sensoren können hier hinzugefügt werden:
            // {
            //     id: 'humidity02',
            //     name: 'Wohnzimmer Sensor',
            //     host: '192.168.1.101',
            //     port: 80,
            // }
        ],

        // --------------------------------------------------------------------
        // Wasserleck-Sensoren
        // --------------------------------------------------------------------
        leak: [
            {
                id: 'leak01',
                name: 'Leak Sensor 01',
                host: '192.168.1.101',  // IP-Adresse des Sensors
                port: 80,
                // Alternativ: Home Assistant API verwenden
                // homeAssistantEntity: 'binary_sensor.leak01_wasserleck'
            },
            // Weitere Sensoren können hier hinzugefügt werden:
            // {
            //     id: 'leak02',
            //     name: 'Badezimmer Leak Sensor',
            //     host: '192.168.1.102',
            //     port: 80,
            // }
        ]
    },

    // ========================================================================
    // Home Assistant Integration (optional)
    // ========================================================================
    // Wenn du Home Assistant verwendest, kannst du die API hier konfigurieren
    // ========================================================================
    homeAssistant: {
        enabled: false,  // Auf true setzen, um Home Assistant zu verwenden
        url: 'http://homeassistant.local:8123',
        accessToken: 'YOUR_LONG_LIVED_ACCESS_TOKEN',  // Long-lived access token
        // Token erstellen: Home Assistant -> Profil -> Long-lived access token
    },

    // ========================================================================
    // Update-Intervalle
    // ========================================================================
    updateIntervals: {
        sensors: 10000,     // Sensor-Daten alle 10 Sekunden aktualisieren (optimiert)
        charts: 60000,      // Charts alle 60 Sekunden aktualisieren
        history: 300000,    // Historie alle 5 Minuten aktualisieren
    },

    // ========================================================================
    // Chart-Konfiguration
    // ========================================================================
    charts: {
        historyHours: 24,       // Wie viele Stunden Historie anzeigen
        maxDataPoints: 288,     // Maximale Anzahl an Datenpunkten (24h * 12 = alle 5 Min)

        // Chart-Farben
        colors: {
            temperature: {
                line: 'rgb(239, 68, 68)',
                fill: 'rgba(239, 68, 68, 0.1)'
            },
            humidity: {
                line: 'rgb(37, 99, 235)',
                fill: 'rgba(37, 99, 235, 0.1)'
            }
        }
    },

    // ========================================================================
    // Alarm-Konfiguration
    // ========================================================================
    alerts: {
        // Temperatur-Schwellenwerte
        temperature: {
            min: 10,        // Alarm bei unter 10°C
            max: 35,        // Alarm bei über 35°C
        },
        // Luftfeuchtigkeits-Schwellenwerte
        humidity: {
            min: 30,        // Alarm bei unter 30%
            max: 70,        // Alarm bei über 70%
        },
        // Wasserleck-Alarm
        leakAlert: true,    // Alarm bei Wasserleck

        // Benachrichtigungen
        notifications: {
            browser: true,   // Browser-Benachrichtigungen aktivieren
            sound: true,     // Sound-Benachrichtigungen
        }
    },

    // ========================================================================
    // UI-Optionen
    // ========================================================================
    ui: {
        language: 'de',              // Sprache: 'de' oder 'en'
        theme: 'dark',               // Theme: 'dark' oder 'light'
        showOfflineSensors: true,    // Offline-Sensoren anzeigen
        animateCharts: true,         // Chart-Animationen
        compactMode: false,          // Kompakter Modus für mobile Geräte
    },

    // ========================================================================
    // Debug-Modus
    // ========================================================================
    debug: false,  // Auf true setzen für detaillierte Console-Logs
};

// ============================================================================
// Demo-Modus für Entwicklung
// ============================================================================
// Wenn kein ESPHome-Server verfügbar ist, kann der Demo-Modus aktiviert werden
// Dieser generiert zufällige Sensor-Daten für Tests
// ============================================================================
const DemoMode = {
    enabled: false,  // Auf true setzen für Demo-Daten

    // Zufallsdaten-Generierung
    generateData: () => {
        return {
            temperature: (20 + Math.random() * 10).toFixed(1),
            humidity: (40 + Math.random() * 30).toFixed(1),
            leak: Math.random() > 0.95,  // 5% Chance für Leak-Alarm
            wifiSignal: (-70 + Math.random() * 20).toFixed(0),
            uptime: Math.floor(Math.random() * 86400),
            status: Math.random() > 0.1 ? 'online' : 'offline',
        };
    }
};

// ============================================================================
// Export für ES6 Module (falls benötigt)
// ============================================================================
if (typeof module !== 'undefined' && module.exports) {
    module.exports = { DashboardConfig, DemoMode };
}
