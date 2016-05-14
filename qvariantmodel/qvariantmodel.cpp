#include "qvariantmodel.h"

#include <QThreadPool>
#include <QMutexLocker>
#include <QDebug>

#include "qvariantdatainfo.h"


#define DBG_NODE(p, root) (QString(p) + "node(key=" + QVariantDataInfo(root->keyInParent).displayText(1) + ", value=" + QVariantDataInfo(root->value).displayText(1) + ", type=" + QString::number(root->value.userType()) + "-" + root->value.typeName() + ")")
#define DBG_NODE_CACHE(p, root) (QString(p) + "cache(TEXT: key=" + root->cache.key.text + ", value=" + root->cache.value.text + ", type=" + root->cache.type.text) << "; FLAGS: key=" << root->cache.key.flags << ", value=" << root->cache.value.flags << ", type=" << root->cache.type.flags << ")"



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
    m_dynamicSort(true),
    m_dynamicFilter(true)
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

    clearChildren(mp_root.data());

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
    Q_ASSERT(root->visibleChildren.isEmpty());

    // if node is still loading, that's too bad
    bool canDeleteLoader = false;
    if (root->loader) {
        QMutexLocker lock(&root->loader->mutex);

        canDeleteLoader = root->loader->exclusive.isDone;

        // to avoid leaks, the thread pool will delete it as soon as it
        // finishes loading
        if (canDeleteLoader == false)
            root->loader->setAutoDelete(true);
    }

    // we do not delete a locked mutex, so deletion is forbidden with active lock
    if (canDeleteLoader) {
        delete root->loader;
        root->loader = nullptr;
    }

    // delete children
    while (root->children.isEmpty() == false) {
        node_t* child = root->children.takeLast();

        Q_ASSERT(child->visible == false);
        Q_ASSERT(root == child->parent);

        clearChildren(child);

        delete child;
    }
    root->children.reserve(reserveSize);
}

