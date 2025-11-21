#pragma once

#include "esphome.h"
#include "component.h"
#include "registry.h"
#include "auth.h"
#include "config_storage.h"
#include "deep_sleep.h"

#include <esp_http_server.h>
#include <esp_wifi.h>
#include <string>
#include <cstring>

namespace esphome_ui {

/**
 * @brief Main UI Framework class
 *
 * Orchestrates all framework components and provides the web server
 * with RESTful API endpoints. This is the main entry point for the framework.
 *
 * Features:
 * - HTTP web server on port 80
 * - RESTful API for sensors and actuators
 * - Session-based authentication
 * - Component auto-discovery
 * - Deep sleep integration
 * - Configuration management
 *
 * Usage in ESPHome YAML:
 *
 * esphome:
 *   on_boot:
 *     priority: -10
 *     then:
 *       - lambda: |-
 *           static UIFramework* framework = new UIFramework();
 *           framework->setup();
 *
 * Components register themselves:
 *   auto sensor = new BME280Sensor("temp1");
 *   UIFramework::getInstance()->registerComponent(sensor);
 */
class UIFramework {
private:
    // Singleton instance
    static UIFramework* s_instance;

    // HTTP server
    httpd_handle_t m_server;

    // Core components
    ComponentRegistry m_registry;
    AuthController m_auth;
    ConfigStorage m_config_storage;
    DeepSleepController m_deep_sleep;
    ConfigStorage::Config m_current_config;

    // Server configuration
    static constexpr uint16_t HTTP_PORT = 80;
    static constexpr uint16_t MAX_URI_HANDLERS = 30;
    static constexpr uint32_t STACK_SIZE = 16384;  // 16KB
    static constexpr uint8_t MAX_CONNECTIONS = 3;

    /**
     * @brief Parse session ID from Cookie header
     *
     * @param cookie_header Cookie header string
     * @return Session ID, or empty string if not found
     */
    std::string parseSessionCookie(const char* cookie_header) {
        if (!cookie_header) return "";

        const char* session_start = strstr(cookie_header, "session=");
        if (!session_start) return "";

        session_start += 8;  // Skip "session="
        const char* session_end = strchr(session_start, ';');

        if (session_end) {
            return std::string(session_start, session_end - session_start);
        } else {
            return std::string(session_start);
        }
    }

    /**
     * @brief Check if request has valid session
     *
     * @param req HTTP request
     * @return true if authenticated
     */
    bool isAuthenticated(httpd_req_t* req) {
        char cookie[256] = {0};
        if (httpd_req_get_hdr_value_str(req, "Cookie", cookie, sizeof(cookie)) != ESP_OK) {
            return false;
        }

        std::string session_id = parseSessionCookie(cookie);
        return m_auth.validateSession(session_id);
    }

    /**
     * @brief URL decode helper
     */
    static std::string urlDecode(const char* str) {
        std::string result;
        char ch;
        int i, ii;
        for (i = 0; i < strlen(str); i++) {
            if (int(str[i]) == 37) {  // '%'
                sscanf(str + i + 1, "%2x", &ii);
                ch = static_cast<char>(ii);
                result += ch;
                i = i + 2;
            } else if (str[i] == '+') {
                result += ' ';
            } else {
                result += str[i];
            }
        }
        return result;
    }

    /**
     * @brief Parse POST body into key-value pairs
     */
    static std::map<std::string, std::string> parsePostBody(const char* body) {
        std::map<std::string, std::string> params;
        std::string str(body);
        size_t pos = 0;

        while (pos < str.length()) {
            size_t eq_pos = str.find('=', pos);
            if (eq_pos == std::string::npos) break;

            size_t amp_pos = str.find('&', eq_pos);
            if (amp_pos == std::string::npos) amp_pos = str.length();

            std::string key = str.substr(pos, eq_pos - pos);
            std::string value = str.substr(eq_pos + 1, amp_pos - eq_pos - 1);
            params[key] = urlDecode(value.c_str());

            pos = amp_pos + 1;
        }

        return params;
    }

    // ========================================================================
    // HTTP Handlers
    // ========================================================================

