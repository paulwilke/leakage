/**
 * Unit Tests for AuthController
 *
 * Tests authentication, session management, and rate limiting functionality.
 */

#include <gtest/gtest.h>
#include "esp_mock.h"  // Must be included before framework headers
#include "../../framework/include/auth.h"
#include <string>
#include <thread>
#include <chrono>

using namespace esphome_ui;

// ============================================================================
// Test Fixture
// ============================================================================

class AuthControllerTest : public ::testing::Test {
protected:
    AuthController* auth;

    void SetUp() override {
        auth = new AuthController();

        // Set default credentials for testing
        auth->setCredentials("admin", "admin123");
    }

    void TearDown() override {
        delete auth;
    }

    // Helper to simulate passage of time
    void waitMs(int ms) {
        std::this_thread::sleep_for(std::chrono::milliseconds(ms));
    }
};

// ============================================================================
// Authentication Tests
// ============================================================================

TEST_F(AuthControllerTest, AuthenticateValidCredentials) {
    EXPECT_TRUE(auth->authenticate("admin", "admin123", "192.168.1.100"));
}

TEST_F(AuthControllerTest, AuthenticateInvalidUsername) {
    EXPECT_FALSE(auth->authenticate("wrong", "admin123", "192.168.1.100"));
}

TEST_F(AuthControllerTest, AuthenticateInvalidPassword) {
    EXPECT_FALSE(auth->authenticate("admin", "wrongpass", "192.168.1.100"));
}

TEST_F(AuthControllerTest, AuthenticateEmptyCredentials) {
    EXPECT_FALSE(auth->authenticate("", "", "192.168.1.100"));
    EXPECT_FALSE(auth->authenticate("admin", "", "192.168.1.100"));
    EXPECT_FALSE(auth->authenticate("", "admin123", "192.168.1.100"));
}

TEST_F(AuthControllerTest, AuthenticateCaseSensitiveUsername) {
    EXPECT_FALSE(auth->authenticate("Admin", "admin123", "192.168.1.100"));
    EXPECT_FALSE(auth->authenticate("ADMIN", "admin123", "192.168.1.100"));
}

// ============================================================================
// Session Management Tests
// ============================================================================

TEST_F(AuthControllerTest, CreateSession) {
    std::string session_id = auth->createSession("admin", "192.168.1.100");

    EXPECT_FALSE(session_id.empty());
    EXPECT_GE(session_id.length(), 16);  // Should have good entropy
}

TEST_F(AuthControllerTest, CreateSessionUniqueness) {
    std::string session1 = auth->createSession("admin", "192.168.1.100");
    std::string session2 = auth->createSession("admin", "192.168.1.100");

    EXPECT_NE(session1, session2);  // Each session should be unique
}

TEST_F(AuthControllerTest, ValidateValidSession) {
    std::string session_id = auth->createSession("admin", "192.168.1.100");

    EXPECT_TRUE(auth->validateSession(session_id));
}

TEST_F(AuthControllerTest, ValidateInvalidSession) {
    EXPECT_FALSE(auth->validateSession("invalid_session_id"));
    EXPECT_FALSE(auth->validateSession(""));
}

TEST_F(AuthControllerTest, ValidateExpiredSession) {
    // Set very short session timeout for testing
    auth->setSessionTimeout(100);  // 100ms

    std::string session_id = auth->createSession("admin", "192.168.1.100");
    EXPECT_TRUE(auth->validateSession(session_id));

    // Wait for session to expire
    waitMs(150);

    EXPECT_FALSE(auth->validateSession(session_id));
}

TEST_F(AuthControllerTest, DestroySession) {
    std::string session_id = auth->createSession("admin", "192.168.1.100");
    EXPECT_TRUE(auth->validateSession(session_id));

    EXPECT_TRUE(auth->destroySession(session_id));
    EXPECT_FALSE(auth->validateSession(session_id));
}

TEST_F(AuthControllerTest, DestroyNonexistentSession) {
    EXPECT_FALSE(auth->destroySession("nonexistent_session"));
}

TEST_F(AuthControllerTest, GetSessionUsername) {
    std::string session_id = auth->createSession("admin", "192.168.1.100");

    std::string username = auth->getSessionUsername(session_id);
    EXPECT_EQ(username, "admin");
}

TEST_F(AuthControllerTest, GetSessionUsernameInvalid) {
    std::string username = auth->getSessionUsername("invalid_session");
    EXPECT_EQ(username, "");
}

// ============================================================================
// Rate Limiting Tests
// ============================================================================

