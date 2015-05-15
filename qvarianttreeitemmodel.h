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
    void open(QIODevice* file);
    void open(QString filename);

    void save(QIODevice* file) const { _tree.toFile(file); }
    void save(QString filename) const { _tree.toFile(filename); }

    QVariantTree& tree() { return _tree; }
    void setTreeContent(QVariant content);
    void moveToChild(const QVariant &key);
    void moveToParent();

    void clearTree();
    void clear();

    // Columns
    int columnKey() const  { return 0; }
    int columnValue() const { return 1; }
    int columnType() const { return 2; }

    // Model
    int rowCount(const QModelIndex& index = QModelIndex()) const;
    int columnCount(const QModelIndex& index = QModelIndex()) const
        { Q_UNUSED(index) return 3; }
    QVariant data(const QModelIndex& index, int role) const;
    Qt::ItemFlags flags(const QModelIndex & index) const;
    QVariant headerData(int section, Qt::Orientation orientation, int role) const;

    QVariant rawData(int row, int column) const;
    QVariant rawData(const QModelIndex& index) const;
    QVariantList rawDatas(int row) const; // key, value, type

    // To Display
    QString typeToString(const QVariant::Type& type) const;
    static QString keyToString(const QVariant& key);
    static QString valueToString(const QVariant& value);
    static QString stringify(const QVariant& value, int depth = 1);

    QHash<QVariant::Type, QString> typesToName() const { return _typesName; }
    bool typeIsHandled(QVariant::Type type) const { return _typesName.contains(type); }

    void silentUpdateContentFromTree();

    // use after big changed
    void updateTableAfterInsertDelete(int initialRowCount);

signals:
    void valueKeyChanged(const QVariant& key,
                         const QVariant& oldKey);
    void valueContentChanged(const QVariant &key);
    void valueTypeChanged(const QVariant::Type& type,
                          const QVariant::Type& oldType);

    void insertedValue(const QVariant& key);
    void deletedValue(const QVariant& key);

private:

    int valueRowCount(const QVariant& value) const;

private:
    QVariantTree _tree;

    QVariant _content;
    bool _isEmpty;

    QHash<QVariant::Type, QString> _typesName;

};

#endif // QVARIANTTREEITEMMODEL_H
