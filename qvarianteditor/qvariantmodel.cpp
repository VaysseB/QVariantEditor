#include "qvariantmodel.h"

#include <QDebug>

#include "qvariantdatainfo.h"


QVariantModel::QVariantModel(QObject *parent) :
    QAbstractItemModel(parent),
    mp_root(new node_t),
    m_depth(1)
{
}

QVariantModel::~QVariantModel()
{
}

//------------------------------------------------------------------------------

void QVariantModel::setRootData(const QVariant& rootData)
{
    beginResetModel();
    mp_root.reset(new node_t);
    buildTree(*mp_root, rootData);
    endResetModel();
}

QVariant QVariantModel::rootData() const
{
    return mp_root->value;
}

void QVariantModel::buildTree(
        node_t& node,
        const QVariant& data,
        node_t *parent,
        const QVariant& keyInParent) const
{
    node.value = data;
    node.parent = parent;
    node.keyInParent = keyInParent;

    QVariantDataInfo dInfo(node.value);

    if (dInfo.isValid() && dInfo.isCollection()) {
        QVariantList keys = dInfo.collectionKeys();
        node.children.reserve(keys.count());
        for (auto itkey = keys.constBegin(); itkey != keys.constEnd(); ++itkey){
            QVariant childData = dInfo.collectionValue(*itkey);
            node.children.append(new node_t);
            buildTree(*node.children.last(), childData, &node, *itkey);
        }
    }
}

uint QVariantModel::displayDepth() const
{
    return m_depth;
}

void QVariantModel::setDisplayDepth(uint depth)
{
    m_depth = depth;
    // update all the column
    emit dataChanged(index(0, 1), index(rowCount()-1, 1));
}

//------------------------------------------------------------------------------

QModelIndex QVariantModel::index(int row, int column, const QModelIndex& parent) const
{
    Q_ASSERT(column >= 0 && column < columnCount());

    if (!parent.isValid()) {
        int childrenCount = mp_root->children.count();
        Q_ASSERT(row >= 0 && row < childrenCount);
        return createIndex(row, column, mp_root->children[row]);
    }

    node_t* pnode = static_cast<node_t*>(parent.internalPointer());
    Q_ASSERT(pnode);
    int childrenCount = pnode->children.count();
    Q_ASSERT(row >= 0 && row < childrenCount);
    return createIndex(row, column, pnode->children[row]);
}

QModelIndex QVariantModel::parent(const QModelIndex& child) const
{
    if (child.isValid()) {
        node_t* node = static_cast<node_t*>(child.internalPointer());
        Q_ASSERT(node);
        if (node->parent && node->parent != mp_root.data()) {
            QVariantDataInfo dInfo(node->parent->value);
            Q_ASSERT(dInfo.isValid() && dInfo.isCollection());
            int row = dInfo.collectionKeys().indexOf(node->keyInParent);
            Q_ASSERT(row >= 0);
            return createIndex(row, child.column(), node->parent);
        }
    }
    return QModelIndex();
}

bool QVariantModel::hasChildren(const QModelIndex& parent) const
{
    if (parent.isValid()) {
        node_t* pnode = static_cast<node_t*>(parent.internalPointer());
        Q_ASSERT(pnode);
        return !pnode->children.isEmpty();
    }
    return !mp_root->children.isEmpty();
}

int QVariantModel::rowCount(const QModelIndex& parent) const
{
    if (parent.isValid()) {
        node_t* pnode = static_cast<node_t*>(parent.internalPointer());
        Q_ASSERT(pnode);
        return pnode->children.count();
    }
    return mp_root->children.count();
}

int QVariantModel::columnCount(const QModelIndex& parent) const
{
    Q_UNUSED(parent);
    return 3;
}

Qt::ItemFlags QVariantModel::flags(const QModelIndex& index) const
{
    Q_UNUSED(index);
    return Qt::ItemIsSelectable | Qt::ItemIsEnabled;
}

QVariant QVariantModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    if (orientation == Qt::Horizontal) {
        if (role == Qt::DisplayRole) {
            switch (section)
            {
            case 0:
                return QLatin1Literal("Key / Index");
            case 1:
                return QLatin1Literal("Value");
            case 2:
                return QLatin1Literal("Data type");
            default:
                break;
            }
        }
    }
    else {
        if (role == Qt::DisplayRole)
            return section;
    }
    return QVariant();
}

QVariant QVariantModel::data(const QModelIndex& index, int role) const
{
    qDebug() << Q_FUNC_INFO << index << role;
    if (!index.isValid())
        return QVariant();

    node_t* node = static_cast<node_t*>(index.internalPointer());
    Q_ASSERT(node);

    if (role == Qt::DisplayRole) {
        switch (index.column())
        {
        case 0:
            return QVariantDataInfo(node->keyInParent).displayText();
        case 1:
            return QVariantDataInfo(node->value).displayText(m_depth);
        case 2:
            return node->value.typeName();
        default:
            break;
        }
    }

    return QVariant();
}

//void QVariantModel::setData(const QModelIndex& index, const QVariant& value, int role);
