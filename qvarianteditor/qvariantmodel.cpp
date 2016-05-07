#include "qvariantmodel.h"

#include <QDebug>

#include "qvariantdatainfo.h"


QVariantModel::QVariantModel(QObject *parent) :
    QAbstractItemModel(parent),
    mp_root(new node_t),
    m_depth(1),
    m_dynamicSort(false)
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
            int row = node->parent->children.indexOf(node);
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

    if (index.column() == column(KeyColumn)) {
        // key should be atomic
        QVariantDataInfo dInfoKey(node->keyInParent);
        if (dInfoKey.isAtomic()) {
            Q_ASSERT(node->parent);

            bool parentUpdatable = true;

            // editable only if the direct parent can update keys
            QMutableVariantDataInfo mutDInfoDirectParent(node->parent->value);
            Q_ASSERT(mutDInfoDirectParent.isValid());
            Q_ASSERT(mutDInfoDirectParent.isContainer());
            parentUpdatable = mutDInfoDirectParent.editableKeys();

            if (parentUpdatable == false)
                return flags;

            node = node->parent;

            // editable only if all the parents can update values
            // (from the node parent)
            while (node->parent != nullptr && parentUpdatable) {
                QMutableVariantDataInfo mutDInfoParent(node->parent->value);
                Q_ASSERT(mutDInfoParent.isValid());
                Q_ASSERT(mutDInfoParent.isContainer());
                parentUpdatable = mutDInfoParent.editableValues();
                node = node->parent;
            }

            // if no parent or all parents are editable
            if (parentUpdatable) {
                flags |= Qt::ItemIsEditable;
            }
        }
    }
    else if (index.column() == column(ValueColumn)) {
        Q_ASSERT(node->parent);

        // value should be atomic
        if (mutDInfo.isAtomic()) {
            // editable only if all the parents can update values
            bool parentUpdatable = true;
            while (node->parent != nullptr && parentUpdatable) {
                QMutableVariantDataInfo mutDInfoParent(node->parent->value);
                Q_ASSERT(mutDInfoParent.isValid());
                Q_ASSERT(mutDInfoParent.isContainer());
                parentUpdatable = mutDInfoParent.editableValues();
                node = node->parent;
            }

            // if no parent or all parents are editable
            if (parentUpdatable) {
                flags |= Qt::ItemIsEditable;
            }
        }
    }
    else if (index.column() == column(TypeColumn)) {
        flags |= Qt::ItemIsEditable;
    }

    return flags;
}

