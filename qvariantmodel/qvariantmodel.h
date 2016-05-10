#ifndef QVARIANTMODEL_H
#define QVARIANTMODEL_H

#include <QSharedPointer>
#include <QObject>
#include <QAbstractItemModel>
#include <QRegExp>
#include <QRunnable>
#include <QMutex>

#define QVARIANTMODEL_DEBUG


#ifdef QVARIANTMODEL_DEBUG
//#define QVM_DEBUG_MODEL_FUNC // index(), hasChildren(), rowCount()
//#define QVM_DEBUG_DATA // flags(), data()
//#define QVM_DEBUG_LOAD // canFetchMore(), loadNode()
//#define QVM_DEBUG_BUILD // buildNode()
//#define QVM_DEBUG_FILTER // isAcceptedNode()
//#define QVM_DEBUG_CACHE // cached(), recachedTree(), flags(), data()
//#define QVM_DEBUG_CHANGE_MODEL // begin/end{Reset,Insert,Remove,Move}() + filterTree() calls
#endif


class QVariantModelDataLoader;

class QVariantModel : public QAbstractItemModel
{
    Q_OBJECT
    Q_PROPERTY(QVariantList rootDatas READ rootDatas WRITE setRootDatas NOTIFY rootDatasChanged)
    Q_PROPERTY(uint displayDepth READ displayDepth WRITE setDisplayDepth NOTIFY displayDepthChanged)
    Q_PROPERTY(bool dynamicSort READ dynamicSort WRITE setDynamicSort NOTIFY dynamicSortChanged)
    Q_PROPERTY(QString filterText READ filterText WRITE setFilterText NOTIFY filterTextChanged)
    Q_PROPERTY(FilterType filterTypeColumn READ filterTypeColumn WRITE setFilterType NOTIFY filterTypeChanged)
    Q_PROPERTY(Columns filterColumns READ filterColumns WRITE setFilterColumns NOTIFY filterColumnsChanged)

    friend class QVariantModelDataLoader;

    enum {
        SizeLimitToLoadAsync = 200,
        NoChangedSignalsEmitted = 0
    };

    struct cache_row_t {
        Qt::ItemFlags flags = 0;
        QString text;
    };

    enum CacheRole {
        NoCache = 0,
        CacheText = 1,
        CacheFlags = 2,
        FullCache
    };
    Q_DECLARE_FLAGS(CacheRoles, CacheRole)

    struct node_t {
        QVariant value;

        node_t* parent = nullptr;
        QVariant keyInParent;
        int indexInParent = -1;

        QList<node_t*> children;
        QList<node_t*> visibleChildren;
        bool visible = false;

        struct {
            struct cache_row_t key;
            struct cache_row_t value;
            struct cache_row_t type;
        } cache;

        // lazing loading
        bool loaded = true;
        node_t* hintNode = nullptr; // dummy to show hint
        QVariantModelDataLoader* loader = nullptr;
    };

    enum InternalSortPolicy {
        ForceSort,
        DynamicSortPolicy,
        NoSort
    };

    enum InternalSortStrategy {
        SortNodeOnly,
        SortNodeAndChildren
    };

public:
    enum Column {
        KeyColumn = 1,
        ValueColumn = 2,
        TypeColumn = 4,
        ColumnCount = 3,
        IndexColumn = KeyColumn,
        AllColumns = KeyColumn | ValueColumn | TypeColumn
    };
    Q_DECLARE_FLAGS(Columns, Column)

    enum FilterType {
        Contains,
        WildCard,
        Regex,
        Fixed
    };

    static const char* FILTER_TYPE_NAMES[];

public:
    explicit QVariantModel(QObject* parent = nullptr);
    virtual ~QVariantModel();

    // model display
    QModelIndex index(int row, int column,
                      const QModelIndex& parent = QModelIndex()) const;
    QModelIndex parent(const QModelIndex& child) const;
    bool hasChildren(const QModelIndex& parent = QModelIndex()) const;

    int rowCount(const QModelIndex& parent = QModelIndex()) const;
    int columnCount(const QModelIndex& parent = QModelIndex()) const;

    Qt::ItemFlags flags(const QModelIndex& index) const;
    QVariant headerData(int section, Qt::Orientation orientation,
                        int role) const;

    // model datas
    QVariant data(const QModelIndex& index, int role) const;
    //    bool setData(const QModelIndex& index, const QVariant& value, int role);
    //    bool insertRows(int row, int count, const QModelIndex& parent = QModelIndex());
    //    bool removeRows(int row, int count, const QModelIndex &parent = QModelIndex());

