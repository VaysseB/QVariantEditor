#include "qvariantmodel.h"

#include <QThreadPool>
#include <QMutexLocker>
#include <QDebug>

#include "qvariantdatainfo.h"


#define DBG_NODE(p, root) (QString(p) + "node(key=" + QVariantDataInfo(root->keyInParent).displayText(1) + ", value=" + QVariantDataInfo(root->value).displayText(1) + ", type=" + QString::number(root->value.userType()) + "-" + root->value.typeName() + ")")
#define DBG_NODE_CACHE(p, root) (QString(p) + "cache(TEXT: key=" + root->cache.key.text + ", value=" + root->cache.value.text + ", type=" + root->cache.type.text) << "; FLAGS: type=" << root->cache.key.flags << ", value=" << root->cache.value.flags << ", type=" << root->cache.type.flags << ")"



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
    beginResetModel();

    mp_root->value = rootDatas;
    mp_root->parent = nullptr;
    mp_root->loaded = rootDatas.isEmpty();

    if (mp_root->loaded == false)
        loadNode(mp_root.data(), NoChangedSignalsEmitted);

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
         it != node->visibleChildren.constEnd() && length-- != 0; ++it) {
        node_t* child = *it;
        child->indexInParent = iChild++;
    }
}

void QVariantModel::cached(node_t* node, int textDepth, CacheRoles roles)
{
    Q_ASSERT(node != nullptr);

#ifdef QVM_DEBUG_CACHE
    qDebug() << "cached" << keyPath(node) << roles;
#endif

    if (roles == NoCache)
        return;

    Qt::ItemFlags baseFlags = Qt::ItemIsSelectable | Qt::ItemIsEnabled;
    bool hasParent = (node->parent != nullptr);
    QVariantDataInfo dInfoKey(node->keyInParent);
    QVariantDataInfo dInfoValue(node->value);

    // case were the node cannot have children, because its value is not a
    // container
    if ((roles & CacheFlags) && dInfoValue.isContainer() == false)
        baseFlags |= Qt::ItemNeverHasChildren;

    // work for key column
    {
        if ((roles & CacheText)) {
            node->cache.key.text = QVariantDataInfo(node->keyInParent)
                    .displayText(textDepth);
        }

        if ((roles & CacheFlags)) {
            node->cache.key.flags = baseFlags;
            bool parentUpdatable = false;

            // if value is not valid, we still can edit the key, and if has parent
            // key should be atomic to be edited
            if (dInfoKey.isAtomic() && hasParent) {
                // editable only if the direct parent can update keys
                QMutableVariantDataInfo mutDInfoDirectParent(node->parent->value);
                Q_ASSERT(mutDInfoDirectParent.isContainer());
                parentUpdatable = mutDInfoDirectParent.editableKeys();
                // now we check parents of the direct parent
                if (parentUpdatable) {
                    node_t* pnode = node;
                    // editable only if all the parents can update values
                    // (from the node parent)
                    while (pnode->parent != nullptr && parentUpdatable) {
                        QMutableVariantDataInfo mutDInfoParent(pnode->parent->value);
                        Q_ASSERT(mutDInfoParent.isContainer());
                        parentUpdatable = mutDInfoParent.editableValues();
                        pnode = pnode->parent;
                    }
                }
            }

            // if no parent or all parents are editable
            if (parentUpdatable)
                node->cache.key.flags |= Qt::ItemIsEditable;
        }
    }

    // work for value column
    {
        if ((roles & CacheText))
            node->cache.value.text = QVariantDataInfo(node->value).displayText(textDepth);

        if ((roles & CacheFlags)) {
            node->cache.value.flags = baseFlags;
            bool parentUpdatable = false;

            // if value is not valid, the value cannot be edited
            // value should be atomic to be edited
            if (node->value.isValid() && dInfoValue.isAtomic()) {
                node_t* pnode = node;
                // editable only if all the parents can update values
                parentUpdatable = true;
                while (pnode->parent != nullptr && parentUpdatable) {
                    QMutableVariantDataInfo mutDInfoParent(pnode->parent->value);
                    Q_ASSERT(mutDInfoParent.isContainer());
                    parentUpdatable = mutDInfoParent.editableValues();
                    pnode = pnode->parent;
                }
            }

            // if no parent or all parents are editable
            if (parentUpdatable)
                node->cache.value.flags |= Qt::ItemIsEditable;
        }
    }

    // work for type column
    {
        if ((roles & CacheText))
            node->cache.type.text = node->value.typeName();

        if ((roles & CacheFlags)) {
            // even if the value is not valid, we can edit the type
            node->cache.type.flags = baseFlags | Qt::ItemIsEditable;
        }
    }

#ifdef QVM_DEBUG_CACHE
    // test valid cache
    Q_ASSERT(node->cache.key.text.trimmed().isEmpty() == false);
    Q_ASSERT(node->cache.value.text.trimmed().isEmpty() == false);
    Q_ASSERT(node->cache.type.text.trimmed().isEmpty() == false);
    Q_ASSERT(node->cache.key.flags != Qt::NoItemFlags);
    Q_ASSERT(node->cache.value.flags != Qt::NoItemFlags);
    Q_ASSERT(node->cache.type.flags != Qt::NoItemFlags);
#endif
}

