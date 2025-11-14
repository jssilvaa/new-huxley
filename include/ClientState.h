// ClientState.h
#pragma once

#include <deque>
#include <string>
#include <time.h>
#include <pthread.h>

#include "ClientNotifier.h"

// Represents per-connection state owned by a specific worker thread.
class ClientState {
public:
    ClientState(ClientNotifier* owner, int socketFd);
    ~ClientState();

    int socket() const { return socketFd; }
    ClientNotifier* ownerThread() const { return owner; }

    bool isAuthenticated() const { return authenticated; }
    void setAuthenticated(bool value);
    const std::string& username() const { return username_; }
    void setUsername(std::string name);

    time_t lastActivity() const { return lastActivityTs; }
    void updateActivity(time_t now);

    std::string& mutableRecvBuffer() { return recvBuffer; }
    void clearRecvBuffer();

    void queueResponse(const std::string& message);
    void queueFramedResponse(const std::string& message);
    void pushFrontResponse(const std::string& message);
    bool popQueuedResponse(std::string& outMessage);

private:
    ClientNotifier* owner;
    int socketFd;
    std::string username_;
    bool authenticated;
    time_t lastActivityTs;
    std::string recvBuffer;

    std::deque<std::string> sendQueue;
    pthread_mutex_t sendMutex;
};
