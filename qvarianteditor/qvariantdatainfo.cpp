#include "qvariantdatainfo.h"

#include <QDebug>



QVariantDataInfo::QVariantDataInfo(const QVariant &cdata) :
    m_cdata(cdata)
{
}

bool QVariantDataInfo::isValid() const
{
    // can only knows about Qt predefined type
    if (!m_cdata.isValid() || (int)m_cdata.type() != m_cdata.userType())
        return false;

    bool v = false;

    // we handle only a reduced number
    switch(m_cdata.type())
    {
    case QVariant::Bool:
    case QVariant::UInt:
    case QVariant::Int:
    case QVariant::Double:
    case QVariant::Map:
    case QVariant::List:
    case QVariant::String:
    case QVariant::Hash:
        v = true;
        break;

        //    case QVariant::UInt:
        //    case QVariant::LongLong:
        //    case QVariant::ULongLong:
        //    case QVariant::Double:
        //    case QVariant::Char:
        //    case QVariant::StringList:
        //    case QVariant::ByteArray:
        //    case QVariant::BitArray:
        //    case QVariant::Date:
        //    case QVariant::Time:
        //    case QVariant::DateTime:
        //    case QVariant::Url:
        //    case QVariant::Locale:
        //    case QVariant::Rect:
        //    case QVariant::RectF:
        //    case QVariant::Size:
        //    case QVariant::SizeF:
        //    case QVariant::Line:
        //    case QVariant::LineF:
        //    case QVariant::Point:
        //    case QVariant::PointF:
        //    case QVariant::RegExp:
        //    case QVariant::RegularExpression:
        //    case QVariant::EasingCurve:
        //    case QVariant::Uuid:
        //    case QVariant::ModelIndex:

        //    case QVariant::Font:
        //    case QVariant::Pixmap:
        //    case QVariant::Brush:
        //    case QVariant::Color:
        //    case QVariant::Palette:
        //    case QVariant::Image:
        //    case QVariant::Polygon:
        //    case QVariant::Region:
        //    case QVariant::Bitmap:
        //    case QVariant::Cursor:
        //    case QVariant::KeySequence:
        //    case QVariant::Pen:
        //    case QVariant::TextLength:
        //    case QVariant::TextFormat:
        //    case QVariant::Matrix:
        //    case QVariant::Transform:
        //    case QVariant::Matrix4x4:
        //    case QVariant::Vector2D:
        //    case QVariant::Vector3D:
        //    case QVariant::Vector4D:
        //    case QVariant::Quaternion:
        //    case QVariant::PolygonF:
        //    case QVariant::Icon:

        //    case QVariant::SizePolicy:

    case QVariant::Invalid:
    default:
        break;
    }

    return v;
}

//------------------------------------------------------------------------------

bool QVariantDataInfo::isAtomic() const
{
    bool v = false;

    // we handle only a reduced number
    switch(m_cdata.type())
    {
    case QVariant::Bool:
    case QVariant::UInt:
    case QVariant::Int:
    case QVariant::String:
    case QVariant::Double:
        v = true;
        break;
    default:
        break;
    }

    return v;
}

bool QVariantDataInfo::isContainer() const
{
    bool v = false;

    // we handle only a reduced number
    switch(m_cdata.type())
    {
    case QVariant::List:
    case QVariant::Hash:
    case QVariant::Map:
        v = true;
        break;
    default:
        break;
    }

    return v;
}

//------------------------------------------------------------------------------

QList<QVariant> QVariantDataInfo::containerKeys() const
{
    Q_ASSERT(isContainer());
    QList<QVariant> keys;

    // we handle only a reduced number
    switch(m_cdata.type())
    {
    case QVariant::List:
        keys = qtprivate::IndexCollection(m_cdata.toList()).keys();
        break;
    case QVariant::Hash:
        keys = qtprivate::AssociativeCollection(m_cdata.toHash()).keys();
        break;
    case QVariant::Map:
        keys = qtprivate::AssociativeCollection(m_cdata.toMap()).keys();
        break;
    default:
        break;
    }

    return keys;
}

QVariant QVariantDataInfo::containerValue(const QVariant& key) const
{
    Q_ASSERT(isContainer());
    QVariant value;

    // we handle only a reduced number
    switch(m_cdata.type())
    {
    case QVariant::List:
        value = m_cdata.toList().value(key.toUInt());
        break;
    case QVariant::Hash:
        value = m_cdata.toHash().value(key.toString());
        break;
    case QVariant::Map:
        value = m_cdata.toMap().value(key.toString());
        break;
    default:
        break;
    }

    return value;
}

