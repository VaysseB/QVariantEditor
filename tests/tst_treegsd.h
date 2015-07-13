#include <QObject>
#include <QtTest>

#include "qvarianttree.h"


class TreeGSD : public QObject
{
    Q_OBJECT
public:
    TreeGSD();
    ~TreeGSD();

private Q_SLOTS:
    void testTypes();

private:
    QVariantTree m_tree;
};
