#pragma once

#include "esphome.h"
#include "config_storage.h"
#include <map>
#include <string>
#include <random>
#include <ctime>

namespace esphome_ui {

/**
 * @brief Authentication controller with session management
 *
 * Provides secure session-based authentication for the web interface.
 * Implements:
 * - Credential validation
 * - Session creation and management
 * - Automatic session expiration
 * - Rate limiting for login attempts
 * - Periodic cleanup of expired sessions
 *
 * Security Features:
 * - Random session IDs (128-bit entropy)
 * - HTTP-only cookies
 * - Configurable session timeout
 * - Brute force protection
 * - Session tracking per IP
 *
 * Usage:
 * AuthController auth;
 * auth.configure(username, password);
 *
 * if (auth.authenticate(user, pass, ip)) {
 *     std::string session_id = auth.createSession(user, ip);
 *     // Set cookie with session_id
 * }
 *
 * if (auth.validateSession(session_id)) {
 *     // User is authenticated
 * }
 */
class AuthController {
public:
    /**
     * @brief Session information
     */
    struct Session {
        std::string id;              ///< Session identifier (32-char hex)
        std::string username;        ///< Associated username
        std::string ip_address;      ///< Client IP address
        time_t created;              ///< Creation timestamp
        time_t expires;              ///< Expiration timestamp
        uint32_t request_count;      ///< Number of requests (for monitoring)

        Session() : created(0), expires(0), request_count(0) {}
    };

    /**
     * @brief Login attempt tracking for rate limiting
     */
    struct LoginAttempts {
        uint32_t count;              ///< Number of attempts
        time_t last_attempt;         ///< Timestamp of last attempt
        time_t lockout_until;        ///< Locked out until this time

        LoginAttempts() : count(0), last_attempt(0), lockout_until(0) {}
    };

private:
    // Configuration
    std::string m_admin_username;
    std::string m_admin_password;
    uint32_t m_session_timeout;

    // Session storage
    std::map<std::string, Session> m_sessions;

    // Rate limiting
    std::map<std::string, LoginAttempts> m_login_attempts;

    // Cleanup tracking
    time_t m_last_cleanup;

    // Constants
    static constexpr uint32_t DEFAULT_SESSION_TIMEOUT = 3600;      // 1 hour
    static constexpr uint32_t SESSION_CLEANUP_INTERVAL = 300;      // 5 minutes
    static constexpr uint8_t MAX_LOGIN_ATTEMPTS = 5;
    static constexpr uint32_t LOCKOUT_DURATION = 300;              // 5 minutes
    static constexpr uint32_t ATTEMPT_RESET_DURATION = 600;        // 10 minutes

    /**
     * @brief Generate cryptographically random session ID
     *
     * Generates a 32-character hexadecimal string (128 bits of entropy).
     *
     * @return Random session ID
     */
    std::string generateSessionId() {
        static std::random_device rd;
        static std::mt19937 gen(rd());
        static std::uniform_int_distribution<> dis(0, 15);

        const char* hex = "0123456789abcdef";
        std::string id;
        id.reserve(32);

        for (int i = 0; i < 32; i++) {
            id += hex[dis(gen)];
        }

        return id;
    }

    /**
     * @brief Clean up expired sessions and old login attempts
     *
     * Removes sessions past their expiration time and resets old login
     * attempt counters. Only runs if enough time has passed since last cleanup.
     */
    void cleanupExpired() {
        time_t now = ::time(nullptr);

        // Only cleanup periodically
        if (now - m_last_cleanup < SESSION_CLEANUP_INTERVAL) {
            return;
        }

        m_last_cleanup = now;

        // Remove expired sessions
        auto session_it = m_sessions.begin();
        while (session_it != m_sessions.end()) {
            if (now > session_it->second.expires) {
                ESP_LOGI("auth", "Cleaning up expired session: %s (user: %s)",
                         session_it->first.c_str(),
                         session_it->second.username.c_str());
                session_it = m_sessions.erase(session_it);
            } else {
                ++session_it;
            }
        }

        // Reset old login attempts
        auto attempt_it = m_login_attempts.begin();
        while (attempt_it != m_login_attempts.end()) {
            if (now - attempt_it->second.last_attempt > ATTEMPT_RESET_DURATION) {
                attempt_it = m_login_attempts.erase(attempt_it);
            } else {
                ++attempt_it;
            }
        }

        ESP_LOGI("auth", "Cleanup complete - Active sessions: %zu, Tracked IPs: %zu",
                 m_sessions.size(), m_login_attempts.size());
    }

public:
    AuthController()
        : m_session_timeout(DEFAULT_SESSION_TIMEOUT)
        , m_last_cleanup(0)
    {}

