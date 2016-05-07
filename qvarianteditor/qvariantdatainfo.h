#ifndef QVARIANTDATAINFO_H
#define QVARIANTDATAINFO_H

#include <QVariant>
#include <QCoreApplication>


class QVariantDataInfo
{
public:
    explicit QVariantDataInfo(const QVariant& data);

    bool isValid() const;

    bool isAtomic() const;
    bool isContainer() const;

    QList<QVariant> containerKeys() const;
    QVariant containerValue(const QVariant& key) const;

    QString displayText(int depth = 3) const;

protected:
    const QVariant& m_cdata;
};

class QMutableVariantDataInfo : public QVariantDataInfo
{
    Q_DECLARE_TR_FUNCTIONS(QMutableVariantDataInfo)

public:
    explicit QMutableVariantDataInfo(QVariant& data);
    explicit QMutableVariantDataInfo(const QVariant& data);

    bool editableKeys() const;
    bool editableValues() const;

    bool isNewKeyInsertable() const;
    /**
     * @brief Insert a new key in the container, if possible before the given
     * key. If the given key is invalid, the value should be append.
     * @param beforeKey the key to insert before.
     * @param value the value to insert
     * @return The created key
     */
    void tryInsertNewKey(const QVariant& beforeKey, const QVariant& value);

    void setContainerKey(const QVariant& oldKey, const QVariant& newKey);
    void setContainerValue(const QVariant& key, const QVariant& value);

    inline bool isConst() const { return &m_mutdata == &m_dummy; }

protected:
    const QVariant& constData() const;

protected:
    QVariant& m_mutdata;
    QVariant m_dummy;
};


//==============================================================================

namespace QVariantDataInfoPrivate {

template<typename T>
struct index_collection_t {
    const T& c;

    index_collection_t(const T& collection) : c(collection) {}

    QList<QVariant> keys() const
    {
        QList<QVariant> l;
        for (int i = 0; i < c.count(); i++)
            l.append(QVariant(i));
        std::sort(l.begin(), l.end());
        return l;
    }

    QString displayText(int depth) const
    {
        QStringList items;
        for (auto it = c.constBegin(); it != c.constEnd(); ++it)
            items.append(QVariantDataInfo(*it).displayText(depth));
        return QString("[%1]").arg(items.join(","));
    }

};

template<typename T>
struct associative_collection_t {
    const T& c;

    associative_collection_t(const T& collection) : c(collection) {}

    QList<QVariant> keys() const
    {
        QList<QVariant> l;
        for (auto it = c.constBegin(); it != c.constEnd(); ++it)
            l.append(QVariant::fromValue(it.key()));
        std::sort(l.begin(), l.end());
        return l;
    }

    QString displayText(int depth) const
    {
        QStringList items;
        for (auto it = c.constBegin(); it != c.constEnd(); ++it) {
            const QString& skey = QVariantDataInfo(it.key()).displayText(depth);
            const QString& svalue = QVariantDataInfo(it.value()).displayText(depth);
            items.append(QString("%1:%2").arg(skey).arg(svalue));
        }
        return QString("{%1}").arg(items.join(", "));
    }

};

template<typename T>
struct mutable_index_collection_t {
    T& c;

    mutable_index_collection_t(T& collection) : c(collection) {}

    void insertNewKey(int beforeKey,
                      const QVariant& value)
    {
        int key;
        // if insert pos is not known or collection is empty
        if (beforeKey < 0) {
            if (c.isEmpty())
                key = 0;
            else
                key = c.count();
        }
        else
            key = beforeKey;

        c.insert(key, value);
    }

};

template<typename T>
struct mutable_associative_collection_t {
    T& c;

    mutable_associative_collection_t(T& collection) : c(collection) {}

    void insertNewKey(const QString& beforeKey,
                      const QVariant& value)
    {
        QString key;
        // if insert pos is not known or collection is empty
        if (beforeKey.isEmpty()) {
            if (c.isEmpty())
                key = QMutableVariantDataInfo::tr("key");
            else
                key = c.keys().last();
        }
        else
            key = beforeKey;

        // tries to look for an not existing name
        int tries = 1;
        QString newName = key + "1";
        while (c.contains(newName))
            newName = key + QString::number(++tries);
        key = newName;

        c.insert(key, value);
    }

};


template<typename T>
inline index_collection_t<T> IndexCollection(const T& collection) {
    return index_collection_t<T>(collection);
}

template<typename T>
inline associative_collection_t<T> AssociativeCollection(const T& collection) {
    return associative_collection_t<T>(collection);
}

template<typename T>
inline mutable_index_collection_t<T> MutableIndexCollection(T& collection) {
    return mutable_index_collection_t<T>(collection);
}

template<typename T>
inline mutable_associative_collection_t<T> MutableAssociativeCollection(T& collection) {
    return mutable_associative_collection_t<T>(collection);
}

} // namespace QVariantDataInfoPrivate


#endif // QVARIANTDATAINFO_H
