#include "mainwindow.h"
#include "ui_mainwindow.h"

#include <QFileDialog>
#include <QCloseEvent>
#include <QLabel>
#include <QMessageBox>
#include <QStandardPaths>


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
}

MainWindow::~MainWindow()
{
    delete ui;
}

//------------------------------------------------------------------------------

QMessageBox::StandardButton askToSave(QWidget* parent)
{
    QMessageBox::StandardButton resp = QMessageBox::NoButton;
    if (parent->isWindowModified()) {
        resp = QMessageBox::question(
                    parent,
                    MainWindow::tr("Save datas"),
                    MainWindow::tr("Do you wish to save ?"),
                    QMessageBox::Save | QMessageBox::Cancel | QMessageBox::No,
                    QMessageBox::Cancel);
    }
    return resp;
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
    switch (askToSave(this))
    {
    case QMessageBox::Cancel:
        event->ignore();
        break;
    case QMessageBox::Save:
        saveAll();
    default:
        break;
    }
}

void MainWindow::new_()
{
    switch (askToSave(this))
    {
    case QMessageBox::Cancel:
        return;
    case QMessageBox::Save:
        saveAll();
    default:
        break;
    }

    setWindowTitle(QString("%1 - QVariantEditor").arg(tr("untitled")));
    setWindowModified(false);
}

void MainWindow::open()
{
    switch (askToSave(this))
    {
    case QMessageBox::Cancel:
        return;
    case QMessageBox::Save:
        saveAll();
    default:
        break;
    }
}

void MainWindow::save(bool forceAsk)
{
    QString saveFilename = QStandardPaths::writableLocation(
                QStandardPaths::HomeLocation);
    if (saveFilename.isEmpty() || forceAsk) {
        saveFilename = QFileDialog::getSaveFileName(this, tr("Save file"), saveFilename);
    }

    if (!saveFilename.isEmpty()) {
        setWindowTitle(QString("%1 - QVariantEditor").arg(QDir(saveFilename).dirName()));
        setWindowModified(false);
    }
}

void MainWindow::close()
{
    switch (askToSave(this))
    {
    case QMessageBox::Cancel:
        return;
    case QMessageBox::Save:
        saveAll();
    default:
        break;
    }
}

void MainWindow::quit()
{
    switch (askToSave(this))
    {
    case QMessageBox::Cancel:
        return;
    case QMessageBox::Save:
        saveAll();
    default:
        break;
    }

    qApp->quit();
}

//------------------------------------------------------------------------------

void MainWindow::saveAll()
{
}
