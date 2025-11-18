/* ============================================================================
   ESPHome Sensor Dashboard - Main JavaScript
   ============================================================================ */

// ============================================================================
// Global State
// ============================================================================
const DashboardState = {
    sensors: {
        humidity: [],
        leak: []
    },
    charts: {
        temperature: null,
        humidity: null
    },
    history: {
        temperature: [],
        humidity: [],
        timestamps: []
    },
    alerts: [],
    updateIntervals: [],
    lastUpdate: null
};

// ============================================================================
// Dashboard-Klasse
// ============================================================================
class Dashboard {
    constructor() {
        this.init();
    }

    async init() {
        this.log('Dashboard wird initialisiert...');

        // Event-Listener registrieren
        this.registerEventListeners();

        // Charts initialisieren
        this.initCharts();

        // Erste Datenabfrage
        await this.updateAllSensors();

        // Auto-Update starten
        this.startAutoUpdate();

        // Browser-Benachrichtigungen anfragen (falls aktiviert)
        if (DashboardConfig.alerts.notifications.browser) {
            this.requestNotificationPermission();
        }

        this.log('Dashboard erfolgreich initialisiert!');
    }

    // ========================================================================
    // Event-Listener
    // ========================================================================
    registerEventListeners() {
        // Refresh-Button
        const refreshBtn = document.getElementById('refresh-btn');
        if (refreshBtn) {
            refreshBtn.addEventListener('click', () => {
                this.updateAllSensors();
                this.animateRefreshButton();
            });
        }
    }

    animateRefreshButton() {
        const btn = document.getElementById('refresh-btn');
        const icon = btn.querySelector('i');
        icon.style.transform = 'rotate(360deg)';
        setTimeout(() => {
            icon.style.transform = 'rotate(0deg)';
        }, 300);
    }

    // ========================================================================
    // Sensor-Daten aktualisieren
    // ========================================================================
    async updateAllSensors() {
        this.log('Aktualisiere alle Sensoren...');

        try {
            // Humidity-Sensoren aktualisieren
            await Promise.all(
                DashboardConfig.sensors.humidity.map(sensor =>
                    this.updateHumiditySensor(sensor)
                )
            );

            // Leak-Sensoren aktualisieren
            await Promise.all(
                DashboardConfig.sensors.leak.map(sensor =>
                    this.updateLeakSensor(sensor)
                )
            );

            // System-Status aktualisieren
            this.updateSystemStatus();

            // Charts aktualisieren
            this.updateChartData();

            // Letzte Aktualisierung speichern
            DashboardState.lastUpdate = new Date();
            this.updateLastUpdateDisplay();

            this.log('Alle Sensoren aktualisiert!');

        } catch (error) {
            this.logError('Fehler beim Aktualisieren der Sensoren:', error);
        }
    }

    // ========================================================================
    // Humidity-Sensor aktualisieren
    // ========================================================================
    async updateHumiditySensor(sensor) {
        this.log(`Aktualisiere ${sensor.name}...`);

        try {
            let data;

            // Demo-Modus oder echte Daten
            if (DemoMode.enabled) {
                data = DemoMode.generateData();
            } else if (DashboardConfig.homeAssistant.enabled) {
                data = await this.fetchHomeAssistantData(sensor);
            } else {
                data = await this.fetchESPHomeData(sensor);
            }

            // Sensor-Daten in State speichern
            const existingIndex = DashboardState.sensors.humidity.findIndex(s => s.id === sensor.id);
            const sensorData = { ...sensor, ...data, lastUpdate: new Date() };

            if (existingIndex >= 0) {
                DashboardState.sensors.humidity[existingIndex] = sensorData;
            } else {
                DashboardState.sensors.humidity.push(sensorData);
            }

            // UI aktualisieren
            this.renderHumiditySensor(sensorData);

            // Alarme prüfen
            this.checkAlerts(sensorData);

        } catch (error) {
            this.logError(`Fehler bei ${sensor.name}:`, error);
            this.renderOfflineSensor('humidity', sensor);
        }
    }

