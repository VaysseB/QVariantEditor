#include "tst_treegsd.h"

#include <QDebug>

QTEST_APPLESS_MAIN(TreeGSD)


TreeGSD::TreeGSD() :
    m_tree()
{
}

TreeGSD::~TreeGSD()
{
}


void TreeGSD::init()
{
    m_tree.clear();

    QVERIFY(m_tree.isValid() == false);
    QVERIFY(m_tree.rootContent().type() == QVariant::Invalid);
    QVERIFY(m_tree.nodeIsContainer() == false);
    QVERIFY(m_tree.address().isEmpty());
}


void TreeGSD::test01Types()
{
    QVERIFY(m_tree.typeIsContainer(QVariant::List));
    QVERIFY(m_tree.typeIsContainer(QVariant::Hash));
    QVERIFY(m_tree.typeIsContainer(QVariant::Map));
    QVERIFY(m_tree.typeIsContainer(QVariant::StringList));

    QVERIFY(m_tree.typeIsContainer(QVariant::Bool) == false);
    QVERIFY(m_tree.typeIsContainer(QVariant::Int) == false);
    QVERIFY(m_tree.typeIsContainer(QVariant::Double) == false);
    QVERIFY(m_tree.typeIsContainer(QVariant::String) == false);
    QVERIFY(m_tree.typeIsContainer(QVariant::Time) == false);
    QVERIFY(m_tree.typeIsContainer(QVariant::Date) == false);
    QVERIFY(m_tree.typeIsContainer(QVariant::DateTime) == false);
}


void TreeGSD::test02GetWithoutContainer()
{
    // default root must be invalid
    QVERIFY(m_tree.isValid() == false);
    QVERIFY(m_tree.rootContent().type() == QVariant::Invalid);
    QVERIFY(m_tree.nodeIsContainer() == false);

    // cheking bool
    check01GetWithoutContainer<bool>(QVariant(false));
    check01GetWithoutContainer<int>(QVariant(5));
    check01GetWithoutContainer<double>(QVariant(-7.397401204));
    check01GetWithoutContainer<QString>(QVariant(QLatin1String("test")));
}


void TreeGSD::test03GetContainerList()
{
    QVariantList subList;
    subList.append(QVariant(QLatin1String("another test")));
    QVariantList list;
    list.append(QVariant(true));
    list.append(QVariant(12.35));
    list.append(QVariant(subList));
    m_tree.setRootContent(list);

    QVERIFY(m_tree.isValid());
    QVERIFY(m_tree.rootContent().type() == QVariant::List);
    QVERIFY(m_tree.nodeType() == QVariant::List);
    QVERIFY(m_tree.nodeIsContainer());
    QVERIFY(m_tree.address().isEmpty());

    // check valid
    bool isValid = false;
    QVERIFY(m_tree.getTreeValue(m_tree.rootContent(), QVariantList() << QVariant(0), &isValid) == QVariant(true));
    QVERIFY(isValid);
    QVERIFY(m_tree.getTreeValue(m_tree.rootContent(), QVariantList() << QVariant(1), &isValid) == QVariant(12.35));
    QVERIFY(isValid);
    QVERIFY(m_tree.getTreeValue(m_tree.rootContent(), QVariantList() << QVariant(2) << QVariant(0), &isValid) == QVariant(QLatin1String("another test")));
    QVERIFY(isValid);
    QVERIFY(m_tree.getTreeValue(m_tree.rootContent(), QVariantList(), &isValid) == list);
    QVERIFY(isValid == true);
    QVERIFY(m_tree.getTreeValue(m_tree.rootContent(), QVariantList() << QVariant(2), &isValid) == subList);
    QVERIFY(isValid == true);

    // check invalid
    QVERIFY(m_tree.getTreeValue(m_tree.rootContent(), QVariantList() << QVariant(3), &isValid).isValid() == false);
    QVERIFY(isValid == false);
    QVERIFY(m_tree.getTreeValue(m_tree.rootContent(), QVariantList() << QVariant(-1), &isValid).isValid() == false);
    QVERIFY(isValid == false);
    QVERIFY(m_tree.getTreeValue(m_tree.rootContent(), QVariantList() << QVariant(2) << QVariant(5), &isValid).isValid() == false);
    QVERIFY(isValid == false);
}

