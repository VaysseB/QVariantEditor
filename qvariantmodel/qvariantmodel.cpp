#include "qvariantmodel.h"

#include <QThreadPool>
#include <QMutexLocker>
#include <QDebug>

#include "qvariantdatainfo.h"

#ifdef QT_DEBUG
#define QVM_DEBUG_MODEL_FUNC
//#define QVM_DEBUG_LOAD
//#define QVM_DEBUG_BUILD
//#define QVM_DEBUG_FILTER
#endif


#define DBG_NODE(root) (QString("node(key=") + QVariantDataInfo(root->keyInParent).displayText(1) + ", value=" + QVariantDataInfo(root->value).displayText(1) + ", type=" + QString::number(root->value.userType()) + "-" + root->value.typeName() + ")")



const char* QVariantModel::FILTER_TYPE_NAMES[] = {
    QT_TRANSLATE_NOOP("QVariantModel", "Contains"),
    QT_TRANSLATE_NOOP("QVariantModel", "Wildcard"),
    QT_TRANSLATE_NOOP("QVariantModel", "Regex"),
    QT_TRANSLATE_NOOP("QVariantModel", "Fixed")
};


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

QVariantList QVariantModel::rootDatas() const
{
    return mp_root->value.toList();
}

void QVariantModel::setRootDatas(const QVariantList &rootDatas)
{
    //    Q_ASSERT(mp_loader.isNull());

    beginResetModel();

    mp_root->value = rootDatas;
    mp_root->parent = nullptr;
    mp_root->isLoaded = rootDatas.isEmpty();

    endResetModel();

    emit rootDatasChanged();
}

void QVariantModel::clearChildren(node_t* root, int reserveSize)
{
    Q_ASSERT(root);
    // resize children
    root->visibleChildren.clear();
    while (root->children.isEmpty() == false) {
        node_t* child = root->children.takeLast();

        // if node is still loading, that's too bad, but to avaid leaks
        // the thread pool will delete it as soon as it finishes loading
        if (child->loader)
            child->loader->setAutoDelete(true);

        clearChildren(child);

        delete child;
    }
    root->children.reserve(reserveSize);
}

void QVariantModel::setLoadingHintNode(node_t *node, bool enable)
{
    Q_ASSERT(node);
    if (node->hintNode && enable == false) {
        delete node->hintNode;
        node->hintNode = nullptr;
    }
    else if (node->hintNode == nullptr && enable) {
        node->hintNode = new node_t;
        node->hintNode->parent = node;
        node->hintNode->isLoaded = true;
    }
}

QString QVariantModel::keyPath(const node_t* node)
{
    QString keyStr = QLatin1Literal("ROOT");
    if (node->parent) {
        keyStr = QVariantDataInfo(node->keyInParent).displayText(0);
        keyStr.prepend("->").prepend(keyPath(node->parent));
    }
    return keyStr;
}

void QVariantModel::invalidateOrder(node_t* node, int start, int length)
{
    int iChild = 0;
    for (auto it = node->visibleChildren.constBegin() + start;
         it != node->visibleChildren.constEnd() && length-- > 0; ++it) {
        node_t* child = *it;
        child->indexInParent = iChild++;
    }
}

//void QVariantModel::buildNode(node_t* root)
//{
//    Q_ASSERT(root);
//    Q_ASSERT(root->loadStatus != NodeLoaded);

//    node_t* hidden_root = mp_root.data();
//    Q_ASSERT((root->parent != nullptr) ^ (root == hidden_root));

//    QVariantDataInfo dInfo(root->value);
//    Q_ASSERT(dInfo.isContainer());

//    if (root->loadStatus == NodeNotLoaded
//            || root->loadStatus == NodeLoading) {
//        int totalCount = dInfo.containerCount();
//        int stepLoadedCount = qMin(totalCount - root->children.count(), 20);

//        if (root->loadStatus == NodeNotLoaded)
//            clearChildren(root, totalCount);

//        QVariantList keys = dInfo.containerPartKeys(root->children.count(),
//                                                    stepLoadedCount);
//        Q_ASSERT(keys.count() == stepLoadedCount);

//        qDebugLoad().nospace()
//                << "build node"
//                << " key:" << QVariantDataInfo(root->keyInParent).displayText()
//                << " loaded " << root->children.count()
//                << "/total " << dInfo.containerCount()
//                << "/loading now " << stepLoadedCount;

//        int prevCount = root->children.count();

//        QList<node_t*> addedChildren;
//        for (auto itkey = keys.constBegin(); itkey != keys.constEnd(); ++itkey){
//            QVariant childData = dInfo.containerValue(*itkey);

//            node_t* child = new node_t;
//            addedChildren.append(child);

//            child->parent = root;
//            child->keyInParent = *itkey;
//            child->value = childData;
//            child->visible = true;

