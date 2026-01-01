// src/Model/ContactListModel.h
#pragma once
#include <QAbstractListModel>
#include <QString> 
#include <QVector> 
#include <qabstractitemmodel.h>
#include <qbytearrayview.h>
#include <qcontainerfwd.h>

struct Contact { 
    QString username; 
    bool online = false; 
    int unread = 0; 
}; 

class ContactListModel final : public QAbstractListModel {
    Q_OBJECT 
public: 
    enum Roles {
        UsernameRole = Qt::UserRole + 1, 
        OnlineRole,
        UnreadRole
    }; 

    explicit ContactListModel(QObject* parent = nullptr); 
    const QVector<Contact>& contacts() const { return m_contacts; }
    const int unreadCount(const QString& user) const
    { 
        for (int i = 0; i < m_contacts.size(); ++i) {
            auto& c = m_contacts[i]; 
            if (c.username == user) {
                return c.unread; 
            }
        }

        return 0; // if none found
    }; 
    void incrementUnread(const QString& user)
    { 
        for (int i = 0; i < m_contacts.size(); ++i) {
            auto& c = m_contacts[i]; 
            if (c.username == user) {
                ++c.unread; 
                emit dataChanged(index(i), index(i), {UnreadRole}); 
                return; 
            }
        }
    }; 
    void clearUnread(const QString& user) {
        for (int i = 0; i < m_contacts.size(); ++i) {
            auto& c = m_contacts[i]; 
            if (c.username == user) {
                c.unread = 0; 
                emit dataChanged(index(i), index(i), {UnreadRole}); 
                return;
            }
        }
    }; 
    
    // QAbstractItemModel interface
    int rowCount(const QModelIndex& parent = QModelIndex()) const override; 
    QVariant data(const QModelIndex& index, int role) const override; 
    QHash<int, QByteArray> roleNames() const override; 
    
    public slots: 
    void setContacts(const QVector<Contact>& contacts); 
    
private: 
    QVector<Contact> m_contacts; 
};
