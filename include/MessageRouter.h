// MessageRouter.h
#pragma once
#include <string>

class CryptoEngine;
class ClientConnection;

class MessageRouter {
public:
    MessageRouter(CryptoEngine* crypto);

    void registerMessage(ClientState* state); 
    bool routeMessage(const std::string& sender, const std::string& recipient, const std::string& message);
    
private:
    CryptoEngine* cryptoEngine;
};
