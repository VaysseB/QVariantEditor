#ifndef QTREEVARIANTITEMDELEGATE_H
#define QTREEVARIANTITEMDELEGATE_H

#include <QStyledItemDelegate>
#include <QList>

class QTreeVariantItemDelegate : public QStyledItemDelegate
{
    Q_OBJECT

public:
    explicit QTreeVariantItemDelegate(QObject* parent = nullptr);
    virtual ~QTreeVariantItemDelegate();

    // editing
    QWidget* createEditor(QWidget* parent,
                          const QStyleOptionViewItem& option,
                          const QModelIndex& index) const;

    void setEditorData(QWidget* editor, const QModelIndex &index) const;
    void setModelData(QWidget* editor,
                      QAbstractItemModel* model,
                      const QModelIndex &index) const;

protected:
    QWidget* createVariantTypeEditor(QWidget* parent,
                                     QVariant::Type varType,
                                     int userType) const;

private slots:
    void editorDeleted();
    void commitEditor();

protected:
    QList<QWidget*> m_openEditors;
};

#endif // QTREEVARIANTITEMDELEGATE_H