    /**
     * @brief GET / - Main web interface
     */
    static esp_err_t rootHandler(httpd_req_t* req) {
        UIFramework* self = static_cast<UIFramework*>(req->user_ctx);

        // For now, send a simple HTML page
        // In Phase 2, this will serve the full SPA
        const char* html = R"(<!DOCTYPE html>
<html>
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>ESPHome UI Framework</title>
    <style>
        body { font-family: Arial, sans-serif; margin: 40px; background: #f5f5f5; }
        .container { max-width: 800px; margin: 0 auto; background: white; padding: 30px; border-radius: 10px; box-shadow: 0 2px 10px rgba(0,0,0,0.1); }
        h1 { color: #333; }
        .status { color: #28a745; font-weight: bold; }
        pre { background: #f8f9fa; padding: 15px; border-radius: 5px; overflow-x: auto; }
    </style>
</head>
<body>
    <div class="container">
        <h1>🚀 ESPHome UI Framework</h1>
        <p class="status">✅ Framework is running!</p>
        <h2>API Endpoints</h2>
        <pre>
GET  /api/status            Device status
GET  /api/components        All components
GET  /api/sensors           Sensor values
GET  /api/actuators         Actuator states
POST /api/auth/login        Login
POST /api/auth/logout       Logout
        </pre>
        <p><a href="/api/components">View Components (JSON)</a></p>
    </div>
</body>
</html>)";

        httpd_resp_set_type(req, "text/html");
        httpd_resp_send(req, html, strlen(html));
        return ESP_OK;
    }

    /**
     * @brief GET /api/status - Device status
     */
    static esp_err_t apiStatusHandler(httpd_req_t* req) {
        UIFramework* self = static_cast<UIFramework*>(req->user_ctx);

        char buffer[512];
        snprintf(buffer, sizeof(buffer),
            "{\"device_name\":\"%s\","
            "\"uptime\":%u,"
            "\"free_heap\":%u,"
            "\"components\":%zu,"
            "\"sensors\":%zu,"
            "\"actuators\":%zu}",
            self->m_current_config.device_name,
            (unsigned)(millis() / 1000),
            esp_get_free_heap_size(),
            self->m_registry.count(),
            self->m_registry.sensorCount(),
            self->m_registry.actuatorCount()
        );

        httpd_resp_set_type(req, "application/json");
        httpd_resp_set_hdr(req, "Access-Control-Allow-Origin", "*");
        httpd_resp_send(req, buffer, strlen(buffer));
        return ESP_OK;
    }

    /**
     * @brief GET /api/components - All registered components
     */
    static esp_err_t apiComponentsHandler(httpd_req_t* req) {
        UIFramework* self = static_cast<UIFramework*>(req->user_ctx);

        std::string json = self->m_registry.toJson(true);  // visible only

        httpd_resp_set_type(req, "application/json");
        httpd_resp_set_hdr(req, "Access-Control-Allow-Origin", "*");
        httpd_resp_send(req, json.c_str(), json.length());
        return ESP_OK;
    }

    /**
     * @brief GET /api/sensors - All sensor values
     */
    static esp_err_t apiSensorsHandler(httpd_req_t* req) {
        UIFramework* self = static_cast<UIFramework*>(req->user_ctx);

        std::string json = self->m_registry.sensorsToJson();

        httpd_resp_set_type(req, "application/json");
        httpd_resp_set_hdr(req, "Access-Control-Allow-Origin", "*");
        httpd_resp_send(req, json.c_str(), json.length());
        return ESP_OK;
    }

    /**
     * @brief GET /api/actuators - All actuator states
     */
    static esp_err_t apiActuatorsHandler(httpd_req_t* req) {
        UIFramework* self = static_cast<UIFramework*>(req->user_ctx);

        std::string json = self->m_registry.actuatorsToJson();

        httpd_resp_set_type(req, "application/json");
        httpd_resp_set_hdr(req, "Access-Control-Allow-Origin", "*");
        httpd_resp_send(req, json.c_str(), json.length());
        return ESP_OK;
    }

    /**
     * @brief POST /api/auth/login - Authenticate user
     */
    static esp_err_t apiLoginHandler(httpd_req_t* req) {
        UIFramework* self = static_cast<UIFramework*>(req->user_ctx);

        char content[256];
        int ret = httpd_req_recv(req, content, sizeof(content) - 1);
        if (ret <= 0) {
            httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "Bad Request");
            return ESP_FAIL;
        }
        content[ret] = '\0';

        auto params = parsePostBody(content);
        std::string username = params["username"];
        std::string password = params["password"];

        // Get client IP (simplified - in production use proper IP extraction)
        std::string ip = "unknown";

        if (self->m_auth.authenticate(username, password, ip)) {
            std::string session_id = self->m_auth.createSession(username, ip);

            // Set cookie
            std::string cookie = "session=" + session_id + "; Path=/; HttpOnly; Max-Age=" +
                               std::to_string(3600);
            httpd_resp_set_hdr(req, "Set-Cookie", cookie.c_str());

            httpd_resp_set_type(req, "application/json");
            httpd_resp_send(req, "{\"success\":true}", HTTPD_RESP_USE_STRLEN);
        } else {
            uint32_t lockout = self->m_auth.getLockoutRemaining(ip);
            char response[128];

            if (lockout > 0) {
                snprintf(response, sizeof(response),
                    "{\"success\":false,\"message\":\"Locked out for %u seconds\"}",
                    lockout);
            } else {
                strcpy(response, "{\"success\":false,\"message\":\"Invalid credentials\"}");
            }

            httpd_resp_set_type(req, "application/json");
            httpd_resp_send(req, response, strlen(response));
        }

        return ESP_OK;
    }

    /**
     * @brief POST /api/auth/logout - Destroy session
     */
    static esp_err_t apiLogoutHandler(httpd_req_t* req) {
        UIFramework* self = static_cast<UIFramework*>(req->user_ctx);

        char cookie[256] = {0};
        if (httpd_req_get_hdr_value_str(req, "Cookie", cookie, sizeof(cookie)) == ESP_OK) {
            std::string session_id = self->parseSessionCookie(cookie);
            self->m_auth.destroySession(session_id);
        }

        // Clear cookie
        httpd_resp_set_hdr(req, "Set-Cookie", "session=; Path=/; HttpOnly; Max-Age=0");

        httpd_resp_set_type(req, "application/json");
        httpd_resp_send(req, "{\"success\":true}", HTTPD_RESP_USE_STRLEN);
        return ESP_OK;
    }

    /**
     * @brief GET /api/sleep - Deep sleep status
     */
    static esp_err_t apiSleepStatusHandler(httpd_req_t* req) {
        UIFramework* self = static_cast<UIFramework*>(req->user_ctx);

        std::string json = self->m_deep_sleep.toJson();

        httpd_resp_set_type(req, "application/json");
        httpd_resp_send(req, json.c_str(), json.length());
        return ESP_OK;
    }

public:
    UIFramework()
        : m_server(nullptr)
    {
        s_instance = this;
    }

    /**
     * @brief Get singleton instance
     */
    static UIFramework* getInstance() {
        return s_instance;
    }

    /**
     * @brief Initialize and start the framework
     *
     * Call this in ESPHome's on_boot lambda.
     *
     * @return true if setup successful
     */
    bool setup() {
        ESP_LOGI("ui_framework", "Initializing ESPHome UI Framework...");

        // Initialize configuration storage
        if (!m_config_storage.init()) {
            ESP_LOGE("ui_framework", "Failed to initialize config storage");
            return false;
        }

        m_config_storage.load(m_current_config);

        // Configure authentication
        m_auth.configure(m_current_config.admin_username,
                        m_current_config.admin_password);

        // Initialize deep sleep
        m_deep_sleep.init();

        // Start HTTP server
        httpd_config_t config = HTTPD_DEFAULT_CONFIG();
        config.server_port = HTTP_PORT;
        config.max_uri_handlers = MAX_URI_HANDLERS;
        config.stack_size = STACK_SIZE;
        config.max_open_sockets = MAX_CONNECTIONS;
        config.lru_purge_enable = true;
        config.recv_wait_timeout = 5;
        config.send_wait_timeout = 5;

        if (httpd_start(&m_server, &config) != ESP_OK) {
            ESP_LOGE("ui_framework", "Failed to start HTTP server");
            return false;
        }

        ESP_LOGI("ui_framework", "HTTP server started on port %d", HTTP_PORT);

        // Register URI handlers
        registerHandlers();

        ESP_LOGI("ui_framework", "✅ UI Framework initialized successfully");
        ESP_LOGI("ui_framework", "Web interface: http://%s/",
                 WiFi.localIP().toString().c_str());

        return true;
    }

    /**
     * @brief Register all HTTP handlers
     */
    void registerHandlers() {
        httpd_uri_t handlers[] = {
            {.uri = "/", .method = HTTP_GET, .handler = rootHandler, .user_ctx = this},
            {.uri = "/api/status", .method = HTTP_GET, .handler = apiStatusHandler, .user_ctx = this},
            {.uri = "/api/components", .method = HTTP_GET, .handler = apiComponentsHandler, .user_ctx = this},
            {.uri = "/api/sensors", .method = HTTP_GET, .handler = apiSensorsHandler, .user_ctx = this},
            {.uri = "/api/actuators", .method = HTTP_GET, .handler = apiActuatorsHandler, .user_ctx = this},
            {.uri = "/api/auth/login", .method = HTTP_POST, .handler = apiLoginHandler, .user_ctx = this},
            {.uri = "/api/auth/logout", .method = HTTP_POST, .handler = apiLogoutHandler, .user_ctx = this},
            {.uri = "/api/sleep", .method = HTTP_GET, .handler = apiSleepStatusHandler, .user_ctx = this},
        };

        for (auto& handler : handlers) {
            httpd_register_uri_handler(m_server, &handler);
        }

        ESP_LOGI("ui_framework", "Registered %zu URI handlers", sizeof(handlers) / sizeof(handlers[0]));
    }

    /**
     * @brief Register a component with the framework
     *
     * Components should call this during their setup() phase.
     *
     * @param component Component to register
     * @return true if registered successfully
     */
    bool registerComponent(IComponent* component) {
        return m_registry.registerComponent(component);
    }

    /**
     * @brief Get the component registry
     */
    ComponentRegistry& getRegistry() {
        return m_registry;
    }

    /**
     * @brief Get the authentication controller
     */
    AuthController& getAuth() {
        return m_auth;
    }

    /**
     * @brief Get the configuration storage
     */
    ConfigStorage& getConfigStorage() {
        return m_config_storage;
    }

    /**
     * @brief Get current configuration
     */
    const ConfigStorage::Config& getConfig() const {
        return m_current_config;
    }

    /**
     * @brief Get deep sleep controller
     */
    DeepSleepController& getDeepSleep() {
        return m_deep_sleep;
    }
};

// Initialize static member
UIFramework* UIFramework::s_instance = nullptr;

} // namespace esphome_ui
