// src/controller/ClientController.cpp
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
    static QString defaultHost() {
#ifdef Q_OS_ANDROID
        return QStringLiteral("10.0.2.2");
#else
        return QStringLiteral("127.0.0.1");
#endif
    }

    static int defaultPort() {
        return 8080;
    }

    static int sanitizePort(int port) {
        if (port < 1 || port > 65535) return defaultPort();
        return port;
    }

    static QString normalizeTimestamp(const QString& isoTs) {
        auto dt = QDateTime::fromString(isoTs, Qt::ISODateWithMs);
        if (!dt.isValid()) dt = QDateTime::fromString(isoTs, Qt::ISODate);
        if (!dt.isValid()) return isoTs; 
        return dt.toLocalTime().toString("yyyy-MM-dd HH:mm:ss");
    }

    static qint64 timestampToMs(const QString& ts) {
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
    QSettings settings;
    m_serverHost = settings.value("connection/host", defaultHost()).toString().trimmed();
    if (m_serverHost.isEmpty()) m_serverHost = defaultHost();
    m_serverPort = sanitizePort(settings.value("connection/port", defaultPort()).toInt());

    // setup timer and server pings 
    const int t_ping_ms = 5000; 
    m_presenceTimer.setInterval(t_ping_ms);
    m_presenceTimer.setTimerType(Qt::CoarseTimer); 

    connect(&m_presenceTimer, &QTimer::timeout, this, [this] {
        if (m_connected && m_authenticated) refreshUsers(); 
    });

    // connection hshkes, auth hshkes, users lists, history lists 
    connect(&m_proto, &ProtocolClient::connected, this, &ClientController::onConnected);
    connect(&m_proto, &ProtocolClient::disconnected, this, &ClientController::onDisconnected);
    connect(&m_proto, &ProtocolClient::errorOcurred, this, &ClientController::onError); 
    connect(&m_msgservice, &MessageService::loginResult, this, &ClientController::onLoginResult);
    connect(&m_msgservice, &MessageService::registerResult, this, &ClientController::onRegisterResult);
    connect(&m_msgservice, &MessageService::usersReceived, this, &ClientController::onUsersReceived);
    connect(&m_msgservice, &MessageService::historyReceived, this, &ClientController::onHistoryReceived);
    connect(&m_msgservice, &MessageService::incomingMessage, this, &ClientController::onIncomingMessage);
    connect(&m_msgservice, &MessageService::sendMessageResponse, this, &ClientController::onSendMessageResponse);

    // fire ready a cycle later
    QTimer::singleShot(0, this, [this] { m_ready = true; }); 
}

void ClientController::start() {
    if (m_connected || m_connecting) return; 
    m_connecting = true; 
    m_proto.connectToHost(m_serverHost, quint16(m_serverPort));
}

void ClientController::reconnect() {
    if (m_connecting) return;
    if (m_connected) {
        m_reconnectPending = true;
        m_proto.disconnectFromHost();
        return;
    }
    start();
}

void ClientController::setServerHost(const QString& host) {
    const QString next = host.trimmed();
    const QString resolved = next.isEmpty() ? defaultHost() : next;
    if (resolved == m_serverHost) return;
    m_serverHost = resolved;

    QSettings settings;
    settings.setValue("connection/host", m_serverHost);
    emit serverHostChanged();
}

void ClientController::setServerPort(int port) {
    const int next = sanitizePort(port);
    if (next == m_serverPort) return;
    m_serverPort = next;

    QSettings settings;
    settings.setValue("connection/port", m_serverPort);
    emit serverPortChanged();
}

void ClientController::login(const QString& user, const QString& pass) {
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
    if (m_registering) return;
    m_registering = true; 
    emit registeringChanged(); 
}

void ClientController::showLogin() {
    if (!m_registering) return; 
    m_registering = false; 
    emit registeringChanged(); 
}

void ClientController::onConnected() {
    // gate before ctor finishes
    if (!m_ready) return; 
    
    m_connecting = false; 
    m_connected = true;
    emit connectedChanged(); 
    emit toast("Connected"); 
}

void ClientController::onDisconnected() {
    // gate before ctor finishes
    if (!m_ready) return; 

    const bool shouldReconnect = m_reconnectPending;
    m_reconnectPending = false;
    
    // reset flags
    m_connecting = false; 
    m_connected = false; 
    m_authenticated = false;
    m_registering = false; 

    // reset object fields
    if (!m_pendingUsername.isNull()) m_pendingUsername.clear(); 
    if (!m_username.isNull()) m_username.clear();
    if (!m_currentPeer.isNull()) m_currentPeer.clear();
    m_prefetchedPreview.clear(); 
    
    // stop timer
    m_presenceTimer.stop();

    // reset history views to null 
    m_chat.resetHistory({});
    
    // emit changes 
    emit connectedChanged();
    emit authenticatedChanged(); 
    emit registeringChanged();
    emit currentPeerChanged(); 
    emit currentPeerOnlineChanged(); 

    // disconnect
    if (!shouldReconnect) emit error("Disconnected");
    if (shouldReconnect) start();
}

