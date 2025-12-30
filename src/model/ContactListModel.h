// src/Model/ContactListModel.h
#pragma once
#include <QAbstractListModel>
#include <QString> 
#include <QVector> 
#include <qabstractitemmodel.h>
#include <qbytearrayview.h>

struct Contact { 
    QString username; 
    bool online = false; 
}; 

class ContactListModel final : public QAbstractListModel {
    Q_OBJECT 
public: 
    enum Roles {
        UsernameRole = Qt::UserRole + 1, 
        OnlineRole 
    }; 

    explicit ContactListModel(QObject* parent = nullptr); 

    // QAbstractItemModel interface
    int rowCount(const QModelIndex& parent = QModelIndex()) const override; 
    QVariant data(const QModelIndex& index, int role) const override; 
    QHash<int, QByteArray> roleNames() const override; 

public slots: 
    void setContacts(const QVector<Contact>& contacts); 

private: 
    QVector<Contact> m_contacts; 
};