#ifndef QVARIANTTREE_H
#define QVARIANTTREE_H

#include <QVariant>

#include "qvarianttreeelement.h"


class QVariantTree
{
public:
    explicit QVariantTree();
    virtual ~QVariantTree();

    void clear();

    void setRootContent(QVariant rootContent);
    QVariant rootContent() const { return _root; }

    QVariantList address() const { return _address; }

    void moveToNode(const QVariant &key);
    void moveToParent();

    bool nodeIsRoot() const { return _address.isEmpty(); }
    void moveToRoot();

    uint nodeType() const { return _nodeType; }
    QVariant nodeValue() const { return getTreeValue(_root, _address); }
    void setNodeValue(QVariant value);

    bool typeIsContainer(uint type) const;

    bool nodeIsContainer() const;
    QVariantList itemContainerKeys() const;
    QVariant getItemContainer(const QVariant& key, const QVariant& defaultValue = QVariant()) const;
    void setItemContainer(const QVariant& key, const QVariant& value);
    void delItemContainer(const QVariant& key);

    bool isValid() const { return _root.isValid(); }

    static void toFile(QString filename, QVariant value);
    static void toFile(QIODevice *file, QVariant value);
    void toFile(QString filename) const { QVariantTree::toFile(filename, rootContent()); }
    void toFile(QIODevice *file) const { QVariantTree::toFile(file, rootContent()); }

    static QVariant fromFile(QString filename);
    static QVariant fromFile(QIODevice *file);
    void setFromFile(QString filename) { setRootContent(QVariantTree::fromFile(filename)); }
    void setFromFile(QIODevice *file) { setRootContent(QVariantTree::fromFile(file)); }

    QVariantTreeElementContainer* setContainer(
            uint type,
            QVariantTreeElementContainer* container);

    QVariant getTreeValue(const QVariant& root,
                          const QVariantList& address,
                          bool* isValid = 0) const;
    void setTreeValue(const QVariant& root,
                      const QVariantList& address,
                      const QVariant& value,
                      bool* isValid = 0);
    void delTreeValue(const QVariant& root,
                      const QVariantList& address,
                      bool* isValid = 0);

private:
    QVariantTreeElementContainer* containerOf(uint type) const;

    QVariant internalSetTreeValue(const QVariant& root,
                                  const QVariantList& address,
                                  const QVariant& value,
                                  bool* isValid = 0) const;
    QVariant internalDelTreeValue(const QVariant& root,
                                  const QVariantList& address,
                                  bool* isValid = 0) const;

private:
    QVariant _root;
    QVariantList _address;
    uint _nodeType;

    QMap<uint, QVariantTreeElementContainer*> m_containers;
};

#endif // QVARIANTTREE_H
