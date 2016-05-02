#ifndef QTREEVARIANTWIDGET_H
#define QTREEVARIANTWIDGET_H

#include <QWidget>

namespace Ui {
class QTreeVariantWidget;
}

class QTreeVariantWidget : public QWidget
{
    Q_OBJECT

public:
    explicit QTreeVariantWidget(QWidget *parent = 0);
    ~QTreeVariantWidget();

private:
    Ui::QTreeVariantWidget *ui;
};

#endif // QTREEVARIANTWIDGET_H
