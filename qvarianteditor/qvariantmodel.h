#ifndef QVARIANTMODEL_H
#define QVARIANTMODEL_H

#include <QSharedPointer>
#include <QObject>
#include <QAbstractItemModel>


class QVariantModel : public QAbstractItemModel
{
    Q_OBJECT

    struct node_t {
        QVariant value;

        node_t* parent = nullptr;
        QVariant keyInParent;

        QList<node_t*> children;
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
//    void setData(const QModelIndex& index, const QVariant& value, int role);

    QVariant rootData() const;

    uint displayDepth() const;

public slots:
    void setRootData(const QVariant& rootData);

    void setDisplayDepth(uint depth);

private:
    void buildTree(node_t& node,
                   const QVariant& data,
                   node_t* parent = nullptr,
                   const QVariant& keyInParent = QVariant()) const;

private:
    QSharedPointer<node_t> mp_root;
    uint m_depth = 0;
};

#endif // QVARIANTMODEL_H
