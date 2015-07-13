#include "mainwindow.h"
#include "ui_mainwindow.h"

#include <QFileDialog>
#include <QCloseEvent>

#include "project.h"
#include "qvarianttreeitemmodel.h"
#include "qvariantitemdelegate.h"


MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::mainwindow),
    _permanentMessage(),
    _currentFilePath()
{
    ui->setupUi(this);

    // init window
    clear();
    reloadUI();
    reloadMenu();

    // signal to action
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

    connect(ui->actionAdd, SIGNAL(triggered()),
            ui->tableBrowser, SLOT(insertValue()));
    connect(ui->actionRemove, SIGNAL(triggered()),
            ui->tableBrowser, SLOT(deleteValue()));

    connect(ui->actionAbout, SIGNAL(triggered()),
            this, SLOT(about()));

    connect(ui->buttonBack, SIGNAL(clicked()),
            ui->tableBrowser, SLOT(openParent()));

    // signal to register modification
    connect(model(), SIGNAL(valueKeyChanged(QVariant,QVariant)),
            this, SLOT(modelChanged()));
    connect(model(), SIGNAL(valueContentChanged(QVariant)),
            this, SLOT(modelChanged()));
    connect(model(), SIGNAL(valueTypeChanged(QVariant::Type,QVariant::Type)),
            this, SLOT(modelChanged()));
    connect(model(), SIGNAL(insertedValue(QVariant)),
            this, SLOT(modelChanged()));
    connect(model(), SIGNAL(deletedValue(QVariant)),
            this, SLOT(modelChanged()));

    // signal to update UI
    connect(ui->tableBrowser, SIGNAL(movedToChild(QVariant)),
            this, SLOT(fullReload()));
    connect(ui->tableBrowser, SIGNAL(movedToParent()),
            this, SLOT(fullReload()));
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
    content << (QString("<h2>") + PROJECT_NAME + "</h2>")
               + tr("QVariant file explorer and editor.");
    content << tr("Version: ") + STR_VERSION + tr(", Qt ") + QT_VERSION_STR;
    content << "";
    content << tr("Author(s): ") + author.join(", ");

    QMessageBox::about(this, tr("QVariantEditor"), content.join("<br>\n"));
}

//------------------------------------------------------------------------------

void MainWindow::showStatusMessage(QString message,
                                   MainWindow::MessageDisplayType type,
                                   int timeout)
{
    switch(type)
    {
    case MainWindow::ClearAndShowTemporary:
        _permanentMessage.clear();
    case MainWindow::ShowTemporary:
        statusBar()->showMessage(message, timeout);
        break;
    case MainWindow::ShowPermanent:
        _permanentMessage = message;
        statusBar()->showMessage(message, 0);
        break;
    }

    qApp->processEvents(QEventLoop::AllEvents, 500);
}

void MainWindow::clearStatusTemporaryMessage()
{
    statusBar()->clearMessage();
    if (!_permanentMessage.isEmpty())
        statusBar()->showMessage(_permanentMessage);
}

void MainWindow::clearStatusPermanentMessage()
{
    _permanentMessage.clear();
    statusBar()->clearMessage();
}

//------------------------------------------------------------------------------

void MainWindow::clear()
{
    ui->tableBrowser->clearTree();
    _currentFilePath.clear();

    // reset title and modification flag
    setTitle();
    setWindowModified(false);
}

void MainWindow::closeEvent(QCloseEvent *event)
{
    if (!quit())
        event->ignore();
    else
        QMainWindow::closeEvent(event);
}

//------------------------------------------------------------------------------

void MainWindow::new_()
{
    if (!askBeforeLoseDatas(tr("Save datas"),
                            tr("Do you want to save changes ?"),
                            QMessageBox::No) == QMessageBox::Cancel)
        return;

    clear();

    setTitle("");
    model()->setTreeContent(QVariantList());

    reloadUI();
    reloadMenu();
}

void MainWindow::open()
{
    if (askBeforeLoseDatas(tr("Save datas"),
                           tr("Do you want to save changes ?"),
                           QMessageBox::No) == QMessageBox::Cancel)
        return;

    QString openFilename = _currentFilePath;
    openFilename = QFileDialog::getOpenFileName(this, tr("Open file"), openFilename);
    if (!openFilename.isEmpty()) {
        _currentFilePath = openFilename;

        showStatusMessage(tr("Loading from \"%1\" ...").arg(_currentFilePath),
                          MainWindow::ShowTemporary);

        model()->open(_currentFilePath);
        ui->tableBrowser->adaptColumnWidth();

        setWindowModified(false);
        setTitle(QDir(_currentFilePath).dirName());

        reloadUI();
        reloadMenu();

        showStatusMessage(tr("\"%1\" loaded.")
                          .arg(QDir(_currentFilePath).dirName()),
                          MainWindow::ShowTemporary, 2000);
    }
}

