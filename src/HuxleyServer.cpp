#include "HuxleyServer.h"

#include "AuthManager.h"
#include "ClientState.h"
#include "CryptoEngine.h"
#include "DatabaseEngine.h"
#include "MessageRouter.h"
#include "ProtocolHandler.h"
#include "StatusManager.h"
#include "WorkerThread.h"

#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>

#include <cstring>
#include <errno.h>
#include <iostream>
#include <thread>

namespace {
constexpr const char* kDefaultDatabasePath = "huxley.db";

bool setReusable(int fd)
{
    int opt = 1;
    return ::setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) == 0;
}
} // namespace

HuxleyServer::HuxleyServer()
    : listenFd(-1)
    , running(false)
    , acceptThread(0)
    , queueMutex()
    , queueCond()
    , socketQueue()
    , workerThreads()
    , authManager()
    , messageRouter()
    , statusManager()
    , cryptoEngine()
    , protocolHandler()
    , database()
    , databasePath(kDefaultDatabasePath)
    , nextWorkerIndex(0)
{
    pthread_mutex_init(&queueMutex, nullptr);
    pthread_cond_init(&queueCond, nullptr);
}

HuxleyServer::~HuxleyServer()
{
    stop();
    pthread_cond_destroy(&queueCond);
    pthread_mutex_destroy(&queueMutex);
}

bool HuxleyServer::start(int port)
{
    if (running.load()) {
        return true;
    }

    if (!initializeServices(port)) {
        return false;
    }

    const auto hardwareThreads = std::max<std::size_t>(1, std::thread::hardware_concurrency());
    startWorkerPool(hardwareThreads);

    running.store(true);
    if (pthread_create(&acceptThread, nullptr, &HuxleyServer::acceptThreadEntry, this) != 0) {
        std::perror("pthread_create");
        running.store(false);
        stopWorkerPool();
        shutdownServices();
        return false;
    }

    statusManager->setState(StatusManager::State::Operational);
    return true;
}

void HuxleyServer::stop()
{
    if (!running.exchange(false)) {
        return;
    }

    if (listenFd != -1) {
        ::shutdown(listenFd, SHUT_RDWR);
        ::close(listenFd);
        listenFd = -1;
    }

    if (acceptThread) {
        pthread_join(acceptThread, nullptr);
        acceptThread = 0;
    }

    pthread_mutex_lock(&queueMutex);
    while (!socketQueue.empty()) {
        ::close(socketQueue.front());
        socketQueue.pop();
    }
    pthread_mutex_unlock(&queueMutex);

    stopWorkerPool();
    shutdownServices();
}

bool HuxleyServer::initializeServices(int port)
{
    database = std::make_unique<Database>(databasePath);
    if (!database->isOpen()) {
        std::cerr << "Failed to open database" << std::endl;
        database.reset();
        return false;
    }

    cryptoEngine = std::make_unique<CryptoEngine>();
    protocolHandler = std::make_unique<ProtocolHandler>();
    statusManager = std::make_unique<StatusManager>();
    authManager = std::make_unique<AuthManager>(*database);
    messageRouter = std::make_unique<MessageRouter>(*database, *cryptoEngine);

    listenFd = ::socket(AF_INET, SOCK_STREAM, 0);
    if (listenFd == -1) {
        std::perror("socket");
        return false;
    }

    int n = setReusable(listenFd);
    if (n == 0) {
        std::perror("setsockopt");
        ::close(listenFd);
        listenFd = -1;
        return false;
    }

    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = htonl(INADDR_ANY);
    addr.sin_port = htons(static_cast<uint16_t>(port));

    if (::bind(listenFd, reinterpret_cast<sockaddr*>(&addr), sizeof(addr)) == -1) {
        std::perror("bind");
        ::close(listenFd);
        listenFd = -1;
        return false;
    }

    if (::listen(listenFd, SOMAXCONN) == -1) {
        std::perror("listen");
        ::close(listenFd);
        listenFd = -1;
        return false;
    }

    statusManager->setState(StatusManager::State::Booting);
    return true;
}

void HuxleyServer::startWorkerPool(std::size_t threadCount)
{
    workerThreads.reserve(threadCount);
    for (std::size_t i = 0; i < threadCount; ++i) {
        auto worker = std::make_unique<WorkerThread>(static_cast<int>(i),
                                 *authManager,
                                 *messageRouter,
                                 *protocolHandler,
                                 *statusManager,
                                 *database,
                                 *cryptoEngine);
        worker->start();
        workerThreads.emplace_back(std::move(worker));
    }
}

void HuxleyServer::stopWorkerPool()
{
    for (auto& worker : workerThreads) {
        if (worker) {
            worker->stop();
        }
    }
    workerThreads.clear();
    nextWorkerIndex = 0;
}

void HuxleyServer::shutdownServices()
{
    messageRouter.reset();
    authManager.reset();
    cryptoEngine.reset();
    protocolHandler.reset();
    statusManager.reset();
    if (database) {
        database.reset();
    }
}

void* HuxleyServer::acceptThreadEntry(void* arg)
{
    auto* server = static_cast<HuxleyServer*>(arg);
    server->acceptLoop();
    return nullptr;
}

void HuxleyServer::acceptLoop()
{
    while (running.load()) {
        sockaddr_in clientAddr{};
        socklen_t addrLen = sizeof(clientAddr);
        const int clientFd = ::accept(listenFd, reinterpret_cast<sockaddr*>(&clientAddr), &addrLen);
        if (clientFd == -1) {
            if (errno == EINTR) {
                continue;
            }
            if (!running.load()) {
                break;
            }
            std::perror("accept");
            continue;
        }

        pthread_mutex_lock(&queueMutex);
        socketQueue.push(clientFd);
        pthread_mutex_unlock(&queueMutex);
        pthread_cond_signal(&queueCond);

        dispatchPendingClients();
    }
}

void HuxleyServer::dispatchPendingClients()
{
    while (true) {
        int clientFd = -1;

        pthread_mutex_lock(&queueMutex);
        if (!socketQueue.empty()) {
            clientFd = socketQueue.front();
            socketQueue.pop();
        }
        pthread_mutex_unlock(&queueMutex);

        if (clientFd == -1) {
            break;
        }

        if (workerThreads.empty()) {
            ::close(clientFd);
            continue;
        }

        WorkerThread* worker = workerThreads[nextWorkerIndex].get();
        nextWorkerIndex = (nextWorkerIndex + 1) % workerThreads.size();
        worker->assignClient(clientFd);
    }
}
