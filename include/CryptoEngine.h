// CryptoEngine.h
#pragma once

#include <array>
#include <string>

// Provides authenticated encryption for payloads routed through the server.
class CryptoEngine {
public:
    struct CipherMessage {
        std::string nonce;
        std::string ciphertext;
    };

    CryptoEngine() : keyLoaded(false) {}
    ~CryptoEngine() noexcept;  

    CipherMessage encryptMessage(const std::string& plaintext);
    bool decryptMessage(const CipherMessage& cipher, std::string& outPlaintext);

private:
    void ensureKeyLoaded() noexcept;

    bool keyLoaded;
    std::array<unsigned char, 32> secretKey;
};
