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
        case UsernameRole: return c.username; 
        case OnlineRole:   return c.online; 
        default:           return {}; 
    }
}

QHash<int, QByteArray> ContactListModel::roleNames() const { 
    return {
        { UsernameRole, "username" }, 
        { OnlineRole, "online"} 
    };
}

void ContactListModel::setContacts(const QVector<Contact>& contacts) {
    beginResetModel(); 
    m_contacts = contacts; 
    endResetModel(); 
}