    // ========================================================================
    // Leak-Sensor aktualisieren
    // ========================================================================
    async updateLeakSensor(sensor) {
        this.log(`Aktualisiere ${sensor.name}...`);

        try {
            let data;

            // Demo-Modus oder echte Daten
            if (DemoMode.enabled) {
                data = DemoMode.generateData();
            } else if (DashboardConfig.homeAssistant.enabled) {
                data = await this.fetchHomeAssistantData(sensor);
            } else {
                data = await this.fetchESPHomeData(sensor);
            }

            // Sensor-Daten in State speichern
            const existingIndex = DashboardState.sensors.leak.findIndex(s => s.id === sensor.id);
            const sensorData = { ...sensor, ...data, lastUpdate: new Date() };

            if (existingIndex >= 0) {
                DashboardState.sensors.leak[existingIndex] = sensorData;
            } else {
                DashboardState.sensors.leak.push(sensorData);
            }

            // UI aktualisieren
            this.renderLeakSensor(sensorData);

            // Alarme prüfen (bei Wasserleck)
            if (data.leak && DashboardConfig.alerts.leakAlert) {
                this.triggerAlert({
                    type: 'leak',
                    sensor: sensor.name,
                    message: 'Wasserleck erkannt!',
                    severity: 'danger'
                });
            }

        } catch (error) {
            this.logError(`Fehler bei ${sensor.name}:`, error);
            this.renderOfflineSensor('leak', sensor);
        }
    }

    // ========================================================================
    // Daten von ESPHome abrufen
    // ========================================================================
    async fetchESPHomeData(sensor) {
        // ESPHome bietet eine JSON-API unter /text_sensor
        // Für genauere Daten kann die Home Assistant ESPHome API verwendet werden

        const url = `http://${sensor.host}:${sensor.port}/text_sensor`;

        try {
            // Timeout mit AbortController implementieren
            const controller = new AbortController();
            const timeoutId = setTimeout(() => controller.abort(), 3000); // 3 Sekunden Timeout

            const response = await fetch(url, {
                method: 'GET',
                mode: 'cors',
                signal: controller.signal
            });

            clearTimeout(timeoutId);

            if (!response.ok) {
                throw new Error(`HTTP error! status: ${response.status}`);
            }

            const data = await response.json();

            // Daten parsen und zurückgeben
            return this.parseESPHomeData(data);

        } catch (error) {
            // Fallback auf Demo-Daten bei Netzwerkfehler
            if (error.name === 'AbortError') {
                this.logError('ESPHome-Verbindung timeout, verwende Demo-Daten');
            } else {
                this.logError('ESPHome-Verbindung fehlgeschlagen, verwende Demo-Daten');
            }
            return DemoMode.generateData();
        }
    }

    parseESPHomeData(data) {
        // ESPHome-Datenstruktur in unser Format konvertieren
        // Dies muss an die tatsächliche ESPHome-API angepasst werden
        return {
            temperature: data.temperature?.value || 0,
            humidity: data.humidity?.value || 0,
            leak: data.leak?.value === 'ON' || false,
            wifiSignal: data.wifi_signal?.value || 0,
            uptime: data.uptime?.value || 0,
            status: 'online'
        };
    }

