#include "qtreevariantwidget.h"
#include "ui_qtreevariantwidget.h"

#include <QFileInfo>
#include <QDebug>

#include "qtreevariantitemdelegate.h"
#include "qvariantdatainfo.h"


QTreeVariantWidget::QTreeVariantWidget(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::QTreeVariantWidget),
    mp_model(new QVariantModel(this))
{
    ui->setupUi(this);

    createEditMenus();

    // model
    mp_model->setDynamicSort(true);
    connect(mp_model.data(), &QVariantModel::rootDatasChanged,
            this, &QTreeVariantWidget::modelDataChanged);

    // search field
    ui->comboFilterField->addItem(tr("All fields"));
    ui->comboFilterField->addItem(tr("Key/Index"), QVariantModel::KeyColumn);
    ui->comboFilterField->addItem(tr("Value"), QVariantModel::ValueColumn);
    ui->comboFilterField->addItem(tr("Type"), QVariantModel::TypeColumn);
    ui->comboFilterField->setCurrentIndex(0);

    // search type
    ui->comboFilterType->addItem(
                qApp->translate(QVariantModel::staticMetaObject.className(),
                                QVariantModel::FILTER_TYPE_NAMES[QVariantModel::Contains]),
            QVariantModel::Contains);
    ui->comboFilterType->addItem(
                qApp->translate(QVariantModel::staticMetaObject.className(),
                                QVariantModel::FILTER_TYPE_NAMES[QVariantModel::WildCard]),
            QVariantModel::WildCard);
    ui->comboFilterType->addItem(
                qApp->translate(QVariantModel::staticMetaObject.className(),
                                QVariantModel::FILTER_TYPE_NAMES[QVariantModel::Regex]),
            QVariantModel::Regex);
    ui->comboFilterType->addItem(
                qApp->translate(QVariantModel::staticMetaObject.className(),
                                QVariantModel::FILTER_TYPE_NAMES[QVariantModel::Fixed]),
            QVariantModel::Fixed);
    ui->comboFilterType->setCurrentIndex(0);

    void (QComboBox::* currentIndexChangedPtr)(int)
            = &QComboBox::currentIndexChanged;
    void (QTimer::* startPtr)()
            = &QTimer::start;

    // search timer
    m_searchUpdateTimer.setSingleShot(false);
    m_searchUpdateTimer.setInterval(300); // time before search is updated
    connect(&m_searchUpdateTimer, &QTimer::timeout,
            this, &QTreeVariantWidget::updateSearch);
    connect(ui->lineSearch, &QLineEdit::textChanged,
            &m_searchUpdateTimer, startPtr);
    connect(ui->comboFilterType, currentIndexChangedPtr,
            &m_searchUpdateTimer, startPtr);
    connect(ui->comboFilterField, currentIndexChangedPtr,
            &m_searchUpdateTimer, startPtr);

    // tree view options
    ui->treeView->setModel(mp_model.data());
    ui->treeView->setExpandsOnDoubleClick(true);
    ui->treeView->setWordWrap(true);
    ui->treeView->setAlternatingRowColors(true);
    ui->treeView->setDragDropOverwriteMode(false);
    ui->treeView->setProperty("showDropIndicator", QVariant(false));
    ui->treeView->setUniformRowHeights(false);

    // tree view edit triggers
    ui->treeView->setEditTriggers(QTreeView::DoubleClicked
                                  | QTreeView::EditKeyPressed);
    ui->treeView->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(ui->treeView, &QTreeView::customContextMenuRequested,
            this, &QTreeVariantWidget::showEditMenu);

    // tree view delegate
    ui->treeView->setItemDelegate(new QTreeVariantItemDelegate(this));

    // tree view header options
    QHeaderView* header = ui->treeView->header();
    header->setStretchLastSection(false);
    header->setSectionsMovable(true);
    header->setSectionResizeMode(mp_model->column(QVariantModel::KeyColumn),
                                 QHeaderView::Interactive);
    header->setSectionResizeMode(mp_model->column(QVariantModel::ValueColumn),
                                 QHeaderView::Stretch);
    header->setSectionResizeMode(mp_model->column(QVariantModel::TypeColumn),
                                 QHeaderView::ResizeToContents);
    header->setMinimumSectionSize(40);
    header->setDefaultSectionSize(120);

    // connect options
    connect(ui->sliderDepth, &QSlider::valueChanged,
            mp_model.data(), &QVariantModel::setDisplayDepth);

    // init with value
    setFilename(QString());
    mp_model->setRootDatas(QVariantList());

    // search options
    updateSearch();
}

