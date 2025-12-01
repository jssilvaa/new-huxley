#include "WorkerThread.h"

#include "AuthManager.h"
#include "ClientState.h"
#include "MessageRouter.h"
#include "OfflineDelivery.h"
#include "ProtocolHandler.h"
#include "CryptoEngine.h"
#include "DatabaseEngine.h"
#include "StatusManager.h"

#include <arpa/inet.h>
#include <errno.h>
#include <fcntl.h>
#include <sys/epoll.h>
#include <sys/eventfd.h>
#include <sys/socket.h>
#include <unistd.h>

#include <chrono>
#include <cstdio>
#include <cstring>
#include <iostream>

namespace {
constexpr uint32_t kBaseEvents = EPOLLIN | EPOLLRDHUP | EPOLLERR; // EPOLLERR not required to be in the base events mask since it's always reported
constexpr std::size_t kFrameHeaderSize = sizeof(uint32_t);
constexpr uint32_t kMaxFrameSize = 64 * 1024; // 64 KiB guardrail

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
    return ::send(fd, buffer, size, MSG_NOSIGNAL); // prevent SIGPIPE on Linux on broken pipe / closed socket
#else
    return ::send(fd, buffer, size, 0);
#endif
}

} // namespace
    
/* 
WorkerThread Constructor 
Intializes the worker thread's necessary components 
Establishes connections between the concurrency layer and the business logic layer 
Other class instances are passed by reference, they're singletons managed by the HuxleyServer 

Here, we also setup the **listenFd** and **wakeupFd**, file descriptors used for 
event notification from (1) sockets and (2) inter-thread wakeup signals respectively (from the main thread)
The epoll instance is created to monitor these file descriptors for events

The *clientsMutex* is initialized to protect access to the *clientStates* map, ensuring thread-safe operations
Other threads may write to the outbound buffer of a ClientState instance, so proper synchronization is crucial
Clients are managed here and only here, which is why they're also std::unique_ptr<ClientState> instances
 */
WorkerThread::WorkerThread(int id,
                           AuthManager& auth,
                           MessageRouter& router,
                           ProtocolHandler& protocol,
                           StatusManager& status,
                           Database& db,
                           CryptoEngine& crypto)
    : workerId(id)
    , epollFd(-1)
    , wakeupFd(-1)
    , running(false)
    , threadHandle(0)
    , authManager(auth)
    , messageRouter(router)
    , protocolHandler(protocol)
    , statusManager(status)
    , database(db)
    , cryptoEngine(crypto)
    , eventBuffer(64)
{

    pthread_mutex_init(&clientsMutex, nullptr);
}

// Destructor, cleans up resources
WorkerThread::~WorkerThread()
{
    stop();
    pthread_mutex_destroy(&clientsMutex);
}

// Start the worker thread's event loop
// Sets up epoll and wakeup mechanisms
void WorkerThread::start()
{
    // if running, return early
    if (running.load()) {
        return;
    }

    // EPOLL_CLOEXEC to close on exec, so child processes don't inherit fds
    // even though we don't expect to fork/exec in this server
    epollFd = ::epoll_create1(EPOLL_CLOEXEC);
    if (epollFd == -1) {
        std::perror("epoll_create1");
        return;
    }

    // Create eventfd for wakeup notifications
    // set EFD_NONBLOCK for no subsequent blocking during I/O operations
    // set EFD_CLOEXEC to close on exec (again, it isn't expected to fork/exec)
    wakeupFd = ::eventfd(0, EFD_NONBLOCK | EFD_CLOEXEC);
    if (wakeupFd == -1) {
        std::perror("eventfd");
        ::close(epollFd);
        epollFd = -1;
        return;
    }

    epoll_event wakeEvent{};
    wakeEvent.events = EPOLLIN;   // events is a bit mask of events, implemented in the kernel as __poll_t (instead of __u32)
    wakeEvent.data.fd = wakeupFd; // data is a union, here we use fd to store the wakeupFd. kernel implements this as a __u64 
    if (::epoll_ctl(epollFd, EPOLL_CTL_ADD, wakeupFd, &wakeEvent) == -1) {
        std::perror("epoll_ctl add wakeup");
        ::close(wakeupFd);
        ::close(epollFd);
        wakeupFd = -1;
        epollFd = -1;
        return;
    }

    running.store(true);
    if (pthread_create(&threadHandle, nullptr, &WorkerThread::threadEntry, this) != 0) {
        std::perror("pthread_create");
        running.store(false);
        ::close(wakeupFd);
        ::close(epollFd);
        wakeupFd = -1;
        epollFd = -1;
    }
}

void WorkerThread::stop()
{
    if (!running.exchange(false)) {
        return;
    }

    // wakes up the event loop if it's blocked in epoll_wait
    if (wakeupFd != -1) {
        const uint64_t value = 1;
        ::write(wakeupFd, &value, sizeof(value));
    }

    // wait for the thread to finish
    if (threadHandle) {
        pthread_join(threadHandle, nullptr);
        threadHandle = 0;
    }

    // close all client connections in the clientStates unordered_map
    pthread_mutex_lock(&clientsMutex);
    for (auto& [fd, state] : clientStates) {
        if (state) {
            ::close(fd);
        }
    }
    clientStates.clear();
    pthread_mutex_unlock(&clientsMutex);

    // finally, close the wakeup and epoll file descriptors
    if (wakeupFd != -1) {
        ::close(wakeupFd);
        wakeupFd = -1;
    }
    if (epollFd != -1) {
        ::close(epollFd);
        epollFd = -1;
    }
}

