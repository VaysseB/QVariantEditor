#ifndef QVARIANTTREEELEMENT_H
#define QVARIANTTREEELEMENT_H

#include <QVariant>


class QVariantTreeElementContainer
{
public:
    virtual QVariantList keys(const QVariant& content) const = 0;
    virtual QVariant item(const QVariant& content, const QVariant& key, const QVariant& defaultValue = QVariant()) const = 0;
    virtual QVariant setItem(const QVariant& content, const QVariant& key, const QVariant& value) const = 0;
    virtual QVariant delItem(const QVariant& content, const QVariant& key) const = 0;

protected:
    static QVariantList fromSize(const int size);
    static QVariantList fromStringList(const QStringList& list);
};

//==============================================================================


#define QVARIANTTREEELEMENTCONTAINER_IMPL(TYPE) \
class QVariantTree ## TYPE ## Container : public QVariantTreeElementContainer \
{ \
    QVariantList keys(const QVariant& content) const; \
    QVariant item(const QVariant& content, const QVariant& key, const QVariant& defaultValue = QVariant()) const; \
    QVariant setItem(const QVariant& content, const QVariant& key, const QVariant& value) const; \
    QVariant delItem(const QVariant& content, const QVariant& key) const; \
};

QVARIANTTREEELEMENTCONTAINER_IMPL(List)
QVARIANTTREEELEMENTCONTAINER_IMPL(Map)
QVARIANTTREEELEMENTCONTAINER_IMPL(Hash)

#endif // QVARIANTTREEELEMENT_H
