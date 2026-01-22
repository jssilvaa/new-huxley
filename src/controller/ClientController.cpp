#include "ClientController.h"
#include "model/ChatHistoryModel.h"
#include "service/MessageService.h"
#include <algorithm>
#include <QSettings>
#include <QtGlobal>
#include <qcontainerfwd.h>
#include <qjsondocument.h>
#include <qjsonobject.h>
#include <qnamespace.h>
#include <qobject.h>

namespace {
    // local helpers
    static QString defaultHost() {
#ifdef Q_OS_ANDROID
        // android emulator host
        return QStringLiteral("10.0.2.2");
#else
        // desktop loopback host
        return QStringLiteral("127.0.0.1");
#endif
    }

    static int defaultPort() {
        // default port
        return 8080;
    }

    static int sanitizePort(int port) {
        // sanitize port range
        if (port < 1 || port > 65535) return defaultPort();
        return port;
    }

    static QString normalizeTimestamp(const QString& isoTs) {
        // normalize timestamp to local time
        auto dt = QDateTime::fromString(isoTs, Qt::ISODateWithMs);
        if (!dt.isValid()) dt = QDateTime::fromString(isoTs, Qt::ISODate);
        if (!dt.isValid()) return isoTs; 
        return dt.toLocalTime().toString("yyyy-MM-dd HH:mm:ss");
    }

    static qint64 timestampToMs(const QString& ts) {
        // parse timestamp for sort
        QDateTime dt = QDateTime::fromString(ts, "yyyy-MM-dd HH:mm:ss");
        if (!dt.isValid()) dt = QDateTime::fromString(ts, Qt::ISODateWithMs);
        if (!dt.isValid()) dt = QDateTime::fromString(ts, Qt::ISODate);
        if (!dt.isValid()) return 0;
        return dt.toMSecsSinceEpoch();
    }
}

ClientController::ClientController(QObject* parent) 
    : QObject(parent), m_msgservice(&m_proto, this) 
{
    // load connection settings
    QSettings settings;
    m_serverHost = settings.value("connection/host", defaultHost()).toString().trimmed();
    if (m_serverHost.isEmpty()) m_serverHost = defaultHost();
    m_serverPort = sanitizePort(settings.value("connection/port", defaultPort()).toInt());

    // presence ping timer
    const int t_ping_ms = 5000; 
    m_presenceTimer.setInterval(t_ping_ms);
    m_presenceTimer.setTimerType(Qt::CoarseTimer); 

    connect(&m_presenceTimer, &QTimer::timeout, this, [this] {
        if (m_connected && m_authenticated) refreshUsers(); 
    });

    // signal wiring for protocol events
    connect(&m_proto, &ProtocolClient::connected, this, &ClientController::onConnected);
    connect(&m_proto, &ProtocolClient::disconnected, this, &ClientController::onDisconnected);
    connect(&m_proto, &ProtocolClient::errorOcurred, this, &ClientController::onError); 
    connect(&m_msgservice, &MessageService::loginResult, this, &ClientController::onLoginResult);
    connect(&m_msgservice, &MessageService::registerResult, this, &ClientController::onRegisterResult);
    connect(&m_msgservice, &MessageService::usersReceived, this, &ClientController::onUsersReceived);
    connect(&m_msgservice, &MessageService::historyReceived, this, &ClientController::onHistoryReceived);
    connect(&m_msgservice, &MessageService::incomingMessage, this, &ClientController::onIncomingMessage);
    connect(&m_msgservice, &MessageService::sendMessageResponse, this, &ClientController::onSendMessageResponse);

    // mark ready on next tick
    QTimer::singleShot(0, this, [this] { m_ready = true; }); 
}

QObject* ClientController::messageService() {
    // service object for qml
    return &m_msgservice;
}

QObject* ClientController::contacts() {
    // contacts model for qml
    return &m_contacts;
}