void MainWindow::save(bool force)
{
    if (model()->isEmpty())
        return;

    QString saveFilename = _currentFilePath;
    if (saveFilename.isEmpty() || force) {
        saveFilename = QFileDialog::getSaveFileName(this, tr("Save file"), saveFilename);
    }
    if (!saveFilename.isEmpty()) {
        _currentFilePath = saveFilename;

        showStatusMessage(tr("Saving to \"%1\" ...").arg(_currentFilePath),
                          MainWindow::ShowTemporary);

        model()->save(_currentFilePath);
        setWindowModified(false);
        setTitle(QDir(_currentFilePath).dirName());

        showStatusMessage(tr("\"%1\" saved.")
                          .arg(QDir(_currentFilePath).dirName()),
                          MainWindow::ShowTemporary, 2000);
    }
}

void MainWindow::close()
{
    if (askBeforeLoseDatas(tr("Save datas"),
                           tr("Do you want to save before closing ?"),
                           QMessageBox::Close) == QMessageBox::Cancel)
        return;

    clear();
    reloadUI();
    reloadMenu();
}

bool MainWindow::quit()
{
    if (askBeforeLoseDatas(tr("Save datas"),
                           tr("Do you want to save before quit ?"),
                           QMessageBox::Close) == QMessageBox::Cancel)
        return false;
    qApp->quit();
    return true;
}

QMessageBox::StandardButton MainWindow::askBeforeLoseDatas(
        const QString& title,
        const QString& question,
        QMessageBox::StandardButton actionButton)
{
    QMessageBox::StandardButton resp = QMessageBox::NoButton;
    if (isWindowModified() && !model()->isEmpty()) {
        resp = QMessageBox::question(this, title, question,
                                     QMessageBox::Save |
                                     QMessageBox::Cancel |
                                     actionButton,
                                     QMessageBox::Cancel);
        if (resp == QMessageBox::Save)
            save();
    }
    return resp;
}

//------------------------------------------------------------------------------

void MainWindow::setTitle(QString title)
{
    if (title.isNull())
        setWindowTitle(PROJECT_NAME);
    else if (title.isEmpty())
        setWindowTitle(tr("new") + "[*] - " + PROJECT_NAME);
    else
        setWindowTitle(tr("%1").arg(title) + "[*] - " + PROJECT_NAME);

}

void MainWindow::reloadUI()
{
    if (!model()->isEmpty())
    {
        ui->buttonBack->setEnabled(true);
        ui->labelNavigation->setEnabled(true);
        ui->tableBrowser->setEnabled(true);

        // ADRESSE
        if (model()->tree().address().isEmpty()) {
            ui->labelNavigation->setText(tr("Address: <Root>"));
            ui->buttonBack->setEnabled(false);
        }
        else {
            ui->labelNavigation->setText(tr("Address: ") + displayableAddress().join(tr(" > ")));
            ui->buttonBack->setEnabled(true);
        }
    }
    else
    {
        ui->labelNavigation->setText("");
        ui->buttonBack->setEnabled(false);
        ui->labelNavigation->setEnabled(false);
        ui->tableBrowser->setEnabled(false);
    }

}

void MainWindow::reloadMenu()
{
    if (!model()->isEmpty())
    {
        ui->actionSave->setEnabled(true);
        ui->actionSaveAs->setEnabled(true);
        ui->actionClose->setEnabled(true);

        ui->actionAdd->setEnabled(true);
        ui->actionRemove->setEnabled(true);
    }
    else
    {
        ui->actionSave->setEnabled(false);
        ui->actionSaveAs->setEnabled(false);
        ui->actionClose->setEnabled(false);

        ui->actionAdd->setEnabled(false);
        ui->actionRemove->setEnabled(false);
    }
}

QStringList MainWindow::displayableAddress() const
{
    QStringList list;
    QVariantList address = model()->tree().address();
    for (int i=0; i<address.size(); i++) {
        QVariant key = address.value(i);
        QString strKey = model()->keyToString(key);
        if (key.type() == QVariant::Int) {
            strKey = tr("#%1").arg(strKey);
        }
        list << strKey;
    }
    return list;
}

//------------------------------------------------------------------------------

QVariantTreeItemModel* MainWindow::model() const
{
    return qobject_cast<QVariantTreeItemModel*>(ui->tableBrowser->model());
}

void MainWindow::modelChanged()
{
    setWindowModified(true);
}

//------------------------------------------------------------------------------

void MainWindow::fullReload()
{
    reloadUI();
}