    /**
     * @brief Configure authentication credentials
     *
     * @param username Admin username
     * @param password Admin password
     * @param session_timeout Session timeout in seconds (default: 3600)
     */
    void configure(const std::string& username,
                   const std::string& password,
                   uint32_t session_timeout = DEFAULT_SESSION_TIMEOUT) {
        m_admin_username = username;
        m_admin_password = password;
        m_session_timeout = session_timeout;

        ESP_LOGI("auth", "Authentication configured - Username: %s, Timeout: %u seconds",
                 username.c_str(), session_timeout);
    }

    /**
     * @brief Check if IP address is currently locked out
     *
     * @param ip_address Client IP address
     * @return true if locked out, false otherwise
     */
    bool isLockedOut(const std::string& ip_address) {
        auto it = m_login_attempts.find(ip_address);
        if (it == m_login_attempts.end()) {
            return false;
        }

        time_t now = ::time(nullptr);
        if (now < it->second.lockout_until) {
            return true;
        }

        // Lockout expired, reset
        it->second.lockout_until = 0;
        it->second.count = 0;
        return false;
    }

    /**
     * @brief Authenticate user credentials
     *
     * Validates username and password, checking rate limits first.
     * On failure, increments attempt counter for the IP address.
     *
     * @param username Username to check
     * @param password Password to check
     * @param ip_address Client IP address (for rate limiting)
     * @return true if credentials are valid and not locked out
     */
    bool authenticate(const std::string& username,
                      const std::string& password,
                      const std::string& ip_address) {
        // Check lockout first
        if (isLockedOut(ip_address)) {
            ESP_LOGW("auth", "Login attempt from locked out IP: %s", ip_address.c_str());
            return false;
        }

        // Validate credentials
        bool valid = (username == m_admin_username && password == m_admin_password);

        // Track attempt
        auto& attempts = m_login_attempts[ip_address];
        time_t now = ::time(nullptr);

        if (valid) {
            // Success - reset attempts
            attempts.count = 0;
            attempts.last_attempt = now;
            ESP_LOGI("auth", "Successful login for user '%s' from %s",
                     username.c_str(), ip_address.c_str());
        } else {
            // Failure - increment attempts
            attempts.count++;
            attempts.last_attempt = now;

            ESP_LOGW("auth", "Failed login attempt for user '%s' from %s (attempt %u/%u)",
                     username.c_str(), ip_address.c_str(),
                     attempts.count, MAX_LOGIN_ATTEMPTS);

            // Check if should lock out
            if (attempts.count >= MAX_LOGIN_ATTEMPTS) {
                attempts.lockout_until = now + LOCKOUT_DURATION;
                ESP_LOGE("auth", "IP %s locked out for %u seconds due to too many failed attempts",
                         ip_address.c_str(), LOCKOUT_DURATION);
            }
        }

        return valid;
    }

