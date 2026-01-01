// src/Model/ContactListModel.cpp

#include "model/ContactListModel.h"

ContactListModel::ContactListModel(QObject* parent) 
    : QAbstractListModel(parent) {}

int ContactListModel::rowCount(const QModelIndex& parent) const {
    if (parent.isValid()) return 0; 
    return m_contacts.size(); 
}

QVariant ContactListModel::data(const QModelIndex& index, int role) const {
    if (!index.isValid()) return {}; 

    const auto& c = m_contacts.at(index.row()); 

    switch (role) {
        case UsernameRole:      return c.username; 
        case OnlineRole:        return c.online;
        case UnreadRole:        return c.unread;
        case LastMessageRole:   return c.lastMessage; 
        case LastTimestampRole: return c.lastTimestamp; 
        default:                return {}; 
    }
}

QHash<int, QByteArray> ContactListModel::roleNames() const { 
    return {
        { UsernameRole, "username" }, 
        { OnlineRole, "online"},
        { UnreadRole, "unread" },
        { LastMessageRole, "lastMessage" },
        { LastTimestampRole, "lastTimestamp" }
    };
}

void ContactListModel::setContacts(const QVector<Contact>& contacts) {
    beginResetModel(); 
    m_contacts = contacts; 
    endResetModel(); 
}

void ContactListModel::mergePresence(const QVector<Contact>& snapshot) {
    // map username to online 
    QHash<QString, bool> onlineByUser; 
    onlineByUser.reserve(snapshot.size()); 
    for (const auto& c : snapshot) {
        onlineByUser.insert(c.username, c.online); 
    }

    // update existing and mark them as seen i.e. handle newly registered users
    QSet<QString> seen; 
    seen.reserve(onlineByUser.size()); 

    for (int i = 0; i < m_contacts.size(); ++i) {
        auto& local = m_contacts[i]; 
        if (!onlineByUser.contains(local.username)) continue; 

        seen.insert(local.username); 
        const bool newOnline = onlineByUser.value(local.username); 

        if (local.online != newOnline) {
                local.online = newOnline; 
                emit dataChanged(index(i), index(i), { OnlineRole });
        }
    }

    // insert new users 
    for (const auto& c : snapshot) {
        if (seen.contains(c.username)) continue;

        const int pos = m_contacts.size(); 
        beginInsertRows(QModelIndex{}, pos, pos); 
        m_contacts.push_back(c); // unread defaults to 0  
        endInsertRows(); 
    }
}