// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "itemviews_p.h"

#include <qheaderview.h>
#if QT_CONFIG(tableview)
#include <qtableview.h>
#endif
#if QT_CONFIG(listview)
#include <qlistview.h>
#endif
#if QT_CONFIG(treeview)
#include <qtreeview.h>
#include <private/qtreeview_p.h>
#endif
#include <private/qwidget_p.h>

#if QT_CONFIG(accessibility)

QT_BEGIN_NAMESPACE

/*
Implementation of the IAccessible2 table2 interface. Much simpler than
the other table interfaces since there is only the main table and cells:

TABLE/LIST/TREE
  |- HEADER CELL
  |- CELL
  |- CELL
  ...
*/


QAbstractItemView *QAccessibleTable::view() const
{
    return qobject_cast<QAbstractItemView*>(object());
}

int QAccessibleTable::logicalIndex(const QModelIndex &index) const
{
    if (!view()->model() || !index.isValid())
        return -1;
    int vHeader = verticalHeader() ? 1 : 0;
    int hHeader = horizontalHeader() ? 1 : 0;
    return (index.row() + hHeader)*(index.model()->columnCount() + vHeader) + (index.column() + vHeader);
}

QAccessibleTable::QAccessibleTable(QWidget *w)
    : QAccessibleObject(w)
{
    Q_ASSERT(view());

#if QT_CONFIG(tableview)
    if (qobject_cast<const QTableView*>(view())) {
        m_role = QAccessible::Table;
    } else
#endif
#if QT_CONFIG(treeview)
    if (qobject_cast<const QTreeView*>(view())) {
        m_role = QAccessible::Tree;
    } else
#endif
#if QT_CONFIG(listview)
    if (qobject_cast<const QListView*>(view())) {
        m_role = QAccessible::List;
    } else
#endif
    {
        // is this our best guess?
        m_role = QAccessible::Table;
    }
}

bool QAccessibleTable::isValid() const
{
    return view() && !qt_widget_private(view())->data.in_destructor;
}

QAccessibleTable::~QAccessibleTable()
{
    for (QAccessible::Id id : qAsConst(childToId))
        QAccessible::deleteAccessibleInterface(id);
}

QHeaderView *QAccessibleTable::horizontalHeader() const
{
    QHeaderView *header = nullptr;
    if (false) {
#if QT_CONFIG(tableview)
    } else if (const QTableView *tv = qobject_cast<const QTableView*>(view())) {
        header = tv->horizontalHeader();
#endif
#if QT_CONFIG(treeview)
    } else if (const QTreeView *tv = qobject_cast<const QTreeView*>(view())) {
        header = tv->header();
#endif
    }
    return header;
}

QHeaderView *QAccessibleTable::verticalHeader() const
{
    QHeaderView *header = nullptr;
    if (false) {
#if QT_CONFIG(tableview)
    } else if (const QTableView *tv = qobject_cast<const QTableView*>(view())) {
        header = tv->verticalHeader();
#endif
    }
    return header;
}

QAccessibleInterface *QAccessibleTable::cellAt(int row, int column) const
{
    if (!view()->model())
        return nullptr;
    Q_ASSERT(role() != QAccessible::Tree);
    QModelIndex index = view()->model()->index(row, column, view()->rootIndex());
    if (Q_UNLIKELY(!index.isValid())) {
        qWarning() << "QAccessibleTable::cellAt: invalid index: " << index << " for " << view();
        return nullptr;
    }
    return child(logicalIndex(index));
}

QAccessibleInterface *QAccessibleTable::caption() const
{
    return nullptr;
}

QString QAccessibleTable::columnDescription(int column) const
{
    if (!view()->model())
        return QString();
    return view()->model()->headerData(column, Qt::Horizontal).toString();
}

int QAccessibleTable::columnCount() const
{
    if (!view()->model())
        return 0;
    return view()->model()->columnCount();
}

int QAccessibleTable::rowCount() const
{
    if (!view()->model())
        return 0;
    return view()->model()->rowCount();
}

int QAccessibleTable::selectedCellCount() const
{
    if (!view()->selectionModel())
        return 0;
    return view()->selectionModel()->selectedIndexes().count();
}

int QAccessibleTable::selectedColumnCount() const
{
    if (!view()->selectionModel())
        return 0;
    return view()->selectionModel()->selectedColumns().count();
}

int QAccessibleTable::selectedRowCount() const
{
    if (!view()->selectionModel())
        return 0;
    return view()->selectionModel()->selectedRows().count();
}

QString QAccessibleTable::rowDescription(int row) const
{
    if (!view()->model())
        return QString();
    return view()->model()->headerData(row, Qt::Vertical).toString();
}

QList<QAccessibleInterface *> QAccessibleTable::selectedCells() const
{
    QList<QAccessibleInterface*> cells;
    if (!view()->selectionModel())
        return cells;
    const QModelIndexList selectedIndexes = view()->selectionModel()->selectedIndexes();
    cells.reserve(selectedIndexes.size());
    for (const QModelIndex &index : selectedIndexes)
        cells.append(child(logicalIndex(index)));
    return cells;
}

