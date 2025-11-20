# ADR-002: Authentication Method

## Status
Accepted

## Context

The framework needs to protect admin functionality while keeping read-only data accessible. We need an authentication method that:

- Works efficiently on ESP32
- Doesn't require HTTPS (complexity/overhead)
- Protects admin credentials
- Supports session management
- Is standard and well-understood

### Options Considered

#### Option A: HTTP Basic Authentication

**Pros:**
- Simple to implement
- Widely supported
- Standard HTTP feature

**Cons:**
- Credentials sent in clear text (base64)
- No session support
- Credentials sent with every request
- Vulnerable without HTTPS

#### Option B: HTTP Digest Authentication

**Pros:**
- Credentials never sent in clear text
- Nonce-based protection against replay attacks
- Standard HTTP authentication
- Works without HTTPS
- Session-like behavior with nonces

**Cons:**
- More complex to implement
- Still vulnerable to MitM without HTTPS
- Nonce management required

#### Option C: Cookie-Based Session (Current Implementation)

**Pros:**
- Simple username/password check
- Session management with cookies
- Easy to implement
- Familiar pattern
- Good UX (login once)

**Cons:**
- Initial login sends password in clear text
- Vulnerable without HTTPS
- Custom implementation

#### Option D: JWT Tokens

**Pros:**
- Stateless authentication
- Industry standard
- Supports claims/roles

**Cons:**
- Overkill for ESP32
- Requires crypto library
- Memory intensive
- Complex for simple use case

## Decision

We will use a **hybrid approach**:

1. **Cookie-Based Sessions** (like current implementation) for user convenience
2. **Rate limiting** to prevent brute force
3. **Secure session IDs** (32-character random hex)
4. **Session timeout** (configurable, default 1 hour)
5. **HTTP-only cookies** (no JavaScript access)

For future enhancement, we'll provide an **optional HTTP Digest upgrade path**.

## Rationale

1. **Simplicity**: Cookie-based sessions are straightforward and proven in current implementation

2. **User Experience**: Login once per session, no credentials on every request

3. **Sufficient Security**: For local network IoT devices:
   - Network isolation (VLAN) is recommended
   - Adding HTTPS would require certificate management (complex for users)
   - Session IDs are cryptographically random
   - Rate limiting prevents brute force

4. **Resource Efficient**: Minimal memory overhead compared to alternatives

5. **Proven**: Current leak sensor implementation works well in production

## Security Measures

### Implemented Protections

```cpp
// 1. Secure session ID generation
std::random_device rd;
std::mt19937 gen(rd());
// 32-character hex ID = 128 bits entropy

// 2. Session timeout
constexpr uint32_t SESSION_TIMEOUT = 3600; // 1 hour

// 3. Automatic cleanup
constexpr uint32_t SESSION_CLEANUP_INTERVAL = 300; // 5 minutes

// 4. HTTP-only cookies
Set-Cookie: session=xxx; Path=/; HttpOnly; Max-Age=3600

// 5. Rate limiting
static constexpr uint8_t MAX_LOGIN_ATTEMPTS = 5;
static constexpr uint32_t LOCKOUT_PERIOD = 60; // seconds
```

### User Guidelines (Documentation)

Users will be advised to:

1. **Change default password immediately**
2. **Use strong passwords** (min 12 characters recommended)
3. **Network isolation**: Place ESP32 devices in separate VLAN
4. **Firewall rules**: Limit access to trusted IPs
5. **Regular updates**: Keep firmware updated
6. **Monitor logs**: Watch for suspicious login attempts

## Consequences

### Positive
- Simple implementation
- Good user experience
- Low resource usage
- Familiar pattern for developers
- Proven in production

### Negative
- Password sent in clear text during initial login
- Vulnerable to local network sniffing
- No MFA support
- Cookie-based attacks possible

### Acceptable Because
- Target environment is trusted local networks
- Professional IoT products typically use VLAN isolation
- HTTPS adds significant complexity for DIY users
- Trade-off: Simplicity vs. Perfect Security

## Future Enhancements

Phase 2 could add:

1. **Optional HTTPS**: For advanced users
2. **TOTP 2FA**: Time-based one-time passwords
3. **Certificate pinning**: For mobile apps
4. **API keys**: For programmatic access

These will be plugin-based to keep core simple.

## Implementation Example

```cpp
class AuthController {
private:
    std::map<std::string, SessionInfo> sessions;
    std::map<std::string, LoginAttempts> attempts;

public:
    bool authenticate(const char* username, const char* password);
    std::string createSession(const char* username);
    bool validateSession(const char* sessionId);
    void cleanupExpiredSessions();
    bool isRateLimited(const char* ip);
};
```

## References

- OWASP Session Management: https://cheatsheetseries.owasp.org/cheatsheets/Session_Management_Cheat_Sheet.html
- ESP32 Random Number Generation: https://docs.espressif.com/projects/esp-idf/en/latest/esp32/api-reference/system/random.html
- HTTP Cookie Spec: RFC 6265

---

**Date:** 2025-11-20
**Author:** ESPHome UI Framework Team
