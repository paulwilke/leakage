/**
 * Main Application Module
 *
 * Coordinates all app functionality:
 * - Routing
 * - State management
 * - UI updates
 * - API communication
 */

import { API } from './api.js';
import { I18n } from './i18n.js';

export class App {
    constructor() {
        this.api = new API();
        this.i18n = new I18n();
        this.currentRoute = '';
        this.isAuthenticated = false;
        this.deviceStatus = null;
        this.updateInterval = null;
    }

    /**
     * Initialize the application
     */
    async init() {
        console.log('🚀 ESPHome UI Framework initializing...');

        try {
            // Load default language
            await this.i18n.load(this.i18n.getCurrentLang());

            // Set up event listeners
            this.setupEventListeners();

            // Initialize router
            this.initRouter();

            // Load initial data
            await this.loadInitialData();

            // Hide loading screen, show app
            this.hideLoadingScreen();

            // Start periodic updates
            this.startPeriodicUpdates();

            console.log('✅ App initialized successfully');
        } catch (error) {
            console.error('Failed to initialize app:', error);
            this.showError('Failed to initialize application');
        }
    }

    /**
     * Load initial data from API
     */
    async loadInitialData() {
        try {
            this.deviceStatus = await this.api.getStatus();
            this.updateDeviceName();
            this.updateFooter();
        } catch (error) {
            console.error('Failed to load initial data:', error);
        }
    }

    /**
     * Set up all event listeners
     */
    setupEventListeners() {
        // Language switcher
        const langButtons = document.querySelectorAll('.lang-btn');
        langButtons.forEach(btn => {
            btn.addEventListener('click', () => this.switchLanguage(btn.dataset.lang));
        });

        // Navigation links
        const navLinks = document.querySelectorAll('.nav-link');
        navLinks.forEach(link => {
            link.addEventListener('click', (e) => {
                e.preventDefault();
                const href = link.getAttribute('href');
                window.location.hash = href;
            });
        });

        // Hash change (routing)
        window.addEventListener('hashchange', () => this.handleRoute());

        // Online/offline detection
        window.addEventListener('online', () => this.handleConnectionChange(true));
        window.addEventListener('offline', () => this.handleConnectionChange(false));
    }

    /**
     * Initialize router and handle initial route
     */
    initRouter() {
        // Set default route if none
        if (!window.location.hash) {
            window.location.hash = '#/dashboard';
        } else {
            this.handleRoute();
        }
    }

    /**
     * Handle route changes
     */
    async handleRoute() {
        const hash = window.location.hash.slice(1); // Remove #
        const route = hash || '/dashboard';

        console.log(`Navigating to: ${route}`);

        // Update active nav link
        this.updateActiveNavLink(route);

        // Route to correct view
        switch (route) {
            case '/dashboard':
                await this.showDashboard();
                break;
            case '/settings':
                await this.showSettings();
                break;
            case '/about':
                this.showAbout();
                break;
            default:
                this.show404();
        }

        this.currentRoute = route;
    }

    /**
     * Update active navigation link
     */
    updateActiveNavLink(route) {
        const navLinks = document.querySelectorAll('.nav-link');
        navLinks.forEach(link => {
            const href = link.getAttribute('href').slice(1); // Remove #
            link.classList.toggle('active', href === route);
        });
    }

    /**
     * Show dashboard view
     */
    async showDashboard() {
        const content = document.getElementById('content');

        try {
            // Fetch sensors and actuators
            const [sensors, actuators] = await Promise.all([
                this.api.getSensors(),
                this.api.getActuators()
            ]);

            // Build dashboard HTML
            let html = '<div class="dashboard">';
            html += '<h2 class="page-title" data-i18n="pages.dashboard">Dashboard</h2>';

            // Sensors
            if (sensors && sensors.length > 0) {
                html += '<div class="sensor-grid">';
                sensors.forEach(sensor => {
                    html += this.renderSensorCard(sensor);
                });
                html += '</div>';
            } else {
                html += '<p class="empty-state" data-i18n="messages.no_sensors">No sensors configured</p>';
            }

            // Actuators
            if (actuators && actuators.length > 0) {
                html += '<h3 data-i18n="sections.controls">Controls</h3>';
                html += '<div class="actuator-grid">';
                actuators.forEach(actuator => {
                    html += this.renderActuatorCard(actuator);
                });
                html += '</div>';
            }

            html += '</div>';

            content.innerHTML = html;

            // Translate new content
            this.i18n.translatePage();

        } catch (error) {
            console.error('Failed to load dashboard:', error);
            content.innerHTML = '<div class="error">Failed to load dashboard data</div>';
        }
    }

    /**
     * Render a sensor card
     */
    renderSensorCard(sensor) {
        const value = sensor.value !== null ? sensor.value.toFixed(1) : '--';
        const unit = sensor.unit || '';

        return `
            <div class="sensor-card" data-sensor-id="${sensor.id}">
                <div class="sensor-value">${value}</div>
                <div class="sensor-unit">${unit}</div>
                <div class="sensor-name">${sensor.name}</div>
            </div>
        `;
    }

    /**
     * Render an actuator card
     */
    renderActuatorCard(actuator) {
        // Simplified for now
        return `
            <div class="card" data-actuator-id="${actuator.id}">
                <div class="card-title">${actuator.name}</div>
                <div>State: ${JSON.stringify(actuator.state)}</div>
            </div>
        `;
    }

    /**
     * Show settings view
     */
    async showSettings() {
        const content = document.getElementById('content');

        content.innerHTML = `
            <div class="settings">
                <h2 class="page-title" data-i18n="pages.settings">Settings</h2>
                <p data-i18n="messages.login_required">Please login to access settings</p>
            </div>
        `;

        this.i18n.translatePage();
    }

    /**
     * Show about view
     */
    showAbout() {
        const content = document.getElementById('content');

        content.innerHTML = `
            <div class="about">
                <h2 class="page-title" data-i18n="pages.about">About</h2>
                <div class="card">
                    <h3>ESPHome UI Framework</h3>
                    <p>Version: 1.0.0-alpha</p>
                    <p>A professional UI framework for ESP32/ESPHome devices</p>
                </div>
            </div>
        `;

        this.i18n.translatePage();
    }

    /**
     * Show 404 page
     */
    show404() {
        const content = document.getElementById('content');
        content.innerHTML = '<div class="error"><h2>404 - Page Not Found</h2></div>';
    }

    /**
     * Switch language
     */
    async switchLanguage(lang) {
        console.log(`Switching to language: ${lang}`);

        await this.i18n.load(lang);
        this.i18n.translatePage();

        // Update active button
        const langButtons = document.querySelectorAll('.lang-btn');
        langButtons.forEach(btn => {
            btn.classList.toggle('active', btn.dataset.lang === lang);
        });

        // Reload current view to translate dynamic content
        this.handleRoute();
    }

    /**
     * Update device name in header
     */
    updateDeviceName() {
        if (this.deviceStatus && this.deviceStatus.device_name) {
            const el = document.getElementById('device-name');
            if (el) {
                el.textContent = this.deviceStatus.device_name;
            }
        }
    }

    /**
     * Update footer status
     */
    updateFooter() {
        if (!this.deviceStatus) return;

        // Uptime
        const uptimeEl = document.getElementById('uptime-display');
        if (uptimeEl) {
            const hours = Math.floor(this.deviceStatus.uptime / 3600);
            const minutes = Math.floor((this.deviceStatus.uptime % 3600) / 60);
            uptimeEl.textContent = `Uptime: ${hours}h ${minutes}m`;
        }

        // Memory
        const memoryEl = document.getElementById('memory-display');
        if (memoryEl) {
            const kb = Math.floor(this.deviceStatus.free_heap / 1024);
            memoryEl.textContent = `Memory: ${kb}KB`;
        }
    }

    /**
     * Handle connection status change
     */
    handleConnectionChange(isOnline) {
        const statusEl = document.getElementById('connection-status');
        if (statusEl) {
            if (isOnline) {
                statusEl.classList.remove('offline');
                statusEl.classList.add('online');
                statusEl.querySelector('span:last-child').textContent = 'Online';
            } else {
                statusEl.classList.remove('online');
                statusEl.classList.add('offline');
                statusEl.querySelector('span:last-child').textContent = 'Offline';
            }
        }
    }

    /**
     * Start periodic updates
     */
    startPeriodicUpdates() {
        // Update status every 5 seconds
        this.updateInterval = setInterval(async () => {
            try {
                this.deviceStatus = await this.api.getStatus();
                this.updateFooter();

                // If on dashboard, update sensor values
                if (this.currentRoute === '/dashboard') {
                    await this.updateDashboardValues();
                }
            } catch (error) {
                console.error('Failed to update:', error);
                this.handleConnectionChange(false);
            }
        }, 5000);
    }

    /**
     * Update dashboard sensor values without full reload
     */
    async updateDashboardValues() {
        try {
            const sensors = await this.api.getSensors();

            sensors.forEach(sensor => {
                const card = document.querySelector(`[data-sensor-id="${sensor.id}"]`);
                if (card) {
                    const valueEl = card.querySelector('.sensor-value');
                    if (valueEl && sensor.value !== null) {
                        valueEl.textContent = sensor.value.toFixed(1);
                    }
                }
            });
        } catch (error) {
            console.error('Failed to update sensor values:', error);
        }
    }

    /**
     * Hide loading screen and show app
     */
    hideLoadingScreen() {
        const loadingScreen = document.getElementById('loading-screen');
        const app = document.getElementById('app');

        if (loadingScreen && app) {
            loadingScreen.classList.add('hidden');
            app.classList.remove('hidden');
        }
    }

    /**
     * Show error toast
     */
    showError(message) {
        console.error(message);
        // TODO: Implement toast notification system
    }
}
