#include "SingleWorker.h"

#include "AuthManager.h"
#include "ClientState.h"
#include "MessageRouter.h"
#include "ProtocolHandler.h"

#include <arpa/inet.h>
#include <cerrno>
#include <cstdio>
#include <cstring>
#include <cstdint>
#include <string>
#include <fcntl.h>
#include <sys/epoll.h>
#include <sys/eventfd.h>
#include <sys/socket.h>
#include <unistd.h>

#include <ctime>
#include <memory>

namespace {
constexpr uint32_t kBaseEvents = EPOLLIN | EPOLLRDHUP | EPOLLERR;
constexpr std::size_t kFrameHeaderSize = sizeof(uint32_t);
constexpr uint32_t kMaxFrameSize = 64 * 1024;

uint32_t eventMaskHasWrite(bool hasPending)
{
    return hasPending ? static_cast<uint32_t>(kBaseEvents | EPOLLOUT) : kBaseEvents;
}

ssize_t recvNonBlocking(int fd, char* buffer, std::size_t size)
{
    return ::recv(fd, buffer, size, 0);
}

ssize_t sendNonBlocking(int fd, const char* buffer, std::size_t size)
{
#ifdef MSG_NOSIGNAL
    return ::send(fd, buffer, size, MSG_NOSIGNAL);
#else
    return ::send(fd, buffer, size, 0);
#endif
}
} // namespace

SingleWorker::SingleWorker(int id,
                           AuthManager& auth,
                           MessageRouter& router,
                           ProtocolHandler& protocol)
    : workerId(id)
    , epollFd(-1)
    , wakeupFd(-1)
    , running(false)
    , ready(false)
    , initFailed(false)
    , eventBuffer(64)
    , authManager(auth)
    , messageRouter(router)
    , protocolHandler(protocol)
{
}

SingleWorker::~SingleWorker()
{
    stop();
}

void SingleWorker::waitUntilReady()
{
    std::unique_lock<std::mutex> lock(readyMutex);
    readyCv.wait(lock, [this] { return ready.load(std::memory_order_acquire) || initFailed.load(std::memory_order_acquire); });
}

void SingleWorker::signalInitFailure()
{
    initFailed.store(true, std::memory_order_release);
    readyCv.notify_all();
}

void SingleWorker::start()
{
    if (running.load()) {
        return;
    }

    initFailed.store(false, std::memory_order_release);
    {
        std::lock_guard<std::mutex> lock(readyMutex);
        ready.store(false, std::memory_order_release);
    }

    epollFd = ::epoll_create1(EPOLL_CLOEXEC);
    if (epollFd == -1) {
        std::perror("epoll_create1");
        signalInitFailure();
        return;
    }

    wakeupFd = ::eventfd(0, EFD_NONBLOCK | EFD_CLOEXEC);
    if (wakeupFd == -1) {
        std::perror("eventfd");
        ::close(epollFd);
        epollFd = -1;
        signalInitFailure();
        return;
    }

    epoll_event wakeEvent {};
    wakeEvent.events = EPOLLIN;
    wakeEvent.data.fd = wakeupFd;
    if (::epoll_ctl(epollFd, EPOLL_CTL_ADD, wakeupFd, &wakeEvent) == -1) {
        std::perror("epoll_ctl add wakeup");
        ::close(wakeupFd);
        ::close(epollFd);
        wakeupFd = -1;
        epollFd = -1;
        signalInitFailure();
        return;
    }

    running.store(true, std::memory_order_release);
    {
        std::lock_guard<std::mutex> lock(readyMutex);
        ready.store(true, std::memory_order_release);
    }
    readyCv.notify_all();

    eventLoop();

    running.store(false, std::memory_order_release);
    {
        std::lock_guard<std::mutex> lock(readyMutex);
        ready.store(false, std::memory_order_release);
    }
    readyCv.notify_all();
}

void SingleWorker::stop()
{
    const bool wasRunning = running.exchange(false, std::memory_order_acq_rel);
    if (!wasRunning) {
        return;
    }
    wakeWorker();
}

void SingleWorker::assignClient(int clientFd)
{
    if (clientFd < 0) {
        return;
    }

    if (!running.load(std::memory_order_acquire) || !ready.load(std::memory_order_acquire)) {
        ::close(clientFd);
        return;
    }

    {
        std::lock_guard<std::mutex> lock(pendingMutex);
        pendingClients.push_back(clientFd);
    }
    wakeWorker();
}

