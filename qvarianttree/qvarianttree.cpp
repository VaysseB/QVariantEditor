#include "qvarianttree.h"

#include <QIODevice>
#include <QFile>
#include <QDataStream>


QVariantTree::QVariantTree() :
    m_containers()
{
    setContainer(QVariant::List, new QVariantTreeListContainer);
    setContainer(QVariant::StringList, new QVariantTreeListContainer);
    setContainer(QVariant::Hash, new QVariantTreeHashContainer);
    setContainer(QVariant::Map, new QVariantTreeMapContainer);

    clear();
}

QVariantTree::~QVariantTree()
{
    qDeleteAll(m_containers);
    m_containers.clear();
}


void QVariantTree::clear()
{
    _root.clear();
    _address.clear();
    _nodeType = QVariant::Invalid;
}

QVariantTreeElementContainer* QVariantTree::setContainer(
        uint type,
        QVariantTreeElementContainer* container)
{
    QVariantTreeElementContainer* oldContainer = NULL;
    if (m_containers.contains(type))
        oldContainer = m_containers.take(type);
    m_containers.insert(type, container);
    return oldContainer;
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

bool QVariantTree::typeIsContainer(uint type) const
{
    return m_containers.keys().contains(type);
}

bool QVariantTree::nodeIsContainer() const
{
    return m_containers.keys().contains(nodeType());
}

QVariantTreeElementContainer* QVariantTree::containerOf(uint type) const
{
    return m_containers.value(type, 0);
}

//------------------------------------------------------------------------------

QVariantList QVariantTree::itemContainerKeys() const
{
    Q_ASSERT(nodeIsContainer());
    QVariantTreeElementContainer* containerType = containerOf(nodeType());
    Q_ASSERT_X(containerType != 0, "QVariantTree", "cannot find container of type");
    return containerType->keys(nodeValue());
}

QVariant QVariantTree::getItemContainer(const QVariant& key,
                                        const QVariant& defaultValue) const
{
    Q_ASSERT(nodeIsContainer());
    QVariantTreeElementContainer* containerType = containerOf(nodeType());
    Q_ASSERT_X(containerType != 0, "QVariantTree", "cannot find container of type");
    return containerType->item(nodeValue(), key, defaultValue);
}

void QVariantTree::setItemContainer(const QVariant& key, const QVariant& value)
{
    Q_ASSERT(nodeIsContainer());
    QVariantTreeElementContainer* containerType = containerOf(nodeType());
    Q_ASSERT_X(containerType != 0, "QVariantTree", "cannot find container of type");

    QVariantList collItemAddress = _address;
    collItemAddress << QVariant(key);
    _root = setTreeValue(_root, collItemAddress, value);
}

void QVariantTree::delItemContainer(const QVariant& key)
{
    Q_ASSERT(nodeIsContainer());
    QVariantTreeElementContainer* containerType = containerOf(nodeType());
    Q_ASSERT_X(containerType != 0, "QVariantTree", "cannot find container of type");

    QVariantList collItemAddress = _address;
    collItemAddress << QVariant(key);
    _root = delTreeValue(_root, collItemAddress);
}

//------------------------------------------------------------------------------

QVariant QVariantTree::getTreeValue(const QVariant& root,
                                    const QVariantList& address,
                                    bool* isValid) const
{
    QVariant result = root;
    int indexAddress = 0;
    bool trueValid = true;

    while (trueValid && indexAddress < address.count()) {
        if (typeIsContainer(result.type())) {
            QVariantTreeElementContainer* containerType = containerOf(result.type());
            if (containerType)
                result = containerType->item(result, address.value(indexAddress++));
            else
                trueValid &= false;
        }
        else {
            trueValid &= (indexAddress >= address.count());
            break;
        }
    }

    if (isValid)
        *isValid = trueValid;

    return result;
}

QVariant QVariantTree::setTreeValue(const QVariant& root,
                                    const QVariantList& address,
                                    const QVariant& value,
                                    bool* isValid) const
{
    QVariant result = root;
    bool trueValid = true;

    if (address.isEmpty())
        result = value;
    else {
        QVariantTreeElementContainer* containerType = containerOf(result.type());
        if (containerType == 0)
            trueValid = false;
        else {
            result = containerType->item(result, address.first());
            result = setTreeValue(result, address.mid(1), value);
            result = containerType->setItem(root, address.first(), result);
        }
    }

    if (isValid)
        *isValid = trueValid;

    return result;
}

QVariant QVariantTree::delTreeValue(const QVariant& root,
                                    const QVariantList& address,
                                    bool* isValid) const
{
    QVariant result = root;
    bool trueValid = true;

    if (address.isEmpty())
        trueValid = false;
    else if (address.count() == 1) {
        QVariantTreeElementContainer* containerType = containerOf(result.type());
        if (containerType == NULL)
            trueValid = false;
        else
            result = containerType->delItem(result, address.first());
    }
    else {
        QVariantTreeElementContainer* containerType = containerOf(result.type());
        if (containerType == 0)
            trueValid = false;
        else {
            result = containerType->item(result, address.first());
            result = delTreeValue(result, address.mid(1));
            result = containerType->setItem(root, address.first(), result);
        }
    }

    if (isValid)
        *isValid = trueValid;

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
    if (file.open(QIODevice::WriteOnly |
                  QIODevice::Truncate)) {
        QVariantTree::toFile(&file, value);
        file.flush();
        file.close();
    }
}
