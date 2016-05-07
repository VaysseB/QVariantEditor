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

    bool wasFiltered = !m_filterRx.pattern().isEmpty();
    if (wasFiltered)
        filterTree(*mp_root);

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
        node.visibleChildren = node.children;
    }
}

void QVariantModel::setDisplayDepth(uint depth)
{
    if (m_depth != depth)
        return;

    m_depth = depth;
    emit displayDepthChanged(m_depth);

    // update all children that are containers, and only them
    if (column(ValueColumn) >= 0) {
        QVector<int> roles;
        roles << Qt::DisplayRole;

        // working list of all nodes that are containers
        QList<node_t*> nodeContainers;

        // add direct containers nodes below root
        for (auto it = mp_root->visibleChildren.constBegin();
             it != mp_root->visibleChildren.constEnd(); ++it) {
            if ((*it)->visibleChildren.isEmpty() == false)
                nodeContainers.append(*it);
        }

        while(nodeContainers.isEmpty() == false) {
            node_t* node = nodeContainers.takeFirst();
            QModelIndex index = indexOfNode(node, ValueColumn);
            emit dataChanged(index, index, roles);

            // add all children that are also containers
            for (auto it = node->visibleChildren.constBegin();
                 it != node->visibleChildren.constEnd(); ++it) {
                if ((*it)->visibleChildren.isEmpty() == false)
                    nodeContainers.append(*it);
            }
        }
    }
}

//------------------------------------------------------------------------------

QModelIndex QVariantModel::indexOfNode(node_t* node, int column) const
{
    Q_ASSERT(node != nullptr);
    int row = 0;
    if (node->parent)
        row = node->parent->visibleChildren.indexOf(node);
    Q_ASSERT(row >= 0);
    return createIndex(row, column, node);
}

QModelIndex QVariantModel::indexOfNode(node_t* node, Column col) const
{
    return indexOfNode(node, column(col));
}

QModelIndex QVariantModel::index(int row, int column,
                                 const QModelIndex& parent) const
{
    Q_ASSERT(column >= 0 && column < columnCount());

    node_t* pnode = mp_root.data();
    if (parent.isValid())
        pnode = static_cast<node_t*>(parent.internalPointer());
    Q_ASSERT(pnode);

    int childrenCount = pnode->visibleChildren.count();
    Q_ASSERT(row >= 0 && row < childrenCount);
    return createIndex(row, column, pnode->visibleChildren.at(row));
}

QModelIndex QVariantModel::parent(const QModelIndex& child) const
{
    if (child.isValid()) {
        node_t* node = static_cast<node_t*>(child.internalPointer());
        Q_ASSERT(node);
        if (node->parent && node->parent != mp_root.data())
            return indexOfNode(node->parent, child.column());
    }
    return QModelIndex();
}

bool QVariantModel::hasChildren(const QModelIndex& parent) const
{
    node_t* pnode = mp_root.data();
    if (parent.isValid())
        pnode = static_cast<node_t*>(parent.internalPointer());
    Q_ASSERT(pnode);
    return !pnode->visibleChildren.isEmpty();
}