QObject* ClientController::chat() {
    // chat model for qml
    return &m_chat;
}

QObject* ClientController::contactsProxy() {
    // contacts proxy model
    return &m_contactsProxy;
}

void ClientController::refreshUsers() {
    // request user list
    m_msgservice.listUsers();
}

void ClientController::selectPeer(const QString& peer) {
    // switch active peer
    if (!m_authenticated) {
        emit error("Not authenticated");
        return;
    }

    if (peer == m_currentPeer) return;
    m_currentPeer = peer;
    m_contacts.clearUnread(peer);
    m_chat.resetHistory({});

    // load history and notify
    m_msgservice.getHistory(peer);
    emit currentPeerChanged();
    emit currentPeerOnlineChanged();
}

void ClientController::sendMessage(const QString& content) {
    // send message to peer
    if (!m_authenticated) return;
    if (m_currentPeer.isEmpty()) {
        emit error("No peer selected");
        return;
    }

    emit messageSubmitted();
    m_msgservice.sendMessage(m_currentPeer, content);
}

int ClientController::unreadCount(const QString& user) const {
    // unread count proxy
    return m_contacts.unreadCount(user);
}

bool ClientController::hasUnread(const QString& user) const {
    // unread status proxy
    return m_contacts.unreadCount(user) > 0;
}

void ClientController::setFocusContacts(bool v) {
    // set contacts focus
    if (m_focusContacts == v) return;
    m_focusContacts = v;
    emit focusContactsChanged();
}

bool ClientController::connected() const {
    // connected state getter
    return m_connected;
}

bool ClientController::authenticated() const {
    // authenticated state getter
    return m_authenticated;
}

bool ClientController::registering() const {
    // registering state getter
    return m_registering;
}

bool ClientController::hasPeer() const {
    // peer presence check
    return !m_currentPeer.isEmpty();
}

QString ClientController::currentPeer() const {
    // current peer getter
    return m_currentPeer;
}

bool ClientController::focusContacts() const {
    // contacts focus getter
    return m_focusContacts;
}

QString ClientController::serverHost() const {
    // server host getter
    return m_serverHost;
}

int ClientController::serverPort() const {
    // server port getter
    return m_serverPort;
}

void ClientController::start() {
    // start connection
    if (m_connected || m_connecting) return; 
    m_connecting = true; 
    m_proto.connectToHost(m_serverHost, quint16(m_serverPort));
}

void ClientController::shutdown() {
    // shutdown guard
    m_ready = false;
    m_reconnectPending = false;
    m_connecting = false;

    m_presenceTimer.stop();

    // disconnect signal handlers
    disconnect(&m_msgservice, nullptr, this, nullptr);
    disconnect(&m_proto, nullptr, this, nullptr);

    m_proto.shutdown();
}

void ClientController::reconnect() {
    // reconnect flow
    if (m_connecting) return;
    if (m_connected) {
        m_reconnectPending = true;
        m_proto.disconnectFromHost();
        return;
    }
    start();
}

void ClientController::setServerHost(const QString& host) {
    // normalize and persist host
    const QString next = host.trimmed();
    const QString resolved = next.isEmpty() ? defaultHost() : next;
    if (resolved == m_serverHost) return;
    m_serverHost = resolved;

    QSettings settings;
    settings.setValue("connection/host", m_serverHost);
    emit serverHostChanged();
}

void ClientController::setServerPort(int port) {
    // sanitize and persist port
    const int next = sanitizePort(port);
    if (next == m_serverPort) return;
    m_serverPort = next;

    QSettings settings;
    settings.setValue("connection/port", m_serverPort);
    emit serverPortChanged();
}

void ClientController::login(const QString& user, const QString& pass) {
    // send login request
    if (!m_connected) {
        emit error("Not connected"); 
        return; 
    }

    QJsonObject obj{
        {"type", "LOGIN"}, 
        {"username", user}, 
        {"password", pass}
    };

    m_proto.sendCommand(obj);
    m_pendingUsername = user;
}