void QVariantModel::recachedTree(node_t* pnode,
                                 CacheRoles cacheRoles,
                                 bool canModifyModel)
{
#ifdef QVM_DEBUG_CACHE
    // test valid cache
    Q_ASSERT(pnode->cache.key.text.trimmed().isEmpty() == false);
    Q_ASSERT(pnode->cache.value.text.trimmed().isEmpty() == false);
    Q_ASSERT(pnode->cache.type.text.trimmed().isEmpty() == false);
    Q_ASSERT(pnode->cache.key.flags != Qt::NoItemFlags);
    Q_ASSERT(pnode->cache.value.flags != Qt::NoItemFlags);
    Q_ASSERT(pnode->cache.type.flags != Qt::NoItemFlags);
#endif

    // first rebuild cache, then emit update (only if not tree root, because
    // this is useless as it is hidden)
    if (pnode != mp_root.data())
        cached(pnode, m_depth, cacheRoles);

    for (auto it = pnode->visibleChildren.constBegin();
         it != pnode->visibleChildren.constEnd(); ++it) {
        recachedTree(*it, cacheRoles);
    }

    // emit update (only if not tree root, because
    // this is useless as it is hidden)
    if (canModifyModel && pnode != mp_root.data()) {
        QVector<int> rolesChanged;
        if ((cacheRoles & CacheText))
            rolesChanged << Qt::DisplayRole;
        if ((cacheRoles & CacheFlags)) // can possibly add/remove edition
            rolesChanged << Qt::EditRole;

        emit dataChanged(indexForNode(pnode, 0),
                         indexForNode(pnode, columnCount()-1),
                         rolesChanged);
    }
}

//------------------------------------------------------------------------------