QTreeVariantWidget::~QTreeVariantWidget()
{
    delete ui;
}

//------------------------------------------------------------------------------

void QTreeVariantWidget::setFilename(const QString& name)
{
    m_filename = name;
    setWindowTitle((m_filename.isEmpty())
                   ? tr("untitled")
                   : QFileInfo(name).fileName());
}

void QTreeVariantWidget::read()
{
    if (m_filename.isEmpty())
        return;

    QFile output(m_filename);
    if (output.open(QIODevice::ReadOnly) == false) {
        qWarning("cannot open file to read %s", qPrintable(m_filename));
        return;
    }

    qDebug() << "begin to read datas from" << QFileInfo(m_filename).fileName();

    QDataStream istream(&output);
    QVariantList datas;

    while (istream.atEnd() == false && istream.status() == QDataStream::Ok) {
        QVariant oneData;
        istream >> oneData;
        datas.append(oneData);
    }

    qDebug() << "done read datas from" << QFileInfo(m_filename).fileName();

    switch (istream.status()) {
    case QDataStream::ReadPastEnd:
        qWarning("read past end while reading %s", qPrintable(m_filename));
        return;
    case QDataStream::ReadCorruptData:
        qWarning("read corrupted datas in %s", qPrintable(m_filename));
        return;
    default:
        break;
    }

    mp_model->setRootDatas(datas);

    output.close();

    setWindowModified(false);
    emit widgetModified(isWindowModified());
}

void QTreeVariantWidget::write()
{
    if (m_filename.isEmpty())
        return;

    QFile output(m_filename);
    if (output.open(QIODevice::WriteOnly | QIODevice::Truncate) == false) {
        qWarning("cannot open file to write %s", qPrintable(m_filename));
        return;
    }

    qDebug() << "begin to writo datas to" << QFileInfo(m_filename).fileName();

    QDataStream ostream(&output);
    QVariantList datas = mp_model->rootDatas()  ;
    for (auto it = datas.constBegin(); it != datas.constEnd()
         && ostream.status() == QDataStream::Ok; ++it) {
        ostream << *it;
    }

    if (ostream.status() == QDataStream::WriteFailed) {
        qWarning("write failed to %s", qPrintable(m_filename));
        return;
    }

    output.close();

    qDebug() << "done write datas to" << QFileInfo(m_filename).fileName();

    // write succeed
    setWindowModified(false);
    emit widgetModified(isWindowModified());
}

//------------------------------------------------------------------------------

void QTreeVariantWidget::setOptionsVisible(bool visible)
{
    ui->widgetOptions->setVisible(visible);
}

void QTreeVariantWidget::setSearchVisible(bool visible)
{
    ui->widgetSearch->setVisible(visible);

    updateSearch();
}

void QTreeVariantWidget::updateSearch()
{
    m_searchUpdateTimer.stop();

    // cancel search
    mp_model->setFilterText(QString());

    bool searchEnabled = ui->widgetSearch->isVisible();
    if (searchEnabled == false)
        return;

    // filter type
    int indexType = ui->comboFilterType->currentIndex();
    int itype = ui->comboFilterType->itemData(indexType).toInt();
    QVariantModel::FilterType fType = (QVariantModel::FilterType) itype;
    if (fType != mp_model->filterType())
        mp_model->setFilterType(fType);

    // filter columns
    int indexField = ui->comboFilterField->currentIndex();
    QVariant vcol = ui->comboFilterField->itemData(indexField);
    int fColumns = 0;
    if (vcol.isNull()) {
        fColumns = QVariantModel::KeyColumn
                | QVariantModel::ValueColumn
                | QVariantModel::TypeColumn;
    }
    else
        fColumns = vcol.toInt();
    if (fColumns != mp_model->filterColumns())
        mp_model->setFilterColumns((QVariantModel::Columns)fColumns);

    // filter text
    QString fText = ui->lineSearch->text();
    if (fText.isEmpty() == false)
        mp_model->setFilterText(fText);
}

