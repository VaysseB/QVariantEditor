#include "tst_treegsd.h"

QTEST_APPLESS_MAIN(TreeGSD)


TreeGSD::TreeGSD() :
    m_tree()
{
}

TreeGSD::~TreeGSD()
{
    m_tree.clear();
}



void TreeGSD::testTypes()
{
    Q_ASSERT(m_tree.typeIsContainer(QVariant::List));
    Q_ASSERT(m_tree.typeIsContainer(QVariant::Hash));
    Q_ASSERT(m_tree.typeIsContainer(QVariant::Map));
    Q_ASSERT(m_tree.typeIsContainer(QVariant::StringList));

    Q_ASSERT(m_tree.typeIsContainer(QVariant::Bool) == false);
    Q_ASSERT(m_tree.typeIsContainer(QVariant::Int) == false);
    Q_ASSERT(m_tree.typeIsContainer(QVariant::Double) == false);
    Q_ASSERT(m_tree.typeIsContainer(QVariant::String) == false);
    Q_ASSERT(m_tree.typeIsContainer(QVariant::Time) == false);
    Q_ASSERT(m_tree.typeIsContainer(QVariant::Date) == false);
    Q_ASSERT(m_tree.typeIsContainer(QVariant::DateTime) == false);
}

