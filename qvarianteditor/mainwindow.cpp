#include "mainwindow.h"
#include "ui_mainwindow.h"

#include <QFileDialog>
#include <QCloseEvent>
#include <QLabel>
#include <QMessageBox>
#include <QStandardPaths>

#include <QDebug>

#include "qtreevariantwidget.h"


MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::mainwindow)
{
    ui->setupUi(this);

    // signals to actions
    connect(ui->actionNew, SIGNAL(triggered()),
            this, SLOT(new_()));
    connect(ui->actionOpen, SIGNAL(triggered()),
            this, SLOT(open()));
    connect(ui->actionSave, SIGNAL(triggered()),
            this, SLOT(save()));
    connect(ui->actionSaveAs, SIGNAL(triggered()),
            this, SLOT(saveAs()));
    connect(ui->actionClose, SIGNAL(triggered()),
            this, SLOT(close()));
    connect(ui->actionQuit, SIGNAL(triggered()),
            this, SLOT(quit()));
    connect(ui->actionAbout, SIGNAL(triggered()),
            this, SLOT(about()));

    connect(ui->tabWidget, SIGNAL(currentChanged(int)),
            this, SLOT(updateWithTab(int)));
}

MainWindow::~MainWindow()
{
    delete ui;
}
//------------------------------------------------------------------------------

void MainWindow::about()
{
    QStringList author;
    author << "Vaysse B.";

    QStringList content;
    content << QString("<h2>%1</h2>%2")
                .arg(QApplication::applicationName())
                .arg(tr("QVariant file explorer and editor."));
    content << tr("Version: ") + QApplication::applicationVersion() + tr(", Qt ") + QT_VERSION_STR;
    content << "";
    content << tr("Author(s): ") + author.join(", ");

    QMessageBox::about(this, tr("QVariantEditor"), content.join("<br>\n"));
}

void MainWindow::closeEvent(QCloseEvent *event)
{
    bool tabModifiedCount = 0;
    for (int i = 0; i < ui->tabWidget->count(); i++) {
        tabModifiedCount += (ui->tabWidget->widget(i)->isWindowModified())
                ? 1 : 0;
    }

    QMessageBox::StandardButton resp = QMessageBox::Yes;

    if (tabModifiedCount > 0) {
        resp = QMessageBox::question(
                    this,
                    tr("Exit"),
                    tr("Some file are not saved, quit anyway ?"),
                    QMessageBox::Yes | QMessageBox::Cancel | QMessageBox::No,
                    QMessageBox::No);
    }

    if (resp != QMessageBox::Yes)
        event->ignore();
}

void MainWindow::new_()
{
    QTreeVariantWidget* tvw = new QTreeVariantWidget;

    connect(tvw, SIGNAL(windowTitleChanged(QString)),
            this, SLOT(tabNameChanged()));
    connect(tvw, SIGNAL(widgetModified(bool)),
            this, SLOT(tabModified()));
    connect(tvw, SIGNAL(widgetModified(bool)),
            this, SLOT(setWindowModified(bool)));

    int index = ui->tabWidget->addTab(tvw, tvw->windowTitle());
    ui->tabWidget->setCurrentIndex(index);
}

void MainWindow::open()
{
    QTreeVariantWidget* tvw = qobject_cast<QTreeVariantWidget*>(
            ui->tabWidget->currentWidget());

    QString openFilename = tvw ? tvw->filename() : QString();

    if (openFilename.isEmpty()) {
        openFilename = QStandardPaths::standardLocations(
                    QStandardPaths::HomeLocation).value(0);
    }

    openFilename = QFileDialog::getOpenFileName(
                this, tr("Open file"), openFilename);

    if (!openFilename.isEmpty()) {
        tvw = new QTreeVariantWidget;

        QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));
        statusBar()->showMessage(tr("Loading %1").arg(openFilename));

        tvw->setFilename(openFilename);
        tvw->read();

        QApplication::restoreOverrideCursor();
        statusBar()->showMessage(tr("File loaded %1").arg(openFilename), 2500);

        connect(tvw, SIGNAL(windowTitleChanged(QString)),
                this, SLOT(tabNameChanged()));
        connect(tvw, SIGNAL(widgetModified(bool)),
                this, SLOT(tabModified()));
        connect(tvw, SIGNAL(widgetModified(bool)),
                this, SLOT(setWindowModified(bool)));

        int index = ui->tabWidget->addTab(tvw, tvw->windowTitle());
        ui->tabWidget->setCurrentIndex(index);
    }

}

void MainWindow::save(bool forceAsk)
{
    if (ui->tabWidget->count() <= 0)
        return;

    QTreeVariantWidget* tvw = qobject_cast<QTreeVariantWidget*>(
            ui->tabWidget->currentWidget());
    Q_ASSERT(tvw);
    bool tvw_is_new = tvw && tvw->filename().isEmpty();

    QString saveFilename = tvw->filename();

    if (saveFilename.isEmpty()) {
        saveFilename = QStandardPaths::writableLocation(
                    QStandardPaths::HomeLocation);
    }

    if (tvw_is_new || forceAsk) {
        saveFilename = QFileDialog::getSaveFileName(
                    this, tr("Save file"), saveFilename);
    }

    if (!saveFilename.isEmpty()) {
        QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));
        statusBar()->showMessage(tr("Saving %1").arg(saveFilename));

        tvw->setFilename(saveFilename);
        tvw->write();

        QApplication::restoreOverrideCursor();
        statusBar()->showMessage(tr("File saved %1").arg(saveFilename), 2500);
    }
}

void MainWindow::close()
{
    if (ui->tabWidget->count() <= 0)
        return;

    QMessageBox::StandardButton resp = QMessageBox::Yes;

    if (isWindowModified()) {
        resp = QMessageBox::question(
                    this,
                    tr("Close"),
                    tr("Some changes are not saved, close anyway ?"),
                    QMessageBox::Yes | QMessageBox::Cancel | QMessageBox::No,
                    QMessageBox::No);
    }

    if (resp == QMessageBox::Yes)
        ui->tabWidget->removeTab(ui->tabWidget->currentIndex());
}

void MainWindow::quit()
{
    QMainWindow::close();
}

//------------------------------------------------------------------------------

void MainWindow::updateWithTab(int index)
{
    if (index < 0) {
        setWindowTitle(QString());
        setWindowModified(false);
    }
    else {
        QTreeVariantWidget* tvw = qobject_cast<QTreeVariantWidget*>(
                    ui->tabWidget->widget(index));
        Q_ASSERT(tvw);

        QString tabName = tvw->windowTitle();
        if (tvw->isWindowModified())
            tabName.prepend(QString("*"));
        ui->tabWidget->setTabText(index, tabName);

        setWindowTitle(tvw->windowTitle() + QString("[*]"));
        setWindowModified(tvw->isWindowModified());
    }
}

void MainWindow::tabNameChanged()
{
    QTreeVariantWidget* tvw = qobject_cast<QTreeVariantWidget*>(sender());
    Q_ASSERT(tvw);

    int index = ui->tabWidget->indexOf(tvw);
    updateWithTab(index);
}

void MainWindow::tabModified()
{
    tabNameChanged();
}
