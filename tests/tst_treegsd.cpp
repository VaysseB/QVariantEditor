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


void TreeGSD::test04SetWithoutContainer()
{
    m_tree.setRootContent(QVariant(0x51201382));
    QVERIFY(qvariant_equals<typeof(0x51201382)>(m_tree.rootContent(), QVariant(0x51201382)));
    QVERIFY(qvariant_equals<typeof(0x51201382)>(m_tree.nodeValue(), QVariant(0x51201382)));

    m_tree.clear();
    m_tree.setNodeValue(QVariant(0x51201382));
    QVERIFY(qvariant_equals<typeof(0x51201382)>(m_tree.rootContent(), QVariant(0x51201382)));
    QVERIFY(qvariant_equals<typeof(0x51201382)>(m_tree.nodeValue(), QVariant(0x51201382)));
}


void TreeGSD::test05SetContainerList()
{
    QVariantList list;
    list.append(QVariant(QLatin1String("first")));
    list.append(QVariant(QLatin1String("second")));
    list.append(QVariant((qlonglong)3l));
    list.append(QVariant(list));
    m_tree.setRootContent(list);

    QVERIFY(m_tree.isValid());
    QVERIFY(m_tree.rootContent().type() == QVariant::List);
    QVERIFY(m_tree.nodeType() == QVariant::List);
    QVERIFY(m_tree.nodeIsContainer());
    QVERIFY(m_tree.address().isEmpty());

    bool isValid = false;
    QVariantList addr;
    addr << QVariant(2);
    QVERIFY(m_tree.getTreeValue(m_tree.rootContent(), addr, &isValid) == QVariant((qlonglong)3l));
    QVERIFY(isValid);
    m_tree.setTreeValue(m_tree.rootContent(), addr, QVariant(QLatin1String("tree")), &isValid);
    QVERIFY(isValid);
    QVERIFY(m_tree.getTreeValue(m_tree.rootContent(), addr, &isValid) == QVariant(QLatin1String("tree")));
    QVERIFY(isValid);

    isValid = false;
    addr.clear();
    addr << QVariant(3) << QVariant(0);
    QVERIFY(m_tree.getTreeValue(m_tree.rootContent(), addr, &isValid) == QVariant(QLatin1String("first")));
    QVERIFY(isValid);
    m_tree.setTreeValue(m_tree.rootContent(), addr, QVariant(-9), &isValid);
    QVERIFY(isValid);
    QVERIFY(m_tree.getTreeValue(m_tree.rootContent(), addr, &isValid) == QVariant(-9));
    QVERIFY(isValid);
}


void TreeGSD::test06DelWithoutContainer()
{
    bool isValid = false;

    m_tree.setRootContent(QVariant("\u1237\u4001")); // rubbish
    QVERIFY(m_tree.isValid());
    m_tree.delTreeValue(m_tree.rootContent(), QVariantList(), &isValid);
    QVERIFY(isValid);
    QVERIFY(m_tree.isValid() == false);
}


void TreeGSD::test07DelContainerList()
{
    QVariantList list;
    list.append(QVariant(0x812451));
    list.append(QVariant(QVariantList()
                         << QVariant(QLatin1String("inner"))
                         << QVariant(false)));
    m_tree.setRootContent(list);

    QVERIFY(m_tree.isValid());
    QVERIFY(m_tree.rootContent().type() == QVariant::List);
    QVERIFY(m_tree.nodeType() == QVariant::List);
    QVERIFY(m_tree.nodeIsContainer());
    QVERIFY(m_tree.address().isEmpty());

    bool isValid = false;
    QVariantList addr;
    addr << QVariant(1) << QVariant(0);
    QVERIFY(m_tree.getTreeValue(m_tree.rootContent(), addr, &isValid) == QVariant(QLatin1String("inner")));
    QVERIFY(isValid);
    m_tree.delTreeValue(m_tree.rootContent(), addr, &isValid);
    QVERIFY(isValid);
    QVERIFY(m_tree.getTreeValue(m_tree.rootContent(), addr, &isValid) == QVariant(false));
    QVERIFY(isValid);

    addr.clear();
    addr << QVariant(0);
    QVERIFY(m_tree.getTreeValue(m_tree.rootContent(), addr, &isValid) == QVariant(0x812451));
    QVERIFY(isValid);
    m_tree.delTreeValue(m_tree.rootContent(), addr, &isValid);
    QVERIFY(isValid);
    addr << QVariant(0);
    QVERIFY(m_tree.getTreeValue(m_tree.rootContent(), addr, &isValid) == QVariant(false));
    QVERIFY(isValid);
}
