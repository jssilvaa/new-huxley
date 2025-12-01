#include "ClientState.h"

#include <arpa/inet.h>
#include <ctime>
#include <cstring>
#include <unistd.h>
#include <utility>

namespace {
std::string framePayload(const std::string& payload)
{
    const uint32_t payloadSize = static_cast<uint32_t>(payload.size());
    std::string frame;
    frame.resize(sizeof(uint32_t) + payloadSize);

    uint32_t netOrder = htonl(payloadSize);
    std::memcpy(frame.data(), &netOrder, sizeof(netOrder));
    if (payloadSize > 0) {
        std::memcpy(frame.data() + sizeof(netOrder), payload.data(), payloadSize);
    }

    return frame;
}

void queueFramedResponse(ClientState& state, const std::string& message)
{
    const std::string frame = framePayload(message);
    if (frame.empty()) {
        return;
    }
    state.queueResponse(frame);
}
} // namespace

ClientState::ClientState(ClientNotifier* ownerThread, int fd, ProtocolHandler& protocol)
    : owner(ownerThread)
    , socketFd(fd)
    , protocolHandler(protocol)
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
    sendQueue.push_back(message);
    pthread_mutex_unlock(&sendMutex);

    if (owner) {
        owner->notifyEvent(socketFd);
    }
}

void ClientState::queueProtocolResponse(const Response& response)
{
    queueFramedResponse(*this, protocolHandler.serializeResponse(response));
}

void ClientState::queueIncomingMessage(const std::string& sender,
                                       const std::string& content,
                                       const std::string& timestamp,
                                       std::optional<int> messageId)
{
    Response notification;
    notification.command = "incoming_message";
    notification.message = "";
    notification.sender = sender;
    notification.content = content;
    if (!timestamp.empty()) {
        notification.timestamp = timestamp;
    }
    if (messageId.has_value()) {
        notification.id = messageId;
    }
    queueFramedResponse(*this, protocolHandler.serializeResponse(notification));
}

void ClientState::pushFrontResponse(const std::string& message)
{
    pthread_mutex_lock(&sendMutex);
    sendQueue.push_front(message);
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
    sendQueue.pop_front();
    pthread_mutex_unlock(&sendMutex);
    return true;
}