void SingleWorker::wakeWorker()
{
    if (wakeupFd == -1) {
        return;
    }
    const uint64_t value = 1;
    ::write(wakeupFd, &value, sizeof(value));
}

void SingleWorker::notifyEvent(int clientFd)
{
    if (epollFd == -1) {
        return;
    }

    const auto it = clientStates.find(clientFd);
    if (it != clientStates.end()) {
        epoll_event ev {};
        ev.events = eventMaskHasWrite(true);
        ev.data.fd = clientFd;
        ::epoll_ctl(epollFd, EPOLL_CTL_MOD, clientFd, &ev);
    }

    wakeWorker();
}

void SingleWorker::eventLoop()
{
    while (running.load(std::memory_order_acquire)) {
        drainPendingClients();

        const int readyEvents = ::epoll_wait(epollFd, eventBuffer.data(), static_cast<int>(eventBuffer.size()), 1000);
        if (readyEvents == -1) {
            if (errno == EINTR) {
                continue;
            }
            std::perror("epoll_wait");
            break;
        }

        for (int i = 0; i < readyEvents; ++i) {
            const epoll_event& ev = eventBuffer[i];
            const int fd = ev.data.fd;

            if (fd == wakeupFd) {
                uint64_t value = 0;
                ::read(wakeupFd, &value, sizeof(value));
                continue;
            }

            if ((ev.events & (EPOLLERR | EPOLLRDHUP | EPOLLHUP)) != 0) {
                closeClient(fd);
                continue;
            }

            if (ev.events & EPOLLIN) {
                handleReadEvent(fd);
            }
            if (ev.events & EPOLLOUT) {
                handleWriteEvent(fd);
            }
        }
    }

    closePendingClients();

    for (auto& [fd, state] : clientStates) {
        if (state && state->isAuthenticated()) {
            messageRouter.unregisterClient(state->username());
            authManager.logoutUser(state->username());
        }
        if (epollFd != -1) {
            ::epoll_ctl(epollFd, EPOLL_CTL_DEL, fd, nullptr);
        }
        ::close(fd);
    }
    clientStates.clear();

    if (wakeupFd != -1) {
        ::close(wakeupFd);
        wakeupFd = -1;
    }
    if (epollFd != -1) {
        ::close(epollFd);
        epollFd = -1;
    }
}

void SingleWorker::drainPendingClients()
{
    std::vector<int> pending;
    {
        std::lock_guard<std::mutex> lock(pendingMutex);
        if (pendingClients.empty()) {
            return;
        }
        pending.swap(pendingClients);
    }

    for (int fd : pending) {
        if (!running.load(std::memory_order_acquire)) {
            ::close(fd);
            continue;
        }

        const int flags = ::fcntl(fd, F_GETFL, 0);
        if (flags != -1) {
            ::fcntl(fd, F_SETFL, flags | O_NONBLOCK);
        }

        epoll_event clientEvent {};
        clientEvent.events = kBaseEvents;
        clientEvent.data.fd = fd;
        if (::epoll_ctl(epollFd, EPOLL_CTL_ADD, fd, &clientEvent) == -1) {
            std::perror("epoll_ctl add client");
            ::close(fd);
            continue;
        }

        auto state = std::make_unique<ClientState>(this, fd);
        clientStates[fd] = std::move(state);
    }
}

void SingleWorker::closePendingClients()
{
    std::vector<int> pending;
    {
        std::lock_guard<std::mutex> lock(pendingMutex);
        pending.swap(pendingClients);
    }
    for (int fd : pending) {
        ::close(fd);
    }
}

ClientState* SingleWorker::getClient(int clientFd)
{
    const auto it = clientStates.find(clientFd);
    if (it == clientStates.end()) {
        return nullptr;
    }
    return it->second.get();
}

void SingleWorker::removeClient(int clientFd)
{
    clientStates.erase(clientFd);
}

