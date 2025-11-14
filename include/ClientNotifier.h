#pragma once

// Lightweight interface that lets ClientState signal its owner worker
// when new outbound data is available.
class ClientNotifier {
public:
    virtual ~ClientNotifier() = default;
    virtual void notifyEvent(int clientFd) = 0;
};
