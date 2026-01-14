// src/model/ChatHistoryModel.cpp
#include "ChatHistoryModel.h"
#include <QDateTime>
#include <QLocale>
#include <qstringview.h>

namespace {
    QDateTime parseTimestamp(const QString& ts) {
        if (ts.isEmpty()) return {};

        QDateTime dt = QDateTime::fromString(ts, "yyyy-MM-dd HH:mm:ss");
        if (!dt.isValid()) dt = QDateTime::fromString(ts, Qt::ISODate);
        if (dt.isValid()) dt = dt.toLocalTime();
        return dt;
    }

    QDate dateFromTimestamp(const QString& ts) {
        const auto dt = parseTimestamp(ts);
        if (!dt.isValid()) return {};
        return dt.date();
    }

    QString dayLabelFromTimestamp(const QString& ts) {
        const QDate date = dateFromTimestamp(ts);
        if (!date.isValid()) return {};

        const QDate today = QDate::currentDate();
        if (date == today) return QStringLiteral("Today");
        if (date == today.addDays(-1)) return QStringLiteral("Yesterday");

        const QLocale locale(QLocale::English);
        return locale.toString(date, "d MMMM yyyy");
    }
}

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
        case DayLabelRole: return dayLabelFromTimestamp(m.timestamp);
        case DayStartRole: {
            const QDate currentDate = dateFromTimestamp(m.timestamp);
            if (!currentDate.isValid()) return false;
            if (index.row() == 0) return true;

            const auto& prev = m_messages.at(index.row() - 1);
            const QDate prevDate = dateFromTimestamp(prev.timestamp);
            return !prevDate.isValid() || prevDate != currentDate;
        }
        default: return {}; 
    }
}

QHash<int, QByteArray> ChatHistoryModel::roleNames() const {
    return {
        { SenderRole, "sender" },
        { ContentRole, "content" },
        { TimestampRole, "timestamp" },
        { IsOwnRole, "isOwn" },
        { DayLabelRole, "dayLabel" },
        { DayStartRole, "dayStart" }
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
