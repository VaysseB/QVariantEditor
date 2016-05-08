#ifndef QVARIANTDATAINFO_H
#define QVARIANTDATAINFO_H

#include <QVariant>
#include <QCoreApplication>


/**
 * @brief The QVariantDataInfo class help to get info from QVariant value.
 *
 * This primary used is to be a generic interface to query the QVariant value.
 * This class can only extract informations from the given data.
 *
 * As it keeps a const reference to the data (does not copy it), the user must
 * not destroy the data while this class is in use.
 */
class QVariantDataInfo
{
public:
    /**
     * @brief Build the QVariantDataInfo around the given data.
     * @param data data to manipulate
     */
    explicit QVariantDataInfo(const QVariant& data);

    /**
     * @brief Test that the QVariantDataInfo know the QVariant data type.
     * @return true if the data type is handled
     */
    bool isValid() const;


    /**
     * @brief Test if the data is atomic.
     * For this class, a atomic data means that the data cannot grow/shrink in
     * size. So, appart from list/map/hash and such, the data must be
     * considered atomic.
     * This implies `isValid()`.
     * @return true if data is atomic
     * @see isContainer
     */
    bool isAtomic() const;
    /**
     * @brief Test if the data is a container.
     * For this class, a container can be a collections (list/map/hash), or a
     * structure/class with properties.
     * This implies `isValid()`.
     * @return true if data is a container.
     * @see isAtomic
     */
    bool isContainer() const;


    /**
     * @brief Extract the keys of the container.
     * The data must be a container.
     * @return List of keys
     * @see containerValue
     */
    QList<QVariant> containerKeys() const;
    /**
     * @brief Extract the data of the key from the container.
     * The data must be a container.
     * @return data for the key
     * @see containerKeys
     */
    QVariant containerValue(const QVariant& key) const;


    /**
     * @brief Create a string representation of the data.
     * If the data is a container, the parameter depth is used to continue
     * the stringification deeper, stopping when depth equals 0.
     * @param depth depth of representation
     * @return string representation of the data
     */
    QString displayText(int depth = 3) const;

protected:
    /**
     * @brief Read-only data.
     */
    const QVariant& m_cdata;
};


/**
 * @brief The QMutableVariantDataInfo class extends QVariantDataInfo with data
 * manipulation.
 *
 * This primary used is to be a generic interface to manipulate the QVariant
 * value.
 * This class has two ways : the first is to help to know if the inner data
 * can be edited (via it's const methods like `editableValues()`), and the
 * second way is to really edit the inner data.
 *
 * If the data is given as const, the data cannot be edited and only queried.
 *
 * As it keeps a reference to the data (does not copy it), the user must
 * not destroy the data while this class is in use.
 *
 * When subclass, use `m_cdata` for const methods, and `m_mutdata` for
 * non-const methods. Always assert in non-const methods if `isConst()` is true.
 */
class QMutableVariantDataInfo : public QVariantDataInfo
{
    Q_DECLARE_TR_FUNCTIONS(QMutableVariantDataInfo)

public:
    /**
     * @brief Build a editable data QMutableVariantDataInfo instance.
     * @param data data to edit or query
     * @see isConst
     */
    explicit QMutableVariantDataInfo(QVariant& data);
    /**
     * @brief Build a const data QMutableVariantDataInfo instance.
     * @param data data to query only
     * @see isConst
     */
    explicit QMutableVariantDataInfo(const QVariant& data);


    /**
     * @brief Test if the keys of the data can be edited.
     * The data must be a container.
     * @return True if keys are editable (aka can be changed)
     * @see editableValues
     * @see setContainerKey
     */
    bool editableKeys() const;
    /**
     * @brief Test if the values of the data can be edited.
     * The data must be a container.
     * @return True if values are editable
     * @see editableKeys
     * @see setContainerValue
     */
    bool editableValues() const;


    /**
     * @brief Replace the old key by the new key.
     * The data must be a container and keys must be editable.
     * The old key must exists.
     * @param oldKey the old key
     * @param newKey the new key
     * @see editableKeys
     * @see setContainerValue
     */
    void setContainerKey(const QVariant& oldKey, const QVariant& newKey);
    /**
     * @brief Replace the old value at the key by the new value.
     * The data must be a container and values must be editable.
     * The key must exists.
     * @param key the key of the value
     * @param value the value to set
     * @see editableValues
     * @see setContainerKey
     */
    void setContainerValue(const QVariant& key, const QVariant& value);


    /**
     * @brief Test if a new key can be created and inserted.
     * The data must be a container.
     * @return True if new key is creatable and insertable/appenabled.
     * @see tryInsertNewKey
     */
    bool isNewKeyInsertable() const;
    /**
     * @brief Create and insert a new key in the container, if possible before
     * the given key. If the before key is invalid, the data should be append.
     * The data must be a container and new keys must be insertable.
     * Be warn that keys order might not be kept after new key is inserted.
     * @param beforeKey the key to insert before
     * @param value the data to insert
     * @see isNewKeyInsertable
     */
    void tryInsertNewKey(const QVariant& beforeKey, const QVariant& value);


    /**
     * @brief Test if a key can be removed.
     * The data must be a container.
     * @return True if keys are removable
     * @see removeKey
     */
    bool isKeyRemovable() const;
    /**
     * @brief Remove a key from the container
     * The data must be a container, keys must be removable, and the key must
     * exists in the container.
     * Be warn that keys order might not be kept after key is removed.
     * @param key the key to remove
     * @see isKeyRemovable
     */
    void removeKey(const QVariant& key);


    /**
     * @brief Test if the QMutableVariantDataInfo is const.
     * This is true if it was build with const data.
     * @return False if data can be changed
     */
    inline bool isConst() const { return &m_mutdata == &m_dummy; }

protected:
    /**
     * @brief Mutable data.
     * This will reference the constructor data if not const, or `m_dummy` if
     * was build with const data.
     */
    QVariant& m_mutdata;
    /**
     * @brief Dummy data for `m_mutdata` when build as const.
     */
    QVariant m_dummy;
};


//==============================================================================

namespace QtPrivate {
namespace VariantDataInfo {

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

    void removeKey(int key)
    {
        c.removeAt(key);
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

    void removeKey(const QString& key)
    {
        c.remove(key);
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

} // namespace VariantDataInfo
} // namespace QtPrivate


#endif // QVARIANTDATAINFO_H
