#pragma once 
#include <QObject> 
#include <QJsonObject>
#include <QVector> 
#include <QString> 
#include <qjsonobject.h>

class ProtocolClient; 

class MessageService final : public QObject {
    Q_OBJECT 
public:
    explicit MessageService(ProtocolClient* proto, QObject* parent = nullptr); 

    void listUsers(); 
    void getHistory(const QString& peer, int limit = 50); 
    void sendMessage(const QString& peer, const QString& content); 

signals: 
    void loginResult(bool success, QString msg); 
    void sendMessageResponse(bool ok, QString message); 
    void usersReceived(QVector<QJsonObject> users); 
    void historyReceived(QString peer, QVector<QJsonObject> messages);
    void incomingMessage(QJsonObject message); 
    void commandError(QString command, QString message); 

private slots: 
    void onResponse(QJsonObject obj);

private: 
    ProtocolClient* m_proto; 
};


