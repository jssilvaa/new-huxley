// CryptoEngine.h
#pragma once
#include <string>

// Where are we getting the key? Do we need to fetch it directly with os.read()? What's the safest option? Do we need to provide an 
// additional interface for that with getKey() (SOLID Interface Segregation)
class CryptoEngine {
public:
    std::string encrypt(const std::string& plain, const std::string& key);
    std::string decrypt(const std::string& cipher, const std::string& key);
};
