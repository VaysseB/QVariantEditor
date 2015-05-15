#include "qvariantitemdelegate.h"

#include <QComboBox>
#include <QLineEdit>
#include <qmath.h>

#include <limits>


QVariantItemDelegate::QVariantItemDelegate(QObject *parent) :
    QItemDelegate(parent)
{
}

//------------------------------------------------------------------------------
// editing
QWidget* QVariantItemDelegate::createEditor(QWidget *parent,
                                            const QStyleOptionViewItem &option,
                                            const QModelIndex &index) const
{
    Q_UNUSED(option)

    QWidget* editor = NULL;

    if (!index.isValid())
        return editor;

    QVariantTreeItemModel* model = extractModel(index);

    // si colonne cle -> edition possible
    if (index.column() == model->columnKey())
    {
        // if list displayed
        if (model->tree().nodeIsList()) {
            editor = new QSpinBox;
        }
        // if value is collection
        else if (model->tree().nodeIsCollection()) {
            editor = new QLineEdit;
        }
        // else, it is something...
        // end I don't know what to do with it
        // because this should not append!
    }
    // si colonne valeur -> edition possible
    else if (index.column() == model->columnValue())
    {
        QVariant item = model->rawData(index.row(), model->columnValue());

        switch (item.type())
        {
        case QVariant::Bool:
            editor = new QComboBox;
            break;
        case QVariant::UInt:
        case QVariant::Int:
            editor = new QSpinBox;
            break;
        case QVariant::Double:
            editor = new QDoubleSpinBox;
            break;
        case QVariant::String:
            editor = new QLineEdit;
            break;
        case QVariant::List:
        case QVariant::Map:
        case QVariant::Hash:
        case QVariant::Invalid:
            // cannot edit
            break;
        default:
            // unknown editor
            break;
        }
    }
    // si colonne type -> edition possible
    else if (index.column() == model->columnType())
    {
        QVariant::Type item = model->rawData(index.row(), model->columnType()).type();
        if (model->typeIsHandled(item))
            editor = new QComboBox;
    }

    // never let a child alone
    if (editor)
        editor->setParent(parent);
    return editor;
}

void QVariantItemDelegate::setEditorData(QWidget *editor,
                                         const QModelIndex &index) const
{
    if (!index.isValid())
        return;

    QVariantTreeItemModel* model = extractModel(index);

    // si colonne cle -> edition possible
    if (index.column() == model->columnKey())
    {
        // if list displayed
        if (model->tree().nodeIsList()) {
            QSpinBox* spinbox = qobject_cast<QSpinBox*>(editor);

            // [-1, n-1], -1 = last
            spinbox->setRange(-1, model->tree().nodeValue().toList().size() -1);
            spinbox->setValue(index.row());
        }
        // if value is collection
        else if (model->tree().nodeIsCollection()) {
            QLineEdit* line = qobject_cast<QLineEdit*>(editor);

            line->setText(model->rawData(index).toString());
        }
        // else, it is something...
        // end I don't know what to do with it
        // because this should not append!
    }
    // si colonne valeur -> edition possible
    else if (index.column() == model->columnValue())
    {
        QVariant item = model->rawData(index.row(), model->columnValue());

        switch (item.type())
        {
        case QVariant::Bool:
        {
            QComboBox* combo = qobject_cast<QComboBox*>(editor);
            combo->addItem("false", false);
            combo->addItem("true", true);
            combo->setCurrentIndex(item.toBool() ? 1 : 0);
        }
            break;
        case QVariant::UInt:
        {
            QSpinBox* spinbox = qobject_cast<QSpinBox*>(editor);
            spinbox->setRange(0, std::numeric_limits<int>::max());
            spinbox->setValue(item.toUInt());
        }
            break;
        case QVariant::Int:
        {
            QSpinBox* spinbox = qobject_cast<QSpinBox*>(editor);
            spinbox->setRange(std::numeric_limits<int>::min(),
                              std::numeric_limits<int>::max());
            spinbox->setValue(item.toInt());
        }
            break;
        case QVariant::Double:
        {
            QDoubleSpinBox* spinbox = qobject_cast<QDoubleSpinBox*>(editor);
            spinbox->setRange((std::numeric_limits<double>::max() -1) * -1,
                              std::numeric_limits<double>::max());
            bool ok = false;
            double dblValue = item.toDouble(&ok);
            spinbox->setValue(ok ? dblValue : 0);
        }
            break;
        case QVariant::String:
        {
            QLineEdit* line = qobject_cast<QLineEdit*>(editor);
            line->setText(item.toString());
        }
            break;
        case QVariant::List:
        case QVariant::Map:
        case QVariant::Hash:
        case QVariant::Invalid:
            // cannot edit
            break;
        default:
            // unknown editor
            break;
        }
    }
    // si colonne type -> edition possible
    else if (index.column() == model->columnType())
    {
        QVariant::Type itemType = model->rawData(index).type();
        QComboBox* combo = qobject_cast<QComboBox*>(editor);

        // if value is list
        if (itemType == QVariant::List) {
            // only list !
            // for now, later will be stringlist
            combo->addItem("List", QVariant::List);
            combo->addItem("Invalid", QVariant::Invalid);
            combo->setCurrentIndex(0);
        }
        // if value is collection
        else if (itemType == QVariant::Map ||
                 itemType == QVariant::Hash) {
            // only collection
            combo->addItem("Map", QVariant::Map);
            combo->addItem("Hash", QVariant::Hash);
            combo->addItem("Invalid", QVariant::Invalid);

            if (itemType == QVariant::Map)
                combo->setCurrentIndex(0);
            else
                combo->setCurrentIndex(1);
        }
        // else, it is something...
        else {
            QHash<QVariant::Type, QString> names = model->typesToName();
            names.remove(QVariant::Invalid);
            // purging list/collection if type valid
            names.remove(QVariant::List);
            names.remove(QVariant::Map);
            names.remove(QVariant::Hash);

            QHash<QVariant::Type, QString>::const_iterator it = names.constBegin();
            for (; it != names.constEnd(); ++it) {
                combo->addItem(it.value(), it.key());
                if (it.key() == itemType)
                    combo->setCurrentIndex(combo->count()-1);
            }

            // purging list/collection if type not valid
            if (itemType == QVariant::Invalid) {
                combo->addItem("List", QVariant::List);
                combo->addItem("Map", QVariant::Map);
                combo->addItem("Hash", QVariant::Hash);
            }

            combo->addItem("Invalid", QVariant::Invalid);
            if (itemType == QVariant::Invalid)
                combo->setCurrentIndex(combo->count() - 1);
        }


        // adding a star in front of the current
        if (combo->currentIndex() >= 0) {
            int indexCombo = combo->currentIndex();
            combo->insertItem(indexCombo,
                              QString("*%1").arg(combo->currentText()),
                              combo->currentData());
            combo->setCurrentIndex(indexCombo);
            combo->removeItem(indexCombo+1);
        }
    }
}

void QVariantItemDelegate::setModelData(QWidget *editor,
                                        QAbstractItemModel *p_model,
                                        const QModelIndex &index) const
{
    if (!index.isValid())
        return;

    QVariantTreeItemModel* model = qobject_cast<QVariantTreeItemModel*>(p_model);

    // si colonne cle -> edition possible
    if (index.column() == model->columnKey())
    {
        // if in displayed
        if (model->tree().nodeIsList()) {
            QSpinBox* spinbox = qobject_cast<QSpinBox*>(editor);

            // old & new row
            int oldRowIndex = index.row();
            int newRowIndex = spinbox->value();

            // if the same -> RAF
            if (oldRowIndex == newRowIndex)
                return;

            // if last (= -1), -> getting the true index
            if (newRowIndex < 0)
                newRowIndex = model->tree().itemListSize() - 1;

            // setting in model
            QVariant value = model->tree().getItemList(oldRowIndex);
            model->tree().delItemList(oldRowIndex);
            model->tree().insertItemList(newRowIndex, value);
            model->silentUpdateContentFromTree();

            // updating model
            QModelIndex oldIndex = model->index(oldRowIndex, index.column());
            if (oldIndex.row() > index.row())
                emit model->dataChanged(index, oldIndex);
            else
                emit model->dataChanged(oldIndex, index);

            // updating others
            emit model->valueKeyChanged(newRowIndex, oldRowIndex);
        }
        // if in collection
        else if (model->tree().nodeIsCollection()) {
            QLineEdit* line = qobject_cast<QLineEdit*>(editor);

            QStringList allKeys = model->tree().itemCollectionKeys();
            QString newKey = line->text();
            QString oldKey = allKeys.value(index.row());

            if (newKey.isEmpty() ||
                    newKey == oldKey)
                return;

            QStringList prevCollectionKeys = model->tree().itemCollectionKeys();

            // setting in model
            QVariant value = model->tree().getItemCollection(oldKey);
            model->tree().delItemCollection(oldKey);
            model->tree().setItemCollection(newKey, value);
            model->silentUpdateContentFromTree();

            QStringList newCollectionKeys = model->tree().itemCollectionKeys();

            // search the first and last row different after tree changes
            // because full order can be modified (no strict order in hash/map)
            int firstRowChanged = 0;
            int lastRowChanged = newCollectionKeys.size() -1;
            while (!prevCollectionKeys.isEmpty() &&
                   !newCollectionKeys.isEmpty() &&
                   prevCollectionKeys.takeLast() ==
                   newCollectionKeys.takeLast()) {
                lastRowChanged--;
            }
            while (!prevCollectionKeys.isEmpty() &&
                   !newCollectionKeys.isEmpty() &&
                   prevCollectionKeys.takeFirst() ==
                   newCollectionKeys.takeFirst()) {
                firstRowChanged++;
            }

            // updating model
            model->updateTableAfterInsertDelete(allKeys.count());

            // updating others
            emit model->valueKeyChanged(newKey, oldKey);
        }
        // else, it is something...
        // end I don't know what to do with it
        // because this should not append!

    }
    // si colonne valeur -> edition possible
    else if (index.column() == model->columnValue())
    {
        QVariant item = model->rawData(index.row(), model->columnValue());
        QVariant newValue;
        bool isValid = true;

        switch (item.type())
        {
        case QVariant::Bool:
            newValue = qobject_cast<QComboBox*>(editor)->currentData().toBool();
            break;
        case QVariant::UInt:
            newValue = (uint)qobject_cast<QSpinBox*>(editor)->value();
            break;
        case QVariant::Int:
            newValue = (int)qobject_cast<QSpinBox*>(editor)->value();
            break;
        case QVariant::Double:
            newValue = (double)qobject_cast<QDoubleSpinBox*>(editor)->value();
            break;
        case QVariant::String:
            newValue = qobject_cast<QLineEdit*>(editor)->text();
            break;
        case QVariant::List:
        case QVariant::Map:
        case QVariant::Hash:
        case QVariant::Invalid:
            // cannot edit
            isValid = false;
            break;
        default:
            // unknown editor
            isValid = false;
            break;
        }

        if (isValid)
        {
            QVariant key = model->rawData(index.row(), model->columnKey());

            // if in list
            if (model->tree().nodeIsList()) {
                model->tree().setItemList(key.toInt(), newValue);
                model->silentUpdateContentFromTree();

                // updating model
                emit model->dataChanged(model->index(index.row(), 0),
                                        model->index(index.row(), model->columnCount()));

                // updating others
                emit model->valueContentChanged(key);
            }
            // if in collection
            else if (model->tree().nodeIsCollection()) {
                model->tree().setItemCollection(key.toString(), newValue);
                model->silentUpdateContentFromTree();

                // updating model
                emit model->dataChanged(model->index(index.row(), 0),
                                        model->index(index.row(), model->columnCount()));

                // updating others
                emit model->valueContentChanged(key);
            }
        }

    }
    // si colonne type -> edition possible
    else if (index.column() == model->columnType())
    {
        QComboBox* combo = qobject_cast<QComboBox*>(editor);

        // if nothing selected
        if (combo->currentIndex() < 0)
            return;

        QVariantList itemInfos = model->rawDatas(index.row());
        QVariant::Type newItemType = combo->currentData().type();

        QVariant key = itemInfos.value(model->columnKey());
        QVariant value = itemInfos.value(model->columnValue());

        // convertion
        value = convertValue(value, newItemType);

        // setting in model
        if (model->tree().nodeIsList())
            model->tree().setItemList(key.toInt(), value);
        else if (model->tree().nodeIsCollection())
            model->tree().setItemCollection(key.toString(), value);
        model->silentUpdateContentFromTree();

        // updating model (all columns)
        emit model->dataChanged(model->index(index.row(), 0),
                                model->index(index.row(), model->columnCount()));

        // updating others
        emit model->valueTypeChanged(newItemType,
                                     itemInfos.value(model->columnType()).type());

    }
}

//------------------------------------------------------------------------------

QVariantTreeItemModel* QVariantItemDelegate::extractModel(const QModelIndex& index) const
{
    return qobject_cast<QVariantTreeItemModel*>(
                const_cast<QAbstractItemModel*>(index.model()));
}

//------------------------------------------------------------------------------

QVariant QVariantItemDelegate::convertValue(QVariant value,
                                            QVariant::Type returnType,
                                            bool* ok) const
{
    if (returnType == value.type()) {
        if (ok)
            *ok = true;
        return value;
    }
    // from here, conversion cannot be meaningless

    QVariant result;
    bool easyOk = false;
    // to hash
    if (returnType == QVariant::List)
    {
        result = QVariantList();
    }
    // to map
    else if (returnType == QVariant::Map)
    {
        QVariantMap map;
        // hash -> map
        if (value.type() == QVariant::Hash) {
            QVariantHash hash = value.toHash();
            QVariantHash::const_iterator it = hash.constBegin();
            for (; it != hash.constEnd(); ++it) {
                map[it.key()] = it.value();
            }
            easyOk = true;
        }
        result = map;
    }
    // to hash
    else if (returnType == QVariant::Hash)
    {
        QVariantHash hash;
        // map -> hash
        if (value.type() == QVariant::Map) {
            QVariantMap map = value.toMap();
            QVariantMap::const_iterator it = map.constBegin();
            for (; it != map.constEnd(); ++it) {
                hash[it.key()] = it.value();
            }
            easyOk = true;
        }
        result = hash;
    }
    // to bool
    else if (returnType == QVariant::Bool)
    {
        result = false;
        easyOk = true;

        switch(value.type())
        {
        case QVariant::Int:
            result = value.toInt() > 0;
            break;
        case QVariant::UInt:
            result = value.toUInt() > 0;
            break;
        case QVariant::Double:
            result = value.toDouble() > 0;
            break;
        case QVariant::String:
            result = !value.toString().isEmpty();
            break;
        case QVariant::Hash:
            result = !value.toHash().isEmpty();
            break;
        case QVariant::Map:
            result = !value.toMap().isEmpty();
            break;
        case QVariant::List:
            result = !value.toList().isEmpty();
            break;
        case QVariant::Invalid:
            break;
        default:
            easyOk = false;
            break;
        }
    }
    // to string
    else if (returnType == QVariant::String)
    {
        result = QString();
        easyOk = true;

        switch(value.type())
        {
        case QVariant::Int:
            result = QString::number(value.toInt());
            break;
        case QVariant::UInt:
            result = QString::number(value.toUInt());
            break;
        case QVariant::Double:
            result = QString::number(value.toDouble());
            break;
        case QVariant::Invalid:
            result = QString("");
            break;
        case QVariant::Bool:
            result = value.toBool() ? QString("true") : QString("false");
            break;
        case QVariant::Hash:
        case QVariant::Map:
        case QVariant::List:
            break;
        default:
            easyOk = false;
            break;
        }
    }
    // to int / uint / double = number
    else if (returnType == QVariant::Int ||
             returnType == QVariant::UInt||
             returnType == QVariant::Double)
    {
        result = 0;
        easyOk = true;

        switch(value.type())
        {
        case QVariant::Int:
            result = value.toInt();
            break;
        case QVariant::UInt:
            result = value.toUInt();
            break;
        case QVariant::Double:
            result = value.toDouble();
            break;
        case QVariant::String:
            result = value.toString().size();
            break;
        case QVariant::Hash:
            result = value.toHash().size();
            break;
        case QVariant::Map:
            result = value.toMap().size();
            break;
        case QVariant::List:
            result = value.toList().size();
            break;
        case QVariant::Bool:
            result = value.toBool() ? 1 : 0;
            break;
        case QVariant::Invalid:
            break;
        default:
            easyOk = false;
            break;
        }

        if (returnType == QVariant::UInt) {
            if (value.type() == QVariant::Int) {
                if (result.toInt() < 0)
                    result = (uint)0;
                else
                    result = (uint)result.toUInt();
            }
            else if (value.type() == QVariant::Double) {
                bool okConv = false;
                double dblValue = result.toDouble(&okConv);
                if (dblValue < 0.0 || !okConv)
                    result = (uint)0;
                else
                    result = (uint)qFloor(dblValue);
            }
        }
        else if (returnType == QVariant::Int) {
            if (value.type() == QVariant::UInt)
                result = (int)result.toUInt();
            else if (value.type() == QVariant::Double)
                result = (int)qFloor(result.toDouble());
        }
        else if (returnType == QVariant::Double) {
            if (value.type() == QVariant::UInt)
                result = (double)(result.toUInt() * 1.0);
            else if (value.type() == QVariant::Int)
                result = (double)(result.toInt() * 1.0);
        }
    }

    if (ok)
        *ok = easyOk;
    return result;
}