//            // optimization: give the child loaded status as fully loaded
//            // if doesn't required to load something
//            child->loadStatus = NodeLoaded;
//            QVariantDataInfo childDInfo(child->value);
//            if (childDInfo.isContainer()
//                    && childDInfo.isEmptyContainer() == false) {
//                child->loadStatus = NodeNotLoaded;
//                setLoadingHintNode(child, true);
//            }
//        }

//        QModelIndex index = indexOfNode(root);

//        // add the newly created children
//        beginInsertRows(index, prevCount, prevCount + stepLoadedCount - 1);
//        root->children.append(addedChildren);
//        root->visibleChildren.append(addedChildren);
//        endInsertRows();

//        // todo: sort

//        // filter if enable
//        if (isFilterEnabled())
//            filterTree(root);

//        root->loadStatus = NodeLoading;

//        // if done loading, we saved it
//        if (root->children.count() == totalCount) {
//            root->loadStatus = NodeLoaded;
//            setLoadingHintNode(root, false);
//        }
//    }
//}

void QVariantModel::setDisplayDepth(uint depth)
{
    if (m_depth == depth)
        return;

    m_depth = depth;
    emit displayDepthChanged(m_depth);

    // update all children that are containers, and only them
    //    if (column(ValueColumn) >= 0) {
    //        QVector<int> roles;
    //        roles << Qt::DisplayRole;

    //        // working list of all nodes that are containers
    //        QList<node_t*> nodeContainers;

    //        // add direct containers nodes below root
    //        for (auto it = mp_root->visibleChildren.constBegin();
    //             it != mp_root->visibleChildren.constEnd(); ++it) {
    //            if ((*it)->visibleChildren.isEmpty() == false)
    //                nodeContainers.append(*it);
    //        }

    //        while(nodeContainers.isEmpty() == false) {
    //            node_t* node = nodeContainers.takeFirst();
    //            QModelIndex index = indexOfNode(node, ValueColumn);
    //            emit dataChanged(index, index, roles);

    //            // add all children that are also containers
    //            for (auto it = node->visibleChildren.constBegin();
    //                 it != node->visibleChildren.constEnd(); ++it) {
    //                if ((*it)->visibleChildren.isEmpty() == false)
    //                    nodeContainers.append(*it);
    //            }
    //        }
    //    }
}

//------------------------------------------------------------------------------

void QVariantModel::fetchMore(const QModelIndex &parent)
{
    node_t* pnode = mp_root.data();
    if (parent.isValid())
        pnode = static_cast<node_t*>(parent.internalPointer());
    Q_ASSERT(pnode != nullptr);
    Q_ASSERT(pnode->isLoaded == false);

#ifdef QVM_DEBUG_LOAD
    qDebug().nospace()
            << "fetchMore"
            << " key:" << keyPath(pnode)
            << " loader:" << ((void*)pnode->loader);
#endif

    // if never asked to load
    if (pnode->loader == nullptr) {
        // as we are supposed to never been asked to load, we should be empty
        Q_ASSERT(pnode->children.isEmpty());

        pnode->loader = new QVariantModelDataLoader(pnode);

        QVariantDataInfo dInfo(pnode->value);
        Q_ASSERT(dInfo.isContainer());
        int count = dInfo.containerCount();

        // if too much datas, load async
        // otherwise load sync
        if (count >= (int)SizeLimitToLoadAsync) {
#ifdef QVM_DEBUG_LOAD
            qDebug().nospace()
                    << "fetchMore"
                    << " key:" << keyPath(pnode)
                    << " start async";
#endif

            // load async
            QThreadPool* thPool = QThreadPool::globalInstance();
            Q_ASSERT(thPool);
            thPool->start(pnode->loader);

            // we insert the "please wait data are loading" row
            beginInsertRows(parent, 0, 0);
            setLoadingHintNode(pnode, true);
            endInsertRows();
        }
        else {
#ifdef QVM_DEBUG_LOAD
            qDebug().nospace()
                    << "fetchMore"
                    << " key:" << keyPath(pnode)
                    << " load sync";
#endif

            // load now
            pnode->loader->run();

            int count = pnode->loader->exclusive.createdChildren.count();

#ifdef QVM_DEBUG_LOAD
            qDebug().nospace()
                    << "fetchMore"
                    << " key:" << keyPath(pnode)
                    << " done count:" << count;
#endif

            // beware of this line: `isLoaded == false` supposed the hint node
            // is visible, so 1 is added to rowCount() if false
            pnode->isLoaded = true;

            beginInsertRows(parent, 0, count-1);

            pnode->children.append(pnode->loader->exclusive.createdChildren);
            pnode->visibleChildren.append(pnode->loader->exclusive.createdChildren);
            pnode->loader->exclusive.createdChildren.clear();
            invalidateOrder(pnode);

            endInsertRows();
        }
    }
    // if asked to load previously, we gather the created nodes
    else {
        // this block is made by the locker constraint mecanism only
        {
            QMutexLocker locker(&pnode->loader->mutex);

            int createdCount = pnode->loader->exclusive.createdChildren.count();
            if (createdCount > 0) {

                int shownCount = pnode->visibleChildren.count();
                beginInsertRows(parent, shownCount, shownCount+createdCount-1);

                pnode->children.append(pnode->loader->exclusive.createdChildren);
                pnode->visibleChildren.append(pnode->loader->exclusive.createdChildren);
                pnode->loader->exclusive.createdChildren.clear();
                invalidateOrder(pnode, shownCount);

                endInsertRows();

                // end loading state if done
                if (pnode->loader->exclusive.isDone) {
                    pnode->isLoaded = true;

#ifdef QVM_DEBUG_LOAD
                    qDebug().nospace()
                            << "fetchMore"
                            << " key:" << keyPath(pnode)
                            << " done count:" << pnode->children.count();
#endif

                    // we remove the "please wait data are loading" row
                    int count = pnode->children.count();
                    beginRemoveRows(parent, count-1, count-1);
                    setLoadingHintNode(pnode, false);
                    endRemoveRows();
                }
#ifdef QVM_DEBUG_LOAD
                else {
                    qDebug().nospace()
                            << "fetchMore"
                            << " key:" << keyPath(pnode)
                            << " state:" << pnode->children.count();
                }
#endif

                // todo: sort & filter
            }
        }

        // if node is loaded, we delete the loader
        // but we delete it here because we mustn't delete a locked mutex
        if (pnode->isLoaded) {
            delete pnode->loader;
            pnode->loader = nullptr;
        }
    }
}

