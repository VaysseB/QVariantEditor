#include "qtablevarianttree.h"

#include <QHeaderView>
#include <QKeyEvent>

#include "qvariantitemdelegate.h"


QTableVariantTree::QTableVariantTree(QWidget *parent) :
    QTableView(parent),
    _model(),
    _selectedRowsPath()
{
    setObjectName(QStringLiteral("QTableVariantTree"));

    // model
    setModel(&_model);

    // delegate
    setItemDelegate(new QVariantItemDelegate(this));

    // edition
    setEditTriggers(
                QAbstractItemView::DoubleClicked |
                QAbstractItemView::EditKeyPressed);

    // header
    QHeaderView* header = horizontalHeader();
    header->setStretchLastSection(false);
    header->setSectionResizeMode(0, QHeaderView::Fixed);
    header->setSectionResizeMode(1, QHeaderView::Fixed);
    header->setSectionResizeMode(2, QHeaderView::Fixed);
    header->setDefaultSectionSize(60);
    header->setMinimumSectionSize(60);
    adaptColumnWidth();

    // scroll
    setVerticalScrollMode(QAbstractItemView::ScrollPerPixel);
    setHorizontalScrollMode(QAbstractItemView::ScrollPerPixel);

    // view
    setAlternatingRowColors(true);
    setDragDropOverwriteMode(false);
    setProperty("showDropIndicator", QVariant(false));
    setWordWrap(true);

    // signal editor
    connect(this, SIGNAL(doubleClicked(QModelIndex)),
            this, SLOT(openChild(QModelIndex)));
}

//------------------------------------------------------------------------------

void QTableVariantTree::clear()
{
    _selectedRowsPath.clear();
    selectionModel()->clear();
    _model.clear();
}

void QTableVariantTree::clearTree()
{
    _selectedRowsPath.clear();
    selectionModel()->clear();
    _model.clearTree();
}

//------------------------------------------------------------------------------

void QTableVariantTree::keyPressEvent(QKeyEvent* event)
{
    QTableView::keyPressEvent(event);
    // if event exist, not accepted, and table not in edition
    if (event && !event->isAccepted() &&
            state() != QAbstractItemView::EditingState)
    {
        // back to parent
        if (event->key() == Qt::Key_Backspace) {
            // if already on root -> RAF
            if (!_model.tree().nodeIsRoot()) {
                event->accept();
                openParent();
            }
        }
        // open child OR open editor
        else if (event->key() == Qt::Key_Return) {
            event->accept();

            int adrSizePrev = _model.tree().address().size();
            openChild(currentIndex());
            int adrSizeNext = _model.tree().address().size();

            // if not moved -> try edit
            if (adrSizePrev == adrSizeNext) {
                QTableView::edit(currentIndex());
            }
        }
    }
}

//------------------------------------------------------------------------------

void QTableVariantTree::openChild(const QModelIndex& index)
{
    if (!index.isValid() ||
            index.column() != _model.columnValue())
        return;

    openChild(index.row());
}

void QTableVariantTree::openChild(int row)
{
    if (row < 0)
        return;

    QVariantList item = _model.rawDatas(row);
    // deplacement SI list ou collection
    if (QVariantTree::typeIsList(item.value(_model.columnType()).type()) ||
            QVariantTree::typeIsCollection(item.value(_model.columnType()).type()))
    {
        _selectedRowsPath.append(row);
        selectionModel()->clear();
        setCurrentIndex(QModelIndex());

        _model.moveToChild(item.value(_model.columnKey()));
        emit movedToChild(item.value(_model.columnKey()));

        adaptColumnWidth();
    }
}

void QTableVariantTree::openParent()
{
    selectionModel()->clear();
    setCurrentIndex(QModelIndex());

    _model.moveToParent();

    QModelIndex selectAndVisibleIndex = model()->index(
                _selectedRowsPath.takeLast(),
                model()->columnValue());
    scrollTo(selectAndVisibleIndex, QAbstractItemView::PositionAtCenter);
    setCurrentIndex(selectAndVisibleIndex);

    emit movedToParent();

    adaptColumnWidth();
}

//------------------------------------------------------------------------------
// Calls for adaptColumnWidth

void QTableVariantTree::dataChanged(const QModelIndex &topLeft,
                                    const QModelIndex &bottomRight,
                                    const QVector<int> &roles)
{
    QTableView::dataChanged(topLeft, bottomRight, roles);
    adaptColumnWidth();
}

void QTableVariantTree::rowsAboutToBeRemoved(const QModelIndex &parent,
                                             int start,
                                             int end)
{
    QTableView::rowsAboutToBeRemoved(parent, start, end);
    adaptColumnWidth();
}

void QTableVariantTree::rowsInserted(const QModelIndex &parent,
                                     int start,
                                     int end)
{
    QTableView::rowsInserted(parent, start, end);
    adaptColumnWidth();
}

