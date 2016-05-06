#include "qfullfilterproxymodel.h"

#include <QDebug>


QFullFilterProxyModel::QFullFilterProxyModel(QObject *parent) :
    QSortFilterProxyModel(parent),
    m_filterType(Contains)
{
}

QFullFilterProxyModel::~QFullFilterProxyModel()
{
}

bool QFullFilterProxyModel::filterAcceptsColumn(
        int source_column,
        const QModelIndex &source_parent) const
{
    Q_UNUSED(source_column);
    Q_UNUSED(source_parent);
    return true;
}

bool QFullFilterProxyModel::filterAcceptsRow(
        int source_row,
        const QModelIndex &source_parent) const
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

    for (auto itCol = m_filterColumns.constBegin();
         itCol != m_filterColumns.constEnd(); ++itCol) {
        QModelIndex child = sourceModel()->index(source_row, *itCol, source_parent);

        QString text = child.data(filterRole()).toString();

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

bool QFullFilterProxyModel::lessThan(
        const QModelIndex &source_left,
        const QModelIndex &source_right) const
{
    // invalid < valid
    // valid > invalid
    if (source_left.isValid() == false)
        return source_right.isValid();
    else if (source_right.isValid() == false)
        return false;

    bool isLess = true;

    QVariant leftValue = source_left.data(sortRole());
    QVariant rightValue = source_right.data(sortRole());

    bool sameType = (leftValue.type() == rightValue.type());

    if (sameType && leftValue.type() == QVariant::String) {
        QString leftString = leftValue.toString();
        QString rightString = rightValue.toString();
        if (leftString.count() == rightString.count())
            isLess = (leftString.count() < rightString.count());
        else if (isSortLocaleAware())
            isLess = QString::localeAwareCompare(leftString, rightString) < 0;
        else
            isLess = QString::compare(leftString, rightString) < 0;
    }
    else if (sameType && leftValue.type() == QVariant::Bool)
        isLess = (leftValue.toBool() < rightValue.toBool());
    else if (sameType && leftValue.type() == QVariant::Int)
        isLess = (leftValue.toInt() < rightValue.toInt());
    else if (sameType && leftValue.type() == QVariant::UInt)
        isLess = (leftValue.toUInt() < rightValue.toUInt());
    else if (leftValue.type() == QVariant::UInt
             && rightValue.type() == QVariant::Int) {
        isLess = ((int)leftValue.toUInt() < rightValue.toInt());
    }
    else if (leftValue.type() == QVariant::Int
             && rightValue.type() == QVariant::UInt) {
        isLess = (leftValue.toInt() < (int)rightValue.toUInt());
    }
    else
        isLess = QSortFilterProxyModel::lessThan(source_left, source_right);

    return isLess;
}

void QFullFilterProxyModel::forceUpdate()
{
    // force to filter again
    QString pattern = filterRegExp().pattern();
    setFilterWildcard(QString());
    setFilterWildcard(pattern);
}

void QFullFilterProxyModel::setFilterType(FilterType filterType)
{
    m_filterType = filterType;
    emit filterTypeChanged(filterType);

    forceUpdate();
}

void QFullFilterProxyModel::setFilterKeyColumn(int column)
{
    m_filterColumns.clear();
    m_filterColumns.append(column);

    forceUpdate();
}

void QFullFilterProxyModel::setFilterKeyColumns(QList<int> columns)
{
    m_filterColumns = columns;

    forceUpdate();
}