TEST_F(AuthControllerTest, RateLimitingAfterFailedAttempts) {
    std::string ip = "192.168.1.100";

    // First 5 attempts should be allowed
    for (int i = 0; i < 5; i++) {
        EXPECT_FALSE(auth->authenticate("admin", "wrongpass", ip));
        EXPECT_FALSE(auth->isLockedOut(ip));
    }

    // 6th attempt should trigger lockout
    EXPECT_FALSE(auth->authenticate("admin", "wrongpass", ip));
    EXPECT_TRUE(auth->isLockedOut(ip));
}

TEST_F(AuthControllerTest, RateLimitingBlocksValidCredentials) {
    std::string ip = "192.168.1.100";

    // Trigger lockout with failed attempts
    for (int i = 0; i < 6; i++) {
        auth->authenticate("admin", "wrongpass", ip);
    }

    // Even valid credentials should be blocked
    EXPECT_TRUE(auth->isLockedOut(ip));
    EXPECT_FALSE(auth->authenticate("admin", "admin123", ip));
}

TEST_F(AuthControllerTest, RateLimitingExpiresAfterTimeout) {
    auth->setLockoutDuration(100);  // 100ms lockout for testing
    std::string ip = "192.168.1.100";

    // Trigger lockout
    for (int i = 0; i < 6; i++) {
        auth->authenticate("admin", "wrongpass", ip);
    }
    EXPECT_TRUE(auth->isLockedOut(ip));

    // Wait for lockout to expire
    waitMs(150);

    EXPECT_FALSE(auth->isLockedOut(ip));
    EXPECT_TRUE(auth->authenticate("admin", "admin123", ip));
}

TEST_F(AuthControllerTest, RateLimitingPerIP) {
    std::string ip1 = "192.168.1.100";
    std::string ip2 = "192.168.1.101";

    // Lock out IP1
    for (int i = 0; i < 6; i++) {
        auth->authenticate("admin", "wrongpass", ip1);
    }

    // IP1 should be locked out
    EXPECT_TRUE(auth->isLockedOut(ip1));

    // IP2 should still work
    EXPECT_FALSE(auth->isLockedOut(ip2));
    EXPECT_TRUE(auth->authenticate("admin", "admin123", ip2));
}

TEST_F(AuthControllerTest, RateLimitingResetOnSuccess) {
    std::string ip = "192.168.1.100";

    // Make a few failed attempts
    auth->authenticate("admin", "wrongpass", ip);
    auth->authenticate("admin", "wrongpass", ip);
    auth->authenticate("admin", "wrongpass", ip);

    // Successful login should reset counter
    EXPECT_TRUE(auth->authenticate("admin", "admin123", ip));

    // Should be able to make more attempts now
    for (int i = 0; i < 5; i++) {
        auth->authenticate("admin", "wrongpass", ip);
    }

    // Should not be locked out yet (counter was reset)
    EXPECT_FALSE(auth->isLockedOut(ip));
}

// ============================================================================
// Session Cleanup Tests
// ============================================================================

TEST_F(AuthControllerTest, CleanupExpiredSessions) {
    auth->setSessionTimeout(100);  // 100ms timeout

    // Create several sessions
    std::string session1 = auth->createSession("admin", "192.168.1.100");
    std::string session2 = auth->createSession("admin", "192.168.1.101");
    std::string session3 = auth->createSession("admin", "192.168.1.102");

    EXPECT_TRUE(auth->validateSession(session1));
    EXPECT_TRUE(auth->validateSession(session2));
    EXPECT_TRUE(auth->validateSession(session3));

    // Wait for sessions to expire
    waitMs(150);

    // Run cleanup
    int cleaned = auth->cleanupExpiredSessions();
    EXPECT_EQ(cleaned, 3);

    // All sessions should be invalid
    EXPECT_FALSE(auth->validateSession(session1));
    EXPECT_FALSE(auth->validateSession(session2));
    EXPECT_FALSE(auth->validateSession(session3));
}

TEST_F(AuthControllerTest, CleanupDoesNotRemoveActiveSessions) {
    auth->setSessionTimeout(10000);  // 10 second timeout

    std::string session1 = auth->createSession("admin", "192.168.1.100");
    std::string session2 = auth->createSession("admin", "192.168.1.101");

    // Run cleanup immediately
    int cleaned = auth->cleanupExpiredSessions();
    EXPECT_EQ(cleaned, 0);

    // Sessions should still be valid
    EXPECT_TRUE(auth->validateSession(session1));
    EXPECT_TRUE(auth->validateSession(session2));
}