QList<int> QAccessibleTable::selectedColumns() const
{
    if (!view()->selectionModel())
        return QList<int>();
    QList<int> columns;
    const QModelIndexList selectedColumns = view()->selectionModel()->selectedColumns();
    columns.reserve(selectedColumns.size());
    for (const QModelIndex &index : selectedColumns)
        columns.append(index.column());

    return columns;
}

QList<int> QAccessibleTable::selectedRows() const
{
    if (!view()->selectionModel())
        return QList<int>();
    QList<int> rows;
    const QModelIndexList selectedRows = view()->selectionModel()->selectedRows();
    rows.reserve(selectedRows.size());
    for (const QModelIndex &index : selectedRows)
        rows.append(index.row());

    return rows;
}

QAccessibleInterface *QAccessibleTable::summary() const
{
    return nullptr;
}

bool QAccessibleTable::isColumnSelected(int column) const
{
    if (!view()->selectionModel())
        return false;
    return view()->selectionModel()->isColumnSelected(column, QModelIndex());
}

bool QAccessibleTable::isRowSelected(int row) const
{
    if (!view()->selectionModel())
        return false;
    return view()->selectionModel()->isRowSelected(row, QModelIndex());
}

bool QAccessibleTable::selectRow(int row)
{
    if (!view()->model() || !view()->selectionModel())
        return false;
    QModelIndex index = view()->model()->index(row, 0, view()->rootIndex());

    if (!index.isValid() || view()->selectionBehavior() == QAbstractItemView::SelectColumns)
        return false;

    switch (view()->selectionMode()) {
    case QAbstractItemView::NoSelection:
        return false;
    case QAbstractItemView::SingleSelection:
        if (view()->selectionBehavior() != QAbstractItemView::SelectRows && columnCount() > 1 )
            return false;
        view()->clearSelection();
        break;
    case QAbstractItemView::ContiguousSelection:
        if ((!row || !view()->selectionModel()->isRowSelected(row - 1, view()->rootIndex()))
            && !view()->selectionModel()->isRowSelected(row + 1, view()->rootIndex()))
            view()->clearSelection();
        break;
    default:
        break;
    }

    view()->selectionModel()->select(index, QItemSelectionModel::Select | QItemSelectionModel::Rows);
    return true;
}

bool QAccessibleTable::selectColumn(int column)
{
    if (!view()->model() || !view()->selectionModel())
        return false;
    QModelIndex index = view()->model()->index(0, column, view()->rootIndex());

    if (!index.isValid() || view()->selectionBehavior() == QAbstractItemView::SelectRows)
        return false;

    switch (view()->selectionMode()) {
    case QAbstractItemView::NoSelection:
        return false;
    case QAbstractItemView::SingleSelection:
        if (view()->selectionBehavior() != QAbstractItemView::SelectColumns && rowCount() > 1)
            return false;
        Q_FALLTHROUGH();
    case QAbstractItemView::ContiguousSelection:
        if ((!column || !view()->selectionModel()->isColumnSelected(column - 1, view()->rootIndex()))
            && !view()->selectionModel()->isColumnSelected(column + 1, view()->rootIndex()))
            view()->clearSelection();
        break;
    default:
        break;
    }

    view()->selectionModel()->select(index, QItemSelectionModel::Select | QItemSelectionModel::Columns);
    return true;
}

bool QAccessibleTable::unselectRow(int row)
{
    if (!view()->model() || !view()->selectionModel())
        return false;

    QModelIndex index = view()->model()->index(row, 0, view()->rootIndex());
    if (!index.isValid())
        return false;

    QItemSelection selection(index, index);

    switch (view()->selectionMode()) {
    case QAbstractItemView::SingleSelection:
        //In SingleSelection and ContiguousSelection once an item
        //is selected, there's no way for the user to unselect all items
        if (selectedRowCount() == 1)
            return false;
        break;
    case QAbstractItemView::ContiguousSelection:
        if (selectedRowCount() == 1)
            return false;

        if ((!row || view()->selectionModel()->isRowSelected(row - 1, view()->rootIndex()))
            && view()->selectionModel()->isRowSelected(row + 1, view()->rootIndex())) {
            //If there are rows selected both up the current row and down the current rown,
            //the ones which are down the current row will be deselected
            selection = QItemSelection(index, view()->model()->index(rowCount() - 1, 0, view()->rootIndex()));
        }
    default:
        break;
    }

    view()->selectionModel()->select(selection, QItemSelectionModel::Deselect | QItemSelectionModel::Rows);
    return true;
}

