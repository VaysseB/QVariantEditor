#include "qvarianttreeitemmodel.h"

#include <QSize>


QVariantTreeItemModel::QVariantTreeItemModel(QObject *parent) :
    QAbstractTableModel(parent),
    _tree(),
    _content(),
    _isEmpty(true),
    _typesName()
{

    //
    _typesName[QVariant::Invalid]  = "Invalid";
    _typesName[QVariant::Bool]     = "Boolean";
    _typesName[QVariant::Int]      = "Integer";
    _typesName[QVariant::UInt]     = "Unsigned integer";
    _typesName[QVariant::Double]   = "Double";
    _typesName[QVariant::String]   = "String";
    _typesName[QVariant::List]     = "List";
    _typesName[QVariant::Map]      = "Map";
    _typesName[QVariant::Hash]     = "Hash";
}

//------------------------------------------------------------------------------
// Tree

void QVariantTreeItemModel::open(QIODevice* file)
{
    _tree.setFromFile(file);

    updateModelFromTree();
}

void QVariantTreeItemModel::open(QString filename)
{
    _tree.setFromFile(filename);

    // root must be list/collection to work
    if (!_content.type() == QVariant::List &&
            !_tree.nodeIsCollection()) {
        QVariantList list;
        if (_tree.isValid())
            list << _content;
        _tree.setRootContent(list);
    }

    updateModelFromTree();
}

void QVariantTreeItemModel::setTreeContent(QVariant content)
{
    _tree.setRootContent(content);

    updateModelFromTree();
}

void QVariantTreeItemModel::moveToChild(const QVariant& key)
{
    _tree.moveToNode(key);

    updateModelFromTree();
}

void QVariantTreeItemModel::moveToParent()
{
    _tree.moveToParent();

    updateModelFromTree();
}

void QVariantTreeItemModel::updateModelFromTree()
{
    int oldNbRow = rowCount();
    _isEmpty = false;
    int newNbRow = valueRowCount(_tree.nodeValue());

    // si suppression de ligne
    if (oldNbRow > newNbRow) {
        beginRemoveRows(QModelIndex(), newNbRow, oldNbRow-1);
        _content = _tree.nodeValue();
        endRemoveRows();
    }
    // si ajout de ligne
    else if (oldNbRow < newNbRow) {
        beginInsertRows(QModelIndex(), oldNbRow, newNbRow-1);
        _content = _tree.nodeValue();
        endInsertRows();
    }
    else
        _content = _tree.nodeValue();

    // signal modification
    if (newNbRow > 0) {
        emit dataChanged(index(0, 0),
                         index(newNbRow-1, columnCount()-1),
                         QVector<int>() << Qt::DisplayRole << Qt::EditRole);
    }
}

void QVariantTreeItemModel::clearTree()
{
    clear();
    _tree.clear();
}

void QVariantTreeItemModel::clear()
{
    int nbRows = rowCount();
    if (nbRows > 0)
        beginRemoveRows(QModelIndex(), 0, nbRows-1);
    _content.clear();
    _isEmpty = true;
    if (nbRows > 0)
        endRemoveRows();
}

void QVariantTreeItemModel::silentUpdateContentFromTree()
{
    _content = _tree.nodeValue();
}

//------------------------------------------------------------------------------
// Model

int QVariantTreeItemModel::rowCount(const QModelIndex& index) const
{
    Q_UNUSED(index)

    if (_isEmpty)
        return 0;
    return valueRowCount(_content);
}

int QVariantTreeItemModel::valueRowCount(const QVariant& value) const
{
    if (value.type() == QVariant::List)
        return value.toList().size();
    else if (value.type() == QVariant::Map)
        return value.toMap().size();
    else if (value.type() == QVariant::Hash)
        return value.toHash().size();
    return 1;
}

QVariant QVariantTreeItemModel::data(const QModelIndex& index, int role) const
{
    QVariant result;
    if (!index.isValid())
        return result;

    if (role == Qt::DisplayRole ||
            role == Qt::EditRole) {

        result = rawData(index);

        if (index.column() == columnType())
            result = typeToString(result.type());
        else if (index.column() == columnKey()) {
            if (_content.type() == QVariant::List ||
                    _content.type() == QVariant::Map ||
                    _content.type() == QVariant::Hash ||
                    result.isValid())
                result = keyToString(result);
            else
                result = tr("<no key/index>");
        }
        else if (index.column() == columnValue())
            result = valueToString(result);
    }
    else if (role == Qt::TextAlignmentRole)
        result = Qt::AlignCenter;

    //------------------------------------------------------------------------------

    //    if (column == _model.columnKey()) {
    //        return 100;
    //    }
    //    else if (column == _model.columnValue()) {
    //        return 200;
    //    }
    //    else if (column == _model.columnType()) {
    //        return 120;
    //    }

    return result;
}

QVariant QVariantTreeItemModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    QVariant result;
    if (orientation == Qt::Horizontal)
    {
        if (role == Qt::DisplayRole) {
            if (section == columnType())
                result = tr("Type");
            else if (section == columnKey())
                result = tr("Key / Index");
            else if (section == columnValue())
                result = tr("Value / Content");
        }
    }
    else
    {
        if (role == Qt::DisplayRole)
            result = (section +1);
    }
    return result;
}

//------------------------------------------------------------------------------
// Model (2)

QVariant QVariantTreeItemModel::rawData(int row, int column) const
{
    return rawData(index(row, column));
}

