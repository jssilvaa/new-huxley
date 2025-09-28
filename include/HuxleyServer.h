// HuxleyServer.h
#pragma once
#include <vector>
#include <memory>

class ClientConnection;
class AuthManager;
class MessageRouter;
class StatusManager;

class HuxleyServer {
public:
    HuxleyServer();
    ~HuxleyServer();

    bool start(int port);
    void stop();

private:
    void acceptConnections();
    std::vector<std::shared_ptr<ClientConnection>> clients;
    std::unique_ptr<AuthManager> authManager;
    std::unique_ptr<MessageRouter> messageRouter;
    std::unique_ptr<StatusManager> statusManager;
};
