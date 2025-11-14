#include "ClientState.h"

#include "WorkerThread.h"

#include <ctime>
#include <unistd.h>
#include <utility>

ClientState::ClientState(WorkerThread* ownerThread, int fd)
    : owner(ownerThread)
    , socketFd(fd)
    , username_()
    , authenticated(false)
    , lastActivityTs(::time(nullptr))
    , recvBuffer()
    , sendQueue()
{
    pthread_mutex_init(&sendMutex, nullptr);
}

ClientState::~ClientState()
{
    pthread_mutex_destroy(&sendMutex);
}

void ClientState::setAuthenticated(bool value)
{
    authenticated = value;
}

void ClientState::setUsername(std::string name)
{
    username_ = std::move(name);
}

void ClientState::updateActivity(time_t now)
{
    lastActivityTs = now;
}

void ClientState::clearRecvBuffer()
{
    recvBuffer.clear();
}

void ClientState::queueResponse(const std::string& message)
{
    pthread_mutex_lock(&sendMutex);
    sendQueue.push(message);
    pthread_mutex_unlock(&sendMutex);

    if (owner) {
        owner->notifyEvent(socketFd);
    }
}

bool ClientState::popQueuedResponse(std::string& outMessage)
{
    pthread_mutex_lock(&sendMutex);
    if (sendQueue.empty()) {
        pthread_mutex_unlock(&sendMutex);
        return false;
    }

    outMessage = std::move(sendQueue.front());
    sendQueue.pop();
    pthread_mutex_unlock(&sendMutex);
    return true;
}
