#include "ProtocolClient.h"
#include <QJsonDocument>
#include <qabstractsocket.h>
#include <qcontainerfwd.h>
#include <qjsondocument.h>
#include <qtcpsocket.h>

namespace {
    // length prefix framing helpers
    static quint32 readU32BE(const char* p) {
        return (quint32(quint8(p[0])) << 24) | // big endian shift
               (quint32(quint8(p[1])) << 16) | // next byte shift
               (quint32(quint8(p[2])) <<  8) |
               (quint32(quint8(p[3])) <<  0); 
    }

    static void writeU32BE(QByteArray& out, quint32 v) {
        out.append(char((v >> 24) & 0xFF)); // big endian write
        out.append(char((v >> 16) & 0xFF)); // next byte write
        out.append(char((v >>  8) & 0xFF));
        out.append(char((v >>  0) & 0xFF)); 
    }
}

ProtocolClient::ProtocolClient(QObject* parent) : QObject(parent) {
    // socket signal wiring
    connect(&m_sock, &QTcpSocket::connected, this, &ProtocolClient::connected); 
    connect(&m_sock, &QTcpSocket::disconnected, this, &ProtocolClient::disconnected); 
    connect(&m_sock, &QTcpSocket::readyRead, this, &ProtocolClient::onReadyRead); 
    connect(&m_sock, &QTcpSocket::errorOccurred, this, &ProtocolClient::onSocketError); 
}

void ProtocolClient::shutdown() {
    // mute socket signals
    m_sock.blockSignals(true);
    disconnect(&m_sock, nullptr, this, nullptr);

    if (m_sock.state() != QAbstractSocket::UnconnectedState) {
        m_sock.abort();
    }
    m_sock.close();
    m_buf.clear();
    m_expectedLen = 0;
}

void ProtocolClient::connectToHost(const QString& host, quint16 port) {
    // reset framing state
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
    // frame json with length
    const QByteArray json = QJsonDocument(obj).toJson(QJsonDocument::Compact); 
    QByteArray frame; 

    // reserve frame size
    frame.reserve(4 + json.size()); 
    writeU32BE(frame, quint32(json.size())); // header stores payload length
    
    // payload bytes
    frame.append(json); 
    
    // send frame to socket
    m_sock.write(frame); 
    m_sock.flush(); 
}

void ProtocolClient::onReadyRead() {
    // buffer incoming bytes
    m_buf.append(m_sock.readAll()); 
    tryParseFrames();
}

void ProtocolClient::tryParseFrames() {
    while (true) {
        // read header length
        if (m_expectedLen == 0) {
            if (m_buf.size() < 4) return; 
            m_expectedLen = readU32BE(m_buf.constData()); 
            m_buf.remove(0, 4); 
        }
        
        // incomplete frame wait
        if (m_buf.size() < int(m_expectedLen)) { 
            return; // need more data
        }; 

        // extract payload frame
        const QByteArray payload = m_buf.left(m_expectedLen); // payload slice
        m_buf.remove(0, m_expectedLen); 
        m_expectedLen = 0; // ready for next header

        const auto doc = QJsonDocument::fromJson(payload); 
        if (!doc.isObject()) {
            emit errorOcurred("Bad JSON from server"); // bad json from server
            continue; 
        }

        emit responseReceived(doc.object());
    }
}

void ProtocolClient::onSocketError(QAbstractSocket::SocketError) {
    // forward socket error
    emit errorOcurred(m_sock.errorString()); 
}
