// src/controller/ClientController.cpp
#include "ClientController.h"
#include "service/MessageService.h"
#include <qcontainerfwd.h>
#include <qjsondocument.h>
#include <qjsonobject.h>
#include <qobject.h>

ClientController::ClientController(QObject* parent) : QObject(parent), m_msgservice(&m_proto, this) {
    connect(&m_proto, &ProtocolClient::connected, this, &ClientController::onConnected);
    connect(&m_proto, &ProtocolClient::disconnected, this, &ClientController::onDisconnected);
    connect(&m_proto, &ProtocolClient::errorOcurred, this, &ClientController::onError); 
    connect(&m_msgservice, &MessageService::loginResult, 
            this, [this](bool ok, const QString& msg) { 
                if (ok) {
                    m_authenticated = true; 
                    m_username = m_pendingUsername;
                    refreshUsers(); 
                    emit authenticatedChanged(); 
                    emit toast("Logged in");
                } else {
                    if (!m_pendingUsername.isEmpty()) m_pendingUsername.clear(); 
                    emit error(msg);
                }
             });
    connect(&m_msgservice, &MessageService::usersReceived,
            this, [this](const QVector<QJsonObject>& users) {

                QVector<Contact> out; 
                out.reserve(users.size()); 

                for (const auto& u : users) {
                    Contact c; 
                    c.username = u.value("username").toString();
                    if (c.username == m_username) continue; 
                    c.online = u.value("online").toBool(); 
                    out.push_back(c);
                }

                m_contacts.setContacts(out); 
                emit currentPeerOnlineChanged(); 
            });
    connect(&m_msgservice, &MessageService::historyReceived, 
            this, [this](const QString& peer, 
                         const QVector<QJsonObject>& msgs) {

                if (peer != m_currentPeer) return; 

                QVector<ChatMessage> out; 
                out.reserve(msgs.size());

                for (const auto& m : msgs) { 
                    ChatMessage cm;
                    cm.sender = m.value("from").toString(); 
                    cm.content = m.value("content").toString(); 
                    cm.timestamp = m.value("timestamp").toString(); 
                    cm.isOwn = cm.sender == m_username; 

                    out.push_back(cm); 
                }
                
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
                
                if (peer != m_currentPeer) {
                    m_contacts.incrementUnread(peer); 
                    emit toast(QString("%1: %2")
                               .arg(peer)
                               .arg(m.value("content").toString().left(40))); 
                    return; 
                };

                ChatMessage cm; 
                cm.sender = sender; 
                cm.content = m.value("content").toString(); 
                cm.timestamp = m.value("timestamp").toString(); 
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

void ClientController::onConnected() {
    m_connecting = false; 
    m_connected = true;
    emit connectedChanged(); 
    emit toast("Connected"); 
}

void ClientController::onDisconnected() {
    m_connected = false; 
    m_authenticated = false; 
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