QString QVariantModel::keyPath(const node_t* node) const
{
    QString keyStr;
    if (node == nullptr)
        keyStr = QLatin1Literal("NULL");
    else if (node == mp_root.data())
        keyStr = QLatin1Literal("ROOT");
    else if (node->parent) {
        keyStr = QVariantDataInfo(node->keyInParent).displayText(0);
        keyStr.prepend("->").prepend(keyPath(node->parent));
    }
    return keyStr;
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
                                 bool canEmitChanges)
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
    if (pnode != mp_root)
        cached(pnode, m_depth, cacheRoles);

    for (auto it = pnode->visibleChildren.constBegin();
         it != pnode->visibleChildren.constEnd(); ++it) {
        recachedTree(*it, cacheRoles);
    }

    // emit update (only if not tree root, because
    // this is useless as it is hidden)
    if (canEmitChanges && pnode != mp_root) {
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

void QVariantModel::loadNode(node_t* node, bool canEmitChanges)
{
    Q_ASSERT(node != nullptr);

    node_t* hidden_root = mp_root.data();
    Q_UNUSED(hidden_root); // avoid warning if no assert
    Q_ASSERT((node->parent != nullptr) ^ (node == hidden_root));

    Q_ASSERT(node->loaded == false);

#ifdef QVM_DEBUG_LOAD
    qDebug().nospace()
            << "loadNode"
            << " key:" << keyPath(node)
            << (canEmitChanges ? " canEmitChanges" : " noChangesEmitted")
            << " loader:" << ((void*)node->loader);
#endif

    QList<node_t*> newlyCreatedChildren;

    // if never asked to load
    if (node->loader == nullptr) {
        // as we are supposed to never been asked to load, we should be empty
        Q_ASSERT(node->children.isEmpty());

        node->loader = new QVariantModelDataLoader(node);
        node->loader->to_cache.displayDepth = m_depth;

        QVariantDataInfo dInfo(node->value);
        Q_ASSERT(dInfo.isContainer());
        int count = dInfo.containerCount();

        // if too much datas, load async
        // otherwise load sync
        if (count >= (int)SizeLimitToLoadAsync) {
#ifdef QVM_DEBUG_LOAD
            qDebug().nospace()
                    << "loadMore"
                    << " key:" << keyPath(node)
                    << " start async";
#endif

            // load async
            QThreadPool* thPool = QThreadPool::globalInstance();
            Q_ASSERT(thPool);
            thPool->start(node->loader);

            // we stop here, the loading is started
            return;
        }
        // direct load (so sync)
        else {
            Q_ASSERT(node->loader != nullptr);

#ifdef QVM_DEBUG_LOAD
            qDebug().nospace()
                    << "loadMore"
                    << " key:" << keyPath(node)
                    << " direct load sync";
#endif

            // load now
            node->loader->run();

            // get created children
            newlyCreatedChildren.swap(node->loader->exclusive.createdChildren);
            node->loaded = true;

#ifdef QVM_DEBUG_LOAD
            qDebug().nospace()
                    << "loadMore"
                    << " key:" << keyPath(node)
                    << " done count:" << newlyCreatedChildren.count();
#endif
        }
    }
    // if asked to load previously, we gather the created nodes from the loader
    // this block is also made by the locker constraint mecanism only
    else {
        Q_ASSERT(node->loader != nullptr);

        QMutexLocker locker(&node->loader->mutex);

        // get created children
        newlyCreatedChildren.swap(node->loader->exclusive.createdChildren);

        // we are considere loaded if the loader is done
        node->loaded = node->loader->exclusive.isDone;

#ifdef QVM_DEBUG_LOAD
        if (node->loaded) {
            qDebug().nospace()
                    << "loadMore"
                    << " key:" << keyPath(node)
                    << " done last - count:" << node->children.count()
                    << " + " << newlyCreatedChildren.count();
        }
        else {
            qDebug().nospace()
                    << "loadMore"
                    << " key:" << keyPath(node)
                    << " in progress - count:" << node->children.count()
                    << " + " << newlyCreatedChildren.count();
        }
#endif
    }

    Q_ASSERT(node->loader != nullptr);

    // add the created children if any
    int createdCount = newlyCreatedChildren.count();
    if (createdCount > 0) {
        node->children.append(newlyCreatedChildren);

        // filter & sort
        if (isFilterEnabled())
            filter(canEmitChanges);
        else
            showDirectChildren(node, newlyCreatedChildren, canEmitChanges);
    }

    // if node is loaded, we delete the loader
    // but we delete it here because we mustn't delete a locked mutex
    if (node->loaded && node->loader != nullptr) {
        delete node->loader;
        node->loader = nullptr;
    }
}

void QVariantModel::loadChildren(node_t* node, bool canEmitChanges)
{
    Q_ASSERT(node != nullptr);

    node_t* hidden_root = mp_root.data();
    Q_UNUSED(hidden_root); // avoid warning if no assert
    Q_ASSERT((node->parent != nullptr) ^ (node == hidden_root));

    Q_ASSERT(node->loaded == true);
    Q_ASSERT(node->loader == nullptr);

#ifdef QVM_DEBUG_LOAD
    qDebug().nospace()
            << "load children"
            << " key:" << keyPath(node)
            << (canEmitChanges ? " canEmitChanges" : " noChangesEmitted");
#endif

    //
    QVariantModelDataLoader loader(node);
    loader.excludedKeys.reserve(node->children.count());
    for (auto itChild = node->children.constBegin();
         itChild != node->children.constEnd(); ++itChild) {
        node_t* child = *itChild;
        loader.excludedKeys.append(child->keyInParent);
    }

    // load now
    loader.run();

    // add the created children if any
    int createdCount = loader.exclusive.createdChildren.count();
    if (createdCount > 0) {
        node->children.append(loader.exclusive.createdChildren);

        // filter & sort
        if (isFilterEnabled())
            filter(canEmitChanges);
        else
            showDirectChildren(node, loader.exclusive.createdChildren);
    }
}

void QVariantModel::unloadChildren(node_t* node, bool canEmitChanges)
{
    Q_ASSERT(node != nullptr);

    node_t* hidden_root = mp_root.data();
    Q_UNUSED(hidden_root); // avoid warning if no assert
    Q_ASSERT((node->parent != nullptr) ^ (node == hidden_root));

    Q_ASSERT(node->loaded == true);
    Q_ASSERT(node->loader == nullptr);

#ifdef QVM_DEBUG_LOAD
    qDebug().nospace()
            << "unload children"
            << " key:" << keyPath(node)
            << (canEmitChanges ? " canEmitChanges" : " noChangesEmitted")
            << " children:" << node->children.count()
            << " (visible:" << node->visibleChildren.count() << ")";
#endif

    QVariantDataInfo dInfo(node->value);
    Q_ASSERT(dInfo.isContainer());
    QList<QVariant> keys = dInfo.containerKeys();
    QList<node_t*> legacyChildren;

    // get the legacy children by keys
    // if remove all
    if (node->children.count() == keys.count())
        legacyChildren = node->children;
    // if remove some children, but not all
    else {
        for (auto itChild = node->children.constBegin();
             itChild != node->children.constEnd(); ++itChild) {
            node_t* child = *itChild;
            bool isKept = keys.removeOne(child->keyInParent);
            if (isKept == false)
                legacyChildren.append(child);
        }
    }

    // remove the legacy children if any
    if (legacyChildren.isEmpty())
        return;

    // if remove all
    if (legacyChildren.count() == node->children.count()) {
        hideChildren(node, canEmitChanges);
        clearChildren(node);
    }
    // if remove some children, but not all
    else {
        while (legacyChildren.isEmpty() == false) {
            node_t* child = legacyChildren.takeFirst();

            hidePathsFromNode(child, canEmitChanges);

            // remove the child in memory
            node->children.removeOne(child);
            clearChildren(child);
            child->parent = nullptr;
//            delete child;
        }
    }

    // we suppose the remove do not change the order, so no filter/sort
}

void QVariantModel::fetchMore(const QModelIndex &parent)
{
    node_t* pnode = mp_root.data();
    if (parent.isValid())
        pnode = static_cast<node_t*>(parent.internalPointer());
    Q_ASSERT(pnode != nullptr);

    if (pnode->loaded == false)
        loadNode(pnode, EmitChangedSignals);
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
    Q_ASSERT(node->visible);
    int row = 0;
    if (node->parent) {
        Q_ASSERT(node->indexInParent >= 0); // to debug only
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
    node_t* node = pnode->visibleChildren.at(row);
    Q_ASSERT(node->indexInParent == row);
    return createIndex(row, column, node);
}

QModelIndex QVariantModel::parent(const QModelIndex& child) const
{
    if (child.isValid()) {
        node_t* node = static_cast<node_t*>(child.internalPointer());
        Q_ASSERT(node != nullptr);
        if (node->parent && node->parent != mp_root)
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

void QVariantModel::invalidateParentsData(const QModelIndex& index,
                                          bool canEmitChanges)
{
    node_t* node = mp_root.data();
    if (index.isValid())
        node = static_cast<node_t*>(index.internalPointer());
    Q_ASSERT(node != nullptr);

    QVector<int> displayRole;
    displayRole << Qt::DisplayRole;

    // update its parents
    while (node->parent != nullptr && node != mp_root) {
        // change data in parent
        QMutableVariantDataInfo mutDInfoParent(node->parent->value);
        Q_ASSERT(mutDInfoParent.isContainer());
        Q_ASSERT(mutDInfoParent.editableValues());
        mutDInfoParent.setContainerValue(node->keyInParent,
                                         node->value);

        // update cache
        cached(node->parent, m_depth, CacheText);

        if (canEmitChanges) {
            QModelIndex parent = indexForNode(node->parent, ValueColumn);
            emit dataChanged(parent, parent, displayRole);
        }

        // next parent
        node = node->parent;
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

        // update the cache
        cached(node, m_depth);

        // update direct parent about node key
        QMutableVariantDataInfo mutDInfoParent(node->parent->value);
        Q_ASSERT(mutDInfoParent.isContainer());
        Q_ASSERT(mutDInfoParent.editableKeys());
        mutDInfoParent.setContainerKey(oldKey, node->keyInParent);

        // update the cache
        cached(node->parent, m_depth);

        // update the node index
        if (node->visible)
            emit dataChanged(index, index, QVector<int>() << Qt::DisplayRole);

        // update its parents
        invalidateParentsData(index.parent(), node->visible);

        dataChanges = true;
    }
    else  if (index.column() == column(ValueColumn)) {
        Q_ASSERT(node->parent != nullptr);
        // when change value, type might change

        bool typeChanged = (node->value.userType() != value.userType());

        // update the value
        node->value = value;

        // update the cache
        cached(node, m_depth);

        // emit the update of the row (we want to update value+type columns)
        QVector<int> roles = QVector<int>() << Qt::DisplayRole << role;

        // update value column
        emit dataChanged(index, index, roles);

        // update type column if changed
        if (typeChanged && node->visible) {
            QModelIndex typeIndex = indexForNode(node, TypeColumn);
            emit dataChanged(typeIndex, typeIndex, roles);
        }

        // update its parents
        invalidateParentsData(index, node->visible);

        dataChanges = true;
    }
    else  if (index.column() == column(TypeColumn)) {
        Q_ASSERT(node->parent != nullptr);
        // when change type, value change automatically

        // update the value and the type, as simple as this
        QVariantDataInfo dInfo(node->value);
        node->value = dInfo.tryConvert(value.type(), value.userType());

        // update the cache
        cached(node, m_depth);

        // emit the update of the row (we want to update value+type columns)
        QVector<int> roles = QVector<int>() << Qt::DisplayRole << role;

        if (node->visible) {
            // update type column
            emit dataChanged(index, index, roles);

            // update value column
            QModelIndex valueIndex = indexForNode(node, ValueColumn);
            emit dataChanged(valueIndex, valueIndex, roles);
        }

        // update its parents
        invalidateParentsData(index, node->visible);

        dataChanges = true;
    }

    if (dataChanges) {
        emit rootDatasChanged();

        // re-filter
        if (isFilterEnabled())
            filterTree(node, node->visible);
    }

    // the order might changed if sort
    // do begin/end move rows
    //    if (m_dynamicSort)
    //        sortTree(*node->parent, SortNodeOnly);

    return dataChanges;
}

bool QVariantModel::insertRows(int row, int count, const QModelIndex& parent)
{
    Q_ASSERT(row >= 0 && count >= 0 && row+count <= rowCount(parent)+1);

    bool dataInserted = true;

    node_t* pnode = mp_root.data();
    if (parent.isValid())
        pnode = static_cast<node_t*>(parent.internalPointer());
    Q_ASSERT(pnode != nullptr);

    QMutableVariantDataInfo mutDInfo(pnode->value);
    if (mutDInfo.isContainer() == false
            || mutDInfo.isNewKeyInsertable() == false)
        return false;

    int childrenCount = pnode->visibleChildren.count();
    Q_ASSERT(row >= 0 && row <= childrenCount); // row == count when append

    // add as much rows needed
    while (count-- > 0) {
        node_t* beforeNode = pnode->visibleChildren.value(row, nullptr);
        QVariant beforeKey = beforeNode ? beforeNode->keyInParent : QVariant();
        mutDInfo.tryInsertNewKey(beforeKey, QVariant());
    }

    // data changes
    emit rootDatasChanged();

    // forward change to parents until root
    invalidateParentsData(parent);

    // truely add nodes in the tree
    loadChildren(pnode, pnode->visible);

    return dataInserted;
}

bool QVariantModel::removeRows(int row, int count, const QModelIndex& parent)
{
    Q_ASSERT(row >= 0 && count >= 0 && row+count <= rowCount(parent));

    bool dataRemoved = true;

    node_t* pnode = mp_root.data();
    if (parent.isValid())
        pnode = static_cast<node_t*>(parent.internalPointer());
    Q_ASSERT(pnode != nullptr);

    QMutableVariantDataInfo mutDInfo(pnode->value);
    if (mutDInfo.isContainer() == false
            || mutDInfo.isKeyRemovable() == false)
        return false;

    int childrenCount = pnode->visibleChildren.count();
    Q_ASSERT(row >= 0 && row+count <= childrenCount);

    // remove as much rows asked
    while (count-- > 0) {
        node_t* node = pnode->visibleChildren.value(row+count, nullptr);
        Q_ASSERT(node != nullptr);
        mutDInfo.removeKey(node->keyInParent);
    }

    // data changes
    emit rootDatasChanged();

    // forward change to parents until root
    invalidateParentsData(parent);

    // truely remove nodes from the tree
    unloadChildren(pnode, pnode->visible);

    return dataRemoved;
}

//------------------------------------------------------------------------------

QModelIndex QVariantModel::createIndex(int row, int column, node_t* node) const
{
    Q_ASSERT((node == nullptr) ^ (node != mp_root));
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

void QVariantModel::setDynamicFilter(bool enabled)
{
    if (m_dynamicFilter == enabled)
        return;

    m_dynamicFilter = enabled;
    emit dynamicFilterChanged(enabled);

    if (m_dynamicFilter)
        filter();
}

void QVariantModel::updateFilterRx(QString pattern)
{
    m_filterRx = QRegExp(pattern, Qt::CaseSensitive,
                         (m_filterType == WildCard)
                         ? QRegExp::WildcardUnix : QRegExp::RegExp2);
}

void QVariantModel::filter()
{
    filter(EmitChangedSignals);
}

void QVariantModel::filter(bool canEmitChanges)
{
#ifdef QVM_DEBUG_CHANGE_MODEL
    qDebug() << "begin filter"
             << keyPath(mp_root.data())
             << (canEmitChanges ? "canEmitChanges" : "noChangesEmitted")
             << (mp_root->loaded ? "loaded" : "not_loaded")
             << (isFilterEnabled() ? "select" : "clear")
             << FILTER_TYPE_NAMES[m_filterType]
                << m_filterRx.pattern();
#endif

    filterTree(mp_root.data(), canEmitChanges);

#ifdef QVM_DEBUG_CHANGE_MODEL
    qDebug() << "end filter";
#endif
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

    if (m_dynamicFilter && (wasFiltered || nowFiltered))
        filter();
}

void QVariantModel::setFilterColumns(Columns filterColumns)
{
    if (m_filterColumns == filterColumns)
        return;

    bool wasFiltered = isFilterEnabled();

    m_filterColumns = filterColumns;
    emit filterColumnsChanged(filterColumns);

    bool nowFiltered = isFilterEnabled();

    if (m_dynamicFilter && (wasFiltered || nowFiltered))
        filter();
}

void QVariantModel::setFilterText(const QString& filterText)
{
    if (m_filterRx.pattern() == filterText)
        return;

    bool wasFiltered = isFilterEnabled();

    updateFilterRx(filterText);
    emit filterTextChanged(filterText);

    bool nowFiltered = isFilterEnabled();

    if (m_dynamicFilter && (wasFiltered || nowFiltered))
        filter();
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

void QVariantModel::filterTree(node_t* node, bool canEmitChanges)
{
    Q_ASSERT(node);

    // actual filter
    if (isFilterEnabled()) {
        int amountToFilter = node->children.count();
        bool betterReset = (amountToFilter >= (int)SizeLimitBetterResetWhenDataChanged);

        // if the best strategy is to wipe out node's visibles children rows
        // and then insert them later
        if (betterReset)
            hideChildren(node, canEmitChanges);

        // filter children if loaded
        // or filter data if not (see this later)
        for (auto itChild = node->children.begin();
             itChild != node->children.end(); ++itChild) {
            filterTree(*itChild, canEmitChanges);
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
                showPathToNode(node, canEmitChanges);
            // if disappeared
            else if (!nowVisible && wasVisible)
                hidePathsFromNode(node, canEmitChanges);
        }
    }
    // display all
    else
        showLoadedChildren(node, canEmitChanges);

    //    if (sortPolicy == ForceSort
    //            || (m_dynamicSort && sortPolicy == DynamicSortPolicy)) {
    //        sortTree(root, SortNodeOnly);
    //    }

#ifdef QVARIANTMODEL_DEBUG
    fullConsistencyCheck();
#endif
}

//------------------------------------------------------------------------------

void QVariantModel::showPathToNode(node_t* node, bool canEmitChanges)
{
#ifdef QVM_DEBUG_CHANGE_MODEL
    qDebug() << "show path to node" << keyPath(node)
             << (canEmitChanges ? "canEmitChanges" : "noChangesEmitted")
             << (node && node->visible ? "visible" : "notVisible");
#endif

    if (node == nullptr)
        return;
    else if (node == mp_root.data())
        return;
    else if (node->visible)
        return;

    Q_ASSERT(node->parent != nullptr);

    // make sure its parent parent is visible
    showPathToNode(node->parent);

    int insertPos = node->parent->visibleChildren.count();
    if (canEmitChanges) {
        QModelIndex parent = indexForNode(node->parent);
        beginInsertRows(parent, insertPos, insertPos);
    }

    node->parent->visibleChildren.insert(insertPos, node);
    node->visible = true;
    invalidateOrder(node->parent, insertPos);

    if (canEmitChanges)
        endInsertRows();
}

void QVariantModel::hidePathsFromNode(node_t* node, bool canEmitChanges)
{
#ifdef QVM_DEBUG_CHANGE_MODEL
    qDebug() << "hide paths from node" << keyPath(node)
             << (canEmitChanges ? "canEmitChanges" : "noChangesEmitted")
             << (node && node->visible ? "visible" : "notVisible");
#endif

    if (node == nullptr)
        return;
    else if (node == mp_root.data())
        return;
    else if (node->visible == false)
        return;

    Q_ASSERT(node->parent != nullptr);

    // if still has visible children, we hide them
    hideChildren(node, canEmitChanges);

    // hide itself
    int nodePos = node->parent->visibleChildren.indexOf(node);
    Q_ASSERT(nodePos >= 0);

    if (canEmitChanges) {
        QModelIndex index = indexForNode(node);
        Q_ASSERT(nodePos == index.row());
        beginRemoveRows(index.parent(), index.row(), index.row());
    }

    node->parent->visibleChildren.removeAt(nodePos);
    node->indexInParent = -1;
    node->visible = false;
    invalidateOrder(node->parent, nodePos);

    if (canEmitChanges)
        endRemoveRows();
}

void QVariantModel::showDirectChildren(
        node_t* node, const QList<node_t *>& nodes, bool canEmitChanges)
{
#ifdef QVM_DEBUG_CHANGE_MODEL
    qDebug() << "show direct children" << keyPath(node) << nodes.count()
             << (canEmitChanges ? "canEmitChanges" : "noChangesEmitted")
             << (node && node->visible ? "visible" : "notVisible");
#endif

    if (node == nullptr)
        return;

    showPathToNode(node, canEmitChanges);

    // show all children given, append them after the already shown children
    int count = nodes.count();
    int insertPos = node->visibleChildren.count();
    if (canEmitChanges) {
        QModelIndex parent = indexForNode(node);
        beginInsertRows(parent, insertPos, insertPos+count-1);
    }

    for (auto itChild = nodes.constBegin(); itChild != nodes.constEnd();
         ++itChild) {
        node_t* child = *itChild;
        Q_ASSERT(node->children.contains(child));
        child->visible = true;
        child->indexInParent = insertPos;
        node->visibleChildren.insert(insertPos++, child);
    }

    if (canEmitChanges)
        endInsertRows();
}

void QVariantModel::showLoadedChildren(node_t* node, bool canEmitChanges)
{
#ifdef QVM_DEBUG_CHANGE_MODEL
    qDebug() << "show loaded children" << keyPath(node)
             << (canEmitChanges ? "canEmitChanges" : "noChangesEmitted")
             << (node && node->visible ? "visible" : "notVisible");
#endif

    if (node == nullptr)
        return;

    // if some children are still visible => wipe them out
    int visibleChildrenCount = node->visibleChildren.count();
    if (visibleChildrenCount > 0)
        hideChildren(node, canEmitChanges);

    showPathToNode(node, canEmitChanges);

    int childrenCount = node->children.count();
    if (childrenCount > 0) {
        // show all visible direct children
        if (canEmitChanges) {
            QModelIndex index = indexForNode(node);
            beginInsertRows(index, 0, childrenCount-1);
        }

        int iChild = 0;
        for (auto itChild = node->children.begin();
             itChild != node->children.end(); ++itChild) {
            node_t* child = *itChild;
            child->indexInParent = iChild++;
            child->visible = true;
        }
        node->visibleChildren = node->children;

        if (canEmitChanges)
            endInsertRows();

        // show all visible children's children
        for (auto itChild = node->visibleChildren.begin();
             itChild != node->visibleChildren.end(); ++itChild) {
            node_t* child = *itChild;
            if (child->children.isEmpty() == false)
                showLoadedChildren(child, canEmitChanges);
        }
    }
}

void QVariantModel::hideChildren(node_t* node, bool canEmitChanges)
{
#ifdef QVM_DEBUG_CHANGE_MODEL
    qDebug() << "hide children" << keyPath(node)
             << (canEmitChanges ? "canEmitChanges" : "noChangesEmitted")
             << (node && node->visible ? "visible" : "notVisible");
#endif

    if (node == nullptr)
        return;
    else if (node == mp_root.data())
        return;
    else if (node->visible == false) {
        Q_ASSERT(node->visibleChildren.isEmpty());
        return;
    }

    if (node->visibleChildren.isEmpty())
        return;

    // hide all visible children's children
    for (auto itChild = node->visibleChildren.begin();
         itChild != node->visibleChildren.end(); ++itChild) {
        node_t* child = *itChild;
        hideChildren(child, canEmitChanges);
    }

    // hide all visible direct children
    if (canEmitChanges) {
        QModelIndex index = indexForNode(node);
        int visibleChildrenCount = node->visibleChildren.count();
        beginRemoveRows(index, 0, visibleChildrenCount-1);
    }

    for (auto itChild = node->visibleChildren.begin();
         itChild != node->visibleChildren.end(); ++itChild) {
        node_t* child = *itChild;
        child->indexInParent = -1;
        child->visible = false;
    }
    node->visibleChildren.clear();

    if (canEmitChanges)
        endRemoveRows();
}

void QVariantModel::setTreeVisibility(node_t* node, bool visible) const
{
#ifdef QVM_DEBUG_CHANGE_MODEL
    qDebug() << "set tree visibility" << keyPath(node)
             << (visible ? "show" : "hide");
#endif

    Q_ASSERT(node);

    node->visible = visible;
    node->visibleChildren.clear();
    if (visible)
        node->visibleChildren = node->children;

    // tree visit
    int iChild = 0;
    for (auto itChild = node->children.begin();
         itChild != node->children.end(); ++itChild) {
        node_t* child = *itChild;
        child->indexInParent = (visible) ? iChild++ : -1;
        setTreeVisibility(child, visible);
    }
}

void QVariantModel::invalidateOrder(node_t* node, int start, int length)
{
    int iChild = start;
    for (auto it = node->visibleChildren.constBegin() + start;
         it != node->visibleChildren.constEnd() && length-- != 0; ++it) {
        node_t* child = *it;
        child->indexInParent = iChild++;
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

void QVariantModel::fullConsistencyCheck() const
{
    fullConsistencyCheck(mp_root.data());
}

void QVariantModel::fullConsistencyCheck(node_t* node) const
{
    Q_ASSERT(node);

    if (node != mp_root) {
        Q_ASSERT(node->parent != nullptr);
        Q_ASSERT(node->visible == node->parent->visibleChildren.contains(node));
        if (node->visible)
            Q_ASSERT(node->indexInParent == node->parent->visibleChildren.indexOf(node));
    }

    for (auto itChild = node->children.constBegin();
         itChild != node->children.constEnd(); ++itChild) {
        fullConsistencyCheck(*itChild);
    }
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
    // node can be tree root, so it can have no parent
    // the node can already been tag as loaded, in case of reloading

    QVariantDataInfo dInfo(node->value);
    Q_ASSERT(dInfo.isContainer());

    QVariantList keys = dInfo.containerKeys();
    int keysCount = keys.count();

#ifdef QVM_DEBUG_BUILD
    QString rootKeyStr = keyPath(node);
    int childrenCount = 0;
    qDebug().nospace() << "build node key: " << rootKeyStr
                       << " begin: (" << keysCount
                       << " - excluded:" << excludedKeys.count() << ")";
#endif

    // first: we create all root's children structure
    QList<node_t*> newChildren;
    newChildren.reserve(SprintBuildSize);
    for (auto itkey = keys.constBegin(); itkey != keys.constEnd(); ++itkey){
        QVariant childKey = *itkey;

        bool isExcludedKey = excludedKeys.removeOne(childKey);
        if (isExcludedKey)
            continue;

        QVariant childData = dInfo.containerValue(*itkey);

        node_t* child = new node_t;
        newChildren.append(child);

        child->parent = node;
        child->keyInParent = childKey;
        child->value = childData;
        child->visible = false;
        child->indexInParent = -1;

        // optimization: give the child loaded status as fully loaded
        // if doesn't required to load something
        child->loaded = true;
        QVariantDataInfo childDInfo(child->value);
        if (childDInfo.isContainer() && childDInfo.isEmptyContainer() == false)
            child->loaded = false;

        // build child node cache
        Model::cached(child, to_cache.displayDepth);

#ifdef QVM_DEBUG_BUILD
        childrenCount++;
        if (newChildren.size() >= SprintBuildSize) {
            qDebug().nospace()
                    << "build node"
                    << " key: " << rootKeyStr
                    << " loaded: " << childrenCount
                    << " / (" << keysCount
                    << " - excluded:" << excludedKeys.count() << ")";
        }
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
            << " key: " << rootKeyStr
            << " loaded: " << childrenCount
            << " / (" << keysCount
            << " - excluded:" << excludedKeys.count() << ")";
#endif
}


QString QVariantModelDataLoader::keyPath(const node_t* node) const
{
    QString keyStr;
    if (node == nullptr)
        keyStr = QLatin1Literal("NULL");
    else if (node->parent) {
        keyStr = QVariantDataInfo(node->keyInParent).displayText(0);
        keyStr.prepend("->").prepend(keyPath(node->parent));
    }
    return keyStr;
}
