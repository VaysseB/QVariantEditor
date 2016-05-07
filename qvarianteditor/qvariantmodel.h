#ifndef QVARIANTMODEL_H
#define QVARIANTMODEL_H

#include <QSharedPointer>
#include <QObject>
#include <QAbstractItemModel>


class QVariantModel : public QAbstractItemModel
{
    Q_OBJECT
    Q_PROPERTY(QVariantList rootDatas READ rootDatas WRITE setRootDatas NOTIFY rootDatasChanged)
    Q_PROPERTY(bool dynamicSort READ dynamicSort WRITE setDynamicSort NOTIFY dynamicSortChanged) // sort only keys/index

    struct node_t {
        QVariant value;

        node_t* parent = nullptr;
        QVariant keyInParent;

        QList<node_t*> children;
    };

public:
    enum Column {
        KeyColumn,
        ValueColumn,
        TypeColumn,
        ColumnCount,
        IndexColumn = KeyColumn
    };

public:
    explicit QVariantModel(QObject* parent = nullptr);
    virtual ~QVariantModel();

    QModelIndex index(int row, int column, const QModelIndex& parent = QModelIndex()) const;
    QModelIndex parent(const QModelIndex& child) const;
    bool hasChildren(const QModelIndex& parent = QModelIndex()) const;

    int rowCount(const QModelIndex& parent = QModelIndex()) const;
    int columnCount(const QModelIndex& parent = QModelIndex()) const;

    Qt::ItemFlags flags(const QModelIndex& index) const;
    QVariant headerData(int section, Qt::Orientation orientation, int role) const;
    QVariant data(const QModelIndex& index, int role) const;
    bool setData(const QModelIndex& index, const QVariant& value, int role);

    QVariantList rootDatas() const;

    uint displayDepth() const;

    int column(Column column) const;

    inline bool dynamicSort() const { return m_dynamicSort; }

signals:
    void rootDatasChanged(const QVariantList& rootDatas);
    void dynamicSortChanged(bool enabled);

public slots:
    void setRootDatas(const QVariantList& rootDatas);

    void setDisplayDepth(uint depth);

    void setDynamicSort(bool enabled);

protected:
    virtual bool lessThan(const QVariant& left, const QVariant& right) const;

private:
    void buildTree(node_t& node,
                   const QVariant& data,
                   node_t* parent = nullptr,
                   const QVariant& keyInParent = QVariant()) const;

    QModelIndex indexOfNode(node_t* node, int column) const;
    QModelIndex indexOfNode(node_t* node, Column col) const;

    void sortTree(node_t& root, bool recursive);

#ifdef QT_DEBUG
    void dumpTree(const node_t *root, const QString& prefix = QString()) const;
#endif

private:
    QSharedPointer<node_t> mp_root;
    uint m_depth = 0;
    bool m_dynamicSort;
};

#endif // QVARIANTMODEL_H