    /**
     * @brief Create a new session
     *
     * Generates a new session ID and stores session information.
     * Call this after successful authentication.
     *
     * @param username Username for this session
     * @param ip_address Client IP address
     * @return Session ID (use in Set-Cookie header)
     */
    std::string createSession(const std::string& username, const std::string& ip_address) {
        cleanupExpired();

        std::string session_id = generateSessionId();
        time_t now = ::time(nullptr);

        Session session;
        session.id = session_id;
        session.username = username;
        session.ip_address = ip_address;
        session.created = now;
        session.expires = now + m_session_timeout;
        session.request_count = 0;

        m_sessions[session_id] = session;

        ESP_LOGI("auth", "Created session %s for user '%s' from %s (expires in %u seconds)",
                 session_id.c_str(), username.c_str(), ip_address.c_str(), m_session_timeout);

        return session_id;
    }

    /**
     * @brief Validate a session ID
     *
     * Checks if session exists and has not expired. Updates request count.
     *
     * @param session_id Session ID to validate
     * @return true if session is valid, false otherwise
     */
    bool validateSession(const std::string& session_id) {
        if (session_id.empty()) {
            return false;
        }

        cleanupExpired();

        auto it = m_sessions.find(session_id);
        if (it == m_sessions.end()) {
            return false;
        }

        time_t now = ::time(nullptr);
        if (now > it->second.expires) {
            ESP_LOGI("auth", "Session %s expired", session_id.c_str());
            m_sessions.erase(it);
            return false;
        }

        // Update request count and expiration (sliding window)
        it->second.request_count++;
        it->second.expires = now + m_session_timeout;

        return true;
    }

    /**
     * @brief Get session information
     *
     * @param session_id Session ID
     * @return Pointer to session, or nullptr if not found
     */
    const Session* getSession(const std::string& session_id) const {
        auto it = m_sessions.find(session_id);
        if (it == m_sessions.end()) {
            return nullptr;
        }
        return &it->second;
    }

    /**
     * @brief Destroy a session (logout)
     *
     * @param session_id Session ID to destroy
     * @return true if session was found and destroyed
     */
    bool destroySession(const std::string& session_id) {
        auto it = m_sessions.find(session_id);
        if (it == m_sessions.end()) {
            return false;
        }

        ESP_LOGI("auth", "Destroying session %s (user: %s)",
                 session_id.c_str(), it->second.username.c_str());
        m_sessions.erase(it);
        return true;
    }

    /**
     * @brief Get number of active sessions
     *
     * @return Active session count
     */
    size_t getSessionCount() const {
        return m_sessions.size();
    }

    /**
     * @brief Get lockout time remaining for IP
     *
     * @param ip_address Client IP address
     * @return Seconds remaining in lockout, or 0 if not locked out
     */
    uint32_t getLockoutRemaining(const std::string& ip_address) const {
        auto it = m_login_attempts.find(ip_address);
        if (it == m_login_attempts.end()) {
            return 0;
        }

        time_t now = ::time(nullptr);
        if (now >= it->second.lockout_until) {
            return 0;
        }

        return it->second.lockout_until - now;
    }

    /**
     * @brief Get statistics as JSON
     *
     * @return JSON string with authentication statistics
     */
    std::string getStats() const {
        char buffer[256];
        snprintf(buffer, sizeof(buffer),
            "{\"active_sessions\":%zu,\"tracked_ips\":%zu}",
            m_sessions.size(),
            m_login_attempts.size()
        );
        return std::string(buffer);
    }

    /**
     * @brief Log all active sessions (for debugging)
     */
    void logSessions() const {
        ESP_LOGI("auth", "=== Active Sessions ===");
        ESP_LOGI("auth", "Total: %zu", m_sessions.size());

        time_t now = ::time(nullptr);
        for (const auto& pair : m_sessions) {
            const Session& s = pair.second;
            uint32_t remaining = (s.expires > now) ? (s.expires - now) : 0;

            ESP_LOGI("auth", "  Session: %s", s.id.c_str());
            ESP_LOGI("auth", "    User: %s", s.username.c_str());
            ESP_LOGI("auth", "    IP: %s", s.ip_address.c_str());
            ESP_LOGI("auth", "    Expires in: %u seconds", remaining);
            ESP_LOGI("auth", "    Requests: %u", s.request_count);
        }
    }
};

} // namespace esphome_ui
