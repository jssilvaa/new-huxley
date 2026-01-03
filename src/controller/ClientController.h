// src/controller/ClientController.h
#pragma once 
#include <QObject> 
#include <QJsonObject>
#include <QTimer> 
#include <qjsonobject.h>
#include <qobject.h>
#include <qset.h>
#include <qtmetamacros.h>
#include "../net/ProtocolClient.h"
#include "../service/MessageService.h"
#include "../model/ContactListModel.h"
#include "../model/ChatHistoryModel.h"
#include "../model/ContactProxyModel.h"

class ClientController final : public QObject {
    Q_OBJECT
    Q_PROPERTY(bool connected READ connected NOTIFY connectedChanged)
    Q_PROPERTY(bool authenticated READ authenticated NOTIFY authenticatedChanged)
    Q_PROPERTY(QString currentPeer READ currentPeer NOTIFY currentPeerChanged)
    Q_PROPERTY(bool hasPeer READ hasPeer NOTIFY currentPeerChanged)
    Q_PROPERTY(QObject* messageService READ messageService CONSTANT)
    Q_PROPERTY(QObject* contacts READ contacts CONSTANT)
    Q_PROPERTY(QObject* chat READ chat CONSTANT)
    Q_PROPERTY(bool currentPeerOnline READ currentPeerOnline NOTIFY currentPeerOnlineChanged)
    Q_PROPERTY(bool focusContacts READ focusContacts WRITE setFocusContacts NOTIFY focusContactsChanged)
    Q_PROPERTY(QObject* contactsProxy READ contactsProxy CONSTANT); 
    Q_PROPERTY(bool registering READ registering NOTIFY registeringChanged)
public: 
    explicit ClientController(QObject* parent=nullptr); 
    QObject* messageService() { return &m_msgservice; }
    QObject* contacts() { return &m_contacts; }
    QObject* chat() { return &m_chat; }
    QObject* contactsProxy() { return &m_contactsProxy; }

    Q_INVOKABLE void start(); 
    Q_INVOKABLE void login(const QString& user, const QString& pass);
    Q_INVOKABLE void registerUser(const QString& user, const QString& pass); 
    Q_INVOKABLE void showRegister(); 
    Q_INVOKABLE void showLogin();  
    Q_INVOKABLE void refreshUsers() { m_msgservice.listUsers(); }
    Q_INVOKABLE void selectPeer(const QString& peer) {
        if (!m_authenticated) { emit error("Not authenticated"); return; }
        
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
    void setFocusContacts(bool v) {
        if (m_focusContacts == v) return; 
        m_focusContacts = v; 
        emit focusContactsChanged(); 
    }

    bool connected() const { return m_connected; }
    bool authenticated() const { return m_authenticated; }
    bool registering() const { return m_registering; }
    bool hasPeer() const { return !m_currentPeer.isEmpty(); }
    bool currentPeerOnline() const; 
    QString currentPeer() const { return m_currentPeer; }
    bool focusContacts() const { return m_focusContacts; }

signals: 
    void connectedChanged(); 
    void authenticatedChanged(); 
    void registeringChanged();
    void currentPeerChanged(); 
    void currentPeerOnlineChanged(); 
    void focusContactsChanged(); 
    void clearChat(); // temporary logic, not scalable, but works for now 
    void showChat();

    void toast(QString msg); 
    void error(QString msg);
    
private slots: 
    void onConnected();
    void onDisconnected(); 
    void onError(QString msg);

    void onLoginResult(bool ok, const QString& msg); 
    void onRegisterResult(bool ok, const QString& msg); 
    void onUsersReceived(const QVector<QJsonObject>& users); 
    void onHistoryReceived(const QString& peer, const QVector<QJsonObject>& msgs);
    void onIncomingMessage(const QJsonObject& m); 
    void onSendMessageResponse(bool ok, const QString& msg); 

private: 
    ProtocolClient m_proto; 
    MessageService m_msgservice; 
    ContactListModel m_contacts;
    ChatHistoryModel m_chat;  
    ContactProxyModel m_contactsProxy; 
    bool m_connecting = false; 
    bool m_connected = false; 
    bool m_authenticated = false;
    bool m_registering = false; 
    bool m_focusContacts = false;
    bool m_ready = false; 
    QString m_pendingUsername{}; 
    QString m_username{};  
    QString m_currentPeer{};
    QSet<QString> m_prefetchedPreview{}; 
    QTimer m_presenceTimer; 
}; 