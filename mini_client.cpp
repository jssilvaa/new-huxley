#include <chrono>
#include <filesystem>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>
#include <sodium.h>

// modules under test
#include "src/DatabaseEngine.cpp"
#include "src/AuthManager.cpp"
#include <filesystem>
#include "src/CryptoEngine.cpp"
#include "src/ClientState.cpp"
#include "src/MessageRouter.cpp"
#include "src/ProtocolHandler.cpp"

/**
 * @file mini_client.cpp
 * @brief integration test suite for Huxley business layer 
 * 
 * Test Coverage:
 *   - AuthManager: Registration, login, session management, password hashing (Argon2id)
 *   - Database: User persistence, message storage, transaction integrity
 *   - MessageRouter: Message routing, encryption, delivery semantics
 *   - CryptoEngine: Authenticated encryption (XSalsa20-Poly1305), nonce uniqueness
 * 
 * Architecture:
 *   - Structured test suites with pass/fail assertions
 *   - Performance benchmarking (operation latency, throughput)
 *   - various test groups
 *  */

// ============================= Test Infrastructure =============================

struct TestResult {
    std::string suite;
    std::string name;
    bool passed;
    std::string message;
    double duration_ms;
};

class TestRunner {
public:
    void beginSuite(const std::string& name) {
        currentSuite = name;
        std::cout << "\n" << std::string(60, '=') << "\n";
        std::cout << "SUITE: " << name << "\n";
        std::cout << std::string(60, '=') << "\n";
    }

    void test(const std::string& name, bool condition, const std::string& message = "") {
        auto start = std::chrono::high_resolution_clock::now();
        auto end = std::chrono::high_resolution_clock::now();
        double duration = std::chrono::duration<double, std::milli>(end - start).count();

        TestResult result{currentSuite, name, condition, message, duration};
        results.push_back(result);

        std::cout << "  [" << (condition ? "PASS" : "FAIL") << "] " << name;
        if (!message.empty()) {
            std::cout << " — " << message;
        }
        std::cout << std::endl;

        if (condition) {
            passCount++;
        } else {
            failCount++;
        }
    }

    template<typename Func>
    void benchmark(const std::string& name, Func&& fn, int iterations = 100) {
        std::vector<double> timings;
        timings.reserve(iterations);

        for (int i = 0; i < iterations; ++i) {
            auto start = std::chrono::high_resolution_clock::now();
            fn();
            auto end = std::chrono::high_resolution_clock::now();
            timings.push_back(std::chrono::duration<double, std::milli>(end - start).count());
        }

        double sum = 0.0;
        double min = timings[0];
        double max = timings[0];
        for (double t : timings) {
            sum += t;
            if (t < min) min = t;
            if (t > max) max = t;
        }
        double avg = sum / iterations;

        std::cout << "  [BENCH] " << name << ": "
                  << std::fixed << std::setprecision(3)
                  << "avg=" << avg << "ms, "
                  << "min=" << min << "ms, "
                  << "max=" << max << "ms "
                  << "(" << iterations << " iterations)\n";
    }

    void printSummary() {
        std::cout << "\n" << std::string(60, '=') << "\n";
        std::cout << "TEST SUMMARY\n";
        std::cout << std::string(60, '=') << "\n";
        std::cout << "Total:  " << (passCount + failCount) << "\n";
        std::cout << "Passed: " << passCount << "\n";
        std::cout << "Failed: " << failCount << "\n";
        std::cout << "Pass rate: " << std::fixed << std::setprecision(1)
                  << (100.0 * passCount / (passCount + failCount)) << "%\n";
        std::cout << std::string(60, '=') << "\n";

        if (failCount > 0) {
            std::cout << "\nFailed tests:\n";
            for (const auto& r : results) {
                if (!r.passed) {
                    std::cout << "  - " << r.suite << "::" << r.name;
                    if (!r.message.empty()) {
                        std::cout << " (" << r.message << ")";
                    }
                    std::cout << "\n";
                }
            }
        }
    }

    int exitCode() const {
        return failCount > 0 ? 1 : 0;
    }

private:
    std::string currentSuite;
    std::vector<TestResult> results;
    int passCount = 0;
    int failCount = 0;
};

// ============================= Test Utilities =============================

std::string makeTestUser(int id) {
    static const std::string runSalt = [] {
        const auto now = std::chrono::steady_clock::now().time_since_epoch();
        return std::to_string(std::chrono::duration_cast<std::chrono::microseconds>(now).count());
    }();
    return "testuser_" + std::to_string(id) + "_" + runSalt;
}

std::string makeTestPassword(int id) {
    return "P@ssw0rd_" + std::to_string(id) + "!";
}

// ============================= Main Test Harness =============================

int main(int argc, char* argv[]) {
    TestRunner runner;

    // ========================= System Initialization =========================
    std::cout << "Huxley Messaging Server - Integration Test Suite\n";
    std::cout << "Runtime: " << __DATE__ << " " << __TIME__ << "\n";
    std::cout << "Compiler: " << __VERSION__ << "\n\n";

    if (sodium_init() < 0) {
        std::cerr << "[FATAL] Failed to initialize libsodium\n";
        return 1;
    }
    std::cout << "[INIT] Libsodium initialized\n";

    const std::string testDbPath = "test_integration.db";
    std::error_code fsErr;
    std::filesystem::remove(testDbPath, fsErr); // start on a clean slate if file existed

    Database database(testDbPath);
        if (!database.isOpen()) {
        std::cerr << "[FATAL] Failed to open database\n";
        return 1;
    }
    std::cout << "[INIT] Database opened (test_integration.db)\n";

    CryptoEngine cryptoEngine;
    std::cout << "[INIT] CryptoEngine initialized\n";

    AuthManager authManager(database);
    std::cout << "[INIT] AuthManager initialized\n";

    MessageRouter messageRouter(database, cryptoEngine);
    std::cout << "[INIT] MessageRouter initialized\n";

    // ========================= AuthManager Test Suite =========================
    runner.beginSuite("AuthManager");

    const std::string user1 = makeTestUser(1);
    const std::string pass1 = makeTestPassword(1);
    const std::string user2 = makeTestUser(2);
    const std::string pass2 = makeTestPassword(2);

    // Test 1: User registration
    bool reg1 = authManager.registerUser(user1, pass1);
    runner.test("Register user1", reg1, "New user registration should succeed");

    bool reg2 = authManager.registerUser(user2, pass2);
    runner.test("Register user2", reg2, "New user registration should succeed");

    // Test 2: Duplicate registration
    bool regDup = authManager.registerUser(user1, pass1);
    runner.test("Reject duplicate registration", !regDup, "Duplicate username must fail");

    // Test 3: Login with correct credentials
    bool loginOk = authManager.loginUser(user1, pass1);
    runner.test("Login with correct password", loginOk, "Valid credentials should authenticate");

    // Test 4: Login with incorrect credentials
    bool loginBad = authManager.loginUser(user2, "wrong_password_123");
    runner.test("Reject invalid password", !loginBad, "Incorrect password must fail");

    // Test 6: Successful login for user2
    bool login2 = authManager.loginUser(user2, pass2);
    runner.test("Login user2", login2, "Valid credentials should authenticate");

    // Test 7: Repeat login should remain allowed without explicit logout
    bool relogin = authManager.loginUser(user1, pass1);
    runner.test("Repeat login allowed", relogin, "Multiple login attempts should succeed");

    // ========================= Database Test Suite =========================
    runner.beginSuite("Database");

    // Test 1: User ID lookup
    int userId1 = 0;
    bool foundId = database.findUserId(user1, userId1);
    runner.test("Lookup user ID", foundId && userId1 > 0, "Registered user must have valid ID");

    // Test 2: Username reverse lookup
    std::string foundName;
    bool foundUser = database.findUsername(userId1, foundName);
    runner.test("Reverse lookup username", foundUser && foundName == user1, "User ID should map to correct username");

    // Test 3: Password hash retrieval
    std::string storedHash;
    bool foundHash = database.findUser(user1, storedHash);
    runner.test("Retrieve password hash", foundHash && !storedHash.empty(), "User must have stored hash");
    runner.test("Hash format is Argon2id", storedHash.find("$argon2id$") == 0, "Hash should use Argon2id KDF");

    // ========================= MessageRouter Test Suite =========================
    runner.beginSuite("MessageRouter");

    // Ensure both users are logged in for routing tests
    authManager.loginUser(user1, pass1);
    authManager.loginUser(user2, pass2);

    // Test 1: Route message between authenticated users
    const std::string msg1 = "Integration test message from user1 to user2";
    bool routed1 = messageRouter.routeMessage(user1, user2, msg1);
    runner.test("Route message to online user", routed1, "Message routing should succeed");

    // Test 2: Route to non-existent user
    const std::string fakeUser = "nonexistent_user_xyz";
    const std::string msg2 = "This should fail - user does not exist";
    bool routedFake = messageRouter.routeMessage(user1, fakeUser, msg2);
    runner.test("Reject message to unknown user", !routedFake, "Routing to nonexistent user must fail");

    // Test 3: Verify message persistence
    int userId2 = 0;
    database.findUserId(user2, userId2);
    auto messages = database.getQueuedMessages(userId2);
    runner.test("Message persisted to database", !messages.empty(), "Sent message must be stored");

    if (!messages.empty()) {
        runner.test("Message encrypted in storage", !messages[0].ciphertext.empty(), "Stored message must be encrypted");
        runner.test("Nonce present", messages[0].nonce.size() == 24, "Nonce must be 24 bytes (XSalsa20)");
    }

    // ========================= CryptoEngine Test Suite =========================
    runner.beginSuite("CryptoEngine");

    // Test 1: Encrypt/decrypt round-trip
    const std::string plaintext = "The quick brown fox jumps over the lazy dog";
    auto cipher = cryptoEngine.encryptMessage(plaintext);
    runner.test("Encryption produces ciphertext", !cipher.ciphertext.empty(), "Encrypted data must not be empty");
    runner.test("Nonce generated", cipher.nonce.size() == 24, "Nonce must be 192 bits");

    std::string decrypted;
    bool decryptOk = cryptoEngine.decryptMessage(cipher, decrypted);
    runner.test("Decryption succeeds", decryptOk, "Valid ciphertext should decrypt");
    runner.test("Round-trip preserves plaintext", decrypted == plaintext, "Decrypted text must match original");

    // Test 2: Tampered ciphertext rejection
    auto tampered = cipher;
    if (!tampered.ciphertext.empty()) {
        tampered.ciphertext[0] ^= 0xFF;  // Flip bits in first byte
    }
    std::string tamperedOut;
    bool tamperedFail = cryptoEngine.decryptMessage(tampered, tamperedOut);
    runner.test("Reject tampered ciphertext", !tamperedFail, "Modified ciphertext must fail MAC verification");

    // Test 3: Nonce uniqueness
    auto cipher1 = cryptoEngine.encryptMessage("message A");
    auto cipher2 = cryptoEngine.encryptMessage("message A");
    runner.test("Unique nonces per encryption", cipher1.nonce != cipher2.nonce, "Same plaintext must produce different nonces");

    // ========================= Performance Benchmarks =========================
    runner.beginSuite("Performance");

    std::cout << "\n  Running benchmarks...\n";

    // Benchmark 1: Password hashing (Argon2id)
    runner.benchmark("Password hashing (Argon2id)", [&]() {
        authManager.registerUser("bench_user_" + std::to_string(rand()), "bench_password");
    }, 10);  // Reduced iterations for expensive operation

    // Benchmark 2: Password verification
    const std::string benchUser = "perf_test_user";
    const std::string benchPass = "perf_test_password";
    authManager.registerUser(benchUser, benchPass);
    runner.benchmark("Password verification", [&]() {
        authManager.loginUser(benchUser, benchPass);
    }, 50);

    // Benchmark 3: Message encryption
    runner.benchmark("Message encryption (XSalsa20)", [&]() {
        cryptoEngine.encryptMessage("Benchmark payload for encryption testing");
    }, 1000);

    // Benchmark 4: Message decryption
    auto benchCipher = cryptoEngine.encryptMessage("Benchmark payload");
    std::string benchOut;
    runner.benchmark("Message decryption + MAC verify", [&]() {
        cryptoEngine.decryptMessage(benchCipher, benchOut);
    }, 1000);

    // ========================= Teardown & Reporting =========================
    std::cout << "\n[CLEANUP] Database closed\n";

    runner.printSummary();
    return runner.exitCode();
}