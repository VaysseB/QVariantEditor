#include "qtreevariantwidget.h"
#include "ui_qtreevariantwidget.h"

#include <QFileInfo>
#include <QDebug>


QTreeVariantWidget::QTreeVariantWidget(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::QTreeVariantWidget),
    mp_sfModel(new QFullFilterProxyModel(this)),
    mp_model(new QVariantModel(this))
{
    ui->setupUi(this);

    // filter model
    mp_sfModel->setSourceModel(mp_model.data());
    mp_sfModel->setFilterRole(Qt::DisplayRole);

    ui->comboFilterType->addItem(tr("Contains"), QFullFilterProxyModel::Contains);
    ui->comboFilterType->addItem(tr("Wildcard"), QFullFilterProxyModel::WildCard);
    ui->comboFilterType->addItem(tr("Regex"), QFullFilterProxyModel::Regex);
    ui->comboFilterType->addItem(tr("Fixed"), QFullFilterProxyModel::Fixed);
    ui->comboFilterType->setCurrentIndex(0);

    void (QComboBox::* currentIndexChangedPtr)(int) = &QComboBox::currentIndexChanged;
    connect(ui->comboFilterType, currentIndexChangedPtr,
            this, &QTreeVariantWidget::searchTypeChanged);

    // tree view options
    ui->treeView->setModel(mp_sfModel.data());
    ui->treeView->setWordWrap(true);
    ui->treeView->setAlternatingRowColors(true);
    ui->treeView->setDragDropOverwriteMode(false);
    ui->treeView->setProperty("showDropIndicator", QVariant(false));
    ui->treeView->setUniformRowHeights(false);

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

    // connect search
    connect(ui->lineSearch, &QLineEdit::textChanged,
            mp_sfModel.data(), &QSortFilterProxyModel::setFilterWildcard);

    // connect options
    connect(ui->sliderDepth, &QSlider::valueChanged,
            mp_model.data(), &QVariantModel::setDisplayDepth);

    // init with value
    setFilename(QString());
    setWindowModified(true);
    emit widgetModified(isWindowModified());


    // todo remove
    QVariantList rootD;
    rootD.append(QVariant(5));
    rootD.append(QVariant("string"));

    QVariantList l1;
    l1.append(QVariant(0));
    l1.append(QVariant(5));
    l1.append(QVariant(10));
    l1.append(QVariant(15));

    QVariantList l2;
    l2.append(QVariant("first"));
    l2.append(QVariant("second"));
    l2.append(QVariant("third"));
    l2.append(QVariant("fourth"));

    QVariantMap h1;
    h1.insert("int", QVariant(-54687));
    h1.insert("string", QVariant("name"));
    h1.insert("bool", QVariant(true));

    QVariantHash h2;
    h2.insert("string", QVariant("sub"));
    h2.insert("map", QVariant(h1));
    h2.insert("int", QVariant(INT_MAX));

    rootD.append(QVariant(l1));
    rootD.append(QVariant(l2));
    rootD.append(QVariant(h2));

    mp_model->setRootData(QVariant(rootD));
}

QTreeVariantWidget::~QTreeVariantWidget()
{
    delete ui;
}

void QTreeVariantWidget::setFilename(const QString& name)
{
    m_filename = name;
    setWindowTitle((m_filename.isEmpty())
                   ? tr("untitled")
                   : QFileInfo(name).fileName());
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
    QFullFilterProxyModel::FilterType ftype =
            (QFullFilterProxyModel::FilterType)ui->comboFilterType->itemData(index).toInt();
    mp_sfModel->setFilterType(ftype);
}
