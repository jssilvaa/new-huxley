// src/net/ProtocolClient.h
#pragma once 
#include <QObject> 
#include <QTcpSocket>
#include <QJsonObject> 
#include <qabstractsocket.h>
#include <qjsonobject.h>
#include <qstringview.h>
#include <qtcpsocket.h>
#include <qtmetamacros.h>

class ProtocolClient final : public QObject {
    Q_OBJECT 
public: 
    explicit ProtocolClient(QObject* parent=nullptr); 

    Q_INVOKABLE void connectToHost(const QString& host, quint16 port); 
    Q_INVOKABLE void disconnectFromHost(); 
    Q_INVOKABLE void sendCommand(const QJsonObject& obj);
    
    bool isConnected() const;

signals:
    void connected();
    void disconnected(); 
    void errorOcurred(QString message); 
    void responseReceived(QJsonObject obj); 

private slots: 
    void onReadyRead(); 
    void onSocketError(QAbstractSocket::SocketError); 

private: 
    void tryParseFrames(); 

    QTcpSocket m_sock; 
    QByteArray m_buf; 
    quint32 m_expectedLen = 0; // 0 means expected header
};