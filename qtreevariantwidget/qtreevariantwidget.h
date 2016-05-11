#ifndef QTREEVARIANTWIDGET_H
#define QTREEVARIANTWIDGET_H

#include <QVariant>
#include <QSharedPointer>
#include <QTimer>
#include <QWidget>
#include <QMenu>

#include "qvariantmodel.h"

namespace Ui {
class QTreeVariantWidget;
}


class QTreeVariantWidget : public QWidget
{
    Q_OBJECT
    Q_PROPERTY(int searchUpdateInterval READ searchUpdateInterval WRITE setSearchUpdateInterval NOTIFY searchUpdateIntervalChanged)

public:
    explicit QTreeVariantWidget(QWidget *parent = 0);
    virtual ~QTreeVariantWidget();

    inline QString filename() const { return m_filename; }

    int searchUpdateInterval() const;

signals:
    void searchUpdateIntervalChanged(int searchUpdateInterval);

    void widgetModified(bool isModified);

public slots:
    void setFilename(const QString& name);

    void read();
    void write();

    void setSearchVisible(bool visible);
    void setOptionsVisible(bool visible);

    void setSearchUpdateInterval(int msec);

private slots:
    void modelDataChanged();

    void updateSearch();

    void showEditMenu(const QPoint& pos);
    void createEditMenus();

    void insertNew();
    void insertIntoCurrent();
    void insertBeforeCurrent();
    void insertAfterCurrent();
    void removeCurrent();

private:
    Ui::QTreeVariantWidget *ui;

    QString m_filename;

    QSharedPointer<QVariantModel> mp_model;

    QSharedPointer<QMenu> mp_blankEditMenu;
    QSharedPointer<QMenu> mp_selEditMenu;
    QSharedPointer<QMenu> mp_selContainerEditMenu;

    QTimer m_searchUpdateTimer;
};

#endif // QTREEVARIANTWIDGET_H
