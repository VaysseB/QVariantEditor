#include "qtreevariantwidget.h"
#include "ui_qtreevariantwidget.h"

QTreeVariantWidget::QTreeVariantWidget(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::QTreeVariantWidget)
{
    ui->setupUi(this);
}

QTreeVariantWidget::~QTreeVariantWidget()
{
    delete ui;
}
