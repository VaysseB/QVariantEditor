#ifndef QVARIANTDATAINFO_H
#define QVARIANTDATAINFO_H

#include <QVariant>


class QVariantDataInfo
{
public:
    QVariantDataInfo(const QVariant& data);

    bool isValid() const;

    bool isAtomic() const;
    bool isCollection() const;

    QList<QVariant> collectionKeys() const;
    QVariant collectionValue(const QVariant& key) const;

    QString displayText(int depth = 3) const;

private:
    const QVariant& m_cdata;
};

class QMutableVariantDataInfo : public QVariantDataInfo
{
public:
    QMutableVariantDataInfo(QVariant& data);

    void setCollectionValue(const QVariant& key, const QVariant& value);
    QVariant insertBeforeNewCollectionItem(const QVariant& afterKey);
    QVariant addNewCollectionItem();
    void removeCollectionValue(const QVariant& key);

private:
    QVariant& m_mutdata;
};


//==============================================================================

namespace qtprivate {

template<typename T>
struct index_collection_t {
    const T& c;

    index_collection_t(const T& collection) : c(collection) {}

    QList<QVariant> keys() const
    {
        QList<QVariant> l;
        for (int i = 0; i < c.count(); i++)
            l.append(QVariant(i));
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
inline index_collection_t<T> IndexCollection(const T& collection) {
    return index_collection_t<T>(collection);
}

template<typename T>
inline associative_collection_t<T> AssociativeCollection(const T& collection) {
    return associative_collection_t<T>(collection);
}

} // namespace qtprivate


#endif // QVARIANTDATAINFO_H