bool QAccessibleTable::unselectColumn(int column)
{
    if (!view()->model() || !view()->selectionModel())
        return false;

    QModelIndex index = view()->model()->index(0, column, view()->rootIndex());
    if (!index.isValid())
        return false;

    QItemSelection selection(index, index);

    switch (view()->selectionMode()) {
    case QAbstractItemView::SingleSelection:
        //In SingleSelection and ContiguousSelection once an item
        //is selected, there's no way for the user to unselect all items
        if (selectedColumnCount() == 1)
            return false;
        break;
    case QAbstractItemView::ContiguousSelection:
        if (selectedColumnCount() == 1)
            return false;

        if ((!column || view()->selectionModel()->isColumnSelected(column - 1, view()->rootIndex()))
            && view()->selectionModel()->isColumnSelected(column + 1, view()->rootIndex())) {
            //If there are columns selected both at the left of the current row and at the right
            //of the current rown, the ones which are at the right will be deselected
            selection = QItemSelection(index, view()->model()->index(0, columnCount() - 1, view()->rootIndex()));
        }
    default:
        break;
    }

    view()->selectionModel()->select(selection, QItemSelectionModel::Deselect | QItemSelectionModel::Columns);
    return true;
}

QAccessible::Role QAccessibleTable::role() const
{
    return m_role;
}

QAccessible::State QAccessibleTable::state() const
{
    return QAccessible::State();
}

QAccessibleInterface *QAccessibleTable::childAt(int x, int y) const
{
    QPoint viewportOffset = view()->viewport()->mapTo(view(), QPoint(0,0));
    QPoint indexPosition = view()->mapFromGlobal(QPoint(x, y) - viewportOffset);
    // FIXME: if indexPosition < 0 in one coordinate, return header

    QModelIndex index = view()->indexAt(indexPosition);
    if (index.isValid()) {
        return child(logicalIndex(index));
    }
    return nullptr;
}

QAccessibleInterface *QAccessibleTable::focusChild() const
{
    QModelIndex index = view()->currentIndex();
    if (!index.isValid())
        return nullptr;

    return child(logicalIndex(index));
}

int QAccessibleTable::childCount() const
{
    if (!view()->model())
        return 0;
    int vHeader = verticalHeader() ? 1 : 0;
    int hHeader = horizontalHeader() ? 1 : 0;
    return (view()->model()->rowCount()+hHeader) * (view()->model()->columnCount()+vHeader);
}

int QAccessibleTable::indexOfChild(const QAccessibleInterface *iface) const
{
    if (!view()->model())
        return -1;
    QAccessibleInterface *parent = iface->parent();
    if (parent->object() != view())
        return -1;

    Q_ASSERT(iface->role() != QAccessible::TreeItem); // should be handled by tree class
    if (iface->role() == QAccessible::Cell || iface->role() == QAccessible::ListItem) {
        const QAccessibleTableCell* cell = static_cast<const QAccessibleTableCell*>(iface);
        return logicalIndex(cell->m_index);
    } else if (iface->role() == QAccessible::ColumnHeader){
        const QAccessibleTableHeaderCell* cell = static_cast<const QAccessibleTableHeaderCell*>(iface);
        return cell->index + (verticalHeader() ? 1 : 0);
    } else if (iface->role() == QAccessible::RowHeader){
        const QAccessibleTableHeaderCell* cell = static_cast<const QAccessibleTableHeaderCell*>(iface);
        return (cell->index + 1) * (view()->model()->columnCount() + 1);
    } else if (iface->role() == QAccessible::Pane) {
        return 0; // corner button
    } else {
        qWarning() << "WARNING QAccessibleTable::indexOfChild Fix my children..."
                   << iface->role() << iface->text(QAccessible::Name);
    }
    // FIXME: we are in denial of our children. this should stop.
    return -1;
}

QString QAccessibleTable::text(QAccessible::Text t) const
{
    if (t == QAccessible::Description)
        return view()->accessibleDescription();
    return view()->accessibleName();
}

QRect QAccessibleTable::rect() const
{
    if (!view()->isVisible())
        return QRect();
    QPoint pos = view()->mapToGlobal(QPoint(0, 0));
    return QRect(pos.x(), pos.y(), view()->width(), view()->height());
}

QAccessibleInterface *QAccessibleTable::parent() const
{
    if (view() && view()->parent()) {
        if (qstrcmp("QComboBoxPrivateContainer", view()->parent()->metaObject()->className()) == 0) {
            return QAccessible::queryAccessibleInterface(view()->parent()->parent());
        }
        return QAccessible::queryAccessibleInterface(view()->parent());
    }
    return nullptr;
}

