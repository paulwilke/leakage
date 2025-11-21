/**
 * Internationalization Module
 *
 * Simple, efficient i18n system using JSON files.
 * No dependencies, <3KB footprint.
 */

export class I18n {
    constructor() {
        this.currentLang = localStorage.getItem('lang') || 'en';
        this.fallbackLang = 'en';
        this.translations = {};
        this.loadedLanguages = new Set();
    }

    /**
     * Load a language file
     */
    async load(lang) {
        // Already loaded?
        if (this.loadedLanguages.has(lang)) {
            this.currentLang = lang;
            this.updateDocumentLang();
            return true;
        }

        try {
            const response = await fetch(`/lang/${lang}.json`);
            if (!response.ok) {
                throw new Error(`Failed to load ${lang}.json`);
            }

            const data = await response.json();
            this.translations[lang] = data;
            this.loadedLanguages.add(lang);
            this.currentLang = lang;

            localStorage.setItem('lang', lang);
            this.updateDocumentLang();

            console.log(`✅ Loaded language: ${lang}`);
            return true;
        } catch (error) {
            console.error(`Failed to load language ${lang}:`, error);

            // Try fallback if not already loading fallback
            if (lang !== this.fallbackLang) {
                console.log(`Falling back to ${this.fallbackLang}`);
                return this.load(this.fallbackLang);
            }

            return false;
        }
    }

    /**
     * Translate a key
     *
     * Supports nested keys: "ui.dashboard"
     * Supports parameters: "Hello {name}" with params = {name: "World"}
     */
    t(key, params = {}) {
        // Get translation
        let value = this.getValue(key, this.currentLang);

        // Fallback to default language
        if (value === null && this.currentLang !== this.fallbackLang) {
            value = this.getValue(key, this.fallbackLang);
        }

        // Still not found? Return key itself
        if (value === null) {
            console.warn(`Translation missing: ${key}`);
            return key;
        }

        // Replace parameters
        if (typeof value === 'string' && Object.keys(params).length > 0) {
            return value.replace(/\{(\w+)\}/g, (match, param) => {
                return params[param] !== undefined ? params[param] : match;
            });
        }

        return value;
    }

    /**
     * Get nested value from translation object
     */
    getValue(key, lang) {
        if (!this.translations[lang]) {
            return null;
        }

        const keys = key.split('.');
        let value = this.translations[lang];

        for (const k of keys) {
            if (value && typeof value === 'object' && k in value) {
                value = value[k];
            } else {
                return null;
            }
        }

        return value;
    }

    /**
     * Get current language code
     */
    getCurrentLang() {
        return this.currentLang;
    }

    /**
     * Get available languages
     */
    getAvailableLanguages() {
        return Array.from(this.loadedLanguages);
    }

    /**
     * Update document language attribute
     */
    updateDocumentLang() {
        document.documentElement.lang = this.currentLang;
    }

    /**
     * Translate all elements with data-i18n attribute
     */
    translatePage() {
        const elements = document.querySelectorAll('[data-i18n]');

        elements.forEach(el => {
            const key = el.getAttribute('data-i18n');
            const translation = this.t(key);

            if (translation) {
                el.textContent = translation;
            }
        });
    }
}
