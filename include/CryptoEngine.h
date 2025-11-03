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
        std::string mac;
    };

    CryptoEngine();

    CipherMessage encryptMessage(const std::string& plaintext) const;
    bool decryptMessage(const CipherMessage& cipher, std::string& outPlaintext) const;

private:
    void ensureKeyLoaded() const;

    mutable bool keyLoaded;
    mutable std::array<unsigned char, 32> secretKey;
};