QAccessibleInterface *QAccessibleTable::child(int logicalIndex) const
{
    if (!view()->model())
        return nullptr;

    auto id = childToId.constFind(logicalIndex);
    if (id != childToId.constEnd())
        return QAccessible::accessibleInterface(id.value());

    int vHeader = verticalHeader() ? 1 : 0;
    int hHeader = horizontalHeader() ? 1 : 0;

    int columns = view()->model()->columnCount() + vHeader;

    int row = logicalIndex / columns;
    int column = logicalIndex % columns;

    QAccessibleInterface *iface = nullptr;

    if (vHeader) {
        if (column == 0) {
            if (hHeader && row == 0) {
                iface = new QAccessibleTableCornerButton(view());
            } else {
                iface = new QAccessibleTableHeaderCell(view(), row - hHeader, Qt::Vertical);
            }
        }
        --column;
    }
    if (!iface && hHeader) {
        if (row == 0) {
            iface = new QAccessibleTableHeaderCell(view(), column, Qt::Horizontal);
        }
        --row;
    }

    if (!iface) {
        QModelIndex index = view()->model()->index(row, column, view()->rootIndex());
        if (Q_UNLIKELY(!index.isValid())) {
            qWarning("QAccessibleTable::child: Invalid index at: %d %d", row, column);
            return nullptr;
        }
        iface = new QAccessibleTableCell(view(), index, cellRole());
    }

    QAccessible::registerAccessibleInterface(iface);
    childToId.insert(logicalIndex, QAccessible::uniqueId(iface));
    return iface;
}

void *QAccessibleTable::interface_cast(QAccessible::InterfaceType t)
{
    if (t == QAccessible::TableInterface)
       return static_cast<QAccessibleTableInterface*>(this);
   return nullptr;
}

void QAccessibleTable::modelChange(QAccessibleTableModelChangeEvent *event)
{
    // if there is no cache yet, we don't update anything
    if (childToId.isEmpty())
        return;

    switch (event->modelChangeType()) {
    case QAccessibleTableModelChangeEvent::ModelReset:
        for (QAccessible::Id id : qAsConst(childToId))
            QAccessible::deleteAccessibleInterface(id);
        childToId.clear();
        break;

    // rows are inserted: move every row after that
    case QAccessibleTableModelChangeEvent::RowsInserted:
    case QAccessibleTableModelChangeEvent::ColumnsInserted: {
        int newRows = event->lastRow() - event->firstRow() + 1;
        int newColumns = event->lastColumn() - event->firstColumn() + 1;

        ChildCache newCache;
        ChildCache::ConstIterator iter = childToId.constBegin();

        while (iter != childToId.constEnd()) {
            QAccessible::Id id = iter.value();
            QAccessibleInterface *iface = QAccessible::accessibleInterface(id);
            Q_ASSERT(iface);
            if (event->modelChangeType() == QAccessibleTableModelChangeEvent::RowsInserted
                && iface->role() == QAccessible::RowHeader) {
                QAccessibleTableHeaderCell *cell = static_cast<QAccessibleTableHeaderCell*>(iface);
                if (cell->index >= event->firstRow()) {
                    cell->index += newRows;
                }
            } else if (event->modelChangeType() == QAccessibleTableModelChangeEvent::ColumnsInserted
                   && iface->role() == QAccessible::ColumnHeader) {
                QAccessibleTableHeaderCell *cell = static_cast<QAccessibleTableHeaderCell*>(iface);
                if (cell->index >= event->firstColumn()) {
                    cell->index += newColumns;
                }
            }
            if (indexOfChild(iface) >= 0) {
                newCache.insert(indexOfChild(iface), id);
            } else {
                // ### This should really not happen,
                // but it might if the view has a root index set.
                // This needs to be fixed.
                QAccessible::deleteAccessibleInterface(id);
            }
            ++iter;
        }
        childToId = newCache;
        break;
    }

    case QAccessibleTableModelChangeEvent::ColumnsRemoved:
    case QAccessibleTableModelChangeEvent::RowsRemoved: {
        int deletedColumns = event->lastColumn() - event->firstColumn() + 1;
        int deletedRows = event->lastRow() - event->firstRow() + 1;
        ChildCache newCache;
        ChildCache::ConstIterator iter = childToId.constBegin();
        while (iter != childToId.constEnd()) {
            QAccessible::Id id = iter.value();
            QAccessibleInterface *iface = QAccessible::accessibleInterface(id);
            Q_ASSERT(iface);
            if (iface->role() == QAccessible::Cell || iface->role() == QAccessible::ListItem) {
                Q_ASSERT(iface->tableCellInterface());
                QAccessibleTableCell *cell = static_cast<QAccessibleTableCell*>(iface->tableCellInterface());
                // Since it is a QPersistentModelIndex, we only need to check if it is valid
                if (cell->m_index.isValid())
                    newCache.insert(indexOfChild(cell), id);
                else
                    QAccessible::deleteAccessibleInterface(id);
            } else if (event->modelChangeType() == QAccessibleTableModelChangeEvent::RowsRemoved
                       && iface->role() == QAccessible::RowHeader) {
                QAccessibleTableHeaderCell *cell = static_cast<QAccessibleTableHeaderCell*>(iface);
                if (cell->index < event->firstRow()) {
                    newCache.insert(indexOfChild(cell), id);
                } else if (cell->index > event->lastRow()) {
                    cell->index -= deletedRows;
                    newCache.insert(indexOfChild(cell), id);
                } else {
                    QAccessible::deleteAccessibleInterface(id);
                }
            } else if (event->modelChangeType() == QAccessibleTableModelChangeEvent::ColumnsRemoved
                   && iface->role() == QAccessible::ColumnHeader) {
                QAccessibleTableHeaderCell *cell = static_cast<QAccessibleTableHeaderCell*>(iface);
                if (cell->index < event->firstColumn()) {
                    newCache.insert(indexOfChild(cell), id);
                } else if (cell->index > event->lastColumn()) {
                    cell->index -= deletedColumns;
                    newCache.insert(indexOfChild(cell), id);
                } else {
                    QAccessible::deleteAccessibleInterface(id);
                }
            }
            ++iter;
        }
        childToId = newCache;
        break;
    }

    case QAccessibleTableModelChangeEvent::DataChanged:
        // nothing to do in this case
        break;
    }
}

