#ifndef QTABLEVARIANTTREE_H
#define QTABLEVARIANTTREE_H

#include <QTableView>

#include "qvarianttreeitemmodel.h"


class QTableVariantTree : public QTableView
{
    Q_OBJECT
public:
    explicit QTableVariantTree(QWidget *parent = 0);

    QVariantTreeItemModel* model() { return &_model; }

    void keyPressEvent(QKeyEvent* event);

    void adaptColumnWidth();

public slots:
    void insertValue();
    void deleteValue();

    void clear();
    void clearTree();

    void openChild(int row);
    void openChild(const QModelIndex& index);
    void openParent();

signals:
    void movedToChild(const QVariant& key);
    void movedToParent();

protected slots:
    void dataChanged(const QModelIndex &topLeft,
                     const QModelIndex &bottomRight,
                     const QVector<int> &roles = QVector<int>());
    void rowsAboutToBeRemoved(const QModelIndex &parent, int start, int end);
    void rowsInserted(const QModelIndex &parent, int start, int end);

protected:
    void resizeEvent(QResizeEvent *event);

    virtual QVariant createValue(uint type) const;

private:
    QVariantTreeItemModel _model;

    QList<int> _selectedRowsPath;

};

#endif // QTABLEVARIANTTREE_H
