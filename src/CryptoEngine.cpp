#include "../include/CryptoEngine.h"

#include <array>
#include <stdexcept>
#include <vector> 
#include <sodium.h>


// CryptoEngine::CryptoEngine()
//     : keyLoaded(false)
// {
// }

CryptoEngine::~CryptoEngine()
{
    // Zero out the secret key from memory
    sodium_memzero(secretKey.data(), secretKey.size());
}

void CryptoEngine::ensureKeyLoaded() noexcept
{
    if (keyLoaded) {
        return;
    } 

    // random key generation via crypto_secretbox_keygen (random symmetric key)
    unsigned char key[crypto_secretbox_KEYBYTES];
    crypto_secretbox_keygen(key);
    std::copy(key, key + crypto_secretbox_KEYBYTES, secretKey.begin());
    sodium_memzero(key, sizeof(key));
    keyLoaded = true;
}

CryptoEngine::CipherMessage CryptoEngine::encryptMessage(const std::string& plaintext)
{
    ensureKeyLoaded();

    // Nonce generation
    unsigned char nonce[crypto_secretbox_NONCEBYTES];
    randombytes_buf(nonce, sizeof(nonce));

    // Message encryption, AEAD 
    std::vector<unsigned char> ciphertext(crypto_secretbox_MACBYTES + plaintext.size());
    crypto_secretbox_easy(ciphertext.data(), 
                          reinterpret_cast<const unsigned char*>(plaintext.data()),
                          plaintext.size(),
                          nonce,
                          secretKey.data());

    // Pack up the cipher message and nonce 
    CipherMessage cipherMsg;
    cipherMsg.nonce = std::string(reinterpret_cast<char*>(nonce), sizeof(nonce));
    cipherMsg.ciphertext = std::string(reinterpret_cast<char*>(ciphertext.data()), ciphertext.size());
    return cipherMsg;
}

bool CryptoEngine::decryptMessage(const CipherMessage& cipher, std::string& outPlaintext)
{
    ensureKeyLoaded();

    // Validate nonce size
    if (cipher.nonce.size() != crypto_secretbox_NONCEBYTES) {
        return false;
    }

    // Prepare buffers
    const unsigned char* nonce = reinterpret_cast<const unsigned char*>(cipher.nonce.data());
    const unsigned char* ciphertext = reinterpret_cast<const unsigned char*>(cipher.ciphertext.data());
    size_t ciphertextLen = cipher.ciphertext.size();


    // Validate ciphertext size
    if (ciphertextLen < crypto_secretbox_MACBYTES) {
        return false;
    }
    std::vector<unsigned char> plaintext(ciphertextLen - crypto_secretbox_MACBYTES);

    // Message decryption
    if (crypto_secretbox_open_easy(plaintext.data(), 
                                   ciphertext,
                                   ciphertextLen,
                                   nonce,
                                   secretKey.data()) != 0) {
        return false; // Decryption failed
    }

    // Output plaintext
    outPlaintext.assign(plaintext.begin(), plaintext.end());
    return true;
}