#if QT_CONFIG(treeview)

// TREE VIEW

QModelIndex QAccessibleTree::indexFromLogical(int row, int column) const
{
    if (!isValid() || !view()->model())
        return QModelIndex();

    const QTreeView *treeView = qobject_cast<const QTreeView*>(view());
    if (Q_UNLIKELY(row < 0 || column < 0 || treeView->d_func()->viewItems.count() <= row)) {
        qWarning() << "QAccessibleTree::indexFromLogical: invalid index: " << row << column << " for " << treeView;
        return QModelIndex();
    }
    QModelIndex modelIndex = treeView->d_func()->viewItems.at(row).index;

    if (modelIndex.isValid() && column > 0) {
        modelIndex = view()->model()->index(modelIndex.row(), column, modelIndex.parent());
    }
    return modelIndex;
}

QAccessibleInterface *QAccessibleTree::childAt(int x, int y) const
{
    if (!view()->model())
        return nullptr;
    QPoint viewportOffset = view()->viewport()->mapTo(view(), QPoint(0,0));
    QPoint indexPosition = view()->mapFromGlobal(QPoint(x, y) - viewportOffset);

    QModelIndex index = view()->indexAt(indexPosition);
    if (!index.isValid())
        return nullptr;

    const QTreeView *treeView = qobject_cast<const QTreeView*>(view());
    int row = treeView->d_func()->viewIndex(index) + (horizontalHeader() ? 1 : 0);
    int column = index.column();

    int i = row * view()->model()->columnCount() + column;
    return child(i);
}

QAccessibleInterface *QAccessibleTree::focusChild() const
{
    QModelIndex index = view()->currentIndex();
    if (!index.isValid())
        return nullptr;

    const QTreeView *treeView = qobject_cast<const QTreeView*>(view());
    int row = treeView->d_func()->viewIndex(index) + (horizontalHeader() ? 1 : 0);
    int column = index.column();

    int i = row * view()->model()->columnCount() + column;
    return child(i);
}

int QAccessibleTree::childCount() const
{
    const QTreeView *treeView = qobject_cast<const QTreeView*>(view());
    Q_ASSERT(treeView);
    if (!view()->model())
        return 0;

    int hHeader = horizontalHeader() ? 1 : 0;
    return (treeView->d_func()->viewItems.count() + hHeader)* view()->model()->columnCount();
}

QAccessibleInterface *QAccessibleTree::child(int logicalIndex) const
{
    if (logicalIndex < 0 || !view()->model() || !view()->model()->columnCount())
        return nullptr;

    QAccessibleInterface *iface = nullptr;
    int index = logicalIndex;

    if (horizontalHeader()) {
        if (index < view()->model()->columnCount()) {
            iface = new QAccessibleTableHeaderCell(view(), index, Qt::Horizontal);
        } else {
            index -= view()->model()->columnCount();
        }
    }

    if (!iface) {
        int row = index / view()->model()->columnCount();
        int column = index % view()->model()->columnCount();
        QModelIndex modelIndex = indexFromLogical(row, column);
        if (!modelIndex.isValid())
            return nullptr;
        iface = new QAccessibleTableCell(view(), modelIndex, cellRole());
    }
    QAccessible::registerAccessibleInterface(iface);
    // ### FIXME: get interfaces from the cache instead of re-creating them
    return iface;
}

int QAccessibleTree::rowCount() const
{
    const QTreeView *treeView = qobject_cast<const QTreeView*>(view());
    Q_ASSERT(treeView);
    return treeView->d_func()->viewItems.count();
}

int QAccessibleTree::indexOfChild(const QAccessibleInterface *iface) const
{
    if (!view()->model())
        return -1;
    QAccessibleInterface *parent = iface->parent();
    if (parent->object() != view())
        return -1;

    if (iface->role() == QAccessible::TreeItem) {
        const QAccessibleTableCell* cell = static_cast<const QAccessibleTableCell*>(iface);
        const QTreeView *treeView = qobject_cast<const QTreeView*>(view());
        Q_ASSERT(treeView);
        int row = treeView->d_func()->viewIndex(cell->m_index) + (horizontalHeader() ? 1 : 0);
        int column = cell->m_index.column();

        int index = row * view()->model()->columnCount() + column;
        return index;
    } else if (iface->role() == QAccessible::ColumnHeader){
        const QAccessibleTableHeaderCell* cell = static_cast<const QAccessibleTableHeaderCell*>(iface);
        return cell->index;
    } else {
        qWarning() << "WARNING QAccessibleTable::indexOfChild invalid child"
                   << iface->role() << iface->text(QAccessible::Name);
    }
    // FIXME: add scrollbars and don't just ignore them
    return -1;
}

