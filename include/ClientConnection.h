// ClientConnection.h
#pragma once
#include <string>
#include <thread>

class HuxleyServer;

class ClientConnection {
public:
    ClientConnection(int socketFd, HuxleyServer* server);
    ~ClientConnection();

    void start();
    void stop();

    std::string getUsername() const;

private:
    void run(); // Thread loop
    int socketFd;
    std::string username;
    std::thread workerThread;
    bool active;
};