TEST_F(AuthControllerTest, GetActiveSessionCount) {
    EXPECT_EQ(auth->getActiveSessionCount(), 0);

    std::string session1 = auth->createSession("admin", "192.168.1.100");
    EXPECT_EQ(auth->getActiveSessionCount(), 1);

    std::string session2 = auth->createSession("admin", "192.168.1.101");
    EXPECT_EQ(auth->getActiveSessionCount(), 2);

    auth->destroySession(session1);
    EXPECT_EQ(auth->getActiveSessionCount(), 1);

    auth->destroySession(session2);
    EXPECT_EQ(auth->getActiveSessionCount(), 0);
}

// ============================================================================
// Password Management Tests
// ============================================================================

TEST_F(AuthControllerTest, SetCredentials) {
    auth->setCredentials("newuser", "newpass");

    EXPECT_FALSE(auth->authenticate("admin", "admin123", "192.168.1.100"));
    EXPECT_TRUE(auth->authenticate("newuser", "newpass", "192.168.1.100"));
}

TEST_F(AuthControllerTest, SetCredentialsInvalidatesSessions) {
    std::string session = auth->createSession("admin", "192.168.1.100");
    EXPECT_TRUE(auth->validateSession(session));

    // Change credentials
    auth->setCredentials("admin", "newpassword");

    // Old session should be invalidated
    EXPECT_FALSE(auth->validateSession(session));
}

// ============================================================================
// Security Tests
// ============================================================================

TEST_F(AuthControllerTest, SessionIdEntropy) {
    // Generate many session IDs and check for uniqueness
    std::set<std::string> sessions;

    for (int i = 0; i < 100; i++) {
        std::string session = auth->createSession("admin", "192.168.1.100");
        sessions.insert(session);
    }

    // All should be unique
    EXPECT_EQ(sessions.size(), 100);
}

TEST_F(AuthControllerTest, SessionIdLength) {
    std::string session = auth->createSession("admin", "192.168.1.100");

    // Session ID should be long enough for security
    // 128 bits = 16 bytes = 32 hex characters
    EXPECT_GE(session.length(), 32);
}

TEST_F(AuthControllerTest, NoPasswordInSessionId) {
    std::string session = auth->createSession("admin", "192.168.1.100");

    // Session ID should not contain password
    EXPECT_EQ(session.find("admin123"), std::string::npos);
}

// ============================================================================
// Edge Cases
// ============================================================================

TEST_F(AuthControllerTest, VeryLongUsername) {
    std::string long_username(1000, 'a');

    EXPECT_FALSE(auth->authenticate(long_username, "admin123", "192.168.1.100"));
}

TEST_F(AuthControllerTest, VeryLongPassword) {
    std::string long_password(1000, 'x');

    EXPECT_FALSE(auth->authenticate("admin", long_password, "192.168.1.100"));
}

TEST_F(AuthControllerTest, SpecialCharactersInCredentials) {
    auth->setCredentials("user@example.com", "p@$$w0rd!#%");

    EXPECT_TRUE(auth->authenticate("user@example.com", "p@$$w0rd!#%", "192.168.1.100"));
}

TEST_F(AuthControllerTest, UnicodeInCredentials) {
    auth->setCredentials("用户", "密码");

    EXPECT_TRUE(auth->authenticate("用户", "密码", "192.168.1.100"));
    EXPECT_FALSE(auth->authenticate("用户", "wrong", "192.168.1.100"));
}

TEST_F(AuthControllerTest, IPv6Address) {
    std::string ipv6 = "2001:0db8:85a3:0000:0000:8a2e:0370:7334";

    EXPECT_TRUE(auth->authenticate("admin", "admin123", ipv6));

    std::string session = auth->createSession("admin", ipv6);
    EXPECT_TRUE(auth->validateSession(session));
}

TEST_F(AuthControllerTest, ConcurrentSessionsFromSameIP) {
    std::string ip = "192.168.1.100";

    std::string session1 = auth->createSession("admin", ip);
    std::string session2 = auth->createSession("admin", ip);
    std::string session3 = auth->createSession("admin", ip);

    // All should be valid independently
    EXPECT_TRUE(auth->validateSession(session1));
    EXPECT_TRUE(auth->validateSession(session2));
    EXPECT_TRUE(auth->validateSession(session3));
}

// ============================================================================
// Statistics Tests
// ============================================================================

TEST_F(AuthControllerTest, GetStats) {
    // Create some sessions
    auth->createSession("admin", "192.168.1.100");
    auth->createSession("admin", "192.168.1.101");

    // Make some failed attempts
    auth->authenticate("admin", "wrong", "192.168.1.102");

    std::string stats = auth->getStats();

    // Should contain session count
    EXPECT_NE(stats.find("\"active_sessions\":2"), std::string::npos);

    // Should contain failed attempts info
    EXPECT_NE(stats.find("failed_attempts"), std::string::npos);
}

// ============================================================================
// Main
// ============================================================================

int main(int argc, char **argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
