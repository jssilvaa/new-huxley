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
    Q_PROPERTY(QString serverHost READ serverHost WRITE setServerHost NOTIFY serverHostChanged)
    Q_PROPERTY(int serverPort READ serverPort WRITE setServerPort NOTIFY serverPortChanged)
public: 
    explicit ClientController(QObject* parent=nullptr); 
    QObject* messageService();
    QObject* contacts();
    QObject* chat();
    QObject* contactsProxy();

    Q_INVOKABLE void start(); 
    Q_INVOKABLE void shutdown();
    Q_INVOKABLE void login(const QString& user, const QString& pass);
    Q_INVOKABLE void registerUser(const QString& user, const QString& pass); 
    Q_INVOKABLE void showRegister(); 
    Q_INVOKABLE void showLogin();  
    Q_INVOKABLE void reconnect();
    Q_INVOKABLE void refreshUsers();
    Q_INVOKABLE void selectPeer(const QString& peer);
    Q_INVOKABLE void sendMessage(const QString& content);
    Q_INVOKABLE int unreadCount(const QString& user) const;
    Q_INVOKABLE bool hasUnread(const QString& user) const;
    void setFocusContacts(bool v);

    bool connected() const;
    bool authenticated() const;
    bool registering() const;
    bool hasPeer() const;
    bool currentPeerOnline() const; 
    QString currentPeer() const;
    bool focusContacts() const;
    QString serverHost() const;
    int serverPort() const;

    void setServerHost(const QString& host);
    void setServerPort(int port);

signals: 
    void connectedChanged(); 
    void authenticatedChanged(); 
    void registeringChanged();
    void currentPeerChanged(); 
    void currentPeerOnlineChanged(); 
    void focusContactsChanged(); 
    void serverHostChanged();
    void serverPortChanged();
    void clearChat(); // temporary logic, not scalable, but works for now 
    void showChat();
    void messageSubmitted();

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
    bool m_reconnectPending = false;
    QString m_pendingUsername{}; 
    QString m_username{};  
    QString m_currentPeer{};
    QString m_serverHost{};
    int m_serverPort = 8080;
    QSet<QString> m_prefetchedPreview{}; 
    QTimer m_presenceTimer; 
}; 