void ClientController::registerUser(const QString& user, const QString& pass) {
    // send register request
    if (!m_connected) {
        emit error("Not connected"); 
        return; 
    }

    QJsonObject obj{
        {"type", "REGISTER"},
        {"username", user},
        {"password", pass} 
    };

    m_proto.sendCommand(obj);
}

void ClientController::showRegister() {
    // set register mode
    if (m_registering) return;
    m_registering = true; 
    emit registeringChanged(); 
}

void ClientController::showLogin() {
    // set login mode
    if (!m_registering) return; 
    m_registering = false; 
    emit registeringChanged(); 
}

void ClientController::onConnected() {
    // ctor ready guard
    if (!m_ready) return; 
    
    m_connecting = false; 
    m_connected = true;
    emit connectedChanged(); 
    emit toast("Connected"); // connection toast
}

void ClientController::onDisconnected() {
    // ctor ready guard
    if (!m_ready) return; 

    const bool shouldReconnect = m_reconnectPending;
    m_reconnectPending = false;
    
    // reset connection state
    m_connecting = false; 
    m_connected = false; 
    m_authenticated = false;
    m_registering = false; 

    // clear transient state
    if (!m_pendingUsername.isNull()) m_pendingUsername.clear(); 
    if (!m_username.isNull()) m_username.clear(); // clear cached username
    if (!m_currentPeer.isNull()) m_currentPeer.clear();
    m_prefetchedPreview.clear(); 
    
    // stop presence timer
    m_presenceTimer.stop();

    // clear chat history
    m_chat.resetHistory({});
    
    // emit state signals
    emit connectedChanged();
    emit authenticatedChanged(); 
    emit registeringChanged();
    emit currentPeerChanged(); 
    emit currentPeerOnlineChanged(); 

    // handle reconnect
    if (!shouldReconnect) emit error("Disconnected");
    if (shouldReconnect) start();
}

void ClientController::onError(QString msg) {
    // forward error signal
    emit error(msg); 
}

bool ClientController::currentPeerOnline() const {
    // scan contacts list
    if (m_currentPeer.isEmpty())
        return false;

    for (const auto& c : m_contacts.contacts()) {
        if (c.username == m_currentPeer)
            return c.online;
    }
    return false;
}

void ClientController::onLoginResult(bool ok, const QString& msg) {
    // ignore early signals
    if (!m_ready) return; 

    // login success
    if (ok) {
        // set auth state
        m_authenticated = true; 
        m_username = m_pendingUsername; 

        // refresh users and start presence
        m_prefetchedPreview.clear(); 
        refreshUsers(); 
        m_presenceTimer.start(); // start presence timer

        // emit auth changed
        emit authenticatedChanged(); 
        emit toast("Logged in"); 
    } 
    // login failure
    else {
        // clear pending username
        if (!m_pendingUsername.isEmpty()) m_pendingUsername.clear();
        emit error(msg); 
    }
}

void ClientController::onRegisterResult(bool ok, const QString& msg) {
    // ignore early signals
    if (!m_ready) return; 

    // register success
    if (ok) {
        m_registering = false; 
        emit registeringChanged(); // emit register change
        emit toast("Registered successfuly"); 
    } 
    // register failure
    else {
        emit error(msg); 
    }
}

