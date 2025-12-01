#include "../include/CryptoEngine.h"

#include <array>
#include <stdexcept>
#include <vector>
#include <sodium.h>
#include <fstream>

CryptoEngine::CryptoEngine()
    : keyLoaded(false)
{
    loadMasterKey();
    loadSecretKey();
}

CryptoEngine::~CryptoEngine()
{
    // Zero out the secret key from memory
    sodium_memzero(secretKey.data(), secretKey.size());
}

void CryptoEngine::loadMasterKey()
{
    std::ifstream in("/etc/huxley/master.key", std::ios::binary);
    if (!in)
    {
        throw std::runtime_error("Failed to open master key file");
    }

    in.read(reinterpret_cast<char *>(masterKey.data()), masterKey.size());
    if (in.gcount() != (int)masterKey.size())
    {
        throw std::runtime_error("Master key has an invalid size");
    }

    masterLoaded = true;
}

void CryptoEngine::loadSecretKey()
{
    if (!masterLoaded)
    {
        throw std::runtime_error("Encrypted session key missing");
    }

    std::ifstream in("/etc/huxley/session.key.enc", std::ios::binary);
    if (!in)
    {
        throw std::runtime_error("Failed to open session.key.enc file");
    }

    unsigned char nonce[crypto_secretbox_NONCEBYTES];
    unsigned char sealed[crypto_secretbox_MACBYTES + crypto_secretbox_KEYBYTES];

    in.read(reinterpret_cast<char *>(nonce), sizeof nonce);
    if (in.gcount() != (int)sizeof(nonce))
    {
        throw std::runtime_error("Invalid sealed key (bad nonce size)");
    }

    in.read(reinterpret_cast<char *>(sealed), sizeof sealed);
    if (in.gcount() != (int)sizeof(sealed))
    {
        throw std::runtime_error("Invalid sealed key (bad sealed size)");
    }

    // Decrypt session key
    if (crypto_secretbox_open_easy(secretKey.data(),
                                   sealed,
                                   sizeof sealed,
                                   nonce,
                                   masterKey.data()) != 0)
    {
        throw std::runtime_error("Failed to decrypt session key");
    }
    keyLoaded = true;
}

void CryptoEngine::ensureKeyLoaded() noexcept
{
    if (keyLoaded)
    {
        throw std::runtime_error("Secret key not loaded");
    }
}

CryptoEngine::CipherMessage CryptoEngine::encryptMessage(const std::string &plaintext)
{
    ensureKeyLoaded();

    // Nonce generation
    unsigned char nonce[crypto_secretbox_NONCEBYTES];
    randombytes_buf(nonce, sizeof(nonce));

    // Message encryption, AEAD
    std::vector<unsigned char> ciphertext(crypto_secretbox_MACBYTES + plaintext.size());
    crypto_secretbox_easy(ciphertext.data(),
                          reinterpret_cast<const unsigned char *>(plaintext.data()),
                          plaintext.size(),
                          nonce,
                          secretKey.data());

    // Pack up the cipher message and nonce
    CipherMessage cipherMsg;
    cipherMsg.nonce = std::string(reinterpret_cast<char *>(nonce), sizeof(nonce));
    cipherMsg.ciphertext = std::string(reinterpret_cast<char *>(ciphertext.data()), ciphertext.size());
    return cipherMsg;
}

bool CryptoEngine::decryptMessage(const CipherMessage &cipher, std::string &outPlaintext)
{
    ensureKeyLoaded();

    // Validate nonce size
    if (cipher.nonce.size() != crypto_secretbox_NONCEBYTES)
    {
        return false;
    }

    // Prepare buffers
    const unsigned char *nonce = reinterpret_cast<const unsigned char *>(cipher.nonce.data());
    const unsigned char *ciphertext = reinterpret_cast<const unsigned char *>(cipher.ciphertext.data());
    size_t ciphertextLen = cipher.ciphertext.size();

    // Validate ciphertext size
    if (ciphertextLen < crypto_secretbox_MACBYTES)
    {
        return false;
    }
    std::vector<unsigned char> plaintext(ciphertextLen - crypto_secretbox_MACBYTES);

    // Message decryption
    if (crypto_secretbox_open_easy(plaintext.data(),
                                   ciphertext,
                                   ciphertextLen,
                                   nonce,
                                   secretKey.data()) != 0)
    {
        return false; // Decryption failed
    }

    // Output plaintext
    outPlaintext.assign(plaintext.begin(), plaintext.end());
    return true;
}
