#include "qvarianttree.h"

#include <QIODevice>
#include <QFile>
#include <QDataStream>


QVariantTree::QVariantTree()
{
    clear();
}

void QVariantTree::clear()
{
    _root.clear();
    _address.clear();
    _nodeType = QVariant::Invalid;
}

//------------------------------------------------------------------------------

void QVariantTree::setRootContent(QVariant rootContent)
{
    clear();
    _root = rootContent;
    _nodeType = _root.type();
}

//------------------------------------------------------------------------------

void QVariantTree::moveToNode(const QVariant& key)
{
    _address.append(QVariant(key));
    _nodeType = nodeValue().type();
}

void QVariantTree::moveToParent()
{
    if (!nodeIsRoot()) {
        _address.removeLast();
        _nodeType = nodeValue().type();
    }
}

void QVariantTree::moveToRoot()
{
    _address.clear();
    _nodeType = _root.type();
}

//------------------------------------------------------------------------------

bool QVariantTree::typeIsList(QVariant::Type type)
{
    return (type == QVariant::List);
}
bool QVariantTree::typeIsCollection(QVariant::Type type)
{
    return (type == QVariant::Map ||
            type == QVariant::Hash);
}

bool QVariantTree::nodeIsList() const
{
    return isValid() && typeIsList(_nodeType);
}

bool QVariantTree::nodeIsCollection() const
{
    return isValid() && typeIsCollection(_nodeType);
}

void QVariantTree::setNodeValue(QVariant value)
{
    _root = setTreeValue(_root, _address, value);
}

//------------------------------------------------------------------------------

int QVariantTree::itemListSize() const
{
    int result = -1;
    if (nodeIsList()) {
        QVariantList list = nodeValue().toList();
        result = list.size();
    }
    return result;
}

QVariant QVariantTree::getItemList(int index) const
{
    QVariant result;
    if (nodeIsList()) {
        QVariantList list = nodeValue().toList();
        if (index >= 0)
            result = list.value(index);
        else
            result = list.last();
    }
    return result;
}

void QVariantTree::setItemList(int index, QVariant value)
{
    if (nodeIsList()) {
        if (index < 0)
            index = itemListSize();

        // get & change
        QVariantList list = nodeValue().toList();
        list[index] = value;

        // save changes
        setNodeValue(list);
    }
}

void QVariantTree::insertItemList(int index, QVariant value)
{
    if (nodeIsList()) {
        // get
        QVariantList list = nodeValue().toList();

        // change
        if (index < 0)
            list.append(value);
        else
            list.insert(index, value);

        // save changes
        setNodeValue(list);
    }
}

void QVariantTree::delItemList(int index)
{
    if (nodeIsList()) {
        QVariantList listItemAddress = _address;
        listItemAddress << index;
        _root = delTreeValue(_root, listItemAddress);
    }
}

//------------------------------------------------------------------------------

QStringList QVariantTree::itemCollectionKeys() const
{
    QStringList result;
    if (nodeIsCollection()) {
        if (_nodeType == QVariant::Hash)
            result = nodeValue().toHash().keys();
        else if (_nodeType == QVariant::Map)
            result = nodeValue().toMap().keys();
    }
    return result;
}

QVariant QVariantTree::getItemCollection(QString key, QVariant defaultValue) const
{
    QVariant result;
    if (nodeIsCollection()) {
        if (_nodeType == QVariant::Hash) {
            QVariantHash hash = nodeValue().toHash();
            result = hash.value(key, defaultValue);
        }
        else if (_nodeType == QVariant::Map) {
            QVariantMap map = nodeValue().toMap();
            result = map.value(key, defaultValue);
        }
    }
    return result;
}

void QVariantTree::setItemCollection(QString key, QVariant value)
{
    if (nodeIsCollection()) {
        QVariantList collItemAddress = _address;
        collItemAddress << QVariant(key);
        _root = setTreeValue(_root, collItemAddress, value);
    }
}

void QVariantTree::delItemCollection(QString key)
{
    if (nodeIsCollection()) {
        QVariantList collItemAddress = _address;
        collItemAddress << QVariant(key);
        _root = delTreeValue(_root, collItemAddress);
    }
}

//------------------------------------------------------------------------------

