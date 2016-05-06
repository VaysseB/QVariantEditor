#include "mainwindow.h"
#include "ui_mainwindow.h"

#include <QFileDialog>
#include <QCloseEvent>
#include <QLabel>
#include <QMessageBox>
#include <QStandardPaths>
#include <QSettings>

#include <QDebug>

#include "qtreevariantwidget.h"


MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::mainwindow)
{
    ui->setupUi(this);

    createRecentFileActions();
    updateRecentFileActions();

    bindMenuActions();
    connectMenu();
    connect(ui->tabWidget, SIGNAL(currentChanged(int)),
            this, SLOT(updateWithTab(int)));
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::connectMenu()
{
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

void MainWindow::bindMenuActions()
{
    ui->actionNew->setShortcuts(QKeySequence::New);
    ui->actionOpen->setShortcuts(QKeySequence::Open);
    ui->actionSave->setShortcuts(QKeySequence::Save);
    ui->actionSaveAs->setShortcuts(QKeySequence::SaveAs);
    ui->actionClose->setShortcuts(QKeySequence::Close);
    ui->actionQuit->setShortcuts(QKeySequence::Quit);
    ui->actionShowSearchBox->setShortcut(QKeySequence::Find);
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

    int index = ui->tabWidget->addTab(tvw, tvw->windowTitle());
    ui->tabWidget->setCurrentIndex(index);

    connectTab(tvw);
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

    if (!openFilename.isEmpty())
        openNewTab(openFilename);
}

void MainWindow::openNewTab(const QString& filename)
{
    QTreeVariantWidget* tvw = new QTreeVariantWidget;

    QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));
    statusBar()->showMessage(tr("Loading %1").arg(filename));

    tvw->setFilename(filename);
    tvw->read();

    QApplication::restoreOverrideCursor();
    statusBar()->showMessage(tr("File loaded %1").arg(filename), 2500);

    int index = ui->tabWidget->addTab(tvw, tvw->windowTitle());
    ui->tabWidget->setCurrentIndex(index);

    connectTab(tvw);

    addToRecentFiles(filename);
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

    if (saveFilename.isEmpty())
        return;

    QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));
    statusBar()->showMessage(tr("Saving %1").arg(saveFilename));

    tvw->setFilename(saveFilename);
    tvw->write();

    QApplication::restoreOverrideCursor();
    statusBar()->showMessage(tr("File saved %1").arg(saveFilename), 2500);

    addToRecentFiles(saveFilename);
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
        setWindowModified(false);
        setWindowTitle(QString());
    }
    else {
        QTreeVariantWidget* tvw = qobject_cast<QTreeVariantWidget*>(
                    ui->tabWidget->widget(index));
        Q_ASSERT(tvw);

        QString tabName = tvw->windowTitle();
        if (tvw->isWindowModified())
            tabName.append(QString("*"));
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

void MainWindow::connectTab(QTreeVariantWidget* tvw)
{
    // widget name & state
    connect(tvw, SIGNAL(windowTitleChanged(QString)),
            this, SLOT(tabNameChanged()));
    connect(tvw, SIGNAL(widgetModified(bool)),
            this, SLOT(tabModified()));
    connect(tvw, SIGNAL(widgetModified(bool)),
            this, SLOT(setWindowModified(bool)));

    // search
    connect(ui->actionShowSearchBox, &QAction::toggled,
            tvw, &QTreeVariantWidget::setSearchVisible);
    tvw->setSearchVisible(ui->actionShowSearchBox->isChecked());

    // options
    connect(ui->actionShowOptionSidebar, &QAction::toggled,
            tvw, &QTreeVariantWidget::setOptionsVisible);
    tvw->setOptionsVisible(ui->actionShowOptionSidebar->isChecked());
}

//------------------------------------------------------------------------------

void MainWindow::openRecentFile()
{
    QAction *action = qobject_cast<QAction*>(sender());
    if (action)
        openNewTab(action->data().toString());
}

void MainWindow::clearRecentFiles()
{
    QSettings settings;
    QStringList files = settings.value("recentFileList").toStringList();

    files.clear();

    settings.setValue("recentFileList", files);

    updateRecentFileActions();
}

void MainWindow::createRecentFileActions()
{
    for (int i = 0; i < (int)MaxRecentFiles; i++) {
        m_recentFileActs[i] = new QAction(this);
        ui->menuFile->insertAction(ui->actionQuit, m_recentFileActs[i]);
        m_recentFileActs[i]->setVisible(false);
        connect(m_recentFileActs[i], &QAction::triggered,
                this, &MainWindow::openRecentFile);
    }

    mp_clearRecentFiles = new QAction(this);
    mp_clearRecentFiles->setText(tr("Clear recent files"));
    connect(mp_clearRecentFiles, &QAction::triggered,
            this, &MainWindow::clearRecentFiles);
    ui->menuFile->insertAction(ui->actionQuit, mp_clearRecentFiles);

    mp_recentFileSeparator = ui->menuFile->insertSeparator(ui->actionQuit);
}

void MainWindow::addToRecentFiles(const QString& filename)
{
    QSettings settings;
    QStringList files = settings.value("recentFileList").toStringList();

    files.removeOne(filename);
    files.prepend(filename);

    while (files.count() > (int)MaxRecentFiles)
        files.removeLast();

    settings.setValue("recentFileList", files);

    updateRecentFileActions();
}

void MainWindow::updateRecentFileActions()
{
    QSettings settings;
    QStringList files = settings.value("recentFileList").toStringList();

    int numRecentFiles = qMin(files.size(), (int)MaxRecentFiles);

    for (int i = 0; i < numRecentFiles; ++i) {
        QFileInfo fileInfo(files[i]);
        QString text = tr("&%1 %2").arg(i + 1).arg(fileInfo.fileName());
        m_recentFileActs[i]->setText(text);
        m_recentFileActs[i]->setData(files[i]);
        m_recentFileActs[i]->setVisible(true);
    }
    for (int j = numRecentFiles; j < MaxRecentFiles; ++j)
        m_recentFileActs[j]->setVisible(false);

    mp_clearRecentFiles->setVisible(numRecentFiles > 0);
    mp_recentFileSeparator->setVisible(numRecentFiles > 0);
}