void ClientController::onError(QString msg) {
    emit error(msg); 
}

bool ClientController::currentPeerOnline() const {
    if (m_currentPeer.isEmpty())
        return false;

    for (const auto& c : m_contacts.contacts()) {
        if (c.username == m_currentPeer)
            return c.online;
    }
    return false;
}

void ClientController::onLoginResult(bool ok, const QString& msg) {
    // reject signals before ctor is finished 
    if (!m_ready) return; 

    // login ok 
    if (ok) {
        // authenticate user
        m_authenticated = true; 
        m_username = m_pendingUsername; 

        // clear fetched previews, update with new users
        m_prefetchedPreview.clear(); 
        refreshUsers(); 
        m_presenceTimer.start(); // start timer for periodic server ping

        // emit signas for controller
        emit authenticatedChanged(); 
        emit toast("Logged in"); 
    } 
    // login failed 
    else {
        // clear pending state data, emit error
        if (!m_pendingUsername.isEmpty()) m_pendingUsername.clear();
        emit error(msg); 
    }
}

void ClientController::onRegisterResult(bool ok, const QString& msg) {
    // reject signals before ctor is finished 
    if (!m_ready) return; 

    // registered successfuly 
    if (ok) {
        m_registering = false; 
        emit registeringChanged(); // send signal to controller
        emit toast("Registered successfuly"); 
    } 
    // register failed 
    else {
        emit error(msg); 
    }
}

void ClientController::onUsersReceived(const QVector<QJsonObject>& users) {
    // reject signals befor ctor is finished
    if (!m_ready) return; 

    // fetch all users
    QVector<Contact> snap; 
    snap.reserve(users.size()); 

    for (const auto& u : users) {
        Contact c;
        c.username = u.value("username").toString(); 
        if (c.username == m_username) continue; // skip current user
        c.online = u.value("online").toBool(); 
        snap.push_back(c); 
    }

    // set contacts proxy on the first query 
    if (!m_contactsProxy.sourceModel()) {
        m_contactsProxy.setSourceModel(&m_contacts); 
        m_contactsProxy.sort(0); 
    }

    // merge users not already listed
    QVector<QString> toPrefetch;
    toPrefetch.reserve(snap.size()); 

    for (const auto& c : snap) {
        // if username isn't already in contacts frame, push for merging
        if (!m_prefetchedPreview.contains(c.username)) {
            toPrefetch.push_back(c.username);
            m_prefetchedPreview.insert(c.username); 
        }
    }
    m_contacts.mergePresence(snap);
    emit currentPeerOnlineChanged();
    
    // get history for new users only once each login 
    for (const auto& user : toPrefetch) {
        m_msgservice.getHistory(user, 1);
    } 
}

void ClientController::onHistoryReceived(const QString& peer, const QVector<QJsonObject>& msgs) {
    // reject signals before ctor is finished
    if (!m_ready) return; 
    
    // deserialize message into components
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

    // update preview 
    if (!out.isEmpty()) {
        const auto& last = out.last(); 
        m_contacts.updateLastMessage(peer, last.content, last.timestamp, false);
    }

    // only change chat view if in user window
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

    emit clearChat(); // temp placeholder for later message id greedy append
    m_chat.resetHistory(out);
    emit showChat(); 
}

void ClientController::onIncomingMessage(const QJsonObject& m) {
    // reject signal if ctor not finished
    if (!m_ready) return; 

    // deserialize into chat message object
    const QString sender = m.value("sender").toString();
    const QString peer = (sender == m_username) ? m.value("recipient").toString() : sender; 
    const QString content = m.value("content").toString();
    const QString ts = normalizeTimestamp(
        m.value("timestamp").toString()
    );
    const bool isPeerCurrent = (peer == m_currentPeer); 

    m_contacts.updateLastMessage(peer, content, ts, !isPeerCurrent); 

    // if not current chat, send notification 
    if (!isPeerCurrent) {
        QString notif = (content.size() > 40) ? content.left(37).append("...") : content; 

        emit toast(QString("%1: %2").arg(peer, notif));
        return; 
    }

    // if peer is current, append to chat view
    ChatMessage cm; 
    cm.sender = sender;
    cm.content = content; 
    cm.timestamp = ts; 
    cm.isOwn = (sender == m_username);
    m_chat.appendMessage(cm); 
}

void ClientController::onSendMessageResponse(bool ok, const QString& msg) {
    // reject signals before ctor is finished
    if (!m_ready) return;

    // if ok, getHistory to append new message
    if (ok) {
        m_msgservice.getHistory(m_currentPeer, 1); 
    } else { 
        emit error(QString("Send failed: %1").arg(msg)); 
        // optional mark as failed with options 
    }
}
