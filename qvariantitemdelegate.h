#ifndef QVARIANTITEMDELEGATE_H
#define QVARIANTITEMDELEGATE_H

#include <QItemDelegate>
#include <QSpinBox>

#include "qvarianttreeitemmodel.h"


class QVariantItemDelegate : public QItemDelegate
{
    Q_OBJECT
public:
    explicit QVariantItemDelegate(QObject *parent = 0);

    // editing
    QWidget *createEditor(QWidget *parent,
                          const QStyleOptionViewItem &option,
                          const QModelIndex &index) const;

    void setEditorData(QWidget *editor,
                       const QModelIndex &index) const;

    void setModelData(QWidget *editor,
                      QAbstractItemModel *model,
                      const QModelIndex &index) const;

    virtual QVariant convertValue(QVariant value,
                                  QVariant::Type returnType,
                                  bool* ok = NULL) const;

private:
    QVariantTreeItemModel* extractModel(const QModelIndex& index) const;
};


#endif // QVARIANTITEMDELEGATE_H