int QVariantModel::rowCount(const QModelIndex& parent) const
{
    node_t* pnode = mp_root.data();
    if (parent.isValid())
        pnode = static_cast<node_t*>(parent.internalPointer());
    Q_ASSERT(pnode);
    return pnode->visibleChildren.count();
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

    QVector<int> displayRole;
    displayRole << Qt::DisplayRole;

    if (index.column() == column(KeyColumn)) {
        Q_ASSERT(node->parent != nullptr);

        QVariant oldKey = node->keyInParent;

        // update the value
        node->keyInParent = value;

        // update direct parent about node key
        QMutableVariantDataInfo mutDInfoParent(node->parent->value);
        Q_ASSERT(mutDInfoParent.isValid());
        Q_ASSERT(mutDInfoParent.isContainer());
        Q_ASSERT(mutDInfoParent.editableKeys());
        mutDInfoParent.setContainerKey(oldKey, node->keyInParent);

        // update the node index
        emit dataChanged(index, index, displayRole);

        // update its parents
        node_t* parentNode = node->parent;
        QModelIndex parentIndex = index.parent();
        while (parentNode->parent != nullptr) {
            Q_ASSERT(parentIndex.isValid());
            Q_ASSERT(parentIndex.internalPointer() == parentNode);

            // change data in parent
            QMutableVariantDataInfo mutDInfoParent(parentNode->parent->value);
            Q_ASSERT(mutDInfoParent.isValid());
            Q_ASSERT(mutDInfoParent.isContainer());
            Q_ASSERT(mutDInfoParent.editableValues());
            mutDInfoParent.setContainerValue(parentNode->keyInParent,
                                             parentNode->value);

            // next parent (because dataChanged must take the node's parent)
            parentNode = parentNode->parent;
            parentIndex = parentIndex.parent();

            // update parent of parentIndex
            emit dataChanged(parentIndex, parentIndex, displayRole);
        }

        // the order might changed if sort
        // do begin/end move rows
        if (m_dynamicSort)
            sortTree(*node->parent, false);

        dataChanges = true;
    }
    else  if (index.column() == column(ValueColumn)) {
        Q_ASSERT(node->parent != nullptr);

        bool typeChanged = (node->value.userType() != value.userType());

        // update the value
        node->value = value;

        // emit the update of the row (we want to update value+type columns)
        QVector<int> roles = QVector<int>(displayRole) << role;
        emit dataChanged(index, index, roles); // update value column
        if (typeChanged) {
            // update type column
            emit dataChanged(indexOfNode(node, TypeColumn),
                             indexOfNode(node, TypeColumn),
                             roles);
        }

        // update its parents
        node_t* parentNode = node;
        QModelIndex parentIndex = index;
        while (parentNode->parent != nullptr) {
            Q_ASSERT(parentIndex.isValid());
            Q_ASSERT(parentIndex.internalPointer() == parentNode);

            // change data in parent
            QMutableVariantDataInfo mutDInfoParent(parentNode->parent->value);
            Q_ASSERT(mutDInfoParent.isValid());
            Q_ASSERT(mutDInfoParent.isContainer());
            Q_ASSERT(mutDInfoParent.editableValues());
            mutDInfoParent.setContainerValue(parentNode->keyInParent,
                                             parentNode->value);

            // next parent (because dataChanged must take the node's parent)
            parentNode = parentNode->parent;
            parentIndex = parentIndex.parent();

            // update parent of parentIndex
            emit dataChanged(parentIndex, parentIndex, displayRole);
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

    if (rowCount() > 0)
        sortTree(*mp_root, true);
}

void QVariantModel::sortTree(node_t& root, bool recursive)
{
    QList<node_t*> newChildOrder = root.visibleChildren;

    // sort direct children
    std::sort(newChildOrder.begin(), newChildOrder.end(),
              [this] (const node_t* left, const node_t* right) {
        return lessThan(left->keyInParent, right->keyInParent);
    });

    QModelIndex parent;
    if (root.parent)
        parent = indexOfNode(&root, KeyColumn);

    // move rows
    int count = newChildOrder.count();
    for (int iNewChild = 0; iNewChild < count; iNewChild++) {
        node_t* child = newChildOrder.at(iNewChild);
        int iOldChild = root.visibleChildren.indexOf(child);
        Q_ASSERT(iOldChild >= 0);
        // if the child moved
        if (iNewChild < iOldChild) {
            bool canMoveRow = beginMoveRows(parent, iOldChild, iOldChild,
                                            parent, iNewChild);
            Q_ASSERT(canMoveRow);
            root.visibleChildren.removeAt(iOldChild);
            root.visibleChildren.insert(iNewChild, child);
            endMoveRows();
        }
    }

    if (recursive && root.visibleChildren.isEmpty() == false) {
        for (auto it = root.visibleChildren.begin();
             it != root.visibleChildren.end(); ++it) {
            sortTree(*(*it), recursive);
        }
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
            if (leftString.count() != rightString.count())
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

void QVariantModel::updateFilterRx(QString pattern)
{
    if (pattern.isEmpty() == false) {
        if (!pattern.startsWith("*") && m_filterType == WildCard)
            pattern.prepend(QChar('*'));
        if (!pattern.endsWith("*") && m_filterType == WildCard)
            pattern.append(QChar('*'));
    }

    m_filterRx = QRegExp(pattern, Qt::CaseSensitive,
                         (m_filterType == WildCard)
                         ? QRegExp::WildcardUnix : QRegExp::RegExp2);
}

void QVariantModel::setFilterType(FilterType filterType)
{
    if (m_filterType == filterType)
        return;

    bool wasFiltered = !m_filterRx.pattern().isEmpty();

    m_filterType = filterType;
    updateFilterRx(m_filterRx.pattern());
    emit filterTypeChanged(filterType);

    bool nowFiltered = !m_filterRx.pattern().isEmpty();

    if (wasFiltered || nowFiltered) {
        beginResetModel();
        filterTree(*mp_root);
        endResetModel();

        dumpTree(mp_root.data());
        dumpModel();
    }
}

void QVariantModel::setFilterColumns(Columns filterColumns)
{
    if (m_filterColumns == filterColumns)
        return;

    bool wasFiltered = !m_filterRx.pattern().isEmpty();

    m_filterColumns = filterColumns;
    emit filterColumnsChanged(filterColumns);

    bool nowFiltered = !m_filterRx.pattern().isEmpty();

    if (wasFiltered || nowFiltered) {
        beginResetModel();
        filterTree(*mp_root);
        endResetModel();

        dumpTree(mp_root.data());
        dumpModel();
    }
}

void QVariantModel::setFilterText(const QString& filterText)
{
    if (m_filterRx.pattern() == filterText)
        return;

    bool wasFiltered = !m_filterRx.pattern().isEmpty();

    updateFilterRx(filterText);
    emit filterTextChanged(filterText);

    bool nowFiltered = !m_filterRx.pattern().isEmpty();

    if (wasFiltered || nowFiltered) {
        beginResetModel();
        filterTree(*mp_root);
        endResetModel();

        dumpTree(mp_root.data());
        dumpModel();
    }
}

bool QVariantModel::filterKey(const QVariant& key) const
{
    const QString text = QVariantDataInfo(key).displayText();
    return filterOnDisplayText(text);
}

bool QVariantModel::filterValue(const QVariant& value) const
{
    const QString text = QVariantDataInfo(value).displayText(m_depth);
    return filterOnDisplayText(text);
}

bool QVariantModel::filterType(int type) const
{
    const QString text = QVariant::typeToName(type);
    return filterOnDisplayText(text);
}

bool QVariantModel::filterOnDisplayText(const QString& text) const
{
    bool isRowOk = false;

    if (m_filterType == Contains)
        isRowOk = text.contains(m_filterRx.pattern());
    else if (m_filterType == WildCard || m_filterType == Regex)
        isRowOk = m_filterRx.exactMatch(text);
    else if (m_filterType == Fixed)
        isRowOk = (text == m_filterRx.pattern());

    return isRowOk;
}

bool QVariantModel::filterTree(node_t& root)
{
    bool isVisible = false;

    // display all
    if (m_filterRx.pattern().isEmpty()) {
        root.visibleChildren = root.children;
        foreach(node_t* node, root.visibleChildren)
            filterTree(*node);

        if (m_dynamicSort)
            sortTree(root, false);

        isVisible = true;
    }
    // actual filter
    else {
        // check children first
        root.visibleChildren = root.children;
        foreach(node_t* node, root.visibleChildren) {
            bool childVisible = filterTree(*node);
            isVisible |= childVisible;
            if (childVisible == false)
                root.visibleChildren.removeOne(node);
        }

        if (m_dynamicSort)
            sortTree(root, false);

        // if at least one children is visible, we are visible
        // otherwise, we look for ourself
        if (isVisible == false) {
            bool fkey = m_filterColumns & KeyColumn;
            bool vkey = m_filterColumns & ValueColumn;
            bool tkey = m_filterColumns & TypeColumn;

            isVisible |= (fkey && filterKey(root.keyInParent));
            isVisible |= (vkey && filterValue(root.value));
            isVisible |= (tkey && filterType(root.value.userType()));
        }
    }

    return isVisible;
}

//------------------------------------------------------------------------------

void QVariantModel::dumpTree(const node_t* root,
                             const QString& prefix) const
{
#if defined(QT_DEBUG) && defined(QVARIANTMODEL_DEBUG)
    qDebug().nospace() << (prefix
                           + "node(type="
                           + QString::number(root->value.userType())
                           + "-" + root->value.typeName()
                           + ", key="
                           + root->keyInParent.toString()
                           + ", value="
                           + root->value.toString()
                           + ")");
    foreach(node_t* child, root->visibleChildren)
        dumpTree(child, prefix + "--");
#else
    Q_UNUSED(root)
    Q_UNUSED(prefix)
#endif
}

void QVariantModel::dumpModel(const QModelIndex& rootIndex,
                              const QString& prefix) const
{
#if defined(QT_DEBUG) && defined(QVARIANTMODEL_DEBUG)
    if (rootIndex.isValid()) {
        qDebug().nospace() << (prefix
                               + "index(row="
                               + QString::number(rootIndex.row())
                               + ", column="
                               + QString::number(rootIndex.column())
                               + ", text=\""
                               + data(rootIndex, Qt::DisplayRole).toString()
                               + "\")");
    }
    else {
        qDebug().nospace() << (prefix + "index(invalid)");
    }
    for (int i=0; i<rowCount(rootIndex); i++) {
        for (int j=0; j<columnCount(rootIndex); j++)
            dumpModel(index(i, j, rootIndex), prefix + "--");
    }
#else
    Q_UNUSED(rootIndex)
    Q_UNUSED(prefix)
#endif
}