bool QVariantModel::canFetchMore(const QModelIndex &parent) const
{
    node_t* pnode = mp_root.data();
    if (parent.isValid())
        pnode = static_cast<node_t*>(parent.internalPointer());
    Q_ASSERT(pnode != nullptr);

#ifdef QVM_DEBUG_LOAD
    qDebug().nospace()
            << "canFetchMore"
            << " key:" << keyPath(pnode)
            << " fetch more:" << (!pnode->isLoaded);
#endif

    return !pnode->isLoaded;
}

//------------------------------------------------------------------------------

QModelIndex QVariantModel::indexForNode(node_t* node, int column) const
{
    Q_ASSERT(node != nullptr);
    if (node == mp_root.data())
        return QModelIndex();
    int row = 0;
    if (node->parent) {
        // if we never looked for the index in parent
        if (node->indexInParent < 0)
            node->indexInParent = node->parent->visibleChildren.indexOf(node);
        row = node->indexInParent;
    }
    Q_ASSERT(row >= 0);
    return createIndex(row, column, node);
}

QModelIndex QVariantModel::indexForNode(node_t* node, Column col) const
{
    return indexForNode(node, column(col));
}

QModelIndex QVariantModel::index(int row, int column,
                                 const QModelIndex& parent) const
{
    Q_ASSERT(column >= 0 && column < columnCount());

    node_t* pnode = mp_root.data();
    if (parent.isValid())
        pnode = static_cast<node_t*>(parent.internalPointer());
    Q_ASSERT(pnode != nullptr);

#ifdef QVM_DEBUG_MODEL_FUNC
    qDebug() << "index" << row << column << parent.data()
             << QVariantDataInfo(pnode->keyInParent).displayText()
             << (pnode == mp_root.data() ? "isRoot" : "notRoot");
#endif

    int childrenCount = pnode->visibleChildren.count();

    // if node not fully loaded AND want the last children -> return hint node
    if (pnode->isLoaded == false && row == pnode->visibleChildren.count())
        return createIndex(row, column, pnode->hintNode);
    Q_ASSERT(row >= 0 && row < childrenCount);
    return createIndex(row, column, pnode->visibleChildren.at(row));
}

QModelIndex QVariantModel::parent(const QModelIndex& child) const
{
    if (child.isValid()) {
        node_t* node = static_cast<node_t*>(child.internalPointer());
        Q_ASSERT(node != nullptr);
        if (node->parent && node->parent != mp_root.data())
            return indexForNode(node->parent, child.column());
    }

    return QModelIndex();
}

bool QVariantModel::hasChildren(const QModelIndex& parent) const
{
    node_t* pnode = mp_root.data();
    if (parent.isValid())
        pnode = static_cast<node_t*>(parent.internalPointer());
    Q_ASSERT(pnode != nullptr);

#ifdef QVM_DEBUG_MODEL_FUNC
    qDebug() << "hasChildren"
             << QVariantDataInfo(pnode->keyInParent).displayText()
             << (pnode == mp_root.data() ? "isRoot" : "notRoot")
             << (pnode->isLoaded ? !pnode->visibleChildren.isEmpty() : true);
#endif

    // if the node is fully loaded, we check the real children number
    if (pnode->isLoaded)
        return !pnode->visibleChildren.isEmpty();
    // we know that not loaded node are futur parents
    return true;
}