//------------------------------------------------------------------------------

void QTreeVariantWidget::modelDataChanged()
{
    // we noticed something changed
    setWindowModified(true);
    emit widgetModified(isWindowModified());
}

//------------------------------------------------------------------------------

void QTreeVariantWidget::createEditMenus()
{
    mp_blankEditMenu.reset(new QMenu(ui->treeView));
    mp_selEditMenu.reset(new QMenu(ui->treeView));
    mp_selContainerEditMenu.reset(new QMenu(ui->treeView));

    // blank menu
    {
        QAction* addNew = mp_blankEditMenu->addAction(tr("Add new"));

        addNew->setIcon(QIcon::fromTheme("list-add"));

        connect(addNew, &QAction::triggered,
                this, &QTreeVariantWidget::insertNew);
    }

    // selection menu
    {
        QAction* insertBefore = mp_selEditMenu->addAction(tr("Insert before"));
        QAction* insertAfter = mp_selEditMenu->addAction(tr("Insert after"));
        QAction* remove = mp_selEditMenu->addAction(tr("Remove"));

        mp_selEditMenu->insertSeparator(remove);

        insertBefore->setIcon(QIcon::fromTheme("list-add"));
        insertAfter->setIcon(QIcon::fromTheme("list-add"));
        remove->setIcon(QIcon::fromTheme("list-remove"));

        connect(insertBefore, &QAction::triggered,
                this, &QTreeVariantWidget::insertBeforeCurrent);
        connect(insertAfter, &QAction::triggered,
                this, &QTreeVariantWidget::insertAfterCurrent);
        connect(remove, &QAction::triggered,
                this, &QTreeVariantWidget::removeCurrent);
    }

    // selection menu
    {
        QAction* insertInto = mp_selContainerEditMenu->addAction(tr("Insert into"));
        QAction* insertBefore = mp_selContainerEditMenu->addAction(tr("Insert before"));
        QAction* insertAfter = mp_selContainerEditMenu->addAction(tr("Insert after"));
        QAction* remove = mp_selContainerEditMenu->addAction(tr("Remove"));

        mp_selContainerEditMenu->insertSeparator(insertBefore);
        mp_selContainerEditMenu->insertSeparator(remove);

        insertInto->setIcon(QIcon::fromTheme("list-add"));
        insertBefore->setIcon(QIcon::fromTheme("list-add"));
        insertAfter->setIcon(QIcon::fromTheme("list-add"));
        remove->setIcon(QIcon::fromTheme("list-remove"));

        connect(insertInto, &QAction::triggered,
                this, &QTreeVariantWidget::insertIntoCurrent);
        connect(insertBefore, &QAction::triggered,
                this, &QTreeVariantWidget::insertBeforeCurrent);
        connect(insertAfter, &QAction::triggered,
                this, &QTreeVariantWidget::insertAfterCurrent);
        connect(remove, &QAction::triggered,
                this, &QTreeVariantWidget::removeCurrent);
    }
}

void QTreeVariantWidget::showEditMenu(const QPoint& pos)
{
    QMenu* menu = nullptr;

    // nothing is selected
    if (ui->treeView->selectionModel()->selection().isEmpty())
        menu = mp_blankEditMenu.data();
    // the user have selected something
    else {
        QModelIndex posIndex = ui->treeView->currentIndex();

        // if the item can not have children
        if (mp_model->flags(posIndex) & Qt::ItemNeverHasChildren)
            menu = mp_selEditMenu.data();
        else
            menu = mp_selContainerEditMenu.data();
    }

    if (menu == nullptr)
        return;

    QPoint showPoint = ui->treeView->mapToGlobal(pos);
    showPoint.ry() += ui->treeView->header()->height();
    showPoint += QPoint(2, 2);
    menu->exec(showPoint);
}

