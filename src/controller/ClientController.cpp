// src/controller/ClientController.cpp
#include "ClientController.h"
#include "service/MessageService.h"
#include <qcontainerfwd.h>
#include <qjsondocument.h>
#include <qjsonobject.h>
#include <qnamespace.h>
#include <qobject.h>

namespace {
    static QString normalizeTimestamp(const QString& isoTs) {
        const auto dt = QDateTime::fromString(isoTs, Qt::ISODate);
        if (!dt.isValid()) return isoTs; 
        return dt.toLocalTime().toString("yyyy-MM-dd HH:mm:ss");
    }
}

ClientController::ClientController(QObject* parent) 
    : QObject(parent), m_msgservice(&m_proto, this) 
{
    // Timer and server pings 
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
    connect(&m_msgservice, &MessageService::loginResult, 
            this, [this](bool ok, const QString& msg) { 
                if (ok) {
                    m_authenticated = true; 
                    m_username = m_pendingUsername;
                    
                    m_prefetchedPreview.clear(); 
                    refreshUsers();
                    m_presenceTimer.start();
                    
                    emit authenticatedChanged(); 
                    emit toast("Logged in");
                } else {
                    if (!m_pendingUsername.isEmpty()) m_pendingUsername.clear(); 
                    emit error(msg);
                }
             });
    connect(&m_msgservice, &MessageService::registerResult, 
        this, [this](bool ok, const QString& msg) {
            if (ok) {
                m_registering = false; 
                emit registeringChanged(); 
                emit toast("Registered successfuly");
            } else emit error(msg); 
            });

    connect(&m_msgservice, &MessageService::usersReceived,
            this, [this](const QVector<QJsonObject>& users) {

                QVector<Contact> snap; 
                snap.reserve(users.size()); 

                for (const auto& u : users) {
                    Contact c; 
                    c.username = u.value("username").toString(); 
                    if(c.username == m_username) continue; 
                    c.online = u.value("online").toBool(); 
                    snap.push_back(c); 
                }

                if (!m_contactsProxy.sourceModel()) {
                    m_contactsProxy.setSourceModel(&m_contacts); 
                    m_contactsProxy.sort(0); 
                }

                // see if there are new users
                QVector<QString> toPrefetch; 
                toPrefetch.reserve(snap.size()); 

                for (const auto& c : snap) {
                    if (!m_prefetchedPreview.contains(c.username)) {
                        toPrefetch.push_back(c.username); 
                        m_prefetchedPreview.insert(c.username); 
                    }
                }

                m_contacts.mergePresence(snap); 
                emit currentPeerOnlineChanged(); 

                // prefetch once per user 
                for (const auto& user : toPrefetch) { 
                    m_msgservice.getHistory(user, 1); 
                }
            });
    connect(&m_msgservice, &MessageService::historyReceived, 
            this, [this](const QString& peer, 
                         const QVector<QJsonObject>& msgs) {

                QVector<ChatMessage> out; 
                out.reserve(msgs.size());

                for (const auto& m : msgs) { 
                    ChatMessage cm;
                    cm.sender = m.value("from").toString(); 
                    cm.content = m.value("content").toString(); 
                    cm.timestamp = normalizeTimestamp(
                        m.value("timestamp").toString()
                    ); 
                    cm.isOwn = cm.sender == m_username; 

                    out.push_back(cm); 
                }

                // update preview for received history 
                if (!out.isEmpty()) {
                    const auto& last = out.last(); 
                    m_contacts.updateLastMessage(peer, last.content, last.timestamp, false);
                }

                // only change preview for current user 
                if (peer != m_currentPeer) return; 

                emit clearChat(); // temp placeholder logic 
                m_chat.resetHistory(out);
                emit showChat(); 
            });
    connect(&m_msgservice, &MessageService::incomingMessage, 
            this, [this](const QJsonObject& m) {

                const QString sender = m.value("sender").toString(); 
                const QString peer = 
                    (sender == m_username) ? 
                    m.value("recipient").toString() :
                    sender; 
                const QString content = m.value("content").toString(); 
                const QString ts = normalizeTimestamp( 
                    m.value("timestamp").toString()
                );
                const bool isPeerCurrent = (peer == m_currentPeer); 
                
                m_contacts.updateLastMessage(peer, content, ts, !isPeerCurrent);

                // if not current chat, just send notif 
                if (!isPeerCurrent) {
                    emit toast(QString("%1: %2")
                    .arg(peer)                  // % 1
                    .arg(content.left(40))); // % 2 
                    return; 
                };
                
                ChatMessage cm; 
                cm.sender = sender; 
                cm.content = content; 
                cm.timestamp = ts; 
                cm.isOwn = (sender == m_username); 
                m_chat.appendMessage(cm);

            });
    connect(&m_msgservice, &MessageService::sendMessageResponse,
            this, [this](bool ok, const QString& msg) {
                if (!ok) {
                    emit error(QString("Send failed: %1").arg(msg));
                    // optional mark it as failed with options (resend, delete)
                } else {
                    m_msgservice.getHistory(m_currentPeer); 
                }
            });
}

void ClientController::start() {
    if (m_connected || m_connecting) return; 
    m_connecting = true; 
    m_proto.connectToHost("127.0.0.1", 8080); // connect localhost@8080
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
    m_pendingUsername = std::move(user); // only valid anyway if authenticated 
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
    m_connecting = false; 
    m_connected = true;
    emit connectedChanged(); 
    emit toast("Connected"); 
}

void ClientController::onDisconnected() {
    m_connected = false; 
    m_authenticated = false; 
    m_presenceTimer.stop();
    m_prefetchedPreview.clear(); 
    emit connectedChanged();
    emit authenticatedChanged(); 
    emit error("Disconnected"); 
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