QVariant QVariantModel::headerData(int section,
                                   Qt::Orientation orientation,
                                   int role) const
{
    if (orientation == Qt::Horizontal) {
        if (role == Qt::DisplayRole) {
            if (section == column(KeyColumn))
                return tr("Key / Index");
            else if (section == column(ValueColumn))
                return tr("Value");
            else if (section == column(TypeColumn))
                return tr("Data type");
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

//------------------------------------------------------------------------------

bool QVariantModel::setData(const QModelIndex& index,
                            const QVariant& value,
                            int role)
{
    if (index.isValid() == false)
        return false;
    if (role != Qt::EditRole)
        return false;

    node_t* node = static_cast<node_t*>(index.internalPointer());
    Q_ASSERT(node != nullptr);

    // if no changes (fast comparison)
    if (node->value.toString().isEmpty() == false
            && node->value.toString() == value.toString())
        return true;

    bool dataChanges = false;

    if (index.column() == column(KeyColumn)) {
        Q_ASSERT(node->parent != nullptr);

        QVariant oldKey = node->keyInParent;

        // update the value
        node->keyInParent = value;

        // the order might changed if sort
        if (m_dynamicSort) {
            sortTree(*node->parent, false);

            // emit the update on all children of the node parent
            // (in case of changing order)
            int chCount = node->parent->children.count();
            emit dataChanged(createIndex(0, columnCount()-1, node->parent),
                             createIndex(chCount-1, columnCount()-1, node->parent),
                             QVector<int>() << Qt::DisplayRole);
        }
        // or the order doesn't change at all
        else {
            // emith the update of the index node only
            emit dataChanged(index, index, QVector<int>() << Qt::DisplayRole);
        }

        // update direct parent about node key
        QMutableVariantDataInfo mutDInfoParent(node->parent->value);
        Q_ASSERT(mutDInfoParent.isValid());
        Q_ASSERT(mutDInfoParent.isContainer());
        Q_ASSERT(mutDInfoParent.editableKeys());
        mutDInfoParent.setContainerKey(oldKey, node->keyInParent);

        // emit the update on all columns of the node parent
        // (in case of changing order)
        int chCount = node->parent->children.count();
        emit dataChanged(createIndex(0, columnCount()-1, node->parent),
                         createIndex(chCount-1, columnCount()-1, node->parent),
                         QVector<int>() << Qt::DisplayRole);

        // update all parents
        node = node->parent;
        QModelIndex indexNode = index;
        while (node->parent != nullptr) {
            // change data in parent
            QMutableVariantDataInfo mutDInfoParent(node->parent->value);
            Q_ASSERT(mutDInfoParent.isValid());
            Q_ASSERT(mutDInfoParent.isContainer());
            Q_ASSERT(mutDInfoParent.editableValues());

            mutDInfoParent.setContainerValue(node->keyInParent, node->value);

            QModelIndex parentIndex = indexNode.parent();
            emit dataChanged(parentIndex,
                             parentIndex,
                             QVector<int>() << Qt::DisplayRole);

            // next parent
            node = node->parent;
            indexNode = parentIndex;
        }

        dataChanges = true;
    }
    else  if (index.column() == column(ValueColumn)) {
        Q_ASSERT(node->parent != nullptr);

        // update the value
        node->value = value;
        // emit the update of the row (we want to update value+type)
        emit dataChanged(createIndex(index.row(), 0, node),
                         createIndex(index.row(), columnCount()-1, node),
                         QVector<int>() << role << Qt::DisplayRole);

        // update all parents
        QModelIndex indexNode = index;
        while (node->parent != nullptr) {
            // change data in parent
            QMutableVariantDataInfo mutDInfoParent(node->parent->value);
            Q_ASSERT(mutDInfoParent.isValid());
            Q_ASSERT(mutDInfoParent.isContainer());
            Q_ASSERT(mutDInfoParent.editableValues());
            mutDInfoParent.setContainerValue(node->keyInParent, node->value);

            QModelIndex parentIndex = indexNode.parent();
            emit dataChanged(parentIndex,
                             parentIndex,
                             QVector<int>() << Qt::DisplayRole);

            // next parent
            node = node->parent;
            indexNode = parentIndex;
        }

        dataChanges = true;
    }

    if (dataChanges)
        emit rootDatasChanged(rootDatas());

    return dataChanges;
}

//------------------------------------------------------------------------------

void QVariantModel::setDynamicSort(bool enabled)
{
    if (m_dynamicSort == enabled)
        return;

    m_dynamicSort = enabled;
    emit dynamicSortChanged(enabled);

    if (rowCount() > 0) {
        sortTree(*mp_root, true);
        emit dataChanged(index(0, columnCount()-1),
                         index(rowCount()-1, columnCount()-1),
                         QVector<int>() << Qt::DisplayRole);
    }
}

void QVariantModel::sortTree(node_t& root, bool recursive)
{
    // sort direct children
    std::sort(root.children.begin(), root.children.end(),
              [this] (const node_t* left, const node_t* right) {
        return lessThan(left->keyInParent, right->keyInParent);
    });

    if (recursive && root.children.isEmpty() == false) {
        for (auto it = root.children.begin(); it != root.children.end(); ++it)
            sortTree(*(*it), recursive);
    }
}

bool QVariantModel::lessThan(const QVariant& left, const QVariant& right) const
{
    bool isLess = true;

    QVariant::Type ltype = left.type();
    QVariant::Type rtype = right.type();
    bool sameType = (ltype == rtype);

    // not same type
    if (sameType == false) {
        if (ltype == QVariant::UInt && rtype == QVariant::Int)
            isLess = ((int)left.toUInt() < right.toInt());
        else if (ltype == QVariant::Int && rtype == QVariant::UInt)
            isLess = (left.toInt() < (int)right.toUInt());
        else if (ltype == QVariant::Double && rtype == QVariant::Int)
            isLess = (left.toDouble() < right.toInt());
        else if (ltype == QVariant::Int && rtype == QVariant::Double)
            isLess = (left.toInt() < right.toDouble());
        else if (ltype == QVariant::UInt && rtype == QVariant::Double)
            isLess = (left.toUInt() < right.toDouble());
        else if (ltype == QVariant::Double && rtype == QVariant::UInt)
            isLess = (left.toDouble() < right.toUInt());
        else
            isLess = (left < right);
    }
    // same type
    else {
        if (ltype == QVariant::String) {
            QString leftString = left.toString();
            QString rightString = right.toString();
            if (leftString.count() == rightString.count())
                isLess = (leftString.count() < rightString.count());
            else
                isLess = QString::compare(leftString, rightString) < 0;
        }
        else if (ltype == QVariant::Bool)
            isLess = (left.toBool() < right.toBool());
        else if (ltype == QVariant::Int)
            isLess = (left.toInt() < right.toInt());
        else if (ltype == QVariant::UInt)
            isLess = (left.toUInt() < right.toUInt());
        else if (ltype == QVariant::Double)
            isLess = (left.toUInt() < right.toUInt());
        else
            isLess = (left < right);
    }


    return isLess;
}

//------------------------------------------------------------------------------

void QVariantModel::dumpTree(const node_t* root, const QString& prefix) const
{
    qDebug().nospace() << (prefix + "node(type="
                           + QString::number(root->value.userType())
                           + "-" + root->value.typeName()
                           + ", key=" + root->keyInParent.toString()
                           + ", value=" + root->value.toString()
                           + ")");
    foreach(node_t* child, root->children)
        dumpTree(child, prefix + "--");
}
