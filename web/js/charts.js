/**
 * Chart Module - Lazy Loading uPlot
 *
 * Provides chart functionality using uPlot library.
 * Charts are loaded on-demand to minimize initial page load.
 *
 * uPlot is a fast, memory-efficient time-series chart library:
 * - ~40KB minified
 * - Canvas-based rendering
 * - Highly performant
 * - Responsive
 */

export class Charts {
    constructor() {
        this.uplotLoaded = false;
        this.loadingPromise = null;
        this.charts = new Map();  // Store chart instances
    }

    /**
     * Lazy load uPlot library
     *
     * @return Promise that resolves when uPlot is loaded
     */
    async loadUPlot() {
        // Already loaded?
        if (this.uplotLoaded && window.uPlot) {
            return true;
        }

        // Already loading?
        if (this.loadingPromise) {
            return this.loadingPromise;
        }

        console.log('📊 Loading uPlot library...');

        this.loadingPromise = new Promise((resolve, reject) => {
            // Load CSS
            const cssLink = document.createElement('link');
            cssLink.rel = 'stylesheet';
            cssLink.href = '/vendor/uplot.min.css';
            document.head.appendChild(cssLink);

            // Load JavaScript
            const script = document.createElement('script');
            script.src = '/vendor/uplot.min.js';
            script.async = true;

            script.onload = () => {
                if (window.uPlot) {
                    this.uplotLoaded = true;
                    console.log('✅ uPlot loaded successfully');
                    resolve(true);
                } else {
                    reject(new Error('uPlot library failed to initialize'));
                }
            };

            script.onerror = () => {
                reject(new Error('Failed to load uPlot library'));
            };

            document.head.appendChild(script);
        });

        return this.loadingPromise;
    }

    /**
     * Create a line chart for sensor data
     *
     * @param container DOM element or selector
     * @param data Chart data {labels: [], datasets: [{label, data, color}]}
     * @param options Chart options
     */
    async createLineChart(container, data, options = {}) {
        // Ensure uPlot is loaded
        await this.loadUPlot();

        const el = typeof container === 'string'
            ? document.querySelector(container)
            : container;

        if (!el) {
            throw new Error('Chart container not found');
        }

        // Prepare data for uPlot format
        // uPlot expects: [timestamps, ...value arrays]
        const uplotData = this.convertToUPlotFormat(data);

        // Configure chart
        const config = {
            width: options.width || el.clientWidth || 600,
            height: options.height || 300,
            title: options.title || '',
            series: [
                {
                    label: 'Time',
                },
                ...data.datasets.map(ds => ({
                    label: ds.label || 'Value',
                    stroke: ds.color || '#3b82f6',
                    width: 2,
                    points: { show: false }
                }))
            ],
            axes: [
                {
                    // X-axis (time)
                    space: 50,
                    incrs: [
                        1,           // 1 second
                        60,          // 1 minute
                        3600,        // 1 hour
                        86400        // 1 day
                    ],
                    values: (self, ticks) => ticks.map(t => {
                        const date = new Date(t * 1000);
                        return date.toLocaleTimeString();
                    })
                },
                {
                    // Y-axis (values)
                    space: 40,
                    values: (self, ticks) => ticks.map(t => {
                        const unit = options.unit || '';
                        return t.toFixed(1) + (unit ? ' ' + unit : '');
                    })
                }
            ],
            scales: {
                x: {
                    time: true
                }
            },
            cursor: {
                drag: {
                    x: true,
                    y: false
                }
            },
            ...options.uplotOptions
        };

        // Create chart
        const chart = new window.uPlot(config, uplotData, el);

        // Store reference
        const chartId = `chart_${Date.now()}`;
        this.charts.set(chartId, chart);

        // Make responsive
        this.makeResponsive(chart, el);

        return chartId;
    }

    /**
     * Convert data to uPlot format
     *
     * Input: {labels: [timestamps], datasets: [{data: [values]}]}
     * Output: [timestamps, ...value arrays]
     */
    convertToUPlotFormat(data) {
        const result = [data.labels];  // First array is timestamps

        data.datasets.forEach(dataset => {
            result.push(dataset.data);
        });

        return result;
    }

    /**
     * Make chart responsive
     */
    makeResponsive(chart, container) {
        let resizeTimeout;

        const observer = new ResizeObserver(() => {
            clearTimeout(resizeTimeout);
            resizeTimeout = setTimeout(() => {
                const width = container.clientWidth;
                if (width > 0) {
                    chart.setSize({ width, height: chart.height });
                }
            }, 100);
        });

        observer.observe(container);
    }

    /**
     * Update chart data
     *
     * @param chartId Chart identifier returned from createLineChart
     * @param data New data
     */
    updateChart(chartId, data) {
        const chart = this.charts.get(chartId);
        if (!chart) {
            console.error(`Chart ${chartId} not found`);
            return false;
        }

        const uplotData = this.convertToUPlotFormat(data);
        chart.setData(uplotData);
        return true;
    }

    /**
     * Destroy a chart
     *
     * @param chartId Chart identifier
     */
    destroyChart(chartId) {
        const chart = this.charts.get(chartId);
        if (chart) {
            chart.destroy();
            this.charts.delete(chartId);
        }
    }

