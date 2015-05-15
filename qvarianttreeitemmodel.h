#ifndef QVARIANTTREEITEMMODEL_H
#define QVARIANTTREEITEMMODEL_H

#include <QAbstractTableModel>

#include "qvarianttree.h"


class QVariantTreeItemModel : public QAbstractTableModel
{
    Q_OBJECT
public:
    explicit QVariantTreeItemModel(QObject *parent = 0);


    // Tree
    /**
     * @brief Read from the given file and replace the current tree content
     * with the read one.
     * @param file The device to read from
     */
    void open(QIODevice* file);
    /**
     * @brief Read from the given file and replace the current tree content
     * with the read one.
     * @param filename The file to read from
     */
    void open(QString filename);

    /**
     * @brief Save to the file the tree content.
     * @param file The device to save into
     */
    void save(QIODevice* file) const { _tree.toFile(file); }
    /**
     * @brief Save to the file the tree content.
     * @param file The file to save into
     */
    void save(QString filename) const { _tree.toFile(filename); }

    /**
     * @brief Return the tree used by the model.
     * @return The QVariantTree
     * @see QVariantTreeItemModel::setTreeContent()
     */
    QVariantTree& tree() { return _tree; }
    /**
     * @brief Change the root content of the tree
     * @param content The new root
     * @see QVariantTreeItemModel::tree()
     */
    void setTreeContent(QVariant content);

    /**
     * @brief Move the into the tree to the child given by its key.
     * @param key The key of the child to move one
     * @see QVariantTreeItemModel::moveToParent()
     */
    void moveToChild(const QVariant &key);
    /**
     * @brief Move up to the parent.
     * @see QVariantTreeItemModel::moveToChild()
     */
    void moveToParent();

    /**
     * @brief Clear the tree and the model.
     * @see QVariantTreeItemModel::clear()
     */
    void clearTree();
    /**
     * @brief Clear the current content of the model.
     * The tree remains unmodified.
     * @see QVariantTreeItemModel::clearTree()
     */
    void clear();


    // Columns
    /**
     * @brief The column index / list index for the key.
     * @return Key's index
     */
    int columnKey() const  { return 0; }
    /**
     * @brief The column index / list index for the value.
     * @return Value's index
     */
    int columnValue() const { return 1; }
    /**
     * @brief The column index / list index for the type.
     * @return Type's index
     */
    int columnType() const { return 2; }


    // Model
    int rowCount(const QModelIndex& index = QModelIndex()) const;
    int columnCount(const QModelIndex& index = QModelIndex()) const
    { Q_UNUSED(index) return 3; }
    QVariant data(const QModelIndex& index, int role) const;
    Qt::ItemFlags flags(const QModelIndex & index) const;
    QVariant headerData(int section, Qt::Orientation orientation, int role) const;

    /**
     * @brief Same as QVariantTreeItemModel::data(), but return the unformat datas.
     * @param row The row (aka index) of the wanted information
     * @param column The column if the wanted information
     * @return The key/value/type information
     * @see QVariantTreeItemModel::columnKey()
     * @see QVariantTreeItemModel::columnValue()
     * @see QVariantTreeItemModel::columnType()
     */
    QVariant rawData(int row, int column) const;
    QVariant rawData(const QModelIndex& index) const;
    /**
     * @brief Return all information for the given row / index.
     * @param row
     * @return List of key/value/type, Use column*() for index.
     * @see QVariantTreeItemModel::rawData()
     * @see QVariantTreeItemModel::columnKey()
     * @see QVariantTreeItemModel::columnValue()
     * @see QVariantTreeItemModel::columnType()
     */
    QVariantList rawDatas(int row) const; // key, value, type

    /**
     * @brief Check if the current content is empty.
     * It doesn't mean the tree is empty.
     * @return True if current content is empty.
     */
    bool isEmpty() const { return _isEmpty; }


    // To Display
    /**
     * @brief Format to display the given type.
     * @param type The type to display
     * @return Type's string representation
     */
    QString typeToString(const QVariant::Type& type) const;
    /**
     * @brief Format to display the given key.
     * @param key The key to display
     * @return Key's string representation
     */
    QString keyToString(const QVariant& key) const;
    /**
     * @brief Format to display the given value.
     * @param value The value to display
     * @return Value's string representation
     */
    QString valueToString(const QVariant& value) const;
    /**
     * @brief JSON like stringify method.
     * @param value The value to stringify
     * @param depth The depth to look into (used by list/collection)
     * @return The string representation of the value
     */
    QString stringify(const QVariant& value, int depth = 1) const;

    /**
     * @brief Return the list of the handle type by the model.
     * Each type is paired with its own string name.
     * @return Hash of type and names
     */
    QHash<QVariant::Type, QString> typesToName() const { return _typesName; }
    /**
     * @brief Check if the type is handle by the model
     * @param type The type to check for
     * @return True if handled
     */
    bool typeIsHandled(QVariant::Type type) const { return _typesName.contains(type); }

    /**
     * @brief Hack to reset internal model content with current tree node content.
     * Don't fire any signal, nor update rows nor columns.
     * The object caller must fire signal of modification.
     */
    void silentUpdateContentFromTree();

    /**
     * @brief Smart method to correctly update the model. Replace current
     * model content with tree node content.
     */
    void updateModelFromTree();

signals:
    void valueKeyChanged(const QVariant& key,
                         const QVariant& oldKey);
    void valueContentChanged(const QVariant &key);
    void valueTypeChanged(const QVariant::Type& type,
                          const QVariant::Type& oldType);

    void insertedValue(const QVariant& key);
    void deletedValue(const QVariant& key);

private:
    /**
     * @brief Determine value size.
     * If list/hash/map, return size. Else, return 1;
     * @param value The value to look into
     * @return The value size.
     */
    int valueRowCount(const QVariant& value) const;

private:
    QVariantTree _tree;

    QVariant _content;
    bool _isEmpty;

    QHash<QVariant::Type, QString> _typesName;

};

#endif // QVARIANTTREEITEMMODEL_H
