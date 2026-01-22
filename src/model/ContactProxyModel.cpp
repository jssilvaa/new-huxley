#include "ContactProxyModel.h"
#include "ContactListModel.h"
#include <qabstractitemmodel.h>
#include <qnamespace.h>

bool ContactProxyModel::filterAcceptsRow(int r, const QModelIndex& p) const {
    // empty filter allows all
    if (m_filterText.trimmed().isEmpty()) return true; 

    const auto idx = sourceModel()->index(r, 0, p); 
    const auto user = sourceModel()->data(idx, ContactListModel::UsernameRole).toString(); 
    const auto last = sourceModel()->data(idx, ContactListModel::LastMessageRole).toString(); 

    // substring match filter
    return user.contains(m_filterText, Qt::CaseInsensitive) ||
           last.contains(m_filterText, Qt::CaseInsensitive); 
}

bool ContactProxyModel::lessThan(const QModelIndex& l, const QModelIndex& r) const {
    // sort by last timestamp
    const auto lts = sourceModel()->data(l, ContactListModel::LastTimestampRole).toString(); 
    const auto rts = sourceModel()->data(r, ContactListModel::LastTimestampRole).toString();

    const auto lm = parseTsMs(lts); 
    const auto rm = parseTsMs(rts); 

    if (lm != rm) return lm > rm; // newest first 

    // name tiebreaker
    const auto lu = sourceModel()->data(l, ContactListModel::UsernameRole).toString(); 
    const auto ru = sourceModel()->data(r, ContactListModel::UsernameRole).toString();
    return lu.localeAwareCompare(ru) < 0; 
}

int ContactProxyModel::rowForUser(const QString& username) const {
    // linear scan lookup
    if (username.isEmpty()) return -1;

    for (int i = 0; i < rowCount(); ++i) {
        const auto idx = index(i, 0);
        const auto user = data(idx, ContactListModel::UsernameRole).toString();
        if (user == username) return i;
    }

    return -1;
}
