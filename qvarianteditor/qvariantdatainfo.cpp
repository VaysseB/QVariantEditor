#include "qvariantdatainfo.h"



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
    case QVariant::Int:
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
    case QVariant::Int:
    case QVariant::String:
        v = true;
        break;
    default:
        break;
    }

    return v;
}

bool QVariantDataInfo::isCollection() const
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

QList<QVariant> QVariantDataInfo::collectionKeys() const
{
    Q_ASSERT(isCollection());
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

QVariant QVariantDataInfo::collectionValue(const QVariant& key) const
{
    Q_ASSERT(isCollection());
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
    case QVariant::Int:
        repr = QString::number(m_cdata.toInt());
        break;
    case QVariant::String:
        repr = QString("\"%1\"").arg(m_cdata.toString());
        break;
    case QVariant::List:
        repr = (depth > 0)
                ? qtprivate::IndexCollection(m_cdata.toList()).displayText(--depth)
                : QString("[<fold>]");
        break;
    case QVariant::Hash:
        repr = (depth > 0)
                ? qtprivate::AssociativeCollection(m_cdata.toHash()).displayText(--depth)
                : QString("{<fold>}");
        break;
    case QVariant::Map:
        repr = (depth > 0)
                ? qtprivate::AssociativeCollection(m_cdata.toMap()).displayText(--depth)
                : QString("{<fold>}");
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

void QMutableVariantDataInfo::setCollectionValue(const QVariant& key, const QVariant& value)
{
    Q_ASSERT(isCollection());

}

void QMutableVariantDataInfo::removeCollectionValue(const QVariant& key)
{
    Q_ASSERT(isCollection());

}

QVariant QMutableVariantDataInfo::addNewCollectionItem()
{
    Q_ASSERT(isCollection());
    return QVariant();
}

QVariant QMutableVariantDataInfo::insertBeforeNewCollectionItem(const QVariant& afterKey)
{
    Q_ASSERT(isCollection());
    return QVariant();
}
