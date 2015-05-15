#ifndef QVARIANTTREE_H
#define QVARIANTTREE_H

#include <QDebug>
#include <QVariant>
#include <QStringList>


class QVariantTree
{
public:
    QVariantTree();

    void clear();

    void setRootContent(QVariant rootContent);
    QVariant rootContent() const { return _root; }

    QVariantList address() const { return _address; }

    void moveToNode(const QVariant &key);
    void moveToParent();

    bool nodeIsRoot() const { return _address.isEmpty(); }
    void moveToRoot();

    QVariant::Type nodeType() const { return _nodeType; }
    QVariant nodeValue() const { return getTreeValue(_root, _address); }
    void setNodeValue(QVariant value);

    static bool typeIsList(QVariant::Type type);
    static bool typeIsCollection(QVariant::Type type);

    bool nodeIsList() const;
    int  itemListSize() const;
    QVariant getItemList(int index /* -1 => last */) const;
    void setItemList(int index /* -1 => last */, QVariant value);
    void insertItemList(int index /* -1 => append */, QVariant value);
    void delItemList(int index /* -1 => last */);

    bool nodeIsCollection() const;
    QStringList itemCollectionKeys() const;
    QVariant getItemCollection(QString key, QVariant defaultValue = QVariant()) const;
    void setItemCollection(QString key, QVariant value);
    void delItemCollection(QString key);

    bool isValid() const { return _root.isValid(); }

    static void toFile(QString filename, QVariant value);
    static void toFile(QIODevice *file, QVariant value);
    void toFile(QString filename) const { QVariantTree::toFile(filename, rootContent()); }
    void toFile(QIODevice *file) const { QVariantTree::toFile(file, rootContent()); }

    static QVariant fromFile(QString filename);
    static QVariant fromFile(QIODevice *file);
    void setFromFile(QString filename) { setRootContent(QVariantTree::fromFile(filename)); }
    void setFromFile(QIODevice *file) { setRootContent(QVariantTree::fromFile(file)); }

private:
    static QVariant getTreeValue(const QVariant &root,
                                 QVariantList address);

    static QVariant setTreeValue(QVariant &root,
                                 QVariantList address,
                                 QVariant& value);

    static QVariant delTreeValue(QVariant &root,
                                 QVariantList address);

private:
    QVariant _root;
    QVariantList _address;
    QVariant::Type _nodeType;
};

#endif // QVARIANTTREE_H