//------------------------------------------------------------------------------

QString QVariantDataInfo::displayText(int depth) const
{
    QString repr;

    // we handle only a reduced number
    switch(m_cdata.type())
    {
    case QVariant::Invalid:
        repr = QStringLiteral("<Invalid>");
        break;
    case QVariant::Bool:
        repr = m_cdata.toBool()
                ? QStringLiteral("true")
                : QStringLiteral("false");
        break;
    case QVariant::UInt:
        repr = QString::number(m_cdata.toUInt());
        break;
    case QVariant::Int:
        repr = QString::number(m_cdata.toInt());
        break;
    case QVariant::Double:
        repr = QString::number(m_cdata.toDouble(), 'f');
        break;
    case QVariant::String:
        repr = QString("\"%1\"").arg(m_cdata.toString());
        break;
    case QVariant::List:
        repr = (depth > 0)
                ? qtprivate::IndexCollection(m_cdata.toList()).displayText(--depth)
                : QString("[...]");
        break;
    case QVariant::Hash:
        repr = (depth > 0)
                ? qtprivate::AssociativeCollection(m_cdata.toHash()).displayText(--depth)
                : QString("{...}");
        break;
    case QVariant::Map:
        repr = (depth > 0)
                ? qtprivate::AssociativeCollection(m_cdata.toMap()).displayText(--depth)
                : QString("{...}");
        break;
    default:
        repr = QStringLiteral("<unknow>");
        break;
    }

    return repr;
}

//------------------------------------------------------------------------------

QMutableVariantDataInfo::QMutableVariantDataInfo(QVariant &data) :
    QVariantDataInfo(data),
    m_mutdata(data)
{
}

bool QMutableVariantDataInfo::editableKeys() const
{
    // we handle only a reduced number
    switch(m_mutdata.type())
    {
    case QVariant::Map:
    case QVariant::Hash:
        return true;
    default:
        break;
    }

    return false;
}

bool QMutableVariantDataInfo::editableValues() const
{
    // we handle only a reduced number
    switch(m_mutdata.type())
    {
    case QVariant::Map:
    case QVariant::Hash:
    case QVariant::List:
        return true;
    default:
        break;
    }

    return false;
}

void QMutableVariantDataInfo::setContainerKey(const QVariant& oldKey, const QVariant& newKey)
{
    Q_ASSERT(editableKeys());

    switch(m_mutdata.type())
    {
    case QVariant::Map: {
        QVariantMap map = m_mutdata.toMap();
        Q_ASSERT(map.contains(oldKey.toString()));
        map.insert(newKey.toString(), map.take(oldKey.toString()));
        m_mutdata = map;
    }
        break;
    case QVariant::Hash: {
        QVariantHash hash = m_mutdata.toHash();
        Q_ASSERT(hash.contains(oldKey.toString()));
        hash.insert(newKey.toString(), hash.take(oldKey.toString()));
        m_mutdata = hash;
    }
        break;
    default:
        qDebug("%s:%d: keys should be editable for type %d-%s,"
               " but implementation is missing",
               __FILE__, __LINE__,
               m_mutdata.userType(), qPrintable(m_mutdata.typeName()));
        break;
    }
}

void QMutableVariantDataInfo::setContainerValue(const QVariant& key, const QVariant& value)
{
    Q_ASSERT(editableValues());

    switch(m_mutdata.type())
    {
    case QVariant::Map: {
        QVariantMap map = m_mutdata.toMap();
        Q_ASSERT(map.contains(key.toString()));
        map.remove(key.toString());
        map.insert(key.toString(), value);
        m_mutdata = map;
    }
        break;
    case QVariant::Hash: {
        QVariantHash hash = m_mutdata.toHash();
        Q_ASSERT(hash.contains(key.toString()));
        hash.remove(key.toString());
        hash.insert(key.toString(), value);
        m_mutdata = hash;
    }
        break;
    case QVariant::List: {
        QVariantList list = m_mutdata.toList();
        Q_ASSERT(key.toInt() >= 0 && key.toInt() < list.count());
        list.replace(key.toInt(), value);
        m_mutdata = list;
    }
        break;
    default:
        qDebug("%s:%d: values should be editable for type %d-%s,"
               " but implementation is missing",
               __FILE__, __LINE__,
               m_mutdata.userType(), qPrintable(m_mutdata.typeName()));
        break;
    }
}
