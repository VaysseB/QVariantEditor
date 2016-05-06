#ifndef QFULLFILTERPROXYMODEL_H
#define QFULLFILTERPROXYMODEL_H

#include <QSortFilterProxyModel>


class QFullFilterProxyModel : public QSortFilterProxyModel
{
    Q_OBJECT
    Q_PROPERTY(FilterType filterType READ filterType WRITE setFilterType NOTIFY filterTypeChanged)

public:
    enum FilterType {
        Contains,
        WildCard,
        Regex,
        Fixed
    };

public:
    explicit QFullFilterProxyModel(QObject *parent = 0);
    virtual ~QFullFilterProxyModel();

    bool filterAcceptsColumn(int source_column,
                             const QModelIndex &source_parent) const;
    bool filterAcceptsRow(int source_row,
                          const QModelIndex &source_parent) const;

    inline FilterType filterType() const { return m_filterType; }

public slots:
    void setFilterType(FilterType filterType);

signals:
    void filterTypeChanged(FilterType filterType);

protected:
    bool lessThan(const QModelIndex &source_left,
                  const QModelIndex &source_right) const;

private:
    FilterType m_filterType;
};


Q_DECLARE_METATYPE(QFullFilterProxyModel::FilterType)

#endif // QFULLFILTERPROXYMODEL_H
