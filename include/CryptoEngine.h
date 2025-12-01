// CryptoEngine.h
#pragma once

#include <array>
#include <string>
#include <sodium.h>

// Provides authenticated encryption for payloads routed through the server.
class CryptoEngine {
public:
    struct CipherMessage {
        std::string nonce;
        std::string ciphertext;
    };

    CryptoEngine(); 
    ~CryptoEngine() noexcept;  

    CipherMessage encryptMessage(const std::string& plaintext);
    bool decryptMessage(const CipherMessage& cipher, std::string& outPlaintext);

private:
    std::array<unsigned char, crypto_secretbox_KEYBYTES> secretKey; 
    std::array<unsigned char, crypto_secretbox_KEYBYTES> masterKey;
    bool keyLoaded;
    bool masterLoaded;

    void ensureKeyLoaded() noexcept;
    void loadMasterKey();
    void loadSecretKey();
};
