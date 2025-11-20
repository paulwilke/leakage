# ADR-004: Internationalization Approach

## Status
Accepted

## Context

The framework must support multiple languages for:
- UI labels and buttons
- Error messages
- Component names
- Status messages
- Admin interface

Requirements:
- Easy to add new languages
- Minimal runtime overhead
- Works without build tools
- Supports dynamic language switching
- LLM can easily extend

### Options Considered

#### Option A: JavaScript i18n libraries (i18next, react-intl)

**Pros:**
- Feature-rich
- Pluralization support
- Date/time formatting
- Number formatting

**Cons:**
- Large bundle size (50KB+)
- NPM dependency
- Build step recommended
- Overkill for IoT UI

#### Option B: Gettext-style (.po files)

**Pros:**
- Industry standard
- Tooling available
- Translator-friendly

**Cons:**
- Requires parsing
- Binary compilation
- Not JSON-friendly
- Complex for small projects

#### Option C: Simple JSON files

**Pros:**
- Native JavaScript format
- Easy to edit
- Small size
- No dependencies
- LLM-friendly
- Can be fetched async
- Clear structure

**Cons:**
- Manual implementation
- No advanced features
- Need to write own logic

## Decision

We will use **JSON-based translation files** with a simple custom implementation.

## Rationale

### 1. Simplicity & Size

```json
{
  "meta": {
    "language": "de",
    "name": "Deutsch",
    "direction": "ltr"
  },
  "ui": {
    "dashboard": "Dashboard",
    "settings": "Einstellungen",
    "login": "Anmelden",
    "logout": "Abmelden"
  },
  "components": {
    "temperature": "Temperatur",
    "humidity": "Luftfeuchtigkeit",
    "pressure": "Luftdruck"
  },
  "errors": {
    "network": "Netzwerkfehler",
    "timeout": "Zeitüberschreitung",
    "unauthorized": "Nicht autorisiert"
  }
}
```

Total size per language: ~3KB
Easy to understand and edit.

### 2. Implementation

```javascript
// i18n.js (~3KB)
class I18n {
    constructor() {
        this.lang = localStorage.getItem('lang') || 'en';
        this.translations = {};
        this.fallback = 'en';
    }

    async load(lang) {
        try {
            const response = await fetch(`/lang/${lang}.json`);
            this.translations = await response.json();
            this.lang = lang;
            localStorage.setItem('lang', lang);
            document.documentElement.lang = lang;
            return true;
        } catch (error) {
            console.error(`Failed to load language: ${lang}`, error);
            if (lang !== this.fallback) {
                return this.load(this.fallback);
            }
            return false;
        }
    }

    t(key, params = {}) {
        // Support nested keys: "ui.dashboard"
        const keys = key.split('.');
        let value = this.translations;

        for (const k of keys) {
            if (value && typeof value === 'object') {
                value = value[k];
            } else {
                // Key not found, return key itself
                console.warn(`Translation missing: ${key}`);
                return key;
            }
        }

        // Replace parameters: "Hello {name}" with params = {name: "World"}
        if (typeof value === 'string' && Object.keys(params).length > 0) {
            return value.replace(/\{(\w+)\}/g, (match, param) => {
                return params[param] !== undefined ? params[param] : match;
            });
        }

        return value || key;
    }

    getCurrentLang() {
        return this.lang;
    }

    getAvailableLanguages() {
        return ['en', 'de']; // Can be fetched from server
    }
}

// Global instance
const i18n = new I18n();
```

### 3. Usage Examples

```javascript
// Simple translation
document.title = i18n.t('ui.dashboard');

// With parameters
const message = i18n.t('status.connected_to', { ssid: 'MyWiFi' });
// "status.connected_to": "Verbunden mit {ssid}"
// Result: "Verbunden mit MyWiFi"

// Dynamic language switch
async function switchLanguage(lang) {
    await i18n.load(lang);
    updateAllUI(); // Re-render with new translations
}

// Component usage
class LoginForm extends Component {
    render() {
        return `
            <form>
                <h1>${i18n.t('ui.login')}</h1>
                <input placeholder="${i18n.t('ui.username')}" />
                <input type="password" placeholder="${i18n.t('ui.password')}" />
                <button>${i18n.t('ui.submit')}</button>
            </form>
        `;
    }
}
```