void ClientController::onUsersReceived(const QVector<QJsonObject>& users) {
    // ignore early signals
    if (!m_ready) return; 

    // build contact snapshot
    QVector<Contact> snap; 
    snap.reserve(users.size()); 

    for (const auto& u : users) {
        Contact c;
        c.username = u.value("username").toString(); 
        if (c.username == m_username) continue; // skip current user
        c.online = u.value("online").toBool(); 
        snap.push_back(c); 
    }

    // init contacts proxy
    if (!m_contactsProxy.sourceModel()) {
        m_contactsProxy.setSourceModel(&m_contacts); 
        m_contactsProxy.sort(0); 
    }

    // track new users
    QVector<QString> toPrefetch;
    toPrefetch.reserve(snap.size()); 

    for (const auto& c : snap) {
        // new contact marker
        if (!m_prefetchedPreview.contains(c.username)) {
            toPrefetch.push_back(c.username);
            m_prefetchedPreview.insert(c.username); 
        }
    }
    m_contacts.mergePresence(snap);
    emit currentPeerOnlineChanged();
    
    // prefetch history for new users
    for (const auto& user : toPrefetch) {
        m_msgservice.getHistory(user, 1);
    } 
}

void ClientController::onHistoryReceived(const QString& peer, const QVector<QJsonObject>& msgs) {
    // ignore early signals
    if (!m_ready) return; 
    
    // deserialize message payload
    QVector<ChatMessage> out; 
    out.reserve(msgs.size()); 

    for (const auto& m : msgs) {
        ChatMessage cm;
        cm.sender = m.value("from").toString();
        cm.content = m.value("content").toString(); 
        cm.timestamp = normalizeTimestamp(
         m.value("timestamp").toString()   
        );
        cm.isOwn = (cm.sender == m_username); 

        out.push_back(cm);
    }
    std::sort(out.begin(), out.end(), [](const ChatMessage& a, const ChatMessage& b) {
        return timestampToMs(a.timestamp) < timestampToMs(b.timestamp);
    });

    // update last message preview
    if (!out.isEmpty()) {
        const auto& last = out.last(); 
        m_contacts.updateLastMessage(peer, last.content, last.timestamp, false);
    }

    // ignore non active chat
    if (peer != m_currentPeer) return; 

    if (out.size() == 1 && m_chat.rowCount() > 0) {
        const auto& single = out.front();
        const int lastRow = m_chat.rowCount() - 1;
        const QModelIndex lastIdx = m_chat.index(lastRow);
        const QString lastSender = m_chat.data(lastIdx, ChatHistoryModel::SenderRole).toString();
        const QString lastContent = m_chat.data(lastIdx, ChatHistoryModel::ContentRole).toString();
        const QString lastTimestamp = m_chat.data(lastIdx, ChatHistoryModel::TimestampRole).toString();

        if (lastSender == single.sender && lastContent == single.content && lastTimestamp == single.timestamp) {
            return;
        }

        m_chat.appendMessage(single);
        return;
    }

    emit clearChat(); // reset chat view
    m_chat.resetHistory(out);
    emit showChat(); 
}

void ClientController::onIncomingMessage(const QJsonObject& m) {
    // ignore early signals
    if (!m_ready) return; 

    // parse incoming message
    const QString sender = m.value("sender").toString();
    const QString peer = (sender == m_username) ? m.value("recipient").toString() : sender; 
    const QString content = m.value("content").toString();
    const QString ts = normalizeTimestamp(
        m.value("timestamp").toString()
    );
    const bool isPeerCurrent = (peer == m_currentPeer); 

    m_contacts.updateLastMessage(peer, content, ts, !isPeerCurrent); 

    // toast preview for inactive chat
    if (!isPeerCurrent) {
        QString notif = (content.size() > 40) ? content.left(37).append("...") : content; 

        emit toast(QString("%1: %2").arg(peer, notif));
        return; 
    }

    // append to chat model
    ChatMessage cm; 
    cm.sender = sender;
    cm.content = content; 
    cm.timestamp = ts; 
    cm.isOwn = (sender == m_username);
    m_chat.appendMessage(cm); 
}

void ClientController::onSendMessageResponse(bool ok, const QString& msg) {
    // ignore early signals
    if (!m_ready) return;

    // fetch latest message
    if (ok) {
        m_msgservice.getHistory(m_currentPeer, 1); 
    } else { 
        emit error(QString("Send failed: %1").arg(msg)); 
        // future message status
    }
}
