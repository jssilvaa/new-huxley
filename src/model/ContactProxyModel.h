// src/model/ContactProxyModel.h
#pragma once 
#include <QSortFilterProxyModel> 
#include <QDateTime> 
#include <qnamespace.h>
#include <qsortfilterproxymodel.h>
#include <qtmetamacros.h>

class ContactProxyModel final : public QSortFilterProxyModel {
    Q_OBJECT;
    Q_PROPERTY(QString filterText READ filterText WRITE setFilterText NOTIFY filterTextChanged);
public: 
    explicit ContactProxyModel(QObject* parent = nullptr) : QSortFilterProxyModel(parent) {
        setDynamicSortFilter(true);
        setFilterCaseSensitivity(Qt::CaseInsensitive); 
        setSortCaseSensitivity(Qt::CaseInsensitive); 
    }

    QString filterText() const { return m_filterText; }
    void setFilterText(const QString& t) {
        if (m_filterText == t) return; 
        beginFilterChange(); 
        m_filterText = t; 
        endFilterChange(); 
        emit filterTextChanged(); 
    }

signals: 
    void filterTextChanged();

protected: 
    bool filterAcceptsRow(int sourceRow, const QModelIndex& sourceParent) const override; 
    bool lessThan(const QModelIndex& left, const QModelIndex& right) const override; 

private: 
    QString m_filterText; 

    static qint64 parseTsMs(const QString& ts) {
        const auto dt = QDateTime::fromString(ts, Qt::ISODate);
        return dt.isValid() ? dt.toMSecsSinceEpoch() : 0;
    }
};