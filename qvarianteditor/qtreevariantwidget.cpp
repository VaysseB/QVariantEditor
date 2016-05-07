#include "qtreevariantwidget.h"
#include "ui_qtreevariantwidget.h"

#include <QFileInfo>
#include <QDebug>

#include "qvariantdatainfo.h"


QTreeVariantWidget::QTreeVariantWidget(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::QTreeVariantWidget),
    mp_model(new QVariantModel(this))
{
    ui->setupUi(this);

    // model
    mp_model->setDynamicSort(true);
    connect(mp_model.data(), &QVariantModel::rootDatasChanged,
            this, &QTreeVariantWidget::modelDataChanged);
    connect(mp_model.data(), &QVariantModel::rootDatasChanged,
            this, &QTreeVariantWidget::updateEditMenu);

    // search field
    ui->comboFilterField->addItem(tr("All fields"));
    ui->comboFilterField->addItem(tr("Key/Index"), QVariantModel::KeyColumn);
    ui->comboFilterField->addItem(tr("Value"), QVariantModel::ValueColumn);
    ui->comboFilterField->addItem(tr("Type"), QVariantModel::TypeColumn);
    ui->comboFilterField->setCurrentIndex(0);

    // search type
    ui->comboFilterType->addItem(tr("Contains"), QVariantModel::Contains);
    ui->comboFilterType->addItem(tr("Wildcard"), QVariantModel::WildCard);
    ui->comboFilterType->addItem(tr("Regex"), QVariantModel::Regex);
    ui->comboFilterType->addItem(tr("Fixed"), QVariantModel::Fixed);
    ui->comboFilterType->setCurrentIndex(0);

    // search options
    void (QComboBox::* currentIndexChangedPtr)(int)
            = &QComboBox::currentIndexChanged;
    connect(ui->comboFilterType, currentIndexChangedPtr,
            this, &QTreeVariantWidget::searchTypeChanged);
    connect(ui->comboFilterField, currentIndexChangedPtr,
            this, &QTreeVariantWidget::searchFieldsChanged);
    searchTypeChanged(ui->comboFilterType->currentIndex());
    searchFieldsChanged(ui->comboFilterField->currentIndex());

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

    // connect search + options
    connect(ui->lineSearch, &QLineEdit::textChanged,
            mp_model.data(), &QVariantModel::setFilterText);
    connect(ui->sliderDepth, &QSlider::valueChanged,
            mp_model.data(), &QVariantModel::setDisplayDepth);

    // init with value
    setFilename(QString());
    mp_model->setRootDatas(QVariantList());
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

    QDataStream istream(&output);
    QVariantList datas;

    while (istream.atEnd() == false && istream.status() == QDataStream::Ok) {
        QVariant oneData;
        istream >> oneData;
        datas.append(oneData);
    }

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
}

void QTreeVariantWidget::searchTypeChanged(int index)
{
    int itype = ui->comboFilterType->itemData(index).toInt();
    mp_model->setFilterType((QVariantModel::FilterType) itype);
}

void QTreeVariantWidget::searchFieldsChanged(int index)
{
    QVariant vcol = ui->comboFilterField->itemData(index);
    if (vcol.isNull()) {
        mp_model->setFilterColumns(QVariantModel::KeyColumn
                                   | QVariantModel::ValueColumn
                                   | QVariantModel::TypeColumn);
    }
    else {
        mp_model->setFilterColumns((QVariantModel::Column)vcol.toInt());
    }
}

//------------------------------------------------------------------------------

void QTreeVariantWidget::modelDataChanged()
{
    // we noticed something changed
    setWindowModified(true);
    emit widgetModified(isWindowModified());
}

//------------------------------------------------------------------------------

void QTreeVariantWidget::updateEditMenu(const QVariant& datas)
{
    mp_editMenu.reset(new QMenu(ui->treeView));

    QMutableVariantDataInfo mutDInfo(datas);
    if (mutDInfo.isValid() == false)
        return;
    else if (mutDInfo.isAtomic())
        return;
    else if (mutDInfo.isContainer() == false)
        return;

    if (mutDInfo.isNewKeyInsertable()) {
        if (mutDInfo.containerKeys().isEmpty()) {
            QAction* insertNew = mp_editMenu->addAction(tr("Insert new"));

            insertNew->setIcon(QIcon::fromTheme("edit-add"));

            connect(insertNew, &QAction::triggered,
                    this, &QTreeVariantWidget::insertNew);
        }
        else {
            QAction* insertBefore = mp_editMenu->addAction(tr("Insert before"));
            QAction* insertAfter = mp_editMenu->addAction(tr("Insert after"));

            insertBefore->setIcon(QIcon::fromTheme("edit-add"));
            insertAfter->setIcon(QIcon::fromTheme("edit-add"));

            connect(insertBefore, &QAction::triggered,
                    this, &QTreeVariantWidget::insertBeforeCurrent);
            connect(insertAfter, &QAction::triggered,
                    this, &QTreeVariantWidget::insertAfterCurrent);
        }
    }
}

void QTreeVariantWidget::showEditMenu(const QPoint& pos)
{
    if (mp_editMenu.isNull())
        return;

    QPoint showPoint = ui->treeView->mapToGlobal(pos);
    showPoint.ry() += ui->treeView->header()->height();
    mp_editMenu->exec(showPoint);
}

void QTreeVariantWidget::insertNew()
{

}

void QTreeVariantWidget::insertBeforeCurrent()
{

}

void QTreeVariantWidget::insertAfterCurrent()
{

}
