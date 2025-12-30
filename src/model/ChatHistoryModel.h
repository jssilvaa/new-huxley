// src/model/ChatHistoryModel.h 
#pragma once 
#include <QAbstractListModel> 
#include <QString> 
#include <QVector> 

struct ChatMessage { 
    QString sender;
    QString content; 
    QString timestamp; 
    bool isOwn = false; 
};

class ChatHistoryModel final : public QAbstractListModel {
    Q_OBJECT
public: 
    enum Roles { 
        SenderRole = Qt::UserRole + 1, 
        ContentRole,
        TimestampRole, 
        IsOwnRole
    }; 

    explicit ChatHistoryModel(QObject* parent = nullptr); 

    int rowCount(const QModelIndex& parent = QModelIndex()) const override; 
    QVariant data(const QModelIndex& index, int role) const override; 
    QHash<int, QByteArray> roleNames() const override; 

public slots: 
    void resetHistory(const QVector<ChatMessage>& messages); 
    void appendMessage(const ChatMessage& message); 

private: 
    QVector<ChatMessage> m_messages; 
};