    // ========================================================================
    // Daten von Home Assistant abrufen
    // ========================================================================
    async fetchHomeAssistantData(sensor) {
        if (!sensor.homeAssistantEntity) {
            throw new Error('Keine Home Assistant Entity konfiguriert');
        }

        const url = `${DashboardConfig.homeAssistant.url}/api/states/${sensor.homeAssistantEntity}`;

        // Timeout mit AbortController implementieren
        const controller = new AbortController();
        const timeoutId = setTimeout(() => controller.abort(), 3000); // 3 Sekunden Timeout

        try {
            const response = await fetch(url, {
                method: 'GET',
                headers: {
                    'Authorization': `Bearer ${DashboardConfig.homeAssistant.accessToken}`,
                    'Content-Type': 'application/json'
                },
                signal: controller.signal
            });

            clearTimeout(timeoutId);

            if (!response.ok) {
                throw new Error(`Home Assistant API error! status: ${response.status}`);
            }

            const data = await response.json();

            return {
                temperature: parseFloat(data.attributes.temperature) || 0,
                humidity: parseFloat(data.attributes.humidity) || 0,
                status: data.state === 'unavailable' ? 'offline' : 'online'
            };
        } finally {
            clearTimeout(timeoutId);
        }
    }

    // ========================================================================
    // UI-Rendering: Humidity-Sensor
    // ========================================================================
    renderHumiditySensor(sensor) {
        const container = document.getElementById('humidity-sensors');
        let card = document.getElementById(`sensor-${sensor.id}`);

        if (!card) {
            card = document.createElement('div');
            card.id = `sensor-${sensor.id}`;
            card.className = 'sensor-card fade-in';
            container.appendChild(card);
        }

        const statusClass = sensor.status === 'online' ? 'online' : 'offline';

        card.innerHTML = `
            <div class="sensor-header">
                <h3>${sensor.name}</h3>
                <span class="sensor-status ${statusClass}">${sensor.status === 'online' ? 'Online' : 'Offline'}</span>
            </div>
            <div class="sensor-content">
                <div class="sensor-reading">
                    <i class="fas fa-thermometer-half"></i>
                    <div>
                        <span class="reading-value">${sensor.temperature}°C</span>
                        <div class="reading-label">Temperatur</div>
                    </div>
                </div>
                <div class="sensor-reading">
                    <i class="fas fa-tint"></i>
                    <div>
                        <span class="reading-value">${sensor.humidity}%</span>
                        <div class="reading-label">Luftfeuchtigkeit</div>
                    </div>
                </div>
                <div class="sensor-reading">
                    <i class="fas fa-wifi"></i>
                    <div>
                        <span class="reading-value">${sensor.wifiSignal} dBm</span>
                        <div class="reading-label">WiFi Signal</div>
                    </div>
                </div>
            </div>
        `;

        // Skeleton-Klasse entfernen
        card.classList.remove('skeleton');
    }

    // ========================================================================
    // UI-Rendering: Leak-Sensor
    // ========================================================================
    renderLeakSensor(sensor) {
        const container = document.getElementById('leak-sensors');
        let card = document.getElementById(`sensor-${sensor.id}`);

        if (!card) {
            card = document.createElement('div');
            card.id = `sensor-${sensor.id}`;
            card.className = 'sensor-card fade-in';
            container.appendChild(card);
        }

        const statusClass = sensor.status === 'online' ? 'online' : 'offline';
        const leakClass = sensor.leak ? 'wet' : '';
        const leakText = sensor.leak ? 'Wasserleck erkannt!' : 'Trocken';
        const leakIcon = sensor.leak ? 'fa-exclamation-triangle' : 'fa-check-circle';

        card.innerHTML = `
            <div class="sensor-header">
                <h3>${sensor.name}</h3>
                <span class="sensor-status ${statusClass}">${sensor.status === 'online' ? 'Online' : 'Offline'}</span>
            </div>
            <div class="sensor-content">
                <div class="leak-status ${leakClass}">
                    <i class="fas ${leakIcon}"></i>
                    <span class="reading-value">${leakText}</span>
                </div>
                <div class="sensor-reading">
                    <i class="fas fa-wifi"></i>
                    <div>
                        <span class="reading-value">${sensor.wifiSignal} dBm</span>
                        <div class="reading-label">WiFi Signal</div>
                    </div>
                </div>
            </div>
        `;

        // Skeleton-Klasse entfernen
        card.classList.remove('skeleton');
    }

    // ========================================================================
    // UI-Rendering: Offline-Sensor
    // ========================================================================
    renderOfflineSensor(type, sensor) {
        if (!DashboardConfig.ui.showOfflineSensors) return;

        const container = document.getElementById(`${type}-sensors`);
        let card = document.getElementById(`sensor-${sensor.id}`);

        if (!card) {
            card = document.createElement('div');
            card.id = `sensor-${sensor.id}`;
            card.className = 'sensor-card';
            container.appendChild(card);
        }

        card.innerHTML = `
            <div class="sensor-header">
                <h3>${sensor.name}</h3>
                <span class="sensor-status offline">Offline</span>
            </div>
            <div class="sensor-content">
                <p class="text-tertiary">Sensor nicht erreichbar</p>
            </div>
        `;
    }

    // ========================================================================
    // System-Status aktualisieren
    // ========================================================================
    updateSystemStatus() {
        // Aktive Sensoren zählen
        const activeSensors = [
            ...DashboardState.sensors.humidity.filter(s => s.status === 'online'),
            ...DashboardState.sensors.leak.filter(s => s.status === 'online')
        ].length;

        document.getElementById('active-sensors-count').textContent = activeSensors;

        // Alarme zählen
        const alertCount = DashboardState.alerts.length;
        document.getElementById('alerts-count').textContent = alertCount;

        // Alarm-Icon animieren wenn Alarme vorhanden
        const alertCard = document.getElementById('status-alerts');
        if (alertCount > 0) {
            alertCard.classList.add('has-alerts');
        } else {
            alertCard.classList.remove('has-alerts');
        }

        // System-Health
        const hasOfflineSensors = [
            ...DashboardState.sensors.humidity,
            ...DashboardState.sensors.leak
        ].some(s => s.status === 'offline');

        const healthText = hasOfflineSensors ? 'Warnung' : 'OK';
        document.getElementById('system-health').textContent = healthText;

        // Uptime (längste Uptime aller Sensoren)
        const maxUptime = Math.max(
            ...DashboardState.sensors.humidity.map(s => s.uptime || 0),
            ...DashboardState.sensors.leak.map(s => s.uptime || 0)
        );
        document.getElementById('system-uptime').textContent = this.formatUptime(maxUptime);
    }

    // ========================================================================
    // Charts initialisieren
    // ========================================================================
    initCharts() {
        this.log('Initialisiere Charts...');

        // Temperatur-Chart
        const tempCtx = document.getElementById('temperature-chart').getContext('2d');
        DashboardState.charts.temperature = new Chart(tempCtx, {
            type: 'line',
            data: {
                labels: [],
                datasets: [{
                    label: 'Temperatur (°C)',
                    data: [],
                    borderColor: DashboardConfig.charts.colors.temperature.line,
                    backgroundColor: DashboardConfig.charts.colors.temperature.fill,
                    borderWidth: 2,
                    tension: 0.4,
                    fill: true
                }]
            },
            options: this.getChartOptions('Temperatur (°C)')
        });

        // Luftfeuchtigkeits-Chart
        const humCtx = document.getElementById('humidity-chart').getContext('2d');
        DashboardState.charts.humidity = new Chart(humCtx, {
            type: 'line',
            data: {
                labels: [],
                datasets: [{
                    label: 'Luftfeuchtigkeit (%)',
                    data: [],
                    borderColor: DashboardConfig.charts.colors.humidity.line,
                    backgroundColor: DashboardConfig.charts.colors.humidity.fill,
                    borderWidth: 2,
                    tension: 0.4,
                    fill: true
                }]
            },
            options: this.getChartOptions('Luftfeuchtigkeit (%)')
        });
    }

