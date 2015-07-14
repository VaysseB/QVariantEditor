#include "qvarianttreeelementcontainer.h"


QVariantList QVariantTreeElementContainer::fromSize(const int size)
{
    QVariantList listKeys;
    listKeys.reserve(size);
    for (int i=0; i<size; i++)
        listKeys.append(i);
    return listKeys;
}

QVariantList QVariantTreeElementContainer::fromStringList(const QStringList& list)
{
    QVariantList listKeys;
    listKeys.reserve(list.count());
    for (int i=0; i<list.count(); i++)
        listKeys.append(QVariant(list.value(i)));
    return listKeys;
}

//==============================================================================

QVariantList QVariantTreeListContainer::keys(const QVariant& content) const
{
    return fromSize(content.toList().count());
}

QVariant QVariantTreeListContainer::item(const QVariant& content, const QVariant& key, const QVariant& defaultValue) const
{
    return content.toList().value(key.toInt(), defaultValue);
}

QVariant QVariantTreeListContainer::setItem(const QVariant& content, const QVariant& key, const QVariant& value) const
{
    QVariantList listContent = content.toList();
    int index = key.toInt();
    if (index >= 0 && index < listContent.count())
        listContent[index] = value;
    return listContent;
}

QVariant QVariantTreeListContainer::delItem(const QVariant& content, const QVariant& key) const
{
    QVariantList listContent = content.toList();
    int index = key.toInt();
    if (index >= 0 && index < listContent.count())
        listContent.removeAt(index);
    return listContent;
}

//==============================================================================

QVariantList QVariantTreeMapContainer::keys(const QVariant& content) const
{
    return fromStringList(content.toMap().keys());
}

QVariant QVariantTreeMapContainer::item(const QVariant& content, const QVariant& key, const QVariant& defaultValue) const
{
    return content.toMap().value(key.toString(), defaultValue);
}

QVariant QVariantTreeMapContainer::setItem(const QVariant& content, const QVariant& key, const QVariant& value) const
{
    QVariantMap mapContent = content.toMap();
    mapContent[key.toString()] = value;
    return mapContent;
}

QVariant QVariantTreeMapContainer::delItem(const QVariant& content, const QVariant& key) const
{
    QVariantMap mapContent = content.toMap();
    mapContent.remove(key.toString());
    return mapContent;
}

//==============================================================================

QVariantList QVariantTreeHashContainer::keys(const QVariant& content) const
{
    return fromStringList(content.toHash().keys());
}

QVariant QVariantTreeHashContainer::item(const QVariant& content, const QVariant& key, const QVariant& defaultValue) const
{
    return content.toHash().value(key.toString(), defaultValue);
}

QVariant QVariantTreeHashContainer::setItem(const QVariant& content, const QVariant& key, const QVariant& value) const
{
    QVariantHash hashContent = content.toHash();
    hashContent[key.toString()] = value;
    return hashContent;
}

QVariant QVariantTreeHashContainer::delItem(const QVariant& content, const QVariant& key) const
{
    QVariantHash hashContent = content.toHash();
    hashContent.remove(key.toString());
    return hashContent;
}
