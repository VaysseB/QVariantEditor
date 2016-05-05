#include "qfullfilterproxymodel.h"


QFullFilterProxyModel::QFullFilterProxyModel(QObject *parent) :
    QSortFilterProxyModel(parent),
    m_filterType(Contains)
{
}

QFullFilterProxyModel::~QFullFilterProxyModel()
{
}

bool QFullFilterProxyModel::filterAcceptsColumn(int source_column, const QModelIndex &source_parent) const
{
    Q_UNUSED(source_column);
    Q_UNUSED(source_parent);
    return true;
}

bool QFullFilterProxyModel::filterAcceptsRow(int source_row, const QModelIndex &source_parent) const
{
    QString pattern = filterRegExp().pattern();
    if (!pattern.startsWith("*") && m_filterType == WildCard)
        pattern.prepend(QChar('*'));
    if (!pattern.endsWith("*") && m_filterType == WildCard)
        pattern.append(QChar('*'));

    if (pattern.isEmpty())
        return true;

    QRegExp rx(pattern, Qt::CaseSensitive,
               (m_filterType == WildCard) ? QRegExp::WildcardUnix : QRegExp::RegExp2);

    for (int c = 0; c < sourceModel()->columnCount(source_parent); c++) {
        QModelIndex child = sourceModel()->index(source_row, c, source_parent);

        QString text = child.data(filterRole()).toString();

        // clear string
        if (text.startsWith(QChar('"')))
            text.remove(0, 1);
        if (text.endsWith(QChar('"')))
            text.remove(text.length()-1, text.length());

        bool isRowOk = false;

        if (m_filterType == Contains)
            isRowOk = text.contains(pattern);
        else if (m_filterType == WildCard || m_filterType == Regex)
            isRowOk = rx.exactMatch(text);
        else if (m_filterType == Fixed)
            isRowOk = (text == pattern);

        if (isRowOk)
            return true;

        int rowCount = sourceModel()->rowCount(child);
        for (int r = 0; r < rowCount; r++) {
            bool isSubRowOk = filterAcceptsRow(r, child);
            if (isSubRowOk)
                return true;
        }
    }
    return false;
}

void QFullFilterProxyModel::setFilterType(FilterType filterType)
{
    m_filterType = filterType;
    emit filterTypeChanged(filterType);

    // force to filter again
    QString pattern = filterRegExp().pattern();
    setFilterWildcard(QString());
    setFilterWildcard(pattern);
}
