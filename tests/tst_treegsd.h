#include <QObject>
#include <QtTest>

#include "qvarianttree.h"


template <typename T>
bool qvariant_equals(QVariant va, QVariant vb)
{
    return va.type() == vb.type() &&
            va.value<T>() == vb.value<T>();
}



class TreeGSD : public QObject
{
    Q_OBJECT
public:
    TreeGSD();
    ~TreeGSD();

private Q_SLOTS:
    void init();

    void test01Types();
    void test02GetWithoutContainer();
    void test03GetContainerList();

private:
    template <typename T>
    void check01GetWithoutContainer(const QVariant& value)
    {
        m_tree.setRootContent(value);
        QVERIFY(m_tree.isValid());
        QVERIFY(qvariant_equals<T>(m_tree.rootContent(), value));
        QVERIFY(m_tree.nodeIsContainer() == false);
    }

private:
    QVariantTree m_tree;
};