    getChartOptions(title) {
        return {
            responsive: true,
            maintainAspectRatio: true,
            plugins: {
                legend: {
                    display: false
                },
                title: {
                    display: false
                }
            },
            scales: {
                x: {
                    grid: {
                        color: 'rgba(255, 255, 255, 0.1)'
                    },
                    ticks: {
                        color: '#cbd5e1',
                        maxTicksLimit: 12
                    }
                },
                y: {
                    grid: {
                        color: 'rgba(255, 255, 255, 0.1)'
                    },
                    ticks: {
                        color: '#cbd5e1'
                    }
                }
            },
            animation: DashboardConfig.ui.animateCharts
        };
    }

    // ========================================================================
    // Chart-Daten aktualisieren
    // ========================================================================
    updateChartData() {
        const now = new Date();
        const timeLabel = now.toLocaleTimeString('de-DE', { hour: '2-digit', minute: '2-digit' });

        // Durchschnittswerte aller Sensoren berechnen
        const avgTemp = this.calculateAverage(
            DashboardState.sensors.humidity.map(s => parseFloat(s.temperature) || 0)
        );
        const avgHumidity = this.calculateAverage(
            DashboardState.sensors.humidity.map(s => parseFloat(s.humidity) || 0)
        );

        // Daten zu Historie hinzufügen
        DashboardState.history.timestamps.push(timeLabel);
        DashboardState.history.temperature.push(avgTemp);
        DashboardState.history.humidity.push(avgHumidity);

        // Maximale Datenpunkte begrenzen
        const maxPoints = DashboardConfig.charts.maxDataPoints;
        if (DashboardState.history.timestamps.length > maxPoints) {
            DashboardState.history.timestamps.shift();
            DashboardState.history.temperature.shift();
            DashboardState.history.humidity.shift();
        }

        // Charts aktualisieren
        DashboardState.charts.temperature.data.labels = DashboardState.history.timestamps;
        DashboardState.charts.temperature.data.datasets[0].data = DashboardState.history.temperature;
        DashboardState.charts.temperature.update('none');

        DashboardState.charts.humidity.data.labels = DashboardState.history.timestamps;
        DashboardState.charts.humidity.data.datasets[0].data = DashboardState.history.humidity;
        DashboardState.charts.humidity.update('none');
    }

    // ========================================================================
    // Alarme prüfen
    // ========================================================================
    checkAlerts(sensor) {
        const alerts = [];

        // Temperatur-Alarme
        if (sensor.temperature < DashboardConfig.alerts.temperature.min) {
            alerts.push({
                type: 'temperature',
                sensor: sensor.name,
                message: `Temperatur zu niedrig: ${sensor.temperature}°C`,
                severity: 'warning'
            });
        }
        if (sensor.temperature > DashboardConfig.alerts.temperature.max) {
            alerts.push({
                type: 'temperature',
                sensor: sensor.name,
                message: `Temperatur zu hoch: ${sensor.temperature}°C`,
                severity: 'warning'
            });
        }

        // Luftfeuchtigkeits-Alarme
        if (sensor.humidity < DashboardConfig.alerts.humidity.min) {
            alerts.push({
                type: 'humidity',
                sensor: sensor.name,
                message: `Luftfeuchtigkeit zu niedrig: ${sensor.humidity}%`,
                severity: 'warning'
            });
        }
        if (sensor.humidity > DashboardConfig.alerts.humidity.max) {
            alerts.push({
                type: 'humidity',
                sensor: sensor.name,
                message: `Luftfeuchtigkeit zu hoch: ${sensor.humidity}%`,
                severity: 'warning'
            });
        }

        // Alarme triggern
        alerts.forEach(alert => this.triggerAlert(alert));
    }

