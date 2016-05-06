#ifndef QVARIANTDATAINFO_H
#define QVARIANTDATAINFO_H

#include <QVariant>


class QVariantDataInfo
{
public:
    QVariantDataInfo(const QVariant& data);

    bool isValid() const;

    bool isAtomic() const;
    bool isContainer() const;

    QList<QVariant> containerKeys() const;
    QVariant containerValue(const QVariant& key) const;

    QString displayText(int depth = 3) const;

private:
    const QVariant& m_cdata;
};

class QMutableVariantDataInfo : public QVariantDataInfo
{
public:
    enum EditFlag {
        NoEdit = 0,
        KeysAreEditable = (1 << 1),
        ValuesAreEditable = (1 << 2)
    };
    Q_DECLARE_FLAGS(EditFlags, EditFlag)

public:
    QMutableVariantDataInfo(QVariant& data);

    EditFlags flags() const;

    void setContainerKey(const QVariant& oldKey, const QVariant& newKey);
    void setContainerValue(const QVariant& key, const QVariant& value);

private:
    QVariant& m_mutdata;
};

Q_DECLARE_OPERATORS_FOR_FLAGS(QMutableVariantDataInfo::EditFlags)


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
inline index_collection_t<T> IndexCollection(const T& collection) {
    return index_collection_t<T>(collection);
}

template<typename T>
inline associative_collection_t<T> AssociativeCollection(const T& collection) {
    return associative_collection_t<T>(collection);
}

} // namespace qtprivate


#endif // QVARIANTDATAINFO_H