void QTableVariantTree::resizeEvent(QResizeEvent *event)
{
    QTableView::resizeEvent(event);
    adaptColumnWidth();
}

//------------------------------------------------------------------------------

void QTableVariantTree::adaptColumnWidth()
{
    resizeColumnToContents(model()->columnKey());
    resizeColumnToContents(model()->columnType());

    int width = viewport()->size().width() -
            columnWidth(model()->columnKey()) -
            columnWidth(model()->columnType());

    int minWidth = horizontalHeader()->minimumSectionSize();
    if (width < minWidth)
        width = minWidth;

    horizontalHeader()->resizeSection(model()->columnValue(), width);
}

//------------------------------------------------------------------------------

void QTableVariantTree::insertValue()
{
    // looking where to insert value, under the given selection
    QModelIndex index;
    if (!selectionModel()->selectedIndexes().isEmpty())
        index = currentIndex();

    // number of rows before insertion
    int nbRows = model()->rowCount();
    // the current inserted row
    int row = index.row();
    // the current key
    QVariant key;

    // creating the value
    QVariant newValue;
    if (row >= 0 && nbRows > 0)
        newValue = createValue(model()->rawData(row, model()->columnType()).type());

    if (model()->tree().nodeIsList()) {
        // insertion after the current row
        // if not last, insertion the row after the row selected
        if (row >= 0 && row+1 < nbRows)
            row++;
        // if the real inserted row is the last, we make sure it not failed
        // insertItemList(size()) => error
        else if (row >=0 && row+1 == nbRows)
            row = -1;

        // add into list
        model()->tree().insertItemList(row, newValue);

        // if append to the end -> getting the true index
        if (row < 0)
            row = model()->tree().itemListSize() -1;
        key = row;
    }
    else if (model()->tree().nodeIsCollection()) {
        // getting all keys
        QStringList allKeys = model()->tree().itemCollectionKeys();
        // looking for the key
        QString itemStrKey;
        if (itemStrKey >= 0)
            itemStrKey = allKeys.value(row);
        else if (!allKeys.isEmpty())
            itemStrKey = allKeys.last();
        else
            itemStrKey = "";


        // if the key is already known
        if (allKeys.contains(itemStrKey)) {
            QString model = itemStrKey;
            // if the key contains already an offset number
            // we remove it
            int indexOfSplit = model.lastIndexOf("-");
            if (indexOfSplit >= 0) {
                model = model.mid(0, indexOfSplit);
            }

            // loop through number
            // +2 because if model without number may not exists
            for (int i=1; i<nbRows+2 && allKeys.contains(itemStrKey); i++) {
                itemStrKey = model + "-" + QString::number(i);
            }
        }

        // add into collection
        model()->tree().setItemCollection(itemStrKey, newValue);

        // retrieving true row index
        row = model()->tree().itemCollectionKeys().indexOf(itemStrKey);

        key = itemStrKey;
    }

    model()->silentUpdateContentFromTree();
    if (model()->rowCount() != nbRows) {
        model()->updateTableAfterInsertDelete(nbRows);

        selectionModel()->clearSelection();
        selectRow(row);

        emit model()->insertedValue(key);
    }
}

void QTableVariantTree::deleteValue()
{
    QSet<int> setRows;
    Q_FOREACH(QModelIndex index, selectionModel()->selectedIndexes())
        setRows.insert(index.row());

    QList<int> rows = setRows.toList();
    std::sort(rows.begin(), rows.end());
    std::reverse(rows.begin(), rows.end());

    int nbRows = model()->rowCount();

    if (model()->tree().nodeIsList()) {
        Q_FOREACH(int row, rows) {
            model()->tree().delItemList(row);
            emit model()->deletedValue(row);
            model()->silentUpdateContentFromTree();
        }
    }
    else if (model()->tree().nodeIsCollection()) {
        QStringList keys = model()->tree().itemCollectionKeys();
        Q_FOREACH(int row, rows) {
            model()->tree().delItemCollection(keys.value(row));
            emit model()->deletedValue(keys.value(row));
            model()->silentUpdateContentFromTree();
        }
    }

    model()->updateTableAfterInsertDelete(nbRows);

    selectionModel()->clearSelection();
    setCurrentIndex(QModelIndex());
}

//------------------------------------------------------------------------------

QVariant QTableVariantTree::createValue(QVariant::Type type) const
{
    QVariant result;

    switch(type)
    {
    case QVariant::Invalid:
        break;
    case QVariant::Bool:
        result = false;
        break;
    case QVariant::UInt:
        result = (uint)0;
        break;
    case QVariant::Int:
        result = (int)0;
        break;
    case QVariant::Double:
        result = (double)0.0;
        break;
    case QVariant::String:
        result = QString();
        break;
    case QVariant::List:
        result = QVariantList();
        break;
    case QVariant::Map:
        result = QVariantMap();
        break;
    case QVariant::Hash:
        result = QVariantHash();
        break;
    default:
        break;
    }

    return result;
}