    /**
     * Destroy all charts
     */
    destroyAll() {
        this.charts.forEach(chart => chart.destroy());
        this.charts.clear();
    }

    /**
     * Fetch historical data for a sensor
     *
     * @param sensorId Sensor ID
     * @param duration Duration in seconds (default: 1 hour)
     * @return Promise with {labels: [], values: []}
     */
    async fetchSensorHistory(sensorId, duration = 3600) {
        // TODO: Implement historical data API endpoint
        // For now, generate mock data
        console.warn('Historical data API not yet implemented, using mock data');

        const now = Date.now() / 1000;
        const interval = 60;  // 1 minute intervals
        const points = Math.floor(duration / interval);

        const labels = [];
        const values = [];

        for (let i = points; i >= 0; i--) {
            labels.push(now - (i * interval));
            // Mock data: sine wave with some noise
            values.push(20 + Math.sin(i * 0.1) * 5 + (Math.random() - 0.5) * 2);
        }

        return { labels, values };
    }

    /**
     * Show chart modal for a sensor
     *
     * @param sensor Sensor object {id, name, unit}
     */
    async showSensorChart(sensor) {
        console.log(`📊 Showing chart for ${sensor.name}`);

        try {
            // Create modal
            const modal = this.createChartModal(sensor);
            document.body.appendChild(modal);

            // Fetch historical data
            const history = await this.fetchSensorHistory(sensor.id);

            // Create chart
            const chartContainer = modal.querySelector('.chart-container');
            await this.createLineChart(chartContainer, {
                labels: history.labels,
                datasets: [{
                    label: sensor.name,
                    data: history.values,
                    color: '#3b82f6'
                }]
            }, {
                title: sensor.name,
                unit: sensor.unit,
                height: 400
            });

        } catch (error) {
            console.error('Failed to create chart:', error);
            alert('Failed to load chart: ' + error.message);
        }
    }

    /**
     * Create chart modal HTML
     */
    createChartModal(sensor) {
        const modal = document.createElement('div');
        modal.className = 'chart-modal';
        modal.innerHTML = `
            <div class="chart-modal-backdrop"></div>
            <div class="chart-modal-content">
                <div class="chart-modal-header">
                    <h2>${sensor.name}</h2>
                    <button class="chart-modal-close">&times;</button>
                </div>
                <div class="chart-container"></div>
                <div class="chart-modal-footer">
                    <button class="btn-secondary" data-duration="3600">1 Hour</button>
                    <button class="btn-secondary" data-duration="86400">24 Hours</button>
                    <button class="btn-secondary" data-duration="604800">7 Days</button>
                </div>
            </div>
        `;

        // Close button
        modal.querySelector('.chart-modal-close').onclick = () => {
            modal.remove();
        };

        // Backdrop click
        modal.querySelector('.chart-modal-backdrop').onclick = () => {
            modal.remove();
        };

        // Duration buttons (TODO: implement)
        modal.querySelectorAll('[data-duration]').forEach(btn => {
            btn.onclick = () => {
                console.log(`TODO: Load ${btn.dataset.duration}s of data`);
            };
        });

        return modal;
    }
}

// Add chart modal styles
const chartStyles = document.createElement('style');
chartStyles.textContent = `
.chart-modal {
    position: fixed;
    top: 0;
    left: 0;
    right: 0;
    bottom: 0;
    z-index: 1000;
    display: flex;
    align-items: center;
    justify-content: center;
}

.chart-modal-backdrop {
    position: absolute;
    top: 0;
    left: 0;
    right: 0;
    bottom: 0;
    background: rgba(0, 0, 0, 0.7);
}

.chart-modal-content {
    position: relative;
    background: var(--bg-secondary);
    border-radius: 0.75rem;
    padding: 1.5rem;
    max-width: 90vw;
    max-height: 90vh;
    width: 800px;
    box-shadow: var(--shadow-lg);
    display: flex;
    flex-direction: column;
}

.chart-modal-header {
    display: flex;
    justify-content: space-between;
    align-items: center;
    margin-bottom: 1rem;
    padding-bottom: 1rem;
    border-bottom: 1px solid var(--border-primary);
}

.chart-modal-header h2 {
    margin: 0;
    color: var(--text-primary);
}

.chart-modal-close {
    background: transparent;
    border: none;
    font-size: 2rem;
    color: var(--text-secondary);
    cursor: pointer;
    padding: 0;
    width: 32px;
    height: 32px;
    display: flex;
    align-items: center;
    justify-content: center;
}

.chart-modal-close:hover {
    color: var(--text-primary);
}

.chart-container {
    flex: 1;
    min-height: 300px;
    margin: 1rem 0;
}

.chart-modal-footer {
    display: flex;
    gap: 0.5rem;
    justify-content: center;
    padding-top: 1rem;
    border-top: 1px solid var(--border-primary);
}

@media (max-width: 768px) {
    .chart-modal-content {
        width: 95vw;
        padding: 1rem;
    }

    .chart-container {
        min-height: 200px;
    }
}
`;
document.head.appendChild(chartStyles);