    // model data loading
    void fetchMore(const QModelIndex &parent);
    bool canFetchMore(const QModelIndex &parent) const;

    // specific model properties
    QVariantList rootDatas() const;
    inline uint displayDepth() const { return m_depth; }
    inline bool dynamicSort() const { return m_dynamicSort; }
    inline FilterType filterTypeColumn() const { return m_filterType; }
    inline Columns filterColumns() const { return m_filterColumns; }
    inline QString filterText() const { return m_filterRx.pattern(); }

    int column(Column column) const;

signals:
    void rootDatasChanged();
    void displayDepthChanged(uint depth);
    void dynamicSortChanged(bool enabled);
    void filterTypeChanged(FilterType filterTypeColumn);
    void filterColumnsChanged(Columns filterColumns);
    void filterTextChanged(const QString& filterText);

public slots:
    void setRootDatas(const QVariantList& rootDatas);
    void setDisplayDepth(uint depth);
    void setDynamicSort(bool enabled);
    void setFilterType(FilterType filterTypeColumn);
    void setFilterColumns(Columns filterColumns);
    void setFilterText(const QString& filterText);

protected:
    virtual bool lessThan(const QVariant& left, const QVariant& right) const;
    virtual bool filterText(const QString& text) const;
    virtual bool filterData(const QVariant& value) const;
    virtual bool filterKeyColumn(const QString& cacheKey,
                           const QVariant& key,
                           bool nodeLoaded) const;
    virtual bool filterValueColumn(const QString& cacheValue,
                             const QVariant& value,
                             bool nodeLoaded) const;
    virtual bool filterTypeColumn(const QString& cacheType,
                            int type,
                            bool nodeLoaded) const;


#ifdef QVM_DEBUG_CHANGE_MODEL
    void beginResetModel();
    void beginInsertRows(const QModelIndex &parent, int first, int last);
    void beginRemoveRows(const QModelIndex &parent, int first, int last);
    bool beginMoveRows(const QModelIndex &sourceParent, int sourceFirst, int sourceLast,
                       const QModelIndex &destinationParent, int destinationRow);
    void endResetModel();
    void endInsertRows();
    void endRemoveRows();
    void endMoveRows();
#endif

    //    void invalidateSubTree(QModelIndex index);

private:
    QModelIndex createIndex(int row, int column, node_t* node) const;

    void loadNode(const QModelIndex &parent,
                  node_t* pnode,
                  bool canModifyModel = true);
    static void setLoadingHintNode(node_t *node, bool enable);
    static void invalidateOrder(node_t* node, int start = 0, int length = -1);
    static void cached(node_t* node,
                       int textDepth,
                       CacheRoles roles = FullCache); // build cache
    void recachedTree(node_t* pnode,
                      CacheRoles roles = FullCache,
                      bool canModifyModel = true); // rebuild cache text

    static void clearChildren(node_t* node, int reserveSize = 0);
    static QString keyPath(const node_t* node);

    QModelIndex indexForNode(node_t* node, int column) const;
    QModelIndex indexForNode(node_t* node, Column col = KeyColumn) const;

    void updateFilterRx(QString pattern);

    //    void sortTree(node_t& root, InternalSortStrategy sortStrategy);

    bool isFilterEnabled() const;
    bool isAcceptedNode(node_t* node) const;
    void filterTree(node_t* node, bool canModifyModel = true);

    void dumpTree(const node_t *root = nullptr,
                  const QString& prefix = QString()) const;
    void dumpTreeCache(const node_t *root = nullptr,
                       const QString& prefix = QString()) const;
    void dumpModel(const QModelIndex& rootIndex = QModelIndex(),
                   const QString& prefix = QString()) const;

private:
    QSharedPointer<node_t> mp_root;

    uint m_depth = 0;
    bool m_dynamicSort;
    FilterType m_filterType = Contains;
    Columns m_filterColumns = AllColumns;
    QRegExp m_filterRx;
};

Q_DECLARE_OPERATORS_FOR_FLAGS(QVariantModel::Columns)

//==============================================================================

class QVariantModelDataLoader : public QRunnable
{
    typedef QVariantModel Model;
    typedef Model::node_t node_t;

    enum {
        SprintBuildSize = 50
    };

public:
    QVariantModelDataLoader(node_t* node);

    void run();

    void buildNode();

    node_t* node;
    QMutex mutex; // mutex for `createdChildren` and `isDone`

    struct {
        QList<node_t*> createdChildren;
        bool isDone;
    } exclusive; // section supposed to be protected by mutex

    struct {
        uint displayDepth = 0;
    } to_cache;
};

#endif // QVARIANTMODEL_H