void QVariantModel::loadNode(node_t* pnode,
                             bool canModifyModel)
{
    Q_ASSERT(pnode != nullptr);

    node_t* hidden_root = mp_root.data();
    Q_UNUSED(hidden_root); // avoid warning if no assert
    Q_ASSERT((pnode->parent != nullptr) ^ (pnode == hidden_root));

    Q_ASSERT(pnode->loaded == false);

#ifdef QVM_DEBUG_LOAD
    qDebug().nospace()
            << "loadMore"
            << " key:" << keyPath(pnode)
            << " loader:" << ((void*)pnode->loader);
#endif

    QList<node_t*> newlyCreatedChildren;

    // if never asked to load
    if (pnode->loader == nullptr) {
        // as we are supposed to never been asked to load, we should be empty
        Q_ASSERT(pnode->children.isEmpty());

        pnode->loader = new QVariantModelDataLoader(pnode);
        pnode->loader->to_cache.displayDepth = m_depth;

        QVariantDataInfo dInfo(pnode->value);
        Q_ASSERT(dInfo.isContainer());
        int count = dInfo.containerCount();

        // if too much datas, load async
        // otherwise load sync
        if (count >= (int)SizeLimitToLoadAsync) {
#ifdef QVM_DEBUG_LOAD
            qDebug().nospace()
                    << "loadMore"
                    << " key:" << keyPath(pnode)
                    << " start async";
#endif

            // load async
            QThreadPool* thPool = QThreadPool::globalInstance();
            Q_ASSERT(thPool);
            thPool->start(pnode->loader);

            // we stop here, the loading is started
            return;
        }
        // direct load (so sync)
        else {
            Q_ASSERT(pnode->loader != nullptr);

#ifdef QVM_DEBUG_LOAD
            qDebug().nospace()
                    << "loadMore"
                    << " key:" << keyPath(pnode)
                    << " direct load sync";
#endif

            // load now
            pnode->loader->run();

            // get created children
            newlyCreatedChildren.swap(pnode->loader->exclusive.createdChildren);
            pnode->loaded = true;

#ifdef QVM_DEBUG_LOAD
            qDebug().nospace()
                    << "loadMore"
                    << " key:" << keyPath(pnode)
                    << " done count:" << newlyCreatedChildren.count();
#endif
        }
    }
    // if asked to load previously, we gather the created nodes from the loader
    // this block is also made by the locker constraint mecanism only
    else {
        Q_ASSERT(pnode->loader != nullptr);

        QMutexLocker locker(&pnode->loader->mutex);

        // get created children
        newlyCreatedChildren.swap(pnode->loader->exclusive.createdChildren);

        // we are considere loaded if the loader is done
        pnode->loaded = pnode->loader->exclusive.isDone;

#ifdef QVM_DEBUG_LOAD
        if (pnode->loaded) {
            qDebug().nospace()
                    << "loadMore"
                    << " key:" << keyPath(pnode)
                    << " done last - count:" << pnode->children.count()
                    << " + " << newlyCreatedChildren.count();
        }
        else {
            qDebug().nospace()
                    << "loadMore"
                    << " key:" << keyPath(pnode)
                    << " in progress - count:" << pnode->children.count()
                    << " + " << newlyCreatedChildren.count();
        }
#endif
    }

    Q_ASSERT(pnode->loader != nullptr);

    // add the created children if any
    int createdCount = newlyCreatedChildren.count();
    if (createdCount > 0) {
        pnode->children.append(newlyCreatedChildren);

        // filter & sort
#ifdef QVM_DEBUG_CHANGE_MODEL
        qDebug() << "begin filter"
                 << keyPath(pnode)
                 << (pnode->loaded ? "loaded" : "not_loaded")
                 << (isFilterEnabled() ? "select" : "clear")
                 << FILTER_TYPE_NAMES[m_filterType]
                    << m_filterRx.pattern();
#endif
        filterTree(pnode, canModifyModel);
#ifdef QVM_DEBUG_CHANGE_MODEL
        qDebug() << "end filter";
#endif
    }

    // if node is loaded, we delete the loader
    // but we delete it here because we mustn't delete a locked mutex
    if (pnode->loaded && pnode->loader != nullptr) {
        delete pnode->loader;
        pnode->loader = nullptr;
    }
}

