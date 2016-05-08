#include "qvariantdatainfo.h"

#include <QString>
#include <QStringList>
#include <QByteArray>
#include <QBitArray>
#include <QDate>
#include <QTime>
#include <QDateTime>
#include <QUrl>
#include <QLocale>
#include <QRect>
#include <QRectF>
#include <QSize>
#include <QSizeF>
#include <QLine>
#include <QLineF>
#include <QPoint>
#include <QPointF>
#include <QRegExp>
#include <QRegularExpression>
#include <QEasingCurve>
#include <QUuid>
#include <QModelIndex>
#include <QFont>
#include <QPixmap>
#include <QBrush>
#include <QColor>
#include <QPalette>
#include <QImage>
#include <QPolygon>
#include <QRegion>
#include <QBitmap>
#include <QCursor>
#include <QKeySequence>
#include <QPen>
#include <QTextLength>
#include <QTextFormat>
#include <QMatrix>
#include <QTransform>
#include <QMatrix4x4>
#include <QVector2D>
#include <QVector3D>
#include <QVector4D>
#include <QQuaternion>
#include <QPolygonF>
#include <QIcon>
#include <QtWidgets/qsizepolicy.h>

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
        keys = QtPrivate::VariantDataInfo::IndexCollection(m_cdata.toList()).keys();
        break;
    case QVariant::Hash:
        keys = QtPrivate::VariantDataInfo::AssociativeCollection(m_cdata.toHash()).keys();
        break;
    case QVariant::Map:
        keys = QtPrivate::VariantDataInfo::AssociativeCollection(m_cdata.toMap()).keys();
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
                ? QtPrivate::VariantDataInfo::IndexCollection(m_cdata.toList()).displayText(--depth)
                : QString("[...]");
        break;
    case QVariant::Hash:
        repr = (depth > 0)
                ? QtPrivate::VariantDataInfo::AssociativeCollection(m_cdata.toHash()).displayText(--depth)
                : QString("{...}");
        break;
    case QVariant::Map:
        repr = (depth > 0)
                ? QtPrivate::VariantDataInfo::AssociativeCollection(m_cdata.toMap()).displayText(--depth)
                : QString("{...}");
        break;
    default:
        repr = QStringLiteral("<unknow>");
        break;
    }

    return repr;
}

//------------------------------------------------------------------------------

