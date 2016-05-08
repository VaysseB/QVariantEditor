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

    if (mp_root)
        freeNode(*mp_root);

    mp_root.reset(new node_t);
    mp_root->value = rootDatas;
    mp_root->parent = nullptr;
    rebuildTree(*mp_root);

    // filter & sort
    bool wasFiltered = !m_filterRx.pattern().isEmpty();
    if (wasFiltered)
        filterTree(*mp_root, DynamicSortPolicy);

    endResetModel();

    emit rootDatasChanged(rootDatas);
}

QVariantList QVariantModel::rootDatas() const
{
    return mp_root->value.toList();
}

void QVariantModel::freeNode(node_t& node) const
{
    while (node.children.isEmpty() == false)
        freeNode(*node.children.takeLast());
}

void QVariantModel::rebuildTree(node_t& node) const
{
    // clear children if any
    node.visibleChildren.clear();

    QVariantDataInfo dInfo(node.value);

    if (dInfo.isContainer()) {
        QVariantList keys = dInfo.containerKeys();
        // resize children
        node.children.reserve(keys.count());
        while (node.children.count() > keys.count()) {
            freeNode(*node.children.last());
            delete node.children.takeLast();
        }
        //
        int iChild = 0;
        for (auto itkey = keys.constBegin(); itkey != keys.constEnd(); ++itkey){
            QVariant childData = dInfo.containerValue(*itkey);
            node_t* child = node.children.value(iChild++, nullptr);
            if (child == nullptr) {
                child = new node_t;
                child->parent = &node;
                node.children.append(child);
            }
            child->keyInParent = *itkey;
            child->value = childData;
            rebuildTree(*child);
        }
        node.visibleChildren = node.children;
    }
    else {
        while (node.children.isEmpty() == false) {
            freeNode(*node.children.last());
            delete node.children.takeLast();
        }
    }
}