int QVariantModel::rowCount(const QModelIndex& parent) const
{
    node_t* pnode = mp_root.data();
    if (parent.isValid())
        pnode = static_cast<node_t*>(parent.internalPointer());
    Q_ASSERT(pnode != nullptr);

#ifdef QVM_DEBUG_MODEL_FUNC
    qDebug()<< "rowCount"
            << QVariantDataInfo(pnode->keyInParent).displayText()
            << (pnode == mp_root.data() ? "isRoot" : "notRoot")
            << pnode->visibleChildren.count();
#endif

    int count = pnode->visibleChildren.count();
    // if still loading, add a loading hint node
    if (pnode->isLoaded == false)
        count++;
    return count;
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

    // the index innert value must exists
    node_t* node = static_cast<node_t*>(index.internalPointer());
    Q_ASSERT(node != nullptr);

    // impossible case: we gave the hidden root
    node_t* hidden_root = mp_root.data();
    Q_ASSERT((node->parent != nullptr) ^ (node == hidden_root));

    // special case: node is loading hint node
    if (node == node->parent->hintNode)
        return flags;

    // case were the node cannot have children, because its value is not a
    // container
    QMutableVariantDataInfo mutDInfo(node->value);
    if (mutDInfo.isContainer() == false)
        flags |= Qt::ItemNeverHasChildren;

    // test for each column if the node is editable
    if (index.column() == column(KeyColumn)) {
        // if value is not valid, we still can edit the key

        // key should be atomic
        QVariantDataInfo dInfoKey(node->keyInParent);
        if (dInfoKey.isAtomic()) {
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
        // if value is not valid, the value cannot be edited
        if (node->value.isValid() == false)
            return flags;

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
        // even if the value is not valid, we can edit the type
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

    // the index innert value must exists
    node_t* node = static_cast<node_t*>(index.internalPointer());
    Q_ASSERT(node != nullptr);

    // impossible case: we gave the hidden root
    node_t* hidden_root = mp_root.data();
    Q_ASSERT((node->parent != nullptr) ^ (node == hidden_root));

    // special case: node is loading hint node
    if (node == node->parent->hintNode) {
        if (role == Qt::DisplayRole)
            return tr("Loading datas...");
        else if (role == Qt::ToolTipRole) {
            node_t* pnode = node->parent;
            QVariantDataInfo parentDInfo(pnode->value);
            int totalCount = parentDInfo.containerCount();
            int loadedCount = pnode->visibleChildren.count();
            return tr("Loaded %1 / %2").arg(loadedCount).arg(totalCount);
        }
    }
    else if (role == Qt::DisplayRole) {
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

//void QVariantModel::invalidateSubTree(QModelIndex index)
//{
//    node_t* node = mp_root.data();
//    if (index.isValid())
//        node = static_cast<node_t*>(index.internalPointer());
//    Q_ASSERT(node != nullptr);

//    QVector<int> displayRole;
//    displayRole << Qt::DisplayRole;

//    // update its parents
//    while (node->parent != nullptr) {
//        Q_ASSERT(index.isValid());
//        Q_ASSERT(index.internalPointer() == node);

//        // change data in parent
//        QMutableVariantDataInfo mutDInfoParent(node->parent->value);
//        Q_ASSERT(mutDInfoParent.isContainer());
//        Q_ASSERT(mutDInfoParent.editableValues());
//        mutDInfoParent.setContainerValue(node->keyInParent,
//                                         node->value);

//        // next parent (because dataChanged must take the node's parent)
//        node = node->parent;
//        index = index.parent();

//        // update parent of index
//        emit dataChanged(index, index, displayRole);
//    }
//}

//bool QVariantModel::setData(const QModelIndex& index,
//                            const QVariant& value,
//                            int role)
//{
//    if (index.isValid() == false)
//        return false;
//    if (role != Qt::EditRole)
//        return false;

//    node_t* node = static_cast<node_t*>(index.internalPointer());
//    Q_ASSERT(node != nullptr);

//    // if no changes (fast comparison)
//    if (node->value.toString().isEmpty() == false
//            && node->value.toString() == value.toString())
//        return true;

//    bool dataChanges = false;

//    if (index.column() == column(KeyColumn)) {
//        Q_ASSERT(node->parent != nullptr);

//        QVariant oldKey = node->keyInParent;

//        // update the value
//        node->keyInParent = value;

//        // update direct parent about node key
//        QMutableVariantDataInfo mutDInfoParent(node->parent->value);
//        Q_ASSERT(mutDInfoParent.isContainer());
//        Q_ASSERT(mutDInfoParent.editableKeys());
//        mutDInfoParent.setContainerKey(oldKey, node->keyInParent);

//        // update the node index
//        emit dataChanged(index, index, QVector<int>() << Qt::DisplayRole);

//        // update its parents
//        invalidateSubTree(index.parent());

//        // the order might changed if sort
//        // do begin/end move rows
//        if (m_dynamicSort)
//            sortTree(*node->parent, SortNodeOnly);

//        dataChanges = true;
//    }
//    else  if (index.column() == column(ValueColumn)) {
//        Q_ASSERT(node->parent != nullptr);
//        // when change value, type might change

//        bool typeChanged = (node->value.userType() != value.userType());

//        // update the value
//        node->value = value;

//        // emit the update of the row (we want to update value+type columns)
//        QVector<int> roles = QVector<int>() << Qt::DisplayRole << role;

//        // update value column
//        emit dataChanged(index, index, roles);

//        // update type column if changed
//        if (typeChanged) {
//            QModelIndex typeIndex = indexOfNode(node, TypeColumn);
//            emit dataChanged(typeIndex, typeIndex, roles);
//        }

//        // update its parents
//        invalidateSubTree(index);

//        dataChanges = true;
//    }
//    else  if (index.column() == column(TypeColumn)) {
//        Q_ASSERT(node->parent != nullptr);
//        // when change type, value change automatically

//        // update the value and the type, as simple as this
//        QVariantDataInfo dInfo(node->value);
//        node->value = dInfo.tryConvert(value.type(), value.userType());

//        // emit the update of the row (we want to update value+type columns)
//        QVector<int> roles = QVector<int>() << Qt::DisplayRole << role;

//        // update type column
//        emit dataChanged(index, index, roles);

//        // update value column
//        QModelIndex valueIndex = indexOfNode(node, ValueColumn);
//        emit dataChanged(valueIndex, valueIndex, roles);

//        // update its parents
//        invalidateSubTree(index);

//        dataChanges = true;
//    }

//    if (dataChanges)
//        emit rootDatasChanged();

//    return dataChanges;
//}

//bool QVariantModel::insertRows(int row, int count, const QModelIndex& parent)
//{
//    Q_ASSERT(row >= 0 && count >= 0 && row+count <= rowCount(parent)+1);

//    bool dataInserted = true;

//    node_t* pnode = mp_root.data();
//    if (parent.isValid())
//        pnode = static_cast<node_t*>(parent.internalPointer());
//    Q_ASSERT(pnode != nullptr);

//    QMutableVariantDataInfo mutDInfo(pnode->value);
//    Q_ASSERT(mutDInfo.isContainer());
//    Q_ASSERT(mutDInfo.isNewKeyInsertable());

//    int childrenCount = pnode->visibleChildren.count();
//    Q_ASSERT(row >= 0 && row <= childrenCount); // row == count when append

//    // add as much rows needed
//    while (count-- > 0) {
//        node_t* beforeNode = pnode->visibleChildren.value(row, nullptr);
//        QVariant beforeKey = beforeNode ? beforeNode->keyInParent : QVariant();
//        mutDInfo.tryInsertNewKey(beforeKey, QVariant());
//    }

//    // update all the keys (because the order might have changed with the new
//    // key), rebuilding the node and all if its children
//    // but first, we remove all rows, rebuild, and re-add rows
//    beginRemoveRows(parent, 0, pnode->visibleChildren.count());
//    pnode->visibleChildren.clear();
//    endRemoveRows();

//    rebuildNodeIncr(*pnode);
//    filterTree(*pnode, NoSort); // filter now because row are not visible

//    // take temporary children that are will be visible
//    QList<node_t*> visibleChildren;
//    pnode->visibleChildren.swap(visibleChildren);

//    beginInsertRows(parent, 0, visibleChildren.count());
//    pnode->visibleChildren.swap(visibleChildren);
//    endInsertRows();

//    // now rows are visible, we can sort
//    if (m_dynamicSort)
//        sortTree(*pnode, SortNodeAndChildren);

//    // and update all parents
//    invalidateSubTree(parent);

//    // data changes
//    emit rootDatasChanged();

//    return dataInserted;
//}

//bool QVariantModel::removeRows(int row, int count, const QModelIndex& parent)
//{
//    Q_ASSERT(row >= 0 && count >= 0 && row+count <= rowCount(parent));

//    bool dataRemoved = true;

//    node_t* pnode = mp_root.data();
//    if (parent.isValid())
//        pnode = static_cast<node_t*>(parent.internalPointer());
//    Q_ASSERT(pnode != nullptr);

//    QMutableVariantDataInfo mutDInfo(pnode->value);
//    Q_ASSERT(mutDInfo.isContainer());
//    Q_ASSERT(mutDInfo.isKeyRemovable());

//    int childrenCount = pnode->visibleChildren.count();
//    Q_ASSERT(row >= 0 && row+count <= childrenCount); // row == count when append

//    // remove as much rows asked
//    while (count-- > 0) {
//        node_t* node = pnode->visibleChildren.value(row+count, nullptr);
//        Q_ASSERT(node != nullptr);
//        mutDInfo.removeKey(node->keyInParent);
//    }

//    // update all the keys (because the order might have changed after remove),
//    // rebuilding the node and all if its children
//    // but first, we remove all rows, rebuild, and re-add rows
//    beginRemoveRows(parent, 0, pnode->visibleChildren.count());
//    pnode->visibleChildren.clear();
//    endRemoveRows();

//    rebuildNodeIncr(*pnode);
//    filterTree(*pnode, NoSort); // filter now because row are not visible

//    // take temporary children that are will be visible
//    QList<node_t*> visibleChildren;
//    pnode->visibleChildren.swap(visibleChildren);

//    beginInsertRows(parent, 0, visibleChildren.count());
//    pnode->visibleChildren.swap(visibleChildren);
//    endInsertRows();

//    // now rows are visible, we can sort
//    if (m_dynamicSort)
//        sortTree(*pnode, SortNodeAndChildren);

//    // and update all parents
//    invalidateSubTree(parent);

//    // data changes
//    emit rootDatasChanged();

//    return dataRemoved;
//}


//------------------------------------------------------------------------------

void QVariantModel::setDynamicSort(bool enabled)
{
    if (m_dynamicSort == enabled)
        return;

    m_dynamicSort = enabled;
    emit dynamicSortChanged(enabled);

    //    if (rowCount() > 0)
    //        sortTree(*mp_root, SortNodeAndChildren);
}

//void QVariantModel::sortTree(node_t& root, InternalSortStrategy sortStrategy)
//{
//    QList<node_t*> newChildOrder = root.visibleChildren;

//    // sort direct children
//    std::sort(newChildOrder.begin(), newChildOrder.end(),
//              [this] (const node_t* left, const node_t* right) {
//        return lessThan(left->keyInParent, right->keyInParent);
//    });

//    QModelIndex parent;
//    if (root.parent)
//        parent = indexOfNode(&root, KeyColumn);

//    // move rows
//    int count = newChildOrder.count();
//    for (int iNewChild = 0; iNewChild < count; iNewChild++) {
//        node_t* child = newChildOrder.at(iNewChild);
//        int iOldChild = root.visibleChildren.indexOf(child);
//        Q_ASSERT(iOldChild >= 0);
//        // if the child moved
//        if (iNewChild < iOldChild) {
//            bool canMoveRow = beginMoveRows(parent, iOldChild, iOldChild,
//                                            parent, iNewChild);
//            Q_ASSERT(canMoveRow);
//            root.visibleChildren.removeAt(iOldChild);
//            root.visibleChildren.insert(iNewChild, child);
//            endMoveRows();
//        }
//    }

//    if (sortStrategy == SortNodeAndChildren
//            && root.visibleChildren.isEmpty() == false) {
//        for (auto it = root.visibleChildren.begin();
//             it != root.visibleChildren.end(); ++it) {
//            sortTree(*(*it), sortStrategy);
//        }
//    }
//}

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

    bool wasFiltered = isFilterEnabled();

    m_filterType = filterType;
    updateFilterRx(m_filterRx.pattern());
    emit filterTypeChanged(filterType);

    bool nowFiltered = isFilterEnabled();

    if (wasFiltered || nowFiltered) {
        //        filterTree(mp_root.data(), DynamicSortPolicy);
        //        dumpTree();
        //        dumpModel();
    }
}

void QVariantModel::setFilterColumns(Columns filterColumns)
{
    if (m_filterColumns == filterColumns)
        return;

    bool wasFiltered = isFilterEnabled();

    m_filterColumns = filterColumns;
    emit filterColumnsChanged(filterColumns);

    bool nowFiltered = isFilterEnabled();

    if (wasFiltered || nowFiltered) {
        //        filterTree(mp_root.data(), DynamicSortPolicy);
        //        dumpTree();
        //        dumpModel();
    }
}

void QVariantModel::setFilterText(const QString& filterText)
{
    if (m_filterRx.pattern() == filterText)
        return;

    bool wasFiltered = isFilterEnabled();

    updateFilterRx(filterText);
    emit filterTextChanged(filterText);

    bool nowFiltered = isFilterEnabled();

    if (wasFiltered || nowFiltered) {
        //        filterTree(mp_root.data(), DynamicSortPolicy);
        //        dumpTree();
        //        dumpModel();
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

bool QVariantModel::isAcceptedNode(node_t *root) const
{
    bool isVisible = false;

    bool fkey = m_filterColumns & KeyColumn;
    bool vkey = m_filterColumns & ValueColumn;
    bool tkey = m_filterColumns & TypeColumn;

    isVisible |= (fkey && filterKey(root->keyInParent));
    isVisible |= (vkey && filterValue(root->value));
    isVisible |= (tkey && filterType(root->value.userType()));

#ifdef QVM_DEBUG_FILTER
    qDebug() << (QString("filter node content with ")
                 + m_filterRx.pattern()
                 + " "
                 + DBG_NODE(root)
                 + (isVisible ? " => visible" : " => hidden"));
#endif

    return isVisible;
}

bool QVariantModel::isFilterEnabled() const
{
    return !m_filterRx.pattern().isEmpty();
}

//void QVariantModel::filterTree(node_t* root, InternalSortPolicy sortPolicy)
//{
//    Q_ASSERT(root);

//    // actual filter
//    if (isFilterEnabled()) {
//        if (root == mp_root.data()) {
//            qDebug() << "begin filter"
//                           << FILTER_TYPE_NAMES[m_filterType]
//                              << m_filterRx.pattern();
//            beginResetModel();
//        }

//        bool wasVisible = root->visible;
//        if (root->parent)
//            Q_ASSERT(wasVisible == root->parent->visibleChildren.contains(root));

//        // filter children
//        for (auto itChild = root->children.begin();
//             itChild != root->children.end(); ++itChild) {
//            filterTree(*itChild, sortPolicy);
//        }

//        // if no children to show -> don't show if the node itself is filtered
//        root->visible = (!root->visibleChildren.isEmpty());
//        if (root->visible == false)
//            root->visible = isAcceptedNode(root);

//        if (root->parent) {
//            // if appeared
//            if (root->visible && !wasVisible) {
//                Q_ASSERT(root->parent->visibleChildren.contains(root) == false);
//                int insertPos = root->parent->children.indexOf(root);
//                //                QModelIndex parent = indexOfNode(root->parent);
//                //                beginInsertRows(parent, insertPos, insertPos);
//                root->parent->visibleChildren.insert(insertPos, root);
//                //                endInsertRows();
//            }
//            // if disappeared
//            else if (!root->visible && wasVisible) {
//                Q_ASSERT(root->parent->visibleChildren.contains(root));
//                //                QModelIndex index = indexOfNode(root);
//                //                beginRemoveRows(index.parent(), index.row(), index.row());
//                root->parent->visibleChildren.removeOne(root);
//                //                endRemoveRows();
//            }
//        }

//        if (root == mp_root.data()) {
//            endResetModel();
//            qDebug() << "end filter";
//        }
//    }
//    // display all
//    else {
//        // so much change, better reset than insert hidden row
//        if (root == mp_root.data()) {
//            qDebug().nospace() << "begin clear filter";
//            beginResetModel();
//        }

//        // filter children
//        for (auto itChild = root->children.begin();
//             itChild != root->children.end(); ++itChild) {
//            filterTree(*itChild, sortPolicy);
//        }

//        root->visibleChildren = root->children;
//        root->visible = true;

//        if (root == mp_root.data()) {
//            qDebug().nospace() << "end clear filter";
//            endResetModel();
//        }
//    }

//    //    if (sortPolicy == ForceSort
//    //            || (m_dynamicSort && sortPolicy == DynamicSortPolicy)) {
//    //        sortTree(root, SortNodeOnly);
//    //    }
//}

//------------------------------------------------------------------------------

void QVariantModel::dumpTree(const node_t* root,
                             const QString& prefix) const
{
    if (root == nullptr)
        root = mp_root.data();

#if defined(QT_DEBUG) && defined(QVARIANTMODEL_DEBUG)
    qDebug().nospace() << (prefix + DBG_NODE(root));
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
        QModelIndex indexKey = index(rootIndex.row(),
                                     column(KeyColumn),
                                     rootIndex.parent());
        QModelIndex indexValue = index(rootIndex.row(),
                                       column(ValueColumn),
                                       rootIndex.parent());
        QModelIndex indexType = index(rootIndex.row(),
                                      column(TypeColumn),
                                      rootIndex.parent());

        qDebug().nospace() << (prefix
                               + "index(key=\""
                               + data(indexKey, Qt::DisplayRole).toString()
                               + "\", value=\""
                               + data(indexValue, Qt::DisplayRole).toString()
                               + "\", type=\""
                               + data(indexType, Qt::DisplayRole).toString()
                               + "\")");
    }
    else {
        qDebug().nospace() << (prefix + "index(invalid)");
    }
    for (int i=0; i<rowCount(rootIndex); i++) {
        dumpModel(index(i, 0, rootIndex), prefix + "--");
    }
#else
    Q_UNUSED(rootIndex)
    Q_UNUSED(prefix)
#endif
}

//==============================================================================

QVariantModelDataLoader::QVariantModelDataLoader(node_t* root) :
    QRunnable(),
    root(root),
    mutex(QMutex::NonRecursive)
{
    exclusive.isDone = false;
    setAutoDelete(false);
    Q_ASSERT(root != nullptr);
}

void QVariantModelDataLoader::run()
{
    buildNode(root);

    {
        QMutexLocker locker(&mutex);
        exclusive.isDone = true;
    }
}

void QVariantModelDataLoader::buildNode(node_t* node)
{
    Q_ASSERT(node);
    Q_ASSERT(node->isLoaded  == false);

    QVariantDataInfo dInfo(node->value);
    Q_ASSERT(dInfo.isContainer());

    QVariantList keys = dInfo.containerKeys();
    int keysCount = keys.count();
    Model::clearChildren(node, keysCount);

#ifdef QVM_DEBUG_BUILD
    QString rootKeyStr = Model::keyPath(node);
    int childrenCount = 0;
    qDebug().nospace()
            << "build node key:" << rootKeyStr << " begin total: " << keysCount;
#endif

    // first: we create all root's children structure
    QList<node_t*> newChildren;
    newChildren.reserve(SprintBuildSize);
    for (auto itkey = keys.constBegin(); itkey != keys.constEnd(); ++itkey){
        QVariant childData = dInfo.containerValue(*itkey);

        node_t* child = new node_t;
        newChildren.append(child);

        child->parent = node;
        child->keyInParent = *itkey;
        child->value = childData;
        child->visible = true;

        // optimization: give the child loaded status as fully loaded
        // if doesn't required to load something
        child->isLoaded = true;
        QVariantDataInfo childDInfo(child->value);
        if (childDInfo.isContainer() && childDInfo.isEmptyContainer() == false)
            child->isLoaded = false;

#ifdef QVM_DEBUG_BUILD
        if (newChildren.size() >= SprintBuildSize) {
            qDebug().nospace()
                    << "build node"
                    << " key:" << rootKeyStr
                    << " loaded: " << childrenCount
                    << "/" << keysCount;
        }
        childrenCount++;
#endif

        // if we reached the commit amount
        if (newChildren.size() >= SprintBuildSize) {
            // commit datas
            QMutexLocker locker(&mutex);
            exclusive.createdChildren.append(newChildren);
            newChildren.clear();
        }
    }

    // add the last created
    if (newChildren.size() >= 0) {
        // commit datas
        QMutexLocker locker(&mutex);
        exclusive.createdChildren.append(newChildren);
        newChildren.clear();
    }

#ifdef QVM_DEBUG_BUILD
    qDebug().nospace()
            << "build node"
            << " key:" << rootKeyStr
            << " loaded: " << childrenCount
            << "/" << keysCount;
#endif

    // here: we create all children structure, but the model can not having got
    // the last created children; this implies that `root->createdChildren` can
    // still have some datas, so `root->children` might not reflect the full
    // node content, and moreover, `root->children` might not even be ordered.
    // As this is plausible, we have to looped endlessly through node's children
    // in search of not loaded ones. As soon as with looped the entire
    // collections without any not loaded nodes, we are done.
    // In side of this, we do not set the loading flag here for the same reasons.

    // second: we build recursively root's children
    //    int rootChildrenOffset = 0;
    //    bool isOneNotLoaded = true;
    //    while (isOneNotLoaded) {
    //        // we reset the flag that indicates we have to continue loading
    //        isOneNotLoaded = false;

    //        // gather next children for this loop
    //        QList<node_t*> localChildren;
    //        {
    //            // we supposed `root->children` never shrink while building nodes
    //            QMutexLocker locker(&mutex);

    //            // get nodes from already integrated nodes (and may be chown)
    //            int extractionCount = qBound(
    //                        0, root->children.count() - rootChildrenOffset,
    //                        (int)ChildrenSprintSize);
    //            localChildren = root->children.mid(
    //                        rootChildrenOffset, extractionCount);

    //            // get nodes not yet integrated nodes
    //            extractionCount = qBound(
    //                        0, root->createdChildren.count() - (rootChildrenOffset + localChildren.count() - root->children.count()),
    //                        (int)ChildrenSprintSize - localChildren.count());
    //            localChildren.append(root->createdChildren.mid(
    //                                     rootChildrenOffset + localChildren.count(),
    //                                     extractionCount));

    //            rootChildrenOffset += localChildren.count();

    //            if (rootChildrenOffset >= root->children.count()
    //                    + root->createdChildren.count())
    //                rootChildrenOffset = 0;
    //        }

    //        // build children
    //        for (auto it = localChildren.begin(); it != localChildren.end(); ++it) {
    //            node_t* child = *it;
    //            // if child must be build
    //            if (child->isLoaded == false
    //                    && child->createdChildren.isEmpty()) {
    //                isOneNotLoaded = true;
    //                buildNode(*it);
    //            }
    //        }
    //    }

#ifdef QVM_DEBUG_BUILD
    qDebug().nospace()
            << "build node key:" << rootKeyStr << " done";
#endif
}
