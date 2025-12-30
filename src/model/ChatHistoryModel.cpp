// src/model/ChatHistoryModel.cpp
#include "ChatHistoryModel.h"
#include <qstringview.h>

ChatHistoryModel::ChatHistoryModel(QObject* parent)
    : QAbstractListModel(parent) {}

int ChatHistoryModel::rowCount(const QModelIndex& parent) const {
    if (parent.isValid()) return 0; 
    return m_messages.size(); 
}

QVariant ChatHistoryModel::data(const QModelIndex& index, int role) const {
    if (!index.isValid()) return {}; 

    const auto& m = m_messages.at(index.row()); 

    switch (role) {
        case SenderRole: return m.sender; 
        case ContentRole: return m.content; 
        case TimestampRole: return m.timestamp; 
        case IsOwnRole: return m.isOwn; 
        default: return {}; 
    }
}

QHash<int, QByteArray> ChatHistoryModel::roleNames() const {
    return {
        { SenderRole, "sender" },
        { ContentRole, "content" },
        { TimestampRole, "timestamp" },
        { IsOwnRole, "isOwn" }
    };
}

void ChatHistoryModel::resetHistory(const QVector<ChatMessage>& messages) {
    beginResetModel(); 
    m_messages = messages;
    endResetModel(); 
}

void ChatHistoryModel::appendMessage(const ChatMessage& message) {
    const int row = m_messages.size(); 
    beginInsertRows(QModelIndex(), row, row); 
    m_messages.push_back(message); 
    endInsertRows(); 
}