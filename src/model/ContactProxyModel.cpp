// src/model/ContactProxyModel.cpp
#include "ContactProxyModel.h"
#include "ContactListModel.h"
#include <qabstractitemmodel.h>
#include <qnamespace.h>

bool ContactProxyModel::filterAcceptsRow(int r, const QModelIndex& p) const {
    if (m_filterText.trimmed().isEmpty()) return true; 

    const auto idx = sourceModel()->index(r, 0, p); 
    const auto user = sourceModel()->data(idx, ContactListModel::UsernameRole).toString(); 
    const auto last = sourceModel()->data(idx, ContactListModel::LastMessageRole).toString(); 

    return user.contains(m_filterText, Qt::CaseInsensitive) ||
           last.contains(m_filterText, Qt::CaseInsensitive); 
}

bool ContactProxyModel::lessThan(const QModelIndex& l, const QModelIndex& r) const {
    const auto lts = sourceModel()->data(l, ContactListModel::LastTimestampRole).toString(); 
    const auto rts = sourceModel()->data(r, ContactListModel::LastTimestampRole).toString();

    const auto lm = parseTsMs(lts); 
    const auto rm = parseTsMs(rts); 

    if (lm != rm) return lm > rm; // newest first 

    const auto lu = sourceModel()->data(l, ContactListModel::UsernameRole).toString(); 
    const auto ru = sourceModel()->data(r, ContactListModel::UsernameRole).toString();
    return lu.localeAwareCompare(ru) < 0; 
}