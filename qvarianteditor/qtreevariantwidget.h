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
    virtual ~QTreeVariantWidget();

    QString filename() const { return m_filename; }

signals:
    void widgetModified(bool isModified);

public slots:
    void setFilename(const QString& name);

    void read();
    void write();

private slots:
    void on_buttonUp_clicked();

private:
    Ui::QTreeVariantWidget *ui;

    QString m_filename;
};

#endif // QTREEVARIANTWIDGET_H
