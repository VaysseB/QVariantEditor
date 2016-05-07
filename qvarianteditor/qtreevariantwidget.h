#ifndef QTREEVARIANTWIDGET_H
#define QTREEVARIANTWIDGET_H

#include <QVariant>
#include <QSharedPointer>
#include <QWidget>
#include <QMenu>

#include "qvariantmodel.h"

namespace Ui {
class QTreeVariantWidget;
}


class QTreeVariantWidget : public QWidget
{
    Q_OBJECT

public:
    explicit QTreeVariantWidget(QWidget *parent = 0);
    virtual ~QTreeVariantWidget();

    QString filename() const { return m_filename; }

signals:
    void widgetModified(bool isModified);

public slots:
    void setFilename(const QString& name);

    void read();
    void write();

    void setSearchVisible(bool visible);
    void setOptionsVisible(bool visible);

private slots:
    void searchTypeChanged(int index);
    void searchFieldsChanged(int index);

    void modelDataChanged();

    void showEditMenu(const QPoint& pos);
    void updateEditMenu(const QVariant &datas);

    void insertNew();
    void insertBeforeCurrent();
    void insertAfterCurrent();

private:
    Ui::QTreeVariantWidget *ui;

    QString m_filename;

    QSharedPointer<QVariantModel> mp_model;

    QSharedPointer<QMenu> mp_editMenu;
};

#endif // QTREEVARIANTWIDGET_H