void QVariantModel::fetchMore(const QModelIndex &parent)
{
    node_t* pnode = mp_root.data();
    if (parent.isValid())
        pnode = static_cast<node_t*>(parent.internalPointer());
    Q_ASSERT(pnode != nullptr);

    if (pnode->loaded == false)
        loadNode(pnode);
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
            << " fetch more:" << (!pnode->loaded);
#endif

    return !pnode->loaded;
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
    Q_ASSERT(row >= 0 && row < rowCount(parent));
    Q_ASSERT(column >= 0 && column < columnCount(parent));

    node_t* pnode = mp_root.data();
    if (parent.isValid())
        pnode = static_cast<node_t*>(parent.internalPointer());
    Q_ASSERT(pnode != nullptr);

#ifdef QVM_DEBUG_MODEL_FUNC
    qDebug() << "index" << row << column << parent.data()
             << keyPath(pnode);
#endif

    int childrenCount = pnode->visibleChildren.count();
    Q_UNUSED(childrenCount); // avoid warning if no assert
    Q_ASSERT(row < childrenCount);
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
             << keyPath(pnode)
             << (pnode->loaded ? !pnode->visibleChildren.isEmpty() : true);
#endif

    // if the node is fully loaded, we check the real children number
    if (pnode->loaded)
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
    qDebug() << "rowCount"
             << keyPath(pnode)
             << pnode->visibleChildren.count()
             << (pnode->loaded ? "loaded" : "notLoaded");
#endif

    int count = pnode->visibleChildren.count();
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
    Q_UNUSED(hidden_root); // avoid warning if no assert
    Q_ASSERT((node->parent != nullptr) ^ (node == hidden_root));

#ifdef QVM_DEBUG_DATA
    qDebug() << "flags" << index.row() << index.column()
             << " key:" << keyPath(node);
#ifdef QVM_DEBUG_CACHE
    qDebug().nospace() << "cache flags key:" << node->cache.key.flags
                       << " value:" << node->cache.value.flags
                       << " type:" << node->cache.type.flags;
#endif // QVM_DEBUG_CACHE
#endif // QVM_DEBUG_DATA

    if (index.column() == column(KeyColumn))
        return node->cache.key.flags;
    else if (index.column() == column(ValueColumn))
        return node->cache.value.flags;
    else if (index.column() == column(TypeColumn))
        return node->cache.type.flags;

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
    else { // if (orientation == Qt::Vertical)
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
    Q_UNUSED(hidden_root); // avoid warning if no assert
    Q_ASSERT((node->parent != nullptr) ^ (node == hidden_root));

#ifdef QVM_DEBUG_DATA
    qDebug() << "data" << index.row() << index.column() << role
             << " key:" << keyPath(node);
#ifdef QVM_DEBUG_CACHE
    qDebug().nospace() << "cache text key:" << node->cache.key.text
                       << " value:" << node->cache.value.text
                       << " type:" << node->cache.type.text;
#endif // QVM_DEBUG_CACHE
#endif // QVM_DEBUG_DATA

    if (role == Qt::DisplayRole) {
        if (index.column() == column(KeyColumn))
            return node->cache.key.text;
        else if (index.column() == column(ValueColumn))
            return node->cache.value.text;
        else if (index.column() == column(TypeColumn))
            return node->cache.type.text;
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
//            QModelIndex typeIndex = indexForNode(node, TypeColumn);
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
//        QModelIndex valueIndex = indexForNode(node, ValueColumn);
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

QModelIndex QVariantModel::createIndex(int row, int column, node_t* node) const
{
    Q_ASSERT((node == nullptr) ^ (node != mp_root.data()));
    return QAbstractItemModel::createIndex(row, column, node);
}

//------------------------------------------------------------------------------

void QVariantModel::setDisplayDepth(uint depth)
{
    if (m_depth == depth)
        return;

    m_depth = depth;
    emit displayDepthChanged(m_depth);

    int amoutToRebuild = mp_root->children.count(); // todo: find a better indicator
    bool betterReset = (amoutToRebuild >= (int)SizeLimitBetterResetWhenDataChanged);
    if (betterReset)
        beginResetModel();

    recachedTree(mp_root.data(), CacheText,
                 betterReset ? (bool)NoChangedSignalsEmitted
                             : (bool)EmitChangedSignals);

    if (betterReset)
        endResetModel();
}

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
//        parent = indexForNode(&root, KeyColumn);

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
#ifdef QVM_DEBUG_CHANGE_MODEL
        qDebug() << "begin filter"
                 << keyPath(mp_root.data())
                 << (mp_root->loaded ? "loaded" : "not_loaded")
                 << (isFilterEnabled() ? "select" : "clear")
                 << FILTER_TYPE_NAMES[m_filterType]
                    << m_filterRx.pattern();
#endif
        filterTree(mp_root.data(), DynamicSortPolicy);
#ifdef QVM_DEBUG_CHANGE_MODEL
        qDebug() << "end filter";
#endif
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
#ifdef QVM_DEBUG_CHANGE_MODEL
        qDebug() << "begin filter"
                 << keyPath(mp_root.data())
                 << (mp_root->loaded ? "loaded" : "not_loaded")
                 << (isFilterEnabled() ? "select" : "clear")
                 << FILTER_TYPE_NAMES[m_filterType]
                    << m_filterRx.pattern();
#endif
        filterTree(mp_root.data(), DynamicSortPolicy);
#ifdef QVM_DEBUG_CHANGE_MODEL
        qDebug() << "end filter";
#endif
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
#ifdef QVM_DEBUG_CHANGE_MODEL
        qDebug() << "begin filter"
                 << keyPath(mp_root.data())
                 << (mp_root->loaded ? "loaded" : "not_loaded")
                 << (isFilterEnabled() ? "select" : "clear")
                 << FILTER_TYPE_NAMES[m_filterType]
                    << m_filterRx.pattern();
#endif
        filterTree(mp_root.data(), DynamicSortPolicy);
#ifdef QVM_DEBUG_CHANGE_MODEL
        qDebug() << "end filter";
#endif
    }
}

bool QVariantModel::filterKeyColumn(const QString& cacheKey,
                                    const QVariant& key,
                                    bool nodeLoaded) const
{
    Q_UNUSED(cacheKey);
    // if the node is loaded, we just look at the level of the node, not deeper
    if (nodeLoaded) {
        // we build the minimum key representation to test
        const QString& text = QVariantDataInfo(key).displayText(1);
        return filterText(text);
    }
    return filterData(key);
}

bool QVariantModel::filterValueColumn(const QString& cacheValue,
                                      const QVariant& value,
                                      bool nodeLoaded) const
{
    Q_UNUSED(cacheValue);
    // if the node is loaded, we just look at the level of the node, not deeper
    if (nodeLoaded) {
        // we build the minimum value representation to test
        const QString& text = QVariantDataInfo(value).displayText(1);
        return filterText(text);
    }
    return filterData(value);
}

bool QVariantModel::filterTypeColumn(const QString& cacheType,
                                     int type,
                                     bool nodeLoaded) const
{
    Q_UNUSED(type);
    Q_UNUSED(nodeLoaded);
    return filterText(cacheType);
}

bool QVariantModel::filterText(const QString& text) const
{
    bool isRowOk = false;

    switch (m_filterType)
    {
    case QVariantModel::Contains:
        isRowOk = text.contains(m_filterRx.pattern());
        break;
    case QVariantModel::WildCard:
        isRowOk = m_filterRx.indexIn(text) >= 0;
        break;
    case QVariantModel::Regex:
        isRowOk = m_filterRx.exactMatch(text);
        break;
    case QVariantModel::Fixed:
        isRowOk = (text.compare(m_filterRx.pattern(), Qt::CaseInsensitive) == 0);
        break;

    }

    return isRowOk;
}

bool QVariantModel::filterData(const QVariant& value) const
{
    QVariantDataInfo dInfo(value);

    if (dInfo.isContainer()) {
        bool isVisible = false;

        // deep search on each keys and values
        // until we find the first visible element
        QVariantList keys = dInfo.containerKeys();
        for (auto it = keys.constBegin(); it != keys.constEnd()
             && isVisible == false; ++it) {
            QVariant key = *it;
            isVisible = filterData(key);
            // if key is not accepted
            if (isVisible == false) {
                QVariant value = dInfo.containerValue(key);
                isVisible = filterData(value);
            }
        }

        return isVisible;
    }

    // here, we don't know if value is atomic and we don't bother, because the
    // value cannot be a group of other values, so we know that its
    // representation as string will reflect the value content,
    // so it is testable
    const QString& text = dInfo.displayText(1);
    return filterText(text);
}

bool QVariantModel::isAcceptedNode(node_t *node) const
{
    bool isVisible = false;

    bool fkey = m_filterColumns & KeyColumn;
    bool vkey = m_filterColumns & ValueColumn;
    bool tkey = m_filterColumns & TypeColumn;

    isVisible |= (fkey && filterKeyColumn(node->cache.key.text,
                                          node->keyInParent,
                                          node->loaded));
    isVisible |= (vkey && filterValueColumn(node->cache.value.text,
                                            node->value,
                                            node->loaded));
    isVisible |= (tkey && filterTypeColumn(node->cache.type.text,
                                           node->value.userType(),
                                           node->loaded));

#ifdef QVM_DEBUG_FILTER
    qDebug() << "filter node content with"
             << FILTER_TYPE_NAMES[m_filterType]
                << m_filterRx.pattern()
                << (fkey ? "KeyColumn" : "\0")
                << (vkey ? "ValueColumn" : "\0")
                << (tkey ? "TypeColumn" : "\0")
                << DBG_NODE("", node)
                << (isVisible ? " => visible" : " => hidden");
#endif

    return isVisible;
}

bool QVariantModel::isFilterEnabled() const
{
    return !m_filterRx.pattern().isEmpty();
}

void QVariantModel::filterTree(node_t* node, bool canModifyModel)
{
    Q_ASSERT(node);

    // actual filter
    if (isFilterEnabled()) {
        int amountToFilter = node->children.count();
        bool betterReset = (amountToFilter >= 1);//(int)SizeLimitBetterResetWhenDataChanged);

        // if the best strategy is to wipe out node's visibles children rows
        // and then insert them later
        if (canModifyModel && betterReset)
            resetHideNode(node, canModifyModel, HideOnlyChildren);

        // filter children if loaded
        // or filter data if not (see this later)
        if (node->loaded) {
            for (auto itChild = node->children.begin();
                 itChild != node->children.end(); ++itChild) {
                filterTree(*itChild, canModifyModel);
            }
        }

        // if we don't have any parent, we can not hide, so we don't even
        // test if we can be visible
        if (node->parent) {
            bool wasVisible = node->visible;

            // this assert seems stupid, but in reality,
            // it has some very good use!, so don't remove it
            Q_ASSERT(wasVisible == node->parent->visibleChildren.contains(node));

            // if no children to show -> don't show if the node itself
            // is not accepted
            bool nowVisible = (!node->visibleChildren.isEmpty());
            if (nowVisible == false)
                nowVisible = isAcceptedNode(node);

            // if appeared
            if (nowVisible && !wasVisible)
                resetShowNode(node, canModifyModel);
            // if disappeared
            else if (!nowVisible && wasVisible)
                resetHideNode(node, canModifyModel);
        }

    }
    // display all
    else {
        resetShowNode(node, canModifyModel);
    }

    //    if (sortPolicy == ForceSort
    //            || (m_dynamicSort && sortPolicy == DynamicSortPolicy)) {
    //        sortTree(root, SortNodeOnly);
    //    }

    // rebuild node children `indexInParent` member
    invalidateOrder(node);
}

//------------------------------------------------------------------------------

void QVariantModel::resetShowNode(node_t* node, bool canModifyModel,
                                  bool showOnlyChildren)
{
    Q_ASSERT(node);

    int childrenCount = node->children.count();

    // if root => faster way with reset whole model
    if (node == mp_root.data()) {
        beginResetModel();
        setTreeVisibility(node, Visible);
        endResetModel();
        return;
    }

    // if some children are still visible => wipe them out
    int visibleChildrenCount = node->visibleChildren.count();
    if(visibleChildrenCount > 0)
        resetHideNode(node, canModifyModel, HideOnlyChildren);

    // show itself
    if (node->visible == false && showOnlyChildren == false) {
        if (canModifyModel) {
            QModelIndex parent = indexForNode(node->parent);
            int lastPos = node->parent->visibleChildren.count();
            beginInsertRows(parent, lastPos, lastPos);
        }

        node->parent->visibleChildren.append(node);
        node->visible = true;

        if (canModifyModel)
            endInsertRows();
    }

    if (childrenCount > 0) {
        // show all visible direct children
        if (canModifyModel) {
            QModelIndex index = indexForNode(node);
            Q_ASSERT(rowCount(index) == 0);
            beginInsertRows(index, 0, childrenCount-1);
        }

        for (auto itChild = node->children.begin();
             itChild != node->children.end(); ++itChild) {
            node_t* child = *itChild;
            child->visible = true;
        }
        node->visibleChildren = node->children;

        if (canModifyModel)
            endInsertRows();

        // show all visible children's children
        for (auto itChild = node->visibleChildren.begin();
             itChild != node->visibleChildren.end(); ++itChild) {
            node_t* child = *itChild;
            if (child->children.isEmpty() == false)
                resetShowNode(child, canModifyModel, ShowOnlyChildren);
        }
    }
}

void QVariantModel::resetHideNode(node_t* node, bool canModifyModel,
                                  bool hideOnlyChildren)
{
    Q_ASSERT(node);

    int visibleChildrenCount = node->visibleChildren.count();

    // if root => faster way with reset whole model
    if (node == mp_root.data()) {
        beginResetModel();
        setTreeVisibility(node, Hidden);
        endResetModel();
        return;
    }

    // hide all visible children's children
    for (auto itChild = node->visibleChildren.begin();
         itChild != node->visibleChildren.end(); ++itChild) {
        node_t* child = *itChild;
        if (child->visibleChildren.isEmpty() == false)
            resetHideNode(child, canModifyModel, HideOnlyChildren);
    }

    if (visibleChildrenCount > 0) {
        // hide all visible direct children
        if (canModifyModel) {
            QModelIndex index = indexForNode(node);
            Q_ASSERT(rowCount(index) == visibleChildrenCount);
            beginRemoveRows(index, 0, visibleChildrenCount-1);
        }

        for (auto itChild = node->visibleChildren.begin();
             itChild != node->visibleChildren.end(); ++itChild) {
            node_t* child = *itChild;
            child->visible = false;
        }
        node->visibleChildren.clear();

        if (canModifyModel)
            endRemoveRows();
    }

    // hide itself
    if (node->visible && hideOnlyChildren == false) {
        if (canModifyModel) {
            QModelIndex index = indexForNode(node);
            beginRemoveRows(index.parent(), index.row(), index.row());
        }

        node->parent->visibleChildren.removeOne(node);
        node->visible = false;

        if (canModifyModel)
            endRemoveRows();
    }
}

void QVariantModel::setTreeVisibility(node_t* node, bool visible)
{
    Q_ASSERT(node);

    node->visible = visible;
    node->visibleChildren.clear();
    if (visible)
        node->visibleChildren = node->children;

    // tree visit
    for (auto itChild = node->children.begin();
         itChild != node->children.end(); ++itChild) {
        setTreeVisibility(*itChild, visible);
    }
}

//------------------------------------------------------------------------------

void QVariantModel::dumpTree(const node_t* root,
                             const QString& prefix) const
{
#ifdef QVARIANTMODEL_DEBUG
    if (root == nullptr) {
        root = mp_root.data();
        qDebug().nospace() << (prefix + "node(root)");
    }
    else {
        qDebug().nospace() << DBG_NODE(prefix, root);
    }
    foreach(node_t* child, root->visibleChildren)
        dumpTree(child, prefix + "--");
#else
    Q_UNUSED(root)
    Q_UNUSED(prefix)
#endif
}

void QVariantModel::dumpTreeCache(const node_t* root,
                                  const QString& prefix) const
{
#ifdef QVARIANTMODEL_DEBUG
    if (root == nullptr) {
        root = mp_root.data();
        qDebug().nospace() << (prefix + "cache(root)");
    }
    else {
        // qDebug().nospace() << (prefix + DBG_NODE(root));
        qDebug().nospace() << DBG_NODE_CACHE(prefix, root);
    }
    foreach(node_t* child, root->visibleChildren)
        dumpTreeCache(child, prefix + "--");
#else
    Q_UNUSED(root)
    Q_UNUSED(prefix)
#endif
}

void QVariantModel::dumpModel(const QModelIndex& rootIndex,
                              const QString& prefix) const
{
#ifdef QVARIANTMODEL_DEBUG
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
                               + "index(TEXT: key="
                               + data(indexKey, Qt::DisplayRole).toString()
                               + ", value="
                               + data(indexValue, Qt::DisplayRole).toString()
                               + ", type="
                               + data(indexType, Qt::DisplayRole).toString())
                           << "; FLAGS: key=" << flags(indexKey)
                           << ", value=" << flags(indexValue)
                           << ", type=" << flags(indexType)
                           << ")";
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

//------------------------------------------------------------------------------

#ifdef QVM_DEBUG_CHANGE_MODEL
void QVariantModel::beginResetModel()
{
    qDebug() << "begin reset model";

    QAbstractItemModel::beginResetModel();
}

void QVariantModel::beginInsertRows(
        const QModelIndex &parent, int first, int last)
{
    node_t* pnode = mp_root.data();
    if (parent.isValid())
        pnode = static_cast<node_t*>(parent.internalPointer());
    Q_ASSERT(pnode != nullptr);

    qDebug() << "begin insert rows" << keyPath(pnode) << first << last;

    Q_ASSERT(first >= 0);
    Q_ASSERT(last >= 0);
    Q_ASSERT(first <= last);

    QAbstractItemModel::beginInsertRows(parent, first, last);
}

void QVariantModel::beginRemoveRows(
        const QModelIndex &parent, int first, int last)
{
    node_t* pnode = mp_root.data();
    if (parent.isValid())
        pnode = static_cast<node_t*>(parent.internalPointer());
    Q_ASSERT(pnode != nullptr);

    qDebug() << "begin remove rows" << keyPath(pnode) << first << last;

    Q_ASSERT(first >= 0);
    Q_ASSERT(last >= 0);
    Q_ASSERT(first <= last);

    QAbstractItemModel::beginRemoveRows(parent, first, last);
}

bool QVariantModel::beginMoveRows(
        const QModelIndex &sourceParent, int sourceFirst, int sourceLast,
        const QModelIndex &destinationParent, int destinationRow)
{
    node_t* sourceNode = mp_root.data();
    if (destinationParent.isValid())
        sourceNode = static_cast<node_t*>(destinationParent.internalPointer());
    Q_ASSERT(sourceNode != nullptr);

    node_t* destinationNode = mp_root.data();
    if (destinationParent.isValid())
        destinationNode = static_cast<node_t*>(destinationParent.internalPointer());
    Q_ASSERT(destinationNode != nullptr);

    qDebug() << "begin move rows" << keyPath(sourceNode) << sourceFirst << sourceLast
             << keyPath(destinationNode) << destinationRow;

    bool canMove = QAbstractItemModel::beginMoveRows(
                sourceParent, sourceFirst, sourceLast,
                destinationParent, destinationRow);

    if (canMove == false)
        qDebug() << "cannot move rows";

    return canMove;
}

void QVariantModel::endResetModel()
{
    qDebug() << "end reset model";

    QAbstractItemModel::endResetModel();
}

void QVariantModel::endInsertRows()
{
    qDebug() << "end insert rows";

    QAbstractItemModel::endInsertRows();
}

void QVariantModel::endRemoveRows()
{
    qDebug() << "end remove rows";

    QAbstractItemModel::endRemoveRows();
}

void QVariantModel::endMoveRows()
{
    qDebug() << "end move rows";

    QAbstractItemModel::endMoveRows();
}
#endif

//==============================================================================

QVariantModelDataLoader::QVariantModelDataLoader(node_t* root) :
    QRunnable(),
    node(root),
    mutex(QMutex::NonRecursive)
{
    exclusive.isDone = false;
    setAutoDelete(false);
    Q_ASSERT(root != nullptr);
}

void QVariantModelDataLoader::run()
{
    Q_ASSERT(node != nullptr);

    buildNode();

    {
        QMutexLocker locker(&mutex);
        exclusive.isDone = true;
    }
}

void QVariantModelDataLoader::buildNode()
{
    Q_ASSERT(node);
    Q_ASSERT(node->loaded  == false);
    // node can be tree root, so it can have no parent

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
        child->visible = false;

        // optimization: give the child loaded status as fully loaded
        // if doesn't required to load something
        child->loaded = true;
        QVariantDataInfo childDInfo(child->value);
        if (childDInfo.isContainer() && childDInfo.isEmptyContainer() == false)
            child->loaded = false;

#ifdef QVM_DEBUG_BUILD
        childrenCount++;
        if (newChildren.size() >= SprintBuildSize) {
            qDebug().nospace()
                    << "build node"
                    << " key:" << rootKeyStr
                    << " loaded: " << childrenCount
                    << "/" << keysCount;
        }
#endif

        // build child node cache
        Model::cached(child, to_cache.displayDepth);

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
}