void QTreeVariantWidget::insertNew()
{
    int rowInsert = mp_model->rowCount();
    bool isDataInserted = mp_model->insertRows(rowInsert, 1, QModelIndex());
    Q_UNUSED(isDataInserted); // avoid warning if no assert
    Q_ASSERT(isDataInserted);
}

void QTreeVariantWidget::insertIntoCurrent()
{
    QModelIndex insertIndex = ui->treeView->currentIndex();
    bool isDataInserted = mp_model->insertRows(0, 1, insertIndex);
    Q_UNUSED(isDataInserted); // avoid warning if no assert
    Q_ASSERT(isDataInserted);
}

void QTreeVariantWidget::insertBeforeCurrent()
{
    QModelIndex insertIndex = ui->treeView->currentIndex();
    bool isDataInserted = mp_model->insertRows(
                insertIndex.row(), 1, insertIndex.parent());
    Q_UNUSED(isDataInserted); // avoid warning if no assert
    Q_ASSERT(isDataInserted);
}

void QTreeVariantWidget::insertAfterCurrent()
{
    QModelIndex insertIndex = ui->treeView->currentIndex();
    bool isDataInserted = mp_model->insertRows(
                insertIndex.row()+1, 1, insertIndex.parent());
    Q_UNUSED(isDataInserted); // avoid warning if no assert
    Q_ASSERT(isDataInserted);
}

void QTreeVariantWidget::removeCurrent()
{
    QModelIndexList removeIndexes = ui->treeView->selectionModel()
            ->selectedIndexes();

    QList<QModelIndex> parents;
    QMultiMap<QModelIndex, int> rowsParent;
    for (auto it = removeIndexes.constBegin();
         it != removeIndexes.constEnd(); ++it) {
        parents.append(it->parent());
        rowsParent.insertMulti(it->parent(), it->row());
    }

    std::sort(parents.begin(), parents.end(), [this]
              (const QModelIndex& left, const QModelIndex& right) {
        int lparentCount = 0;
        QModelIndex lparent = left;
        if ((lparent = lparent.parent()).isValid())
            lparentCount++;

        int rparentCount = 0;
        QModelIndex rparent = left;
        if ((rparent = rparent.parent()).isValid())
            rparentCount++;

        if (lparentCount != rparentCount)
            return lparentCount < rparentCount;
        return left.row() < right.row();
    });

    while (parents.isEmpty() == false) {
        QModelIndex parent = parents.takeLast();
        QList<int> rowsOfParent = rowsParent.values(parent);
        rowsParent.remove(parent);

        std::sort(rowsOfParent.begin(), rowsOfParent.end());

        bool parentWasValid = parent.isValid();
        while (rowsOfParent.isEmpty() == false
               && parent.isValid() == parentWasValid) {
            int lastRow = rowsOfParent.takeLast();
            int firstRow = lastRow;

            while (rowsOfParent.isEmpty() == false
                   && rowsOfParent.last() == firstRow-1) {
                rowsOfParent.removeLast();
                firstRow--;
            }

            bool isDataRemoved = mp_model->removeRows(
                        firstRow, (lastRow - firstRow + 1), parent);
            Q_UNUSED(isDataRemoved); // avoid warning if no assert
            Q_ASSERT(isDataRemoved);
        }
    }
}

//------------------------------------------------------------------------------

int QTreeVariantWidget::searchUpdateInterval() const
{
    return m_searchUpdateTimer.interval();
}

void QTreeVariantWidget::setSearchUpdateInterval(int msec)
{
    m_searchUpdateTimer.setInterval(msec);
    emit searchUpdateIntervalChanged(msec);
}
