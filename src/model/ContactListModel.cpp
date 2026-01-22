#include "model/ContactListModel.h"

ContactListModel::ContactListModel(QObject* parent) 
    : QAbstractListModel(parent) {}

int ContactListModel::rowCount(const QModelIndex& parent) const {
    // no child rows
    if (parent.isValid()) return 0; 
    return m_contacts.size(); 
}

QVariant ContactListModel::data(const QModelIndex& index, int role) const {
    // invalid index guard
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
    // role names for qml
    return {
        { UsernameRole, "username" }, 
        { OnlineRole, "online"},
        { UnreadRole, "unread" },
        { LastMessageRole, "lastMessage" },
        { LastTimestampRole, "lastTimestamp" }
    };
}

void ContactListModel::setContacts(const QVector<Contact>& contacts) {
    // replace full list
    beginResetModel(); 
    m_contacts = contacts; 
    endResetModel(); 
}

void ContactListModel::mergePresence(const QVector<Contact>& snapshot) {
    // build online lookup
    QHash<QString, bool> onlineByUser; 
    onlineByUser.reserve(snapshot.size()); 
    for (const auto& c : snapshot) {
        onlineByUser.insert(c.username, c.online); 
    }

    // update existing users
    QSet<QString> seen; 
    seen.reserve(onlineByUser.size()); 

    for (int i = 0; i < m_contacts.size(); ++i) {
        auto& local = m_contacts[i]; 
        if (!onlineByUser.contains(local.username)) continue; 

        seen.insert(local.username); 
        const bool newOnline = onlineByUser.value(local.username); 

        if (local.online != newOnline) {
                local.online = newOnline; 
                emit dataChanged(index(i), index(i), { OnlineRole }); // notify single row
        }
    }

    // insert new users
    for (const auto& c : snapshot) {
        if (seen.contains(c.username)) continue;

        const int pos = m_contacts.size(); 
        beginInsertRows(QModelIndex{}, pos, pos); 
        m_contacts.push_back(c); // unread default zero
        endInsertRows(); 
    }
}