QVariant QVariantDataInfo::tryConvert(QVariant::Type vType,
                                      int userType) const
{
    QVariant data(m_cdata);
    bool convertSuccess = data.convert(userType);

    if (convertSuccess)
        return data;

    // we handle only a reduced number
    switch(vType)
    {
    case QVariant::Bool:
        data = QVariant::fromValue<bool>(false);
        break;
    case QVariant::Int:
        data = QVariant::fromValue<int>(0);
        break;
    case QVariant::UInt:
        data = QVariant::fromValue<unsigned int>(0);
        break;
    case QVariant::LongLong:
        data = QVariant::fromValue<qlonglong>(0L);
        break;
    case QVariant::ULongLong:
        data = QVariant::fromValue<qulonglong>(0UL);
        break;
    case QVariant::Double:
        data = QVariant::fromValue<double>(0.0);
        break;
    case QVariant::Char:
        data = QVariant::fromValue<char>(0);
        break;
    case QVariant::Map:
        data = QVariant::fromValue<>(QVariantMap());
        break;
    case QVariant::List:
        data = QVariant::fromValue<>(QVariantList());
        break;
    case QVariant::String:
        data = QVariant::fromValue<>(QString());
        break;
    case QVariant::StringList:
        data = QVariant::fromValue<>(QStringList());
        break;
    case QVariant::ByteArray:
        data = QVariant::fromValue<>(QByteArray());
        break;
    case QVariant::BitArray:
        data = QVariant::fromValue<>(QBitArray());
        break;
    case QVariant::Date:
        data = QVariant::fromValue<>(QDate());
        break;
    case QVariant::Time:
        data = QVariant::fromValue<>(QTime());
        break;
    case QVariant::DateTime:
        data = QVariant::fromValue<>(QDateTime());
        break;
    case QVariant::Url:
        data = QVariant::fromValue<>(QUrl());
        break;
    case QVariant::Locale:
        data = QVariant::fromValue<>(QLocale());
        break;
    case QVariant::Rect:
        data = QVariant::fromValue<>(QRect());
        break;
    case QVariant::RectF:
        data = QVariant::fromValue<>(QRectF());
        break;
    case QVariant::Size:
        data = QVariant::fromValue<>(QSize());
        break;
    case QVariant::SizeF:
        data = QVariant::fromValue<>(QSizeF());
        break;
    case QVariant::Line:
        data = QVariant::fromValue<>(QLine());
        break;
    case QVariant::LineF:
        data = QVariant::fromValue<>(QLineF());
        break;
    case QVariant::Point:
        data = QVariant::fromValue<>(QPoint());
        break;
    case QVariant::PointF:
        data = QVariant::fromValue<>(QPointF());
        break;
    case QVariant::RegExp:
        data = QVariant::fromValue<>(QRegExp());
        break;
    case QVariant::RegularExpression:
        data = QVariant::fromValue<>(QRegularExpression());
        break;
    case QVariant::Hash:
        data = QVariant::fromValue<>(QVariantHash());
        break;
    case QVariant::EasingCurve:
        data = QVariant::fromValue<>(QEasingCurve());
        break;
    case QVariant::Uuid:
        data = QVariant::fromValue<>(QUuid());
        break;
    case QVariant::ModelIndex:
        data = QVariant::fromValue<>(QModelIndex());
        break;
    case QVariant::Font:
        data = QVariant::fromValue<>(QFont());
        break;
    case QVariant::Pixmap:
        data = QVariant::fromValue<>(QPixmap());
        break;
    case QVariant::Brush:
        data = QVariant::fromValue<>(QBrush());
        break;
    case QVariant::Color:
        data = QVariant::fromValue<>(QColor());
        break;
    case QVariant::Palette:
        data = QVariant::fromValue<>(QPalette());
        break;
    case QVariant::Image:
        data = QVariant::fromValue<>(QImage());
        break;
    case QVariant::Polygon:
        data = QVariant::fromValue<>(QPolygon());
        break;
    case QVariant::Region:
        data = QVariant::fromValue<>(QRegion());
        break;
    case QVariant::Bitmap:
        data = QVariant::fromValue<>(QBitmap());
        break;
    case QVariant::Cursor:
        data = QVariant::fromValue<>(QCursor());
        break;
    case QVariant::KeySequence:
        data = QVariant::fromValue<>(QKeySequence());
        break;
    case QVariant::Pen:
        data = QVariant::fromValue<>(QPen());
        break;
    case QVariant::TextLength:
        data = QVariant::fromValue<>(QTextLength());
        break;
    case QVariant::TextFormat:
        data = QVariant::fromValue<>(QTextFormat());
        break;
    case QVariant::Matrix:
        data = QVariant::fromValue<>(QMatrix());
        break;
    case QVariant::Transform:
        data = QVariant::fromValue<>(QTransform());
        break;
    case QVariant::Matrix4x4:
        data = QVariant::fromValue<>(QMatrix4x4());
        break;
    case QVariant::Vector2D:
        data = QVariant::fromValue<>(QVector2D());
        break;
    case QVariant::Vector3D:
        data = QVariant::fromValue<>(QVector3D());
        break;
    case QVariant::Vector4D:
        data = QVariant::fromValue<>(QVector4D());
        break;
    case QVariant::Quaternion:
        data = QVariant::fromValue<>(QQuaternion());
        break;
    case QVariant::PolygonF:
        data = QVariant::fromValue<>(QPolygonF());
        break;
    case QVariant::Icon:
        data = QVariant::fromValue<>(QIcon());
        break;
    case QVariant::SizePolicy:
        data = QVariant::fromValue<>(QSizePolicy());
        break;
    default:
    case QVariant::Invalid:
        data = QVariant();
        break;
    }

    return data;
}

//==============================================================================

QMutableVariantDataInfo::QMutableVariantDataInfo(QVariant &data) :
    QVariantDataInfo(data),
    m_mutdata(data)
{
}