void SingleWorker::handleReadEvent(int clientFd)
{
    ClientState* state = getClient(clientFd);
    if (!state) {
        closeClient(clientFd);
        return;
    }

    char buffer[4096];
    bool connectionOpen = true;

    while (connectionOpen) {
        const ssize_t bytes = recvNonBlocking(clientFd, buffer, sizeof(buffer));
        if (bytes > 0) {
            state->mutableRecvBuffer().append(buffer, static_cast<std::size_t>(bytes));
            state->updateActivity(::time(nullptr));
        } else if (bytes == 0) {
            connectionOpen = false;
            break;
        } else {
            if (errno == EAGAIN || errno == EWOULDBLOCK) {
                break;
            }
            connectionOpen = false;
            break;
        }
    }

    if (!connectionOpen) {
        closeClient(clientFd);
        return;
    }

    std::string& recvBuffer = state->mutableRecvBuffer();
    while (true) {
        if (recvBuffer.size() < kFrameHeaderSize) {
            break;
        }

        uint32_t netSize = 0;
        std::memcpy(&netSize, recvBuffer.data(), kFrameHeaderSize);
        const uint32_t payloadSize = ntohl(netSize);
        if (payloadSize > kMaxFrameSize) {
            closeClient(clientFd);
            return;
        }
        if (recvBuffer.size() < kFrameHeaderSize + payloadSize) {
            break;
        }

        std::string frame = recvBuffer.substr(kFrameHeaderSize, payloadSize);
        recvBuffer.erase(0, kFrameHeaderSize + payloadSize);
        const Command command = protocolHandler.parseCommand(frame);
        processCommand(*state, command);
    }
}

void SingleWorker::handleWriteEvent(int clientFd)
{
    ClientState* state = getClient(clientFd);
    if (!state) {
        closeClient(clientFd);
        return;
    }

    std::string message;
    while (state->popQueuedResponse(message)) {
        std::size_t totalSent = 0;
        while (totalSent < message.size()) {
            const ssize_t sent = sendNonBlocking(clientFd, message.data() + totalSent, message.size() - totalSent);
            if (sent > 0) {
                totalSent += static_cast<std::size_t>(sent);
                continue;
            }
            if (sent == -1 && (errno == EAGAIN || errno == EWOULDBLOCK)) {
                const std::string remainder = message.substr(totalSent);
                if (!remainder.empty()) {
                    state->pushFrontResponse(remainder);
                }
                return;
            }
            closeClient(clientFd);
            return;
        }
    }

    epoll_event ev {};
    ev.events = eventMaskHasWrite(false);
    ev.data.fd = clientFd;
    ::epoll_ctl(epollFd, EPOLL_CTL_MOD, clientFd, &ev);
}

void SingleWorker::processCommand(ClientState& state, const Command& command)
{
    Response response;

    switch (command.type) {
    case Command::Type::Register: {
        response.command = "register";
        const bool ok = authManager.registerUser(command.username, command.password);
        response.success = ok;
        response.message = ok ? "Registered" : "Registration failed";
        break;
    }
    case Command::Type::Login: {
        response.command = "login";
        if (authManager.loginUser(command.username, command.password)) {
            state.setAuthenticated(true);
            state.setUsername(command.username);
            messageRouter.registerClient(command.username, &state);
            messageRouter.deliverQueuedMessages(command.username, state);
            response.success = true;
            response.message = "Login successful";
        } else {
            response.success = false;
            response.message = "Invalid credentials";
        }
        break;
    }
    case Command::Type::SendMessage: {
        response.command = "send_message";
        if (!state.isAuthenticated()) {
            response.success = false;
            response.message = "Authentication required";
            break;
        }
        const std::string sender = state.username();
        if (command.recipient.empty()) {
            response.success = false;
            response.message = "Missing recipient";
            break;
        }
        if (!messageRouter.routeMessage(sender, command.recipient, command.content)) {
            response.success = false;
            response.message = "Delivery failed";
            break;
        }
        response.success = true;
        response.message = "Message queued";
        break;
    }
    case Command::Type::Logout: {
        response.command = "logout";
        if (state.isAuthenticated()) {
            messageRouter.unregisterClient(state.username());
            authManager.logoutUser(state.username());
            state.setAuthenticated(false);
            state.setUsername({});
            response.success = true;
            response.message = "Logged out";
        } else {
            response.success = false;
            response.message = "Not authenticated";
        }
        break;
    }
    case Command::Type::Unknown:
    default:
        response.command = "unknown";
        response.success = false;
        response.message = "Unknown command";
        break;
    }

    state.queueFramedResponse(protocolHandler.serializeResponse(response));
}

void SingleWorker::closeClient(int clientFd)
{
    ClientState* state = getClient(clientFd);
    if (state) {
        if (state->isAuthenticated()) {
            messageRouter.unregisterClient(state->username());
            authManager.logoutUser(state->username());
        }
    }

    if (epollFd != -1) {
        ::epoll_ctl(epollFd, EPOLL_CTL_DEL, clientFd, nullptr);
    }
    ::close(clientFd);
    removeClient(clientFd);
}