// assign client to this worker thread
void WorkerThread::assignClient(int clientFd)
{

    // client does not exist
    if (clientFd < 0) {
        return;
    }

    // set client socket to non-blocking mode
    // first get current flags
    // then append O_NONBLOCK flag
    const int flags = ::fcntl(clientFd, F_GETFL, 0);
    ::fcntl(clientFd, F_SETFL, flags | O_NONBLOCK);

    epoll_event clientEvent{};
    clientEvent.events = kBaseEvents;
    clientEvent.data.fd = clientFd;
    if (::epoll_ctl(epollFd, EPOLL_CTL_ADD, clientFd, &clientEvent) == -1) {
        std::perror("epoll_ctl add client");
        ::close(clientFd);
        return;
    }

    auto state = std::make_unique<ClientState>(this, clientFd, protocolHandler);
    pthread_mutex_lock(&clientsMutex);
    clientStates[clientFd] = std::move(state);
    pthread_mutex_unlock(&clientsMutex);

    notifyEvent(clientFd);
}

void WorkerThread::notifyEvent(int clientFd)
{
    if (epollFd == -1) {
        return;
    }

    pthread_mutex_lock(&clientsMutex);
    const bool exists = clientStates.find(clientFd) != clientStates.end();
    pthread_mutex_unlock(&clientsMutex);

    if (exists) {
        epoll_event ev{};
        ev.events = eventMaskHasWrite(true);
        ev.data.fd = clientFd;
        ::epoll_ctl(epollFd, EPOLL_CTL_MOD, clientFd, &ev);
    }

    if (wakeupFd != -1) {
        const uint64_t value = 1;
        ::write(wakeupFd, &value, sizeof(value));
    }
}

// Entry point for the worker thread
void* WorkerThread::threadEntry(void* arg)
{
    auto* worker = static_cast<WorkerThread*>(arg);
    worker->eventLoop();
    return nullptr;
}

// Main event loop for the worker thread
void WorkerThread::eventLoop()
{
    while (running.load()) { // ; check if running 
        const int ready = ::epoll_wait(epollFd, eventBuffer.data(), static_cast<int>(eventBuffer.size()), -1); // no timeout, wait blocks thread but cpu efficient 
        if (ready == -1) {
            if (errno == EINTR) {
                continue;
            }
            std::perror("epoll_wait");
            break;
        }

        for (int i = 0; i < ready; ++i) {
            const epoll_event& event = eventBuffer[i];
            const int fd = event.data.fd;

            if (fd == wakeupFd) {
                uint64_t value = 0;
                ::read(wakeupFd, &value, sizeof(value));
                continue;
            }

            // error or connection closed by peer 
            if ((event.events & (EPOLLERR | EPOLLRDHUP | EPOLLHUP)) != 0) {
                closeClient(fd);
                continue;
            }

            // socket is ready for read event 
            if (event.events & EPOLLIN) {
                handleReadEvent(fd);
            } // socket is ready for write event
            if (event.events & EPOLLOUT) {
                handleWriteEvent(fd);
            }
        }
    }
}

// no mutex required here, called from within the event loop thread only
ClientState* WorkerThread::getClient(int clientFd)
{
    //pthread_mutex_lock(&clientsMutex); // lock the mutex to protect access to clientStates
    ClientState* state = nullptr;
    const auto it = clientStates.find(clientFd);
    if (it != clientStates.end()) {
        state = it->second.get();
    }
    //pthread_mutex_unlock(&clientsMutex);
    return state;
}

void WorkerThread::removeClient(int clientFd)
{
    pthread_mutex_lock(&clientsMutex);
    clientStates.erase(clientFd);
    pthread_mutex_unlock(&clientsMutex);
}

void WorkerThread::handleReadEvent(int clientFd)
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

void WorkerThread::handleWriteEvent(int clientFd)
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
                state->queueResponse(remainder);
                return;
            }
            closeClient(clientFd);
            return;
        }
    }

    epoll_event ev{};
    ev.events = eventMaskHasWrite(false);
    ev.data.fd = clientFd;
    ::epoll_ctl(epollFd, EPOLL_CTL_MOD, clientFd, &ev);
}

void WorkerThread::processCommand(ClientState& state, const Command& command)
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
        if (state.isAuthenticated()) {
            response.success = false;
            response.message = "Already logged in!";
        }
        else if (authManager.loginUser(command.username, command.password)) {
            // check if user is already logged in elsewhere?
            if (messageRouter.isRegistered(command.username)) {
                response.success = false;
                response.message = "User already logged in elsewhere";
                break;
            }
            state.setAuthenticated(true);
            state.setUsername(command.username);
            messageRouter.registerClient(command.username, &state);
            // Flush queued messages via OfflineDelivery helper once auth succeeds
            deliverOfflineMessages(database, cryptoEngine, command.username, state);
            statusManager.setState(StatusManager::State::Operational);
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
            const std::string username = state.username();
            messageRouter.unregisterClient(username);
            database.logActivity("INFO", "User logout: " + username);
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

    state.queueProtocolResponse(response);
}

void WorkerThread::closeClient(int clientFd)
{
    ClientState* state = getClient(clientFd);
    if (state) {
        if (state->isAuthenticated()) {
            const std::string username = state->username();
            messageRouter.unregisterClient(username);
            database.logActivity("INFO", "User disconnected: " + username);
        }
    }

    ::epoll_ctl(epollFd, EPOLL_CTL_DEL, clientFd, nullptr);
    ::close(clientFd);
    removeClient(clientFd);
}