### 4. Language Switcher

```javascript
// Language selector component
class LanguageSwitcher extends Component {
    render() {
        const langs = [
            { code: 'en', name: 'English', flag: '🇬🇧' },
            { code: 'de', name: 'Deutsch', flag: '🇩🇪' }
        ];

        const html = langs.map(lang => `
            <button
                class="lang-btn ${lang.code === i18n.getCurrentLang() ? 'active' : ''}"
                onclick="switchLanguage('${lang.code}')">
                ${lang.flag} ${lang.name}
            </button>
        `).join('');

        return html;
    }
}
```

## File Structure

```
web/
└── lang/
    ├── en.json          # English (default/fallback)
    ├── de.json          # German
    ├── fr.json          # French (future)
    ├── es.json          # Spanish (future)
    └── README.md        # Translation guide
```

## JSON Schema

All translation files follow this structure:

```json
{
  "$schema": "translation-schema.json",
  "meta": {
    "language": "string (ISO 639-1 code)",
    "name": "string (Native language name)",
    "direction": "ltr | rtl",
    "contributors": ["array of names"]
  },
  "ui": {
    "common buttons, labels, navigation": "..."
  },
  "components": {
    "sensor types, component names": "..."
  },
  "status": {
    "online, offline, connecting, etc": "..."
  },
  "errors": {
    "error messages": "..."
  },
  "settings": {
    "settings page strings": "..."
  },
  "time": {
    "relative times: minutes_ago, hours_ago": "..."
  }
}
```

## Initial Languages

### Phase 1 (Required):
- **English (en)**: Default, fallback
- **German (de)**: Common in EU IoT market

### Phase 2 (Community):
- French (fr)
- Spanish (es)
- Italian (it)
- Dutch (nl)
- Polish (pl)
- Russian (ru)
- Japanese (ja)
- Chinese (zh)

## Adding New Languages

For LLMs and contributors:

1. Copy `en.json` to `{lang}.json`
2. Update `meta` section
3. Translate all strings
4. Test with `?lang={lang}` query parameter
5. Submit PR

Documentation will include:
- Translation guide
- JSON schema
- Validator script
- Missing key detector

## Advanced Features (Future)

Phase 2 could add:

```javascript
// Pluralization
i18n.t('items_count', { count: 5 })
// "items_count": {"one": "1 item", "other": "{count} items"}

// Number formatting
i18n.formatNumber(1234.56)
// en: "1,234.56"
// de: "1.234,56"

// Date formatting
i18n.formatDate(date, 'short')
// en: "12/25/2025"
// de: "25.12.2025"

// Relative time
i18n.formatRelative(timestamp)
// en: "2 hours ago"
// de: "vor 2 Stunden"
```

These are simple to add later without breaking changes.

## Consequences

### Positive
- Tiny implementation (~3KB)
- Zero dependencies
- Easy to extend
- LLM-friendly
- Community can contribute
- Async loading (no blocking)
- Browser caching
- Clear structure

### Negative
- No advanced features out of box
- Manual implementation
- No tooling for translators
- No compile-time checks

### Mitigation
- Provide JSON schema for validation
- Create validator script
- Clear documentation
- Examples for all patterns
- Community guidelines

## Performance

```javascript
// Load time
fetch('/lang/de.json') // ~3KB gzipped = ~1KB
// < 100ms on slow 3G

// Lookup time
i18n.t('ui.dashboard') // O(1) hash lookup
// < 1ms

// Memory
3KB per language * 2 languages = 6KB
// Negligible on ESP32
```

## References

- BCP 47 Language Tags: https://tools.ietf.org/html/bcp47
- ICU Message Format: https://unicode-org.github.io/icu/
- i18n Best Practices: https://developers.google.com/international/

---

**Date:** 2025-11-20
**Author:** ESPHome UI Framework Team
