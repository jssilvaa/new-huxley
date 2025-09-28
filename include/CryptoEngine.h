// CryptoEngine.h
#pragma once
#include <string>

class CryptoEngine {
public:
    std::string encrypt(const std::string& plain, const std::string& key);
    std::string decrypt(const std::string& cipher, const std::string& key);
};