QAccessibleInterface *QAccessibleTree::cellAt(int row, int column) const
{
    QModelIndex index = indexFromLogical(row, column);
    if (Q_UNLIKELY(!index.isValid())) {
        qWarning("Requested invalid tree cell: %d %d", row, column);
        return nullptr;
    }
    const QTreeView *treeView = qobject_cast<const QTreeView*>(view());
    Q_ASSERT(treeView);
    int logicalIndex = treeView->d_func()->accessibleTable2Index(index);

    return child(logicalIndex); // FIXME ### new QAccessibleTableCell(view(), index, cellRole());
}

QString QAccessibleTree::rowDescription(int) const
{
    return QString(); // no headers for rows in trees
}

bool QAccessibleTree::isRowSelected(int row) const
{
    if (!view()->selectionModel())
        return false;
    QModelIndex index = indexFromLogical(row);
    return view()->selectionModel()->isRowSelected(index.row(), index.parent());
}

bool QAccessibleTree::selectRow(int row)
{
    if (!view()->selectionModel())
        return false;
    QModelIndex index = indexFromLogical(row);

    if (!index.isValid() || view()->selectionBehavior() == QAbstractItemView::SelectColumns)
        return false;

    switch (view()->selectionMode()) {
    case QAbstractItemView::NoSelection:
        return false;
    case QAbstractItemView::SingleSelection:
        if ((view()->selectionBehavior() != QAbstractItemView::SelectRows) && (columnCount() > 1))
            return false;
        view()->clearSelection();
        break;
    case QAbstractItemView::ContiguousSelection:
        if ((!row || !view()->selectionModel()->isRowSelected(row - 1, view()->rootIndex()))
            && !view()->selectionModel()->isRowSelected(row + 1, view()->rootIndex()))
            view()->clearSelection();
        break;
    default:
        break;
    }

    view()->selectionModel()->select(index, QItemSelectionModel::Select | QItemSelectionModel::Rows);
    return true;
}

#endif // QT_CONFIG(treeview)

// TABLE CELL

QAccessibleTableCell::QAccessibleTableCell(QAbstractItemView *view_, const QModelIndex &index_, QAccessible::Role role_)
    : /* QAccessibleSimpleEditableTextInterface(this), */ view(view_), m_index(index_), m_role(role_)
{
    if (Q_UNLIKELY(!index_.isValid()))
        qWarning() << "QAccessibleTableCell::QAccessibleTableCell with invalid index: " << index_;
}

void *QAccessibleTableCell::interface_cast(QAccessible::InterfaceType t)
{
    if (t == QAccessible::TableCellInterface)
        return static_cast<QAccessibleTableCellInterface*>(this);
    if (t == QAccessible::ActionInterface)
        return static_cast<QAccessibleActionInterface*>(this);
    return nullptr;
}

int QAccessibleTableCell::columnExtent() const { return 1; }
int QAccessibleTableCell::rowExtent() const { return 1; }

QList<QAccessibleInterface*> QAccessibleTableCell::rowHeaderCells() const
{
    QList<QAccessibleInterface*> headerCell;
    if (verticalHeader()) {
        // FIXME
        headerCell.append(new QAccessibleTableHeaderCell(view, m_index.row(), Qt::Vertical));
    }
    return headerCell;
}

QList<QAccessibleInterface*> QAccessibleTableCell::columnHeaderCells() const
{
    QList<QAccessibleInterface*> headerCell;
    if (horizontalHeader()) {
        // FIXME
        headerCell.append(new QAccessibleTableHeaderCell(view, m_index.column(), Qt::Horizontal));
    }
    return headerCell;
}

QHeaderView *QAccessibleTableCell::horizontalHeader() const
{
    QHeaderView *header = nullptr;

    if (false) {
#if QT_CONFIG(tableview)
    } else if (const QTableView *tv = qobject_cast<const QTableView*>(view)) {
        header = tv->horizontalHeader();
#endif
#if QT_CONFIG(treeview)
    } else if (const QTreeView *tv = qobject_cast<const QTreeView*>(view)) {
        header = tv->header();
#endif
    }

    return header;
}

QHeaderView *QAccessibleTableCell::verticalHeader() const
{
    QHeaderView *header = nullptr;
#if QT_CONFIG(tableview)
    if (const QTableView *tv = qobject_cast<const QTableView*>(view))
        header = tv->verticalHeader();
#endif
    return header;
}

int QAccessibleTableCell::columnIndex() const
{
    if (!isValid())
        return -1;
    return m_index.column();
}