QMutableVariantDataInfo::QMutableVariantDataInfo(const QVariant &data) :
    QVariantDataInfo(data),
    m_mutdata(m_dummy)
{
}

bool QMutableVariantDataInfo::editableKeys() const
{
    // we handle only a reduced number
    switch(m_cdata.type())
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
    switch(m_cdata.type())
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

void QMutableVariantDataInfo::setContainerKey(
        const QVariant& oldKey, const QVariant& newKey)
{
    Q_ASSERT(isConst() == false);
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

void QMutableVariantDataInfo::setContainerValue(
        const QVariant& key, const QVariant& value)
{
    Q_ASSERT(isConst() == false);
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

//------------------------------------------------------------------------------

bool QMutableVariantDataInfo::isNewKeyInsertable() const
{
    bool insertable = false;

    switch(m_cdata.type())
    {
    case QVariant::Map:
    case QVariant::Hash:
    case QVariant::List:
        insertable = true;
        break;
    default:
        break;
    }

    return insertable;
}

void QMutableVariantDataInfo::tryInsertNewKey(
        const QVariant& beforeKey,
        const QVariant& value)
{
    Q_ASSERT(isConst() == false);
    Q_ASSERT(isNewKeyInsertable());

    switch(m_mutdata.type())
    {
    case QVariant::Map: {
        QVariantMap map = m_mutdata.toMap();
        auto mut = QtPrivate::VariantDataInfo::MutableAssociativeCollection(map);
        mut.insertNewKey(beforeKey.toString(),
                         value);
        m_mutdata = map;
    }
        break;
    case QVariant::Hash: {
        QVariantHash hash = m_mutdata.toHash();
        auto mut = QtPrivate::VariantDataInfo::MutableAssociativeCollection(hash);
        mut.insertNewKey(beforeKey.toString(),
                         value);
        m_mutdata = hash;
    }
        break;
    case QVariant::List: {
        QVariantList list = m_mutdata.toList();
        auto mut = QtPrivate::VariantDataInfo::MutableIndexCollection(list);
        mut.insertNewKey(beforeKey.isValid() ? beforeKey.toInt() : -1,
                         value);
        m_mutdata = list;
    }
        break;
    default:
        qDebug("%s:%d: new key should be inserted for type %d-%s,"
               " but implementation is missing",
               __FILE__, __LINE__,
               m_mutdata.userType(), qPrintable(m_mutdata.typeName()));
        break;
    }
}

//------------------------------------------------------------------------------

bool QMutableVariantDataInfo::isKeyRemovable() const
{
    bool removable = false;

    switch(m_cdata.type())
    {
    case QVariant::Map:
    case QVariant::Hash:
    case QVariant::List:
        removable = true;
        break;
    default:
        break;
    }

    return removable;
}

void QMutableVariantDataInfo::removeKey(const QVariant &key)
{
    Q_ASSERT(isConst() == false);
    Q_ASSERT(isKeyRemovable());

    switch(m_mutdata.type())
    {
    case QVariant::Map: {
        QVariantMap map = m_mutdata.toMap();
        Q_ASSERT(map.contains(key.toString()));
        auto mut = QtPrivate::VariantDataInfo::MutableAssociativeCollection(map);
        mut.removeKey(key.toString());
        m_mutdata = map;
    }
        break;
    case QVariant::Hash: {
        QVariantHash hash = m_mutdata.toHash();
        Q_ASSERT(hash.contains(key.toString()));
        auto mut = QtPrivate::VariantDataInfo::MutableAssociativeCollection(hash);
        mut.removeKey(key.toString());
        m_mutdata = hash;
    }
        break;
    case QVariant::List: {
        QVariantList list = m_mutdata.toList();
        Q_ASSERT(key.toInt() >= 0 && key.toInt() < list.count());
        auto mut = QtPrivate::VariantDataInfo::MutableIndexCollection(list);
        mut.removeKey(key.toInt());
        m_mutdata = list;
    }
        break;
    default:
        qDebug("%s:%d: new key should be inserted for type %d-%s,"
               " but implementation is missing",
               __FILE__, __LINE__,
               m_mutdata.userType(), qPrintable(m_mutdata.typeName()));
        break;
    }
}