    triggerAlert(alert) {
        // Duplikate vermeiden
        const existingAlert = DashboardState.alerts.find(
            a => a.sensor === alert.sensor && a.type === alert.type
        );

        if (existingAlert) return;

        // Alarm zur Liste hinzufügen
        DashboardState.alerts.push({
            ...alert,
            timestamp: new Date()
        });

        // Browser-Benachrichtigung senden
        if (DashboardConfig.alerts.notifications.browser && Notification.permission === 'granted') {
            new Notification('ESPHome Dashboard Alarm', {
                body: alert.message,
                icon: '/assets/icon.png'
            });
        }

        // Sound abspielen (optional)
        if (DashboardConfig.alerts.notifications.sound) {
            this.playAlertSound();
        }

        // Alarm-Historie aktualisieren
        this.updateAlertHistory();
    }

    updateAlertHistory() {
        const container = document.getElementById('alert-history');

        if (DashboardState.alerts.length === 0) {
            container.innerHTML = '<p class="no-alerts"><i class="fas fa-check-circle"></i> Keine Alarme in den letzten 24 Stunden</p>';
            return;
        }

        container.innerHTML = DashboardState.alerts.map(alert => `
            <div class="alert-item ${alert.severity}">
                <h4>${alert.sensor}</h4>
                <p>${alert.message}</p>
                <small>${alert.timestamp.toLocaleString('de-DE')}</small>
            </div>
        `).join('');
    }

    playAlertSound() {
        // Einfacher Beep-Sound mit Web Audio API
        const audioContext = new (window.AudioContext || window.webkitAudioContext)();
        const oscillator = audioContext.createOscillator();
        const gainNode = audioContext.createGain();

        oscillator.connect(gainNode);
        gainNode.connect(audioContext.destination);

        oscillator.frequency.value = 800;
        oscillator.type = 'sine';

        gainNode.gain.setValueAtTime(0.3, audioContext.currentTime);
        gainNode.gain.exponentialRampToValueAtTime(0.01, audioContext.currentTime + 0.5);

        oscillator.start(audioContext.currentTime);
        oscillator.stop(audioContext.currentTime + 0.5);
    }

    // ========================================================================
    // Auto-Update
    // ========================================================================
    startAutoUpdate() {
        this.log('Starte Auto-Update...');

        // Sensoren aktualisieren
        const sensorInterval = setInterval(
            () => this.updateAllSensors(),
            DashboardConfig.updateIntervals.sensors
        );

        DashboardState.updateIntervals.push(sensorInterval);
    }

    stopAutoUpdate() {
        DashboardState.updateIntervals.forEach(interval => clearInterval(interval));
        DashboardState.updateIntervals = [];
    }

    // ========================================================================
    // Hilfsfunktionen
    // ========================================================================
    updateLastUpdateDisplay() {
        const element = document.getElementById('last-update');
        if (element && DashboardState.lastUpdate) {
            element.textContent = `Letzte Aktualisierung: ${DashboardState.lastUpdate.toLocaleTimeString('de-DE')}`;
        }
    }

    calculateAverage(values) {
        if (values.length === 0) return 0;
        const sum = values.reduce((a, b) => a + b, 0);
        return (sum / values.length).toFixed(1);
    }

    formatUptime(seconds) {
        const days = Math.floor(seconds / 86400);
        const hours = Math.floor((seconds % 86400) / 3600);
        const minutes = Math.floor((seconds % 3600) / 60);

        if (days > 0) return `${days}d ${hours}h`;
        if (hours > 0) return `${hours}h ${minutes}m`;
        return `${minutes}m`;
    }

    requestNotificationPermission() {
        if ('Notification' in window && Notification.permission === 'default') {
            Notification.requestPermission();
        }
    }

    log(message) {
        if (DashboardConfig.debug) {
            console.log(`[Dashboard] ${message}`);
        }
    }

    logError(message, error) {
        console.error(`[Dashboard Error] ${message}`, error);
    }
}

// ============================================================================
// Dashboard initialisieren wenn DOM geladen ist
// ============================================================================
document.addEventListener('DOMContentLoaded', () => {
    window.dashboard = new Dashboard();
});
