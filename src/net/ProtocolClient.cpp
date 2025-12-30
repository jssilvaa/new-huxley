// src/net/ProtocolClient.cpp
#include "ProtocolClient.h"
#include <QJsonDocument>
#include <qabstractsocket.h>
#include <qcontainerfwd.h>
#include <qjsondocument.h>
#include <qtcpsocket.h>

namespace {
    static quint32 readU32BE(const char* p) {
        return (quint32(quint8(p[0])) << 24) | // shift first byte to last 
               (quint32(quint8(p[1])) << 16) | // and so on
               (quint32(quint8(p[2])) <<  8) |
               (quint32(quint8(p[3])) <<  0); 
    }

    static void writeU32BE(QByteArray& out, quint32 v) {
        out.append(char((v >> 24) & 0xFF)); // last byte goes first 
        out.append(char((v >> 16) & 0xFF)); // and so on
        out.append(char((v >>  8) & 0xFF));
        out.append(char((v >>  0) & 0xFF)); 
    }
}

ProtocolClient::ProtocolClient(QObject* parent) : QObject(parent) {
    connect(&m_sock, &QTcpSocket::connected, this, &ProtocolClient::connected); 
    connect(&m_sock, &QTcpSocket::disconnected, this, &ProtocolClient::disconnected); 
    connect(&m_sock, &QTcpSocket::readyRead, this, &ProtocolClient::onReadyRead); 
    connect(&m_sock, &QTcpSocket::errorOccurred, this, &ProtocolClient::onSocketError); 
}

void ProtocolClient::connectToHost(const QString& host, quint16 port) {
    m_buf.clear(); 
    m_expectedLen = 0; 
    m_sock.connectToHost(host, port); 
}

void ProtocolClient::disconnectFromHost() { m_sock.disconnectFromHost(); }
bool ProtocolClient::isConnected() const { return m_sock.state() == QAbstractSocket::ConnectedState; }

void ProtocolClient::sendCommand(const QJsonObject& obj) {
    if (!isConnected()) {
        emit errorOcurred("Not connected"); 
        return; 
    }
    // build payload in json format
    const QByteArray json = QJsonDocument(obj).toJson(QJsonDocument::Compact); 
    QByteArray frame; 

    // reserve and write header size
    frame.reserve(4 + json.size()); 
    writeU32BE(frame, quint32(json.size())); // writes size
    
    /// write payload content
    frame.append(json); 
    
    // writes to fds, flushes stream
    m_sock.write(frame); 
    m_sock.flush(); 
}

void ProtocolClient::onReadyRead() {
    m_buf.append(m_sock.readAll()); 
    tryParseFrames();
}

void ProtocolClient::tryParseFrames() {
    while (true) {
        // expectedlen == 0 means reading from fds
        if (m_expectedLen == 0) {
            if (m_buf.size() < 4) return; 
            m_expectedLen = readU32BE(m_buf.constData()); 
            m_buf.remove(0, 4); 
        }
        
        // incomplete frame 
        if (m_buf.size() < int(m_expectedLen)) { 
            return; // wait for more 
        }; 

        // read payload contents
        const QByteArray payload = m_buf.left(m_expectedLen); // get first expectedlen bytes 
        m_buf.remove(0, m_expectedLen); 
        m_expectedLen = 0; // reset payload to read rest 

        const auto doc = QJsonDocument::fromJson(payload); 
        if (!doc.isObject()) {
            emit errorOcurred("Bad JSON from server"); 
            continue; 
        }

        emit responseReceived(doc.object());
    }
}

void ProtocolClient::onSocketError(QAbstractSocket::SocketError) {
    emit errorOcurred(m_sock.errorString()); 
}