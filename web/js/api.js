/**
 * API Client for ESPHome UI Framework
 *
 * Provides a clean interface for all API endpoints.
 * Handles errors, timeouts, and response parsing.
 */

export class API {
    constructor(baseUrl = '') {
        this.baseUrl = baseUrl;
        this.timeout = 10000; // 10 seconds
    }

    /**
     * Fetch with timeout
     */
    async fetchWithTimeout(url, options = {}) {
        const controller = new AbortController();
        const timeoutId = setTimeout(() => controller.abort(), this.timeout);

        try {
            const response = await fetch(url, {
                ...options,
                signal: controller.signal
            });
            clearTimeout(timeoutId);
            return response;
        } catch (error) {
            clearTimeout(timeoutId);
            if (error.name === 'AbortError') {
                throw new Error('Request timeout');
            }
            throw error;
        }
    }

    /**
     * GET request helper
     */
    async get(endpoint) {
        const response = await this.fetchWithTimeout(this.baseUrl + endpoint);

        if (!response.ok) {
            throw new Error(`HTTP ${response.status}: ${response.statusText}`);
        }

        return response.json();
    }

    /**
     * POST request helper
     */
    async post(endpoint, data = {}) {
        const response = await this.fetchWithTimeout(this.baseUrl + endpoint, {
            method: 'POST',
            headers: {
                'Content-Type': 'application/x-www-form-urlencoded'
            },
            body: new URLSearchParams(data)
        });

        if (!response.ok) {
            throw new Error(`HTTP ${response.status}: ${response.statusText}`);
        }

        return response.json();
    }

    // ========================================================================
    // API Endpoints
    // ========================================================================

    /**
     * Get device status
     */
    async getStatus() {
        return this.get('/api/status');
    }

    /**
     * Get all components
     */
    async getComponents() {
        return this.get('/api/components');
    }

    /**
     * Get all sensors
     */
    async getSensors() {
        return this.get('/api/sensors');
    }

    /**
     * Get all actuators
     */
    async getActuators() {
        return this.get('/api/actuators');
    }

    /**
     * Login with credentials
     */
    async login(username, password) {
        return this.post('/api/auth/login', { username, password });
    }

    /**
     * Logout
     */
    async logout() {
        return this.post('/api/auth/logout');
    }

    /**
     * Get deep sleep status
     */
    async getSleepStatus() {
        return this.get('/api/sleep');
    }

    /**
     * Control an actuator
     */
    async controlActuator(id, state) {
        return this.post(`/api/actuators/${id}`, { state: JSON.stringify(state) });
    }
}