int QAccessibleTableCell::rowIndex() const
{
    if (!isValid())
        return -1;
#if QT_CONFIG(treeview)
    if (role() == QAccessible::TreeItem) {
       const QTreeView *treeView = qobject_cast<const QTreeView*>(view);
       Q_ASSERT(treeView);
       int row = treeView->d_func()->viewIndex(m_index);
       return row;
    }
#endif
    return m_index.row();
}

bool QAccessibleTableCell::isSelected() const
{
    if (!isValid())
        return false;
    return view->selectionModel()->isSelected(m_index);
}

QStringList QAccessibleTableCell::actionNames() const
{
    QStringList names;
    names << toggleAction();
    return names;
}

void QAccessibleTableCell::doAction(const QString& actionName)
{
    if (actionName == toggleAction()) {
#if defined(Q_OS_ANDROID)
        QAccessibleInterface *parentInterface = parent();
        while (parentInterface){
            if (parentInterface->role() == QAccessible::ComboBox) {
                selectCell();
                parentInterface->actionInterface()->doAction(pressAction());
                return;
            } else {
                parentInterface = parentInterface->parent();
            }
        }
#endif
        if (isSelected()) {
            unselectCell();
        } else {
            selectCell();
        }
    }
}

QStringList QAccessibleTableCell::keyBindingsForAction(const QString &) const
{
    return QStringList();
}


void QAccessibleTableCell::selectCell()
{
    if (!isValid())
        return;
    QAbstractItemView::SelectionMode selectionMode = view->selectionMode();
    if (selectionMode == QAbstractItemView::NoSelection)
        return;
    Q_ASSERT(table());
    QAccessibleTableInterface *cellTable = table()->tableInterface();

    switch (view->selectionBehavior()) {
    case QAbstractItemView::SelectItems:
        break;
    case QAbstractItemView::SelectColumns:
        if (cellTable)
            cellTable->selectColumn(m_index.column());
        return;
    case QAbstractItemView::SelectRows:
        if (cellTable)
            cellTable->selectRow(m_index.row());
        return;
    }

    if (selectionMode == QAbstractItemView::SingleSelection) {
        view->clearSelection();
    }

    view->selectionModel()->select(m_index, QItemSelectionModel::Select);
}

void QAccessibleTableCell::unselectCell()
{
    if (!isValid())
        return;
    QAbstractItemView::SelectionMode selectionMode = view->selectionMode();
    if (selectionMode == QAbstractItemView::NoSelection)
        return;

    QAccessibleTableInterface *cellTable = table()->tableInterface();

    switch (view->selectionBehavior()) {
    case QAbstractItemView::SelectItems:
        break;
    case QAbstractItemView::SelectColumns:
        if (cellTable)
            cellTable->unselectColumn(m_index.column());
        return;
    case QAbstractItemView::SelectRows:
        if (cellTable)
            cellTable->unselectRow(m_index.row());
        return;
    }

    //If the mode is not MultiSelection or ExtendedSelection and only
    //one cell is selected it cannot be unselected by the user
    if ((selectionMode != QAbstractItemView::MultiSelection)
        && (selectionMode != QAbstractItemView::ExtendedSelection)
        && (view->selectionModel()->selectedIndexes().count() <= 1))
        return;

    view->selectionModel()->select(m_index, QItemSelectionModel::Deselect);
}

QAccessibleInterface *QAccessibleTableCell::table() const
{
    return QAccessible::queryAccessibleInterface(view);
}

QAccessible::Role QAccessibleTableCell::role() const
{
    return m_role;
}

QAccessible::State QAccessibleTableCell::state() const
{
    QAccessible::State st;
    if (!isValid())
        return st;

    QRect globalRect = view->rect();
    globalRect.translate(view->mapToGlobal(QPoint(0,0)));
    if (!globalRect.intersects(rect()))
        st.invisible = true;

    if (view->selectionModel()->isSelected(m_index))
        st.selected = true;
    if (view->selectionModel()->currentIndex() == m_index)
        st.focused = true;

    QVariant checkState = m_index.model()->data(m_index, Qt::CheckStateRole);
    if (checkState.toInt() == Qt::Checked)
        st.checked = true;

    Qt::ItemFlags flags = m_index.flags();
    if ((flags & Qt::ItemIsUserCheckable) && checkState.isValid())
        st.checkable = true;
    if (flags & Qt::ItemIsSelectable) {
        st.selectable = true;
        st.focusable = true;
        if (view->selectionMode() == QAbstractItemView::MultiSelection)
            st.multiSelectable = true;
        if (view->selectionMode() == QAbstractItemView::ExtendedSelection)
            st.extSelectable = true;
    }
#if QT_CONFIG(treeview)
    if (m_role == QAccessible::TreeItem) {
        const QTreeView *treeView = qobject_cast<const QTreeView*>(view);
        if (treeView->model()->hasChildren(m_index))
            st.expandable = true;
        if (treeView->isExpanded(m_index))
            st.expanded = true;
    }
#endif
    return st;
}


QRect QAccessibleTableCell::rect() const
{
    QRect r;
    if (!isValid())
        return r;
    r = view->visualRect(m_index);

    if (!r.isNull()) {
        r.translate(view->viewport()->mapTo(view, QPoint(0,0)));
        r.translate(view->mapToGlobal(QPoint(0, 0)));
    }
    return r;
}

