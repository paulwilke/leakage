# ADR-003: Frontend Framework Choice

## Status
Accepted

## Context

The framework needs a frontend that:

- Runs on resource-constrained devices (served from ESP32)
- Loads quickly on slow connections
- Works without build tools or NPM
- Is maintainable by developers and LLMs
- Supports modern UX patterns
- Can be embedded in flash memory

### Options Considered

#### Option A: React/Vue/Svelte

**Pros:**
- Modern component model
- Large ecosystem
- Developer familiarity
- Good tooling

**Cons:**
- Requires build step
- Large bundle size (even minified)
- NPM dependencies
- Complexity for simple UIs
- Not embeddable easily

#### Option B: Alpine.js or Petite-Vue

**Pros:**
- Lightweight (~15KB)
- No build step
- Reactive data binding
- Simple syntax

**Cons:**
- Still external dependency
- Learning curve
- Overkill for this use case
- Hard to embed

#### Option C: Vanilla JavaScript (ES6+)

**Pros:**
- Zero dependencies
- No build step required
- Maximum control
- Smallest possible size
- Easy to embed
- Transparent and understandable
- LLM-friendly (no framework magic)
- Works in all modern browsers

**Cons:**
- More verbose
- No reactivity framework
- Manual DOM manipulation
- Need to write own patterns

#### Option D: jQuery

**Pros:**
- Simple API
- Wide browser support

**Cons:**
- Outdated pattern
- Adds 30KB+ overhead
- Modern browsers don't need it
- Bad practice in 2025

## Decision

We will use **Vanilla JavaScript (ES6+)** with custom component patterns.

## Rationale

### 1. Size Requirements

Target: <50KB total (HTML+CSS+JS, gzipped)

```
index.html:     ~8KB  (structure + inline critical CSS)
app.js:         ~6KB  (main app logic)
api.js:         ~2KB  (API client)
components.js:  ~5KB  (component system)
i18n.js:        ~3KB  (internationalization)
charts.js:      ~2KB  (lazy loader)
main.css:       ~4KB  (styles)
themes.css:     ~2KB  (theming)
de.json:        ~3KB  (German translations)
en.json:        ~3KB  (English translations)
------------------------------
Total:         ~38KB uncompressed
Gzipped:       ~12KB (well under 50KB target)
```

uPlot.js (~40KB) loaded only when needed (lazy).

### 2. Maintainability Principles

**Transparent Code**: Every line is understandable

```javascript
// Bad (framework magic):
<Component :data="items" @click="handler" />

// Good (transparent):
function createComponent(data) {
    const el = document.createElement('div');
    el.className = 'component';
    el.onclick = handler;
    data.forEach(item => el.appendChild(createItem(item)));
    return el;
}
```

**LLM-Friendly**: No hidden abstractions

```javascript
// Clear data flow
API.get('/api/sensors')
    .then(data => updateUI(data))
    .catch(error => showError(error));

// Clear component pattern
class SensorCard {
    constructor(sensor) {
        this.sensor = sensor;
        this.element = this.render();
    }

    render() {
        // Explicit DOM creation
    }

    update(newData) {
        // Explicit updates
    }
}
```

### 3. Modern Browser Features

We can use ES6+ because:

- ESP32 devices are new hardware (2016+)
- Users access via modern browsers
- No IE11 support needed

Features we'll use:
- `async`/`await` for API calls
- Template literals for HTML
- Classes for components
- Modules (ES6 imports)
- `fetch` API
- `localStorage` for settings
- CSS Grid & Flexbox

### 4. No Build Step Benefits

**Developer Experience:**
```bash
# Traditional React:
npm install
npm run build
wait...
upload dist/

# Our approach:
edit web/js/app.js
save
done!
```

**Debugging:**
- View source = actual source
- No source maps needed
- Console errors show real line numbers
- Easy to inspect in browser

**LLM Extension:**
- LLM can read actual code
- No transpilation to understand
- Direct file editing
- No build system to learn

## Component Pattern

We'll implement a simple component system:

```javascript
// components.js
class Component {
    constructor() {
        this.element = null;
    }

    render() {
        // Returns HTMLElement
    }

    mount(parent) {
        parent.appendChild(this.element);
    }

    unmount() {
        this.element?.remove();
    }
}

class SensorCard extends Component {
    constructor(sensor) {
        super();
        this.sensor = sensor;
        this.element = this.render();
    }

    render() {
        const card = document.createElement('div');
        card.className = 'sensor-card';
        card.innerHTML = `
            <div class="sensor-value">${this.sensor.value}</div>
            <div class="sensor-name">${this.sensor.name}</div>
        `;
        return card;
    }

    update(newData) {
        this.sensor = newData;
        const valueEl = this.element.querySelector('.sensor-value');
        valueEl.textContent = newData.value;
    }
}
```

## File Structure

```
web/
├── index.html          # SPA shell (<8KB)
├── css/
│   ├── main.css        # Core styles
│   └── themes.css      # Theme variables
├── js/
│   ├── app.js          # Main app
│   ├── api.js          # API client
│   ├── components.js   # Component system
│   ├── i18n.js         # Translations
│   └── charts.js       # Lazy chart loader
├── lang/
│   ├── de.json         # German
│   └── en.json         # English
└── vendor/
    └── uplot.min.js    # Only loaded on demand
```

## Consequences

### Positive
- Minimal bundle size
- No build complexity
- Easy to debug
- LLM-friendly code
- Fast loading
- Direct ESP32 embedding
- Clear data flow
- Maintainable

### Negative
- More verbose than frameworks
- Manual DOM updates
- No reactivity out of box
- Need to implement patterns ourselves

### Mitigation
- Create helper functions for common tasks
- Document patterns clearly
- Provide component base classes
- Include examples for common scenarios

## Code Quality Standards

```javascript
// ✅ Good: Clear naming
async function fetchSensorData(sensorId) {
    const response = await fetch(`/api/sensors/${sensorId}`);
    return response.json();
}

// ❌ Bad: Unclear
async function getData(id) {
    return (await fetch(`/api/sensors/${id}`)).json();
}

// ✅ Good: Error handling
try {
    const data = await api.getSensors();
    renderSensors(data);
} catch (error) {
    console.error('Failed to load sensors:', error);
    showErrorMessage(i18n.t('errors.network'));
}

// ❌ Bad: No error handling
const data = await api.getSensors();
renderSensors(data);
```

## Browser Support

Minimum versions:
- Chrome 60+ (2017)
- Firefox 60+ (2018)
- Safari 11+ (2017)
- Edge 79+ (2020)

These cover >95% of users and all modern mobile devices.

## References

- MDN Web Docs: https://developer.mozilla.org/
- You Might Not Need jQuery: https://youmightnotneedjquery.com/
- Vanilla JS Toolkit: https://vanillajstoolkit.com/

---

**Date:** 2025-11-20
**Author:** ESPHome UI Framework Team
