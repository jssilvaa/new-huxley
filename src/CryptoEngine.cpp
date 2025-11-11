#include "CryptoEngine.h"

#include <algorithm>
#include <array>
#include <iomanip>
#include <random>
#include <sstream>

namespace {
std::string toHex(std::size_t value)
{
    std::ostringstream oss;
    oss << std::hex << std::setw(sizeof(std::size_t) * 2) << std::setfill('0') << value;
    return oss.str();
}

bool constantTimeEquals(const std::string& lhs, const std::string& rhs)
{
    if (lhs.size() != rhs.size()) {
        return false;
    }
    unsigned char diff = 0;
    for (std::size_t i = 0; i < lhs.size(); ++i) {
        diff |= static_cast<unsigned char>(lhs[i] ^ rhs[i]);
    }
    return diff == 0;
}

std::string computeMac(const std::string& nonce,
                       const std::string& ciphertext,
                       const std::array<unsigned char, 32>& key)
{
    std::string combined;
    combined.reserve(nonce.size() + ciphertext.size() + key.size());
    combined.append(nonce);
    combined.append(ciphertext);
    combined.append(reinterpret_cast<const char*>(key.data()), key.size());
    return toHex(std::hash<std::string>{}(combined));
}
} // namespace

CryptoEngine::CryptoEngine()
    : keyLoaded(false)
    , secretKey{}
{
}

void CryptoEngine::ensureKeyLoaded() const
{
    if (keyLoaded) {
        return;
    }

    std::random_device rd;
    for (auto& byte : secretKey) {
        byte = static_cast<unsigned char>(rd());
    }
    keyLoaded = true;
}

CryptoEngine::CipherMessage CryptoEngine::encryptMessage(const std::string& plaintext) const
{
    ensureKeyLoaded();

    std::array<unsigned char, 24> nonceBytes{};
    std::random_device rd;
    for (auto& byte : nonceBytes) {
        byte = static_cast<unsigned char>(rd());
    }

    std::string nonce(reinterpret_cast<const char*>(nonceBytes.data()), nonceBytes.size());
    std::string ciphertext(plaintext.size(), '\0');

    for (std::size_t i = 0; i < plaintext.size(); ++i) {
        const unsigned char k = secretKey[i % secretKey.size()];
        const unsigned char n = nonceBytes[i % nonceBytes.size()];
        ciphertext[i] = static_cast<char>(plaintext[i] ^ k ^ n);
    }

    CipherMessage message;
    message.nonce = std::move(nonce);
    message.ciphertext = std::move(ciphertext);
    message.mac = computeMac(message.nonce, message.ciphertext, secretKey);
    return message;
}

bool CryptoEngine::decryptMessage(const CipherMessage& cipher, std::string& outPlaintext) const
{
    ensureKeyLoaded();

    const std::string expectedMac = computeMac(cipher.nonce, cipher.ciphertext, secretKey);
    if (!constantTimeEquals(expectedMac, cipher.mac)) {
        return false;
    }

    if (cipher.nonce.empty()) {
        return false;
    }

    outPlaintext.resize(cipher.ciphertext.size());
    const auto* noncePtr = reinterpret_cast<const unsigned char*>(cipher.nonce.data());

    for (std::size_t i = 0; i < cipher.ciphertext.size(); ++i) {
        const unsigned char k = secretKey[i % secretKey.size()];
        const unsigned char n = noncePtr[i % cipher.nonce.size()];
        outPlaintext[i] = static_cast<char>(cipher.ciphertext[i] ^ k ^ n);
    }

    return true;
}
