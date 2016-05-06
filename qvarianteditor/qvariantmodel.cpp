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

void QVariantModel::setRootDatas(const QVariantList &rootDatas)
{
    beginResetModel();
    mp_root.reset(new node_t);
    buildTree(*mp_root, QVariant(rootDatas));
    endResetModel();

    emit rootDatasChanged(rootDatas);
}

QVariantList QVariantModel::rootDatas() const
{
    return mp_root->value.toList();
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

    if (dInfo.isValid() && dInfo.isContainer()) {
        QVariantList keys = dInfo.containerKeys();
        node.children.reserve(keys.count());
        for (auto itkey = keys.constBegin(); itkey != keys.constEnd(); ++itkey){
            QVariant childData = dInfo.containerValue(*itkey);
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
    if (column(ValueColumn) >= 0)
        emit dataChanged(index(0, column(ValueColumn)), index(rowCount()-1, column(ValueColumn)));
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
            Q_ASSERT(dInfo.isValid() && dInfo.isContainer());
            int row = dInfo.containerKeys().indexOf(node->keyInParent);
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
    return ColumnCount;
}

Qt::ItemFlags QVariantModel::flags(const QModelIndex& index) const
{
    Qt::ItemFlags flags = Qt::ItemIsSelectable | Qt::ItemIsEnabled;
    if (!index.isValid())
        return flags;

    node_t* node = static_cast<node_t*>(index.internalPointer());
    Q_ASSERT(node);

    QMutableVariantDataInfo mutDInfo(node->value);
    if (mutDInfo.isValid() == false)
        return flags;

    if (index.column() == column(ValueColumn)) {
        if (mutDInfo.isAtomic()) {

            // editable only if the parent can update the value
            QMutableVariantDataInfo mutDInfoParent(node->parent->value);
            if (mutDInfoParent.isValid() && mutDInfoParent.isContainer()) {
                if (mutDInfoParent.flags() & QMutableVariantDataInfo::ValuesAreEditable)
                    flags |= Qt::ItemIsEditable;
            }
            // if no parent
            else {
                flags |= Qt::ItemIsEditable;
            }
        }
    }
    else if (index.column() == column(TypeColumn)) {
        flags |= Qt::ItemIsEditable;
    }

    return flags;
}

QVariant QVariantModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    if (orientation == Qt::Horizontal) {
        if (role == Qt::DisplayRole) {
            if (section == column(KeyColumn))
                return QLatin1Literal("Key / Index");
            else if (section == column(ValueColumn))
                return QLatin1Literal("Value");
            else if (section == column(TypeColumn))
                return QLatin1Literal("Data type");
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
    if (!index.isValid())
        return QVariant();

    node_t* node = static_cast<node_t*>(index.internalPointer());
    Q_ASSERT(node);

    if (role == Qt::DisplayRole) {
        if (index.column() == column(KeyColumn))
            return QVariantDataInfo(node->keyInParent).displayText();
        else if (index.column() == column(ValueColumn))
            return QVariantDataInfo(node->value).displayText(m_depth);
        else if (index.column() == column(TypeColumn))
            return node->value.typeName();
    }
    else if (role == Qt::EditRole) {
        if (index.column() == column(KeyColumn))
            return node->keyInParent;
        else if (index.column() == column(ValueColumn))
            return node->value;
        else if (index.column() == column(TypeColumn))
            return node->value.type();
    }

    return QVariant();
}

//------------------------------------------------------------------------------

int QVariantModel::column(Column column) const
{
    switch(column)
    {
    case QVariantModel::KeyColumn:
        return 0;
    case QVariantModel::ValueColumn:
        return 1;
    case QVariantModel::TypeColumn:
        return 2;
    default:
        break;
    }
    return -1;
}

//int QVariantModel::realColumn(Column col) const
//{
//    switch(col)
//    {
//    case KeyColumn:
//        return keyColumn();
//    case ValueColumn:
//        return valueColumn();
//    case TypeColumn:
//        return typeColumn();
//    }
//    return -1;
//}

//void QVariantModel::swapColumn(int column1, int column2)
//{

//}

//void QVariantModel::swapColumn(Column column1, Column column2)
//{
//    int col1 = realColumn(column1);
//    int col2 = realColumn(column2);
//    if (col1 >= 0 && col2 >= 0)
//        swapColumn(col1, col2);
//}

//void QVariantModel::setData(const QModelIndex& index, const QVariant& value, int role);
