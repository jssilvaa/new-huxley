// src/controller/ClientController.h
#pragma once 
#include <QObject> 
#include <QJsonObject>
#include <qtmetamacros.h>
#include "../net/ProtocolClient.h"
#include "../service/MessageService.h"
#include "../model/ContactListModel.h"
#include "../model/ChatHistoryModel.h"

class ClientController final : public QObject {
    Q_OBJECT;
    Q_PROPERTY(bool connected READ connected NOTIFY connectedChanged);
    Q_PROPERTY(bool authenticated READ authenticated NOTIFY authenticatedChanged);
    Q_PROPERTY(QString currentPeer READ currentPeer NOTIFY currentPeerChanged);
    Q_PROPERTY(bool hasPeer READ hasPeer NOTIFY currentPeerChanged); 
    Q_PROPERTY(QObject* messageService READ messageService CONSTANT);
    Q_PROPERTY(QObject* contacts READ contacts CONSTANT); 
    Q_PROPERTY(QObject* chat READ chat CONSTANT);
    Q_PROPERTY(bool currentPeerOnline READ currentPeerOnline NOTIFY currentPeerOnlineChanged);  
public: 
    explicit ClientController(QObject* parent=nullptr); 
    QObject* messageService() { return &m_msgservice; }
    QObject* contacts() { return &m_contacts; }
    QObject* chat() { return &m_chat; }

    Q_INVOKABLE void start(); 
    Q_INVOKABLE void login(const QString& user, const QString& pass); 
    Q_INVOKABLE void refreshUsers() { m_msgservice.listUsers(); }
    Q_INVOKABLE void selectPeer(const QString& peer) {
        if (peer == m_currentPeer) return; 
        m_currentPeer = peer; 
        m_contacts.clearUnread(peer); 
        m_chat.resetHistory({}); 

        // get history and emit peer change
        m_msgservice.getHistory(peer); 
        emit currentPeerChanged(); 
        emit currentPeerOnlineChanged();
    }
    Q_INVOKABLE void sendMessage(const QString& content) {
        if (!m_authenticated) return; 
        if (m_currentPeer.isEmpty()) { emit error("No peer selected"); return; }

        m_msgservice.sendMessage(m_currentPeer, content); 
    }
    Q_INVOKABLE int unreadCount(const QString& user) const {
        return m_contacts.unreadCount(user); 
    }; 
    Q_INVOKABLE bool hasUnread(const QString& user) const {
        return m_contacts.unreadCount(user) > 0; 
    }; 

    bool connected() const { return m_connected; }
    bool authenticated() const { return m_authenticated; }
    bool hasPeer() const { return !m_currentPeer.isEmpty(); }
    bool currentPeerOnline() const; 
    QString currentPeer() const { return m_currentPeer; }

signals: 
    void connectedChanged(); 
    void authenticatedChanged(); 
    void currentPeerChanged(); 
    void currentPeerOnlineChanged(); 
    void clearChat(); // temporary logic, not scalable, but works for now 
    void showChat();  

    void toast(QString msg); 
    void error(QString msg);
    
private slots: 
    void onConnected();
    void onDisconnected(); 
    void onError(QString msg);

private: 
    ProtocolClient m_proto; 
    MessageService m_msgservice; 
    ContactListModel m_contacts;
    ChatHistoryModel m_chat;  
    bool m_connecting = false; 
    bool m_connected = false; 
    bool m_authenticated = false;
    QString m_pendingUsername; 
    QString m_username;  
    QString m_currentPeer; 
}; 