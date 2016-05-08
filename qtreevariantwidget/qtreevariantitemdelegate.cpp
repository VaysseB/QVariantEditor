#include "qtreevariantitemdelegate.h"

#include <QComboBox>
#include <QDebug>


QTreeVariantItemDelegate::QTreeVariantItemDelegate(QObject* parent) :
    QStyledItemDelegate(parent)
{
}

QTreeVariantItemDelegate::~QTreeVariantItemDelegate()
{
}

QWidget* QTreeVariantItemDelegate::createEditor(
        QWidget* parent,
        const QStyleOptionViewItem& option,
        const QModelIndex& index) const
{
    QVariant data = index.data(Qt::EditRole);
    // if edit data type
    if (data.isNull()) {
        QWidget* editor = createVariantTypeEditor(parent,
                                                  data.type(),
                                                  data.userType());

        if (editor) {
            // force no-const to save editor
            // because `createEditor` is supringsily const
            QTreeVariantItemDelegate* superThis =
                    const_cast<QTreeVariantItemDelegate*>(this);
            superThis->m_openEditors.append(editor);

            connect(editor, &QComboBox::destroyed,
                    this, &QTreeVariantItemDelegate::editorDeleted);
        }

        return editor;
    }
    return QStyledItemDelegate::createEditor(parent, option, index);
}

QWidget* QTreeVariantItemDelegate::createVariantTypeEditor(
        QWidget* parent,
        QVariant::Type varType,
        int userType) const
{
    Q_UNUSED(userType)

    // we only create editor for Qt qvariant data type
    if (varType == QVariant::UserType)
        return nullptr;

    const QVariant::Type qtDataTypes[] = {
        // Invalid added by hand
        QVariant::Bool,
        QVariant::Int,
        QVariant::UInt,
        QVariant::LongLong,
        QVariant::ULongLong,
        QVariant::Double,
        QVariant::Char,
        QVariant::Map,
        QVariant::List,
        QVariant::String,
        QVariant::StringList,
        QVariant::ByteArray,
        QVariant::BitArray,
        QVariant::Date,
        QVariant::Time,
        QVariant::DateTime,
        QVariant::Url,
        QVariant::Locale,
        QVariant::Rect,
        QVariant::RectF,
        QVariant::Size,
        QVariant::SizeF,
        QVariant::Line,
        QVariant::LineF,
        QVariant::Point,
        QVariant::PointF,
        QVariant::RegExp,
        QVariant::RegularExpression,
        QVariant::Hash,
        QVariant::EasingCurve,
        QVariant::Uuid,
        QVariant::ModelIndex,
        // LastCoreType

        QVariant::Font,
        QVariant::Pixmap,
        QVariant::Brush,
        QVariant::Color,
        QVariant::Palette,
        QVariant::Image,
        QVariant::Polygon,
        QVariant::Region,
        QVariant::Bitmap,
        QVariant::Cursor,
        QVariant::KeySequence,
        QVariant::Pen,
        QVariant::TextLength,
        QVariant::TextFormat,
        QVariant::Matrix,
        QVariant::Transform,
        QVariant::Matrix4x4,
        QVariant::Vector2D,
        QVariant::Vector3D,
        QVariant::Vector4D,
        QVariant::Quaternion,
        QVariant::PolygonF,
        QVariant::Icon,
        // LastGuiType

        QVariant::SizePolicy,
    };

    QComboBox* combo = new QComboBox(parent);

    combo->addItem(QLatin1Literal("Invalid"));
    int count = sizeof(qtDataTypes) / sizeof(qtDataTypes[0]);
    int indexType = 0;
    for (int i = 0; i < count; i++) {
        QVariant::Type type = qtDataTypes[i];
        QString typeName = QVariant::typeToName(type);
        combo->addItem(typeName, type);

        if (type == varType)
            indexType = i+1;
    }

    // config
    combo->setEditable(false);
    combo->setCurrentIndex(indexType);

    // commit signal
    void (QComboBox::*indexActivated)(int index) = &QComboBox::activated;
    connect(combo, indexActivated,
            this, &QTreeVariantItemDelegate::commitEditor);

    return combo;
}

void QTreeVariantItemDelegate::setEditorData(
        QWidget* editor,
        const QModelIndex& index) const
{
    if (m_openEditors.contains(editor)) {
        QComboBox* combo = qobject_cast<QComboBox*>(editor);
        Q_ASSERT(combo);

        QVariant data = index.data(Qt::EditRole);
        int dataType = data.userType();
        int indexType = 0;
        while (indexType < combo->count()
               && combo->itemData(indexType).userType() != dataType) {
            indexType++;
        }
        if (indexType >= combo->count())
            indexType = 0;

        combo->setCurrentIndex(indexType);
    }
    else
        QStyledItemDelegate::setEditorData(editor, index);
}

void QTreeVariantItemDelegate::setModelData(
        QWidget* editor,
        QAbstractItemModel* model,
        const QModelIndex& index) const
{
    if (m_openEditors.contains(editor)) {
        QComboBox* combo = qobject_cast<QComboBox*>(editor);
        Q_ASSERT(combo);

        QVariant dataType(combo->currentData(Qt::UserRole));
        model->setData(index, dataType);
    }
    else
        QStyledItemDelegate::setModelData(editor, model, index);
}

void QTreeVariantItemDelegate::editorDeleted()
{
    QWidget* widget = qobject_cast<QWidget*>(sender());
    if (widget)
        m_openEditors.removeOne(widget);
}

void QTreeVariantItemDelegate::commitEditor()
{
    QWidget* widget = qobject_cast<QWidget*>(sender());
    Q_ASSERT(widget);
    emit commitData(widget);
}
