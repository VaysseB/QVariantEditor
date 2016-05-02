#include "qtreevariantwidget.h"
#include "ui_qtreevariantwidget.h"

#include <QFileInfo>
#include <QDebug>


QTreeVariantWidget::QTreeVariantWidget(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::QTreeVariantWidget)
{
    ui->setupUi(this);

    setFilename(QString());
    setWindowModified(true);
    emit widgetModified(isWindowModified());
}

QTreeVariantWidget::~QTreeVariantWidget()
{
    delete ui;
}

void QTreeVariantWidget::setFilename(const QString& name)
{
    m_filename = name;
    if (m_filename.isEmpty())
        setWindowTitle(tr("untitled"));
    else
        setWindowTitle(QFileInfo(name).fileName());
}

void QTreeVariantWidget::read()
{
    setWindowModified(false);
    emit widgetModified(isWindowModified());
}

void QTreeVariantWidget::write()
{
    setWindowModified(false);
    emit widgetModified(isWindowModified());
}

void QTreeVariantWidget::on_buttonUp_clicked()
{
    setWindowModified(true);
    emit widgetModified(isWindowModified());
}