void QVariantModel::setDisplayDepth(uint depth)
{
    if (m_depth == depth)
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

    if (node->value.isValid() == false)
        return flags;

    QMutableVariantDataInfo mutDInfo(node->value);
    if (index.column() == column(KeyColumn)) {
        // key should be atomic
        QVariantDataInfo dInfoKey(node->keyInParent);
        if (dInfoKey.isAtomic()) {
            Q_ASSERT(node->parent);

            bool parentUpdatable = true;

            // editable only if the direct parent can update keys
            QMutableVariantDataInfo mutDInfoDirectParent(node->parent->value);
            Q_ASSERT(mutDInfoDirectParent.isContainer());
            parentUpdatable = mutDInfoDirectParent.editableKeys();

            if (parentUpdatable == false)
                return flags;

            node = node->parent;

            // editable only if all the parents can update values
            // (from the node parent)
            while (node->parent != nullptr && parentUpdatable) {
                QMutableVariantDataInfo mutDInfoParent(node->parent->value);
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

void QVariantModel::invalidateSubTree(QModelIndex index)
{
    node_t* node = mp_root.data();
    if (index.isValid())
        node = static_cast<node_t*>(index.internalPointer());
    Q_ASSERT(node != nullptr);

    QVector<int> displayRole;
    displayRole << Qt::DisplayRole;

    // update its parents
    while (node->parent != nullptr) {
        Q_ASSERT(index.isValid());
        Q_ASSERT(index.internalPointer() == node);

        // change data in parent
        QMutableVariantDataInfo mutDInfoParent(node->parent->value);
        Q_ASSERT(mutDInfoParent.isContainer());
        Q_ASSERT(mutDInfoParent.editableValues());
        mutDInfoParent.setContainerValue(node->keyInParent,
                                         node->value);

        // next parent (because dataChanged must take the node's parent)
        node = node->parent;
        index = index.parent();

        // update parent of index
        emit dataChanged(index, index, displayRole);
    }
}

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

        // update direct parent about node key
        QMutableVariantDataInfo mutDInfoParent(node->parent->value);
        Q_ASSERT(mutDInfoParent.isContainer());
        Q_ASSERT(mutDInfoParent.editableKeys());
        mutDInfoParent.setContainerKey(oldKey, node->keyInParent);

        // update the node index
        emit dataChanged(index, index, QVector<int>() << Qt::DisplayRole);

        // update its parents
        invalidateSubTree(index.parent());

        // the order might changed if sort
        // do begin/end move rows
        if (m_dynamicSort)
            sortTree(*node->parent, SortNodeOnly);

        dataChanges = true;
    }
    else  if (index.column() == column(ValueColumn)) {
        Q_ASSERT(node->parent != nullptr);
        // when change value, type might change

        bool typeChanged = (node->value.userType() != value.userType());

        // update the value
        node->value = value;

        // emit the update of the row (we want to update value+type columns)
        QVector<int> roles = QVector<int>() << Qt::DisplayRole << role;

        // update value column
        emit dataChanged(index, index, roles);

        // update type column if changed
        if (typeChanged) {
            QModelIndex typeIndex = indexOfNode(node, TypeColumn);
            emit dataChanged(typeIndex, typeIndex, roles);
        }

        // update its parents
        invalidateSubTree(index);

        dataChanges = true;
    }
    else  if (index.column() == column(TypeColumn)) {
        Q_ASSERT(node->parent != nullptr);
        // when change type, value change automatically

        // update the value and the type, as simple as this
        QVariantDataInfo dInfo(node->value);
        node->value = dInfo.tryConvert(value.type(), value.userType());

        // emit the update of the row (we want to update value+type columns)
        QVector<int> roles = QVector<int>() << Qt::DisplayRole << role;

        // update type column
        emit dataChanged(index, index, roles);

        // update value column
        QModelIndex valueIndex = indexOfNode(node, ValueColumn);
        emit dataChanged(valueIndex, valueIndex, roles);

        // update its parents
        invalidateSubTree(index);

        dataChanges = true;
    }

    if (dataChanges)
        emit rootDatasChanged(rootDatas());

    return dataChanges;
}

bool QVariantModel::insertRows(int row, int count, const QModelIndex& parent)
{
    Q_ASSERT(row >= 0 && count >= 0 && row+count <= rowCount(parent)+1);

    bool dataInserted = true;

    node_t* pnode = mp_root.data();
    if (parent.isValid())
        pnode = static_cast<node_t*>(parent.internalPointer());
    Q_ASSERT(pnode);

    QMutableVariantDataInfo mutDInfo(pnode->value);
    Q_ASSERT(mutDInfo.isContainer());
    Q_ASSERT(mutDInfo.isNewKeyInsertable());

    int childrenCount = pnode->visibleChildren.count();
    Q_ASSERT(row >= 0 && row <= childrenCount); // row == count when append

    // add as much rows needed
    while (count-- > 0) {
        node_t* beforeNode = pnode->visibleChildren.value(row, nullptr);
        QVariant beforeKey = beforeNode ? beforeNode->keyInParent : QVariant();
        mutDInfo.tryInsertNewKey(beforeKey, QVariant());
    }

    // update all the keys (because the order might have changed with the new
    // key), rebuilding the node and all if its children
    // but first, we remove all rows, rebuild, and re-add rows
    beginRemoveRows(parent, 0, pnode->visibleChildren.count());
    pnode->visibleChildren.clear();
    endRemoveRows();

    rebuildTree(*pnode);
    filterTree(*pnode, NoSort); // filter now because row are not visible

    // take temporary children that are will be visible
    QList<node_t*> visibleChildren;
    pnode->visibleChildren.swap(visibleChildren);

    beginInsertRows(parent, 0, visibleChildren.count());
    pnode->visibleChildren.swap(visibleChildren);
    endInsertRows();

    // now rows are visible, we can sort
    if (m_dynamicSort)
        sortTree(*pnode, SortNodeAndChildren);

    // and update all parents
    invalidateSubTree(parent);

    // data changes
    emit rootDatasChanged(rootDatas());

    return dataInserted;
}

bool QVariantModel::removeRows(int row, int count, const QModelIndex& parent)
{
    Q_ASSERT(row >= 0 && count >= 0 && row+count <= rowCount(parent));

    bool dataRemoved = true;

    node_t* pnode = mp_root.data();
    if (parent.isValid())
        pnode = static_cast<node_t*>(parent.internalPointer());
    Q_ASSERT(pnode);

    QMutableVariantDataInfo mutDInfo(pnode->value);
    Q_ASSERT(mutDInfo.isContainer());
    Q_ASSERT(mutDInfo.isKeyRemovable());

    int childrenCount = pnode->visibleChildren.count();
    Q_ASSERT(row >= 0 && row+count <= childrenCount); // row == count when append

    // remove as much rows asked
    while (count-- > 0) {
        node_t* node = pnode->visibleChildren.value(row+count, nullptr);
        Q_ASSERT(node);
        mutDInfo.removeKey(node->keyInParent);
    }

    // update all the keys (because the order might have changed after remove),
    // rebuilding the node and all if its children
    // but first, we remove all rows, rebuild, and re-add rows
    beginRemoveRows(parent, 0, pnode->visibleChildren.count());
    pnode->visibleChildren.clear();
    endRemoveRows();

    rebuildTree(*pnode);
    filterTree(*pnode, NoSort); // filter now because row are not visible

    // take temporary children that are will be visible
    QList<node_t*> visibleChildren;
    pnode->visibleChildren.swap(visibleChildren);

    beginInsertRows(parent, 0, visibleChildren.count());
    pnode->visibleChildren.swap(visibleChildren);
    endInsertRows();

    // now rows are visible, we can sort
    if (m_dynamicSort)
        sortTree(*pnode, SortNodeAndChildren);

    // and update all parents
    invalidateSubTree(parent);

    // data changes
    emit rootDatasChanged(rootDatas());

    return dataRemoved;
}


//------------------------------------------------------------------------------

void QVariantModel::setDynamicSort(bool enabled)
{
    if (m_dynamicSort == enabled)
        return;

    m_dynamicSort = enabled;
    emit dynamicSortChanged(enabled);

    if (rowCount() > 0)
        sortTree(*mp_root, SortNodeAndChildren);
}

void QVariantModel::sortTree(node_t& root, InternalSortStrategy sortStrategy)
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

    if (sortStrategy == SortNodeAndChildren
            && root.visibleChildren.isEmpty() == false) {
        for (auto it = root.visibleChildren.begin();
             it != root.visibleChildren.end(); ++it) {
            sortTree(*(*it), sortStrategy);
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
        if (ltype == QVariant::String)
            isLess = QString::compare(left.toString(), left.toString()) < 0;
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
        filterTree(*mp_root, DynamicSortPolicy);
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
        filterTree(*mp_root, DynamicSortPolicy);
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
        filterTree(*mp_root, DynamicSortPolicy);
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

bool QVariantModel::filterTree(node_t& root, InternalSortPolicy sortPolicy)
{
    bool isVisible = false;

    // display all
    if (m_filterRx.pattern().isEmpty()) {
        root.visibleChildren = root.children;
        foreach(node_t* node, root.visibleChildren)
            filterTree(*node, sortPolicy);

        if (sortPolicy == ForceSort
                || (m_dynamicSort && sortPolicy == DynamicSortPolicy)) {
            sortTree(root, SortNodeOnly);
        }

        isVisible = true;
    }
    // actual filter
    else {
        // check children first
        root.visibleChildren = root.children;
        foreach(node_t* node, root.visibleChildren) {
            bool childVisible = filterTree(*node, sortPolicy);
            isVisible |= childVisible;
            if (childVisible == false)
                root.visibleChildren.removeOne(node);
        }

        if (sortPolicy == ForceSort
                || (m_dynamicSort && sortPolicy == DynamicSortPolicy)) {
            sortTree(root, SortNodeOnly);
        }

        // if at least one children is visible, we are visible
        // otherwise, we look for ourself
        if (isVisible == false)
            isVisible = isAcceptedNode(root);
    }

    return isVisible;
}

bool QVariantModel::isAcceptedNode(node_t& root) const
{
    bool isVisible = false;

    bool fkey = m_filterColumns & KeyColumn;
    bool vkey = m_filterColumns & ValueColumn;
    bool tkey = m_filterColumns & TypeColumn;

    isVisible |= (fkey && filterKey(root.keyInParent));
    isVisible |= (vkey && filterValue(root.value));
    isVisible |= (tkey && filterType(root.value.userType()));

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
