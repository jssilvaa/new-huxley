// ClientState.h
#pragma once

#include <queue>
#include <string>
#include <time.h>
#include <pthread.h>

class WorkerThread;

// Represents per-connection state owned by a specific worker thread.
class ClientState {
public:
    ClientState(WorkerThread* owner, int socketFd);
    ~ClientState();

    int socket() const { return socketFd; }
    WorkerThread* ownerThread() const { return owner; }

    bool isAuthenticated() const { return authenticated; }
    void setAuthenticated(bool value);
    const std::string& username() const { return username; }
    void setUsername(std::string name);

    time_t lastActivity() const { return lastActivityTs; }
    void updateActivity(time_t now);

    std::string& mutableRecvBuffer() { return recvBuffer; }
    void clearRecvBuffer();

    void queueResponse(const std::string& message);
    bool popQueuedResponse(std::string& outMessage);

private:
    WorkerThread* owner;
    int socketFd;
    std::string username;
    bool authenticated;
    time_t lastActivityTs;
    std::string recvBuffer;

    std::queue<std::string> sendQueue;
    pthread_mutex_t sendMutex;
};
