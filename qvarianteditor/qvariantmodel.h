#ifndef QVARIANTMODEL_H
#define QVARIANTMODEL_H

#include <QSharedPointer>
#include <QObject>
#include <QAbstractItemModel>
#include <QRegExp>

//#define QVARIANTMODEL_DEBUG


class QVariantModel : public QAbstractItemModel
{
    Q_OBJECT
    Q_PROPERTY(QVariantList rootDatas READ rootDatas WRITE setRootDatas NOTIFY rootDatasChanged)
    Q_PROPERTY(uint displayDepth READ displayDepth WRITE setDisplayDepth NOTIFY displayDepthChanged)
    Q_PROPERTY(bool dynamicSort READ dynamicSort WRITE setDynamicSort NOTIFY dynamicSortChanged)
    Q_PROPERTY(QString filterText READ filterText WRITE setFilterText NOTIFY filterTextChanged)
    Q_PROPERTY(FilterType filterType READ filterType WRITE setFilterType NOTIFY filterTypeChanged)
    Q_PROPERTY(Columns filterColumns READ filterColumns WRITE setFilterColumns NOTIFY filterColumnsChanged)

    struct node_t {
        QVariant value;

        node_t* parent = nullptr;
        QVariant keyInParent;

        QList<node_t*> visibleChildren;
        QList<node_t*> children;
    };

public:
    enum Column {
        KeyColumn = 1,
        ValueColumn = 2,
        TypeColumn = 4,
        ColumnCount = 3,
        IndexColumn = KeyColumn,
        AllColumns = KeyColumn | ValueColumn | TypeColumn
    };
    Q_DECLARE_FLAGS(Columns, Column)

    enum FilterType {
        Contains,
        WildCard,
        Regex,
        Fixed
    };

public:
    explicit QVariantModel(QObject* parent = nullptr);
    virtual ~QVariantModel();

    QModelIndex index(int row, int column,
                      const QModelIndex& parent = QModelIndex()) const;
    QModelIndex parent(const QModelIndex& child) const;
    bool hasChildren(const QModelIndex& parent = QModelIndex()) const;

    int rowCount(const QModelIndex& parent = QModelIndex()) const;
    int columnCount(const QModelIndex& parent = QModelIndex()) const;

    Qt::ItemFlags flags(const QModelIndex& index) const;
    QVariant headerData(int section, Qt::Orientation orientation,
                        int role) const;
    QVariant data(const QModelIndex& index, int role) const;
    bool setData(const QModelIndex& index, const QVariant& value, int role);

    QVariantList rootDatas() const;
    inline uint displayDepth() const { return m_depth; }
    inline bool dynamicSort() const { return m_dynamicSort; }
    inline FilterType filterType() const { return m_filterType; }
    inline Columns filterColumns() const { return m_filterColumns; }
    inline QString filterText() const { return m_filterRx.pattern(); }

    int column(Column column) const;

signals:
    void rootDatasChanged(const QVariantList& rootDatas);
    void displayDepthChanged(uint depth);
    void dynamicSortChanged(bool enabled);
    void filterTypeChanged(FilterType filterType);
    void filterColumnsChanged(Columns filterColumns);
    void filterTextChanged(const QString& filterText);

public slots:
    void setRootDatas(const QVariantList& rootDatas);
    void setDisplayDepth(uint depth);
    void setDynamicSort(bool enabled);
    void setFilterType(FilterType filterType);
    void setFilterColumns(Columns filterColumns);
    void setFilterText(const QString& filterText);

protected:
    virtual bool lessThan(const QVariant& left, const QVariant& right) const;
    virtual bool filterKey(const QVariant& key) const;
    virtual bool filterValue(const QVariant& value) const;
    virtual bool filterType(int type) const;
    virtual bool filterOnDisplayText(const QString& text) const;

private:
    void buildTree(node_t& node,
                   const QVariant& data,
                   node_t* parent = nullptr,
                   const QVariant& keyInParent = QVariant()) const;

    QModelIndex indexOfNode(node_t* node, int column) const;
    QModelIndex indexOfNode(node_t* node, Column col) const;

    void updateFilterRx(QString pattern);

    void sortTree(node_t& root, bool recursive);
    bool filterTree(node_t& root);

    void dumpTree(const node_t *root,
                  const QString& prefix = QString()) const;
    void dumpModel(const QModelIndex& rootIndex = QModelIndex(),
                   const QString& prefix = QString()) const;

private:
    QSharedPointer<node_t> mp_root;
    uint m_depth = 0;
    bool m_dynamicSort;
    FilterType m_filterType = Contains;
    Columns m_filterColumns = AllColumns;
    QRegExp m_filterRx;
};

Q_DECLARE_OPERATORS_FOR_FLAGS(QVariantModel::Columns)

#endif // QVARIANTMODEL_H
