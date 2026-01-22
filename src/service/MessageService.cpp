#include "MessageService.h" 
#include "../net/ProtocolClient.h"
#include <QJsonDocument> 
#include <qcontainerfwd.h>
#include <qjsonarray.h>

static QString extractCommand(const QJsonObject& obj) {
    // normalize command key
    if (obj.contains("type"))
        return obj.value("type").toString().toLower().trimmed();
    if (obj.contains("command"))
        return obj.value("command").toString().toLower().trimmed();
    return {};
}

MessageService::MessageService(ProtocolClient* proto, QObject* parent)
    : QObject(parent), m_proto(proto) 
{
    assert(proto); // proto required
    // response stream handler
    connect(m_proto, &ProtocolClient::responseReceived, 
            this, &MessageService::onResponse); 
}

void MessageService::listUsers() {
    // request user list
    m_proto->sendCommand(QJsonObject{
        {"type", "LIST_USERS"}
    }); 
}

void MessageService::getHistory(const QString& peer, int limit) {
    // request message history
    m_proto->sendCommand(QJsonObject{
        {"type", "GET_HISTORY"},
        {"with", peer},
        {"limit", limit},
        {"offset", 0} 
    });
}

void MessageService::sendMessage(const QString& peer, const QString& content) {
    // send message command
    m_proto->sendCommand(QJsonObject{
        {"type", "SEND_MESSAGE"}, 
        {"recipient", peer}, 
        {"content", content} 
    });
}

void MessageService::onResponse(QJsonObject obj) {
    // route response
    const QString cmd = extractCommand(obj);

    const bool success = obj.value("success").toBool(false);

    // login response
    if (cmd == "login" || cmd == "login_response") {
        const bool ok = obj.value("success").toBool(false); 
        const QString msg = obj.value("message").toString(); 
        emit loginResult(ok, msg); 
        return; 
    }   

    // register response
    if (cmd == "register" || cmd == "register_response") {
        const bool ok = obj.value("success").toBool(false);
        const QString msg = obj.value("message").toString(); 
        emit registerResult(ok, msg); 
    }

    // send ack response
    if (cmd == "send_message" || cmd == "send_message_response") {
        const bool ok = obj.value("success").toBool(false); 
        const QString msg = obj.value("message").toString(); 
        
        emit sendMessageResponse(ok, msg); 
        if (!ok) emit commandError(cmd, msg); 
        return; 
    }

    // incoming message event
    if (cmd == "incoming_message" || cmd == "incoming_message_response") {
        emit incomingMessage(obj); 
        return; 
    }

    // error response
    if (!success) {
        emit commandError(cmd, obj.value("message").toString()); 
        return; 
    }

    // user list payload
    if (cmd == "list_users") {
        const auto users = 
            obj.value("payload").toObject().value("users").toArray();

        QVector<QJsonObject> out; 
        for (const auto& u : users) 
            out.push_back(u.toObject()); 

        emit usersReceived(out); 
        return; 
    }

    // history payload
    if (cmd == "get_history") {
        const auto payload = obj.value("payload").toObject(); 
        const QString peer = payload.value("with").toString(); 
        const auto msgs = payload.value("messages").toArray(); 

        QVector<QJsonObject> out; 
        for (const auto& m : msgs) 
            out.push_back(m.toObject()); 
    
        emit historyReceived(peer, out); 
        return; 
    }
}