QString QAccessibleTableCell::text(QAccessible::Text t) const
{
    QString value;
    if (!isValid())
        return value;
    QAbstractItemModel *model = view->model();
    switch (t) {
    case QAccessible::Name:
        value = model->data(m_index, Qt::AccessibleTextRole).toString();
        if (value.isEmpty())
            value = model->data(m_index, Qt::DisplayRole).toString();
        break;
    case QAccessible::Description:
        value = model->data(m_index, Qt::AccessibleDescriptionRole).toString();
        break;
    default:
        break;
    }
    return value;
}

void QAccessibleTableCell::setText(QAccessible::Text /*t*/, const QString &text)
{
    if (!isValid() || !(m_index.flags() & Qt::ItemIsEditable))
        return;
    view->model()->setData(m_index, text);
}

bool QAccessibleTableCell::isValid() const
{
    return view && !qt_widget_private(view)->data.in_destructor
            && view->model() && m_index.isValid();
}

QAccessibleInterface *QAccessibleTableCell::parent() const
{
    return QAccessible::queryAccessibleInterface(view);
}

QAccessibleInterface *QAccessibleTableCell::child(int) const
{
    return nullptr;
}

QAccessibleTableHeaderCell::QAccessibleTableHeaderCell(QAbstractItemView *view_, int index_, Qt::Orientation orientation_)
    : view(view_), index(index_), orientation(orientation_)
{
    Q_ASSERT(index_ >= 0);
}

QAccessible::Role QAccessibleTableHeaderCell::role() const
{
    if (orientation == Qt::Horizontal)
        return QAccessible::ColumnHeader;
    return QAccessible::RowHeader;
}

QAccessible::State QAccessibleTableHeaderCell::state() const
{
    QAccessible::State s;
    if (QHeaderView *h = headerView()) {
        s.invisible = !h->testAttribute(Qt::WA_WState_Visible);
        s.disabled = !h->isEnabled();
    }
    return s;
}

QRect QAccessibleTableHeaderCell::rect() const
{
    QHeaderView *header = nullptr;
    if (false) {
#if QT_CONFIG(tableview)
    } else if (const QTableView *tv = qobject_cast<const QTableView*>(view)) {
        if (orientation == Qt::Horizontal) {
            header = tv->horizontalHeader();
        } else {
            header = tv->verticalHeader();
        }
#endif
#if QT_CONFIG(treeview)
    } else if (const QTreeView *tv = qobject_cast<const QTreeView*>(view)) {
        header = tv->header();
#endif
    }
    if (!header)
        return QRect();
    QPoint zero = header->mapToGlobal(QPoint(0, 0));
    int sectionSize = header->sectionSize(index);
    int sectionPos = header->sectionPosition(index);
    return orientation == Qt::Horizontal
            ? QRect(zero.x() + sectionPos, zero.y(), sectionSize, header->height())
            : QRect(zero.x(), zero.y() + sectionPos, header->width(), sectionSize);
}

QString QAccessibleTableHeaderCell::text(QAccessible::Text t) const
{
    QAbstractItemModel *model = view->model();
    QString value;
    switch (t) {
    case QAccessible::Name:
        value = model->headerData(index, orientation, Qt::AccessibleTextRole).toString();
        if (value.isEmpty())
            value = model->headerData(index, orientation, Qt::DisplayRole).toString();
        break;
    case QAccessible::Description:
        value = model->headerData(index, orientation, Qt::AccessibleDescriptionRole).toString();
        break;
    default:
        break;
    }
    return value;
}

void QAccessibleTableHeaderCell::setText(QAccessible::Text, const QString &)
{
    return;
}

bool QAccessibleTableHeaderCell::isValid() const
{
    return view && !qt_widget_private(view)->data.in_destructor
            && view->model() && (index >= 0)
            && ((orientation == Qt::Horizontal) ? (index < view->model()->columnCount()) : (index < view->model()->rowCount()));
}

QAccessibleInterface *QAccessibleTableHeaderCell::parent() const
{
    return QAccessible::queryAccessibleInterface(view);
}

QAccessibleInterface *QAccessibleTableHeaderCell::child(int) const
{
    return nullptr;
}

QHeaderView *QAccessibleTableHeaderCell::headerView() const
{
    QHeaderView *header = nullptr;
    if (false) {
#if QT_CONFIG(tableview)
    } else if (const QTableView *tv = qobject_cast<const QTableView*>(view)) {
        if (orientation == Qt::Horizontal) {
            header = tv->horizontalHeader();
        } else {
            header = tv->verticalHeader();
        }
#endif
#if QT_CONFIG(treeview)
    } else if (const QTreeView *tv = qobject_cast<const QTreeView*>(view)) {
        header = tv->header();
#endif
    }
    return header;
}

QT_END_NAMESPACE

#endif // QT_CONFIG(accessibility)