QVariant QVariantTreeItemModel::rawData(const QModelIndex& index) const
{
    QVariant result;
    if (!index.isValid())
        return result;

    // if a simple list
    if (_content.type() == QVariant::List) {
        if (index.column() == columnType())
            result = _content.toList().value(index.row()).type();
        else if (index.column() == columnKey())
            result = index.row();
        else if (index.column() == columnValue())
            result = _content.toList().value(index.row());
    }
    // if map collection
    else if (_content.type() == QVariant::Map) {
        QVariantMap currentMap = _content.toMap();

        if (index.column() == columnType())
            result = currentMap.value(currentMap.keys().value(index.row())).type();
        else if (index.column() == columnKey())
            result = currentMap.keys().value(index.row());
        else if (index.column() == columnValue())
            result = currentMap.value(currentMap.keys().value(index.row()));
    }
    // if hash collection
    else if (_content.type() == QVariant::Hash) {
        QVariantHash currentHash = _content.toHash();

        if (index.column() == columnType())
            result = currentHash.value(currentHash.keys().value(index.row())).type();
        else if (index.column() == columnKey())
            result = currentHash.keys().value(index.row());
        else if (index.column() == columnValue())
            result = currentHash.value(currentHash.keys().value(index.row()));
    }
    // if atomic value or unknown
    else {
        if (index.column() == columnType())
            result = _content.type();
        else if (index.column() == columnKey())
            result = QVariant();
        else if (index.column() == columnValue())
            result = _content;
    }
    return result;
}

QVariantList QVariantTreeItemModel::rawDatas(int row) const
{
    QVariant key, value;

    // if a simple list
    if (_content.type() == QVariant::List) {
        key = row;
        value = _content.toList().value(row);
    }
    // if map collection
    else if (_content.type() == QVariant::Map) {
        QVariantMap currentMap = _content.toMap();
        key = currentMap.keys().value(row);
        value = currentMap.value(currentMap.keys().value(row));
    }
    // if hash collection
    else if (_content.type() == QVariant::Hash) {
        QVariantHash currentHash = _content.toHash();
        key = currentHash.keys().value(row);
        value = currentHash.value(currentHash.keys().value(row));
    }
    // if atomic value or unknown
    else {
        key = QVariant::Invalid;
        value = _content;
    }

    // building result
    QVariantList result;

    for (int i=0; i<columnCount(); i++) {
        if (columnKey() == i)
            result << key;
        else if (columnValue() == i)
            result << value;
        else if (columnType() == i)
            result << value.type();
    }

    return result;
}

//------------------------------------------------------------------------------

Qt::ItemFlags QVariantTreeItemModel::flags(const QModelIndex & index) const
{
    Qt::ItemFlags itemFlags = Qt::ItemIsEnabled |
            Qt::ItemIsSelectable |
            Qt::ItemNeverHasChildren;
    bool canEdit = true;

    // key is editable, type is editable, value might not be
    if (index.column() == columnValue())
    {
        // if cell is list/collection -> cannot edit
        if (_content.type() == QVariant::List)
        {
            uint itemType = _content.toList().value(index.row()).type();
            canEdit = (itemType != QVariant::List) &&
                    (itemType != QVariant::Map) &&
                    (itemType != QVariant::Hash);
        }
    }

    if (canEdit)
        itemFlags |= Qt::ItemIsEditable;

    // default item is selectable / never with children
    return itemFlags;
}

//------------------------------------------------------------------------------

QString QVariantTreeItemModel::typeToString(const uint& type) const
{
    return _typesName.value(type, tr("<unknown>"));
}

QString QVariantTreeItemModel::keyToString(const QVariant& key) const
{
    return stringify(key, 0);
}

QString QVariantTreeItemModel::valueToString(const QVariant& value) const
{
    return stringify(value, 3);
}

QString QVariantTreeItemModel::stringify(const QVariant& value, int depth) const
{
    depth--;

    QString result;
    switch(value.type())
    {
    case QVariant::Invalid:
        result = "<Invalid>";
        break;
    case QVariant::Bool:
        result = (value.toBool() ? "true" : "false");
        break;
    case QVariant::Int:
        result = QString::number(value.toInt());
        break;
    case QVariant::UInt:
        result = QString("u%1").arg(value.toUInt());
        break;
    case QVariant::Double:
    {
        bool ok = false;
        result = QString::number(value.toDouble(&ok), 'g', 6);
        if (ok && !result.contains("."))
            result += ".0";
    }
        break;
    case QVariant::String:
        result = QString("\"%1\"").arg(value.toString());
        break;
    case QVariant::List:
    {
        QVariantList list = value.toList();
        if (depth <= 0) {
            result = QString("[x%1]").arg(list.size());
        }
        else {
            QStringList items;
            QVariantList::const_iterator it = list.constBegin();
            for (; it != list.constEnd(); ++it) {
                items.append(stringify(*it, depth));
            }
            result = QString("[%1]").arg(items.join(", "));
        }
    }
        break;
    case QVariant::Map:
    {
        QVariantMap map = value.toMap();
        if (depth <= 0) {
            result = QString("{x%1}").arg(map.size());
        }
        else {
            QStringList items;
            QVariantMap::const_iterator it = map.constBegin();
            for (; it != map.constEnd(); ++it) {
                items.append(stringify(it.key(), depth) + ":" + stringify(it.value(), depth));
            }
            result = QString("{%1}").arg(items.join(", "));
        }
    }
        break;
    case QVariant::Hash:
    {
        QVariantHash hash = value.toHash();
        if (depth <= 0) {
            result = QString("{x%1}").arg(hash.size());
        }
        else {
            QStringList items;
            QVariantHash::const_iterator it = hash.constBegin();
            for (; it != hash.constEnd(); ++it) {
                items.append(stringify(it.key(), depth) + ":" + stringify(it.value(), depth));
            }
            result = QString("{%1}").arg(items.join(", "));
        }
    }
        break;
    default:
        result = tr("<unknown>");
        break;
    }
    return result;
}