QVariant QVariantTree::setTreeValue(QVariant& root,
                                    QVariantList address,
                                    QVariant& value)
{
    if (address.isEmpty()) {
        return value;
    }

    switch(root.type())
    {
    case QVariant::List:
    {
        QVariantList list = root.toList();
        int index = address.takeFirst().toInt();
        // ajout
        if (index < 0)
            list.append(value);
        else { // replace
            QVariant tmpValue = setTreeValue(list[index], address, value);
            list.replace(index, tmpValue);
        }
        root = list;
    }
        break;
    case QVariant::Map:
    {
        QVariantMap map = root.toMap();
        QString key = address.takeFirst().toString();
        // ajout
        if (!map.contains(key)) {
            map.insert(key, value);
        }
        else { // replace
            map[key] = setTreeValue(map[key], address, value);
        }
        root = map;
    }
        break;
    case QVariant::Hash:
    {
        QVariantHash hash = root.toHash();
        QString key = address.takeFirst().toString();
        // ajout
        if (!hash.contains(key)) {
            hash.insert(key, value);
        }
        else { // replace
            hash[key] = setTreeValue(hash[key], address, value);
        }
        root = hash;
    }
        break;
    default:
        root = value;
        break;
    }

    return root;
}

QVariant QVariantTree::getTreeValue(const QVariant &root,
                                    QVariantList address)
{
    QVariant result = root;

    if (!address.isEmpty()) {
        switch(root.type())
        {
        case QVariant::List:
        {
            QVariantList list = root.toList();
            QVariant itemList = list.value(address.takeFirst().toInt());
            result = getTreeValue(itemList, address);
            break;
        }
        case QVariant::Map:
        {
            QVariantMap map = root.toMap();
            QVariant itemMap = map[address.takeFirst().toString()];
            result = getTreeValue(itemMap, address);
            break;
        }
        case QVariant::Hash:
        {
            QVariantHash hash = root.toHash();
            QVariant itemHash = hash[address.takeFirst().toString()];
            result = getTreeValue(itemHash, address);
            break;
        }
        default:
            break;
        }
    }

    return result;
}

QVariant QVariantTree::delTreeValue(QVariant &root,
                                    QVariantList address)
{
    QVariant result = root;

    if (!address.isEmpty()) {
        switch(root.type())
        {
        case QVariant::List:
        {
            QVariantList list = root.toList();
            int index = address.takeFirst().toInt();
            if (address.isEmpty()) {
                if (index < 0)
                    list.removeLast();
                else
                    list.removeAt(index);
            }
            else
                list[index] = delTreeValue(list[index], address);
            result = list;
            break;
        }
        case QVariant::Map:
        {
            QVariantMap map = root.toMap();
            QString key = address.takeFirst().toString();
            if (address.isEmpty())
                map.remove(key);
            else
                map[key] = delTreeValue(map[key], address);
            result = map;
            break;
        }
        case QVariant::Hash:
        {
            QVariantHash hash = root.toHash();
            QString key = address.takeFirst().toString();
            if (address.isEmpty())
                hash.remove(key);
            else
                hash[key] = delTreeValue(hash[key], address);
            result = hash;
            break;
        }
        default:
            break;
        }
    }

    return result;
}

//------------------------------------------------------------------------------

QVariant QVariantTree::fromFile(QIODevice *file)
{
    QVariant result;
    QDataStream stream(file);

    if (!stream.atEnd()) {
        stream >> result;

        // si plus d'un seul element
        if (!stream.atEnd()) {
            QVariantList list;
            list << result;

            while (!stream.atEnd()) {
                stream >> result;
                list << result;
            }

            result = list;
        }
    }

    return result;
}

void QVariantTree::toFile(QIODevice *file, QVariant value)
{
    QDataStream stream(file);

    if (!value.isValid())
        return;
    else if (value.type() == QVariant::List) {
        QVariantList list = value.toList();
        QVariantList::const_iterator it = list.constBegin();
        for (; it != list.constEnd(); ++it)
            stream << (*it);
    }
    else
        stream << value;
}

QVariant QVariantTree::fromFile(QString filename)
{
    QVariant result;
    QFile file(filename);
    if (file.open(QFile::ReadOnly)) {
        result = QVariantTree::fromFile(&file);
        file.close();
    }
    return result;
}

void QVariantTree::toFile(QString filename, QVariant value)
{
    QFile file(filename);
    if (file.open(QFile::WriteOnly)) {
        QVariantTree::toFile(&file, value);
        file.flush();
        file.close();
    }
}
