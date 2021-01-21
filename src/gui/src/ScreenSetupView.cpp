/*
 * barrier -- mouse and keyboard sharing utility
 * Copyright (C) 2012-2016 Symless Ltd.
 * Copyright (C) 2008 Volker Lanz (vl@fidra.de)
 *
 * This package is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * found in the file LICENSE that should have accompanied this file.
 *
 * This package is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "ScreenSetupView.h"
#include "ScreenSetupModel.h"
#include "ScreenSettingsDialog.h"

#include <QtCore>
#include <QtGui>
#include <QHeaderView>

ScreenSetupView::ScreenSetupView(QWidget* parent) :
    QTableView(parent)
{
    setDropIndicatorShown(true);
    setDragDropMode(DragDrop);
    setSelectionMode(SingleSelection);
    setTabKeyNavigation(false);

    setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);

    setIconSize(QSize(64, 64));
    horizontalHeader()->hide();
    verticalHeader()->hide();
}

void ScreenSetupView::setModel(QAbstractItemModel* model)
{
    QTableView::setModel(model);
    setTableSize();
}

ScreenSetupModel* ScreenSetupView::model() const
{
    return qobject_cast<ScreenSetupModel*>(QTableView::model());
}

void ScreenSetupView::setTableSize()
{
    for (int i = 0; i < model()->columnCount(); i++)
        setColumnWidth(i, width() / model()->columnCount());

    for (int i = 0; i < model()->rowCount(); i++)
        setRowHeight(i, height() / model()->rowCount());
}

void ScreenSetupView::resizeEvent(QResizeEvent* event)
{
    setTableSize();
    event->ignore();
}

void ScreenSetupView::enter(const QModelIndex& index)
{
    if (!index.isValid())
        return;
    Screen& screen = model()->screen(index);
    if (screen.isNull())
        screen = Screen(tr("Unnamed"));
    ScreenSettingsDialog dlg(this, &screen);
    dlg.exec();
}

void ScreenSetupView::remove(const QModelIndex& index)
{
    if (!index.isValid())
        return;
    Screen& screen = model()->screen(index);
    if (!screen.isNull()) {
        screen = Screen();
        emit dataChanged(index, index);
    }
}

void ScreenSetupView::keyPressEvent(QKeyEvent* event)
{
    if (event->key() == Qt::Key_Return || event->key() == Qt::Key_Delete)
    {
        QModelIndexList indexes = selectedIndexes();
        if (indexes.count() == 1 && indexes[0].isValid())
        {
            if (event->key() == Qt::Key_Return)
                enter(indexes[0]);
            else if (event->key() == Qt::Key_Delete)
                remove(indexes[0]);
        }
        // Do not let base handle the event, at least not for return key because it
        // results in next esc/return key in the opened Screen Settings dialog not
        // only closing that but also the parent Server Configuration dialog.
    }
    else if ((event->modifiers() & Qt::ControlModifier)
        && (event->key() == Qt::Key_Left || event->key() == Qt::Key_Right
         || event->key() == Qt::Key_Up   || event->key() == Qt::Key_Down))
    {
        QModelIndexList indexes = selectedIndexes();
        if (indexes.count() == 1 && indexes[0].isValid())
        {
            const QModelIndex& fromIndex = indexes[0];
            QModelIndex toIndex;

            if (event->key() == Qt::Key_Left)
                toIndex = fromIndex.sibling(fromIndex.row(), fromIndex.column() - 1);
            else if (event->key() == Qt::Key_Right)
                toIndex = fromIndex.sibling(fromIndex.row(), fromIndex.column() + 1);
            else if (event->key() == Qt::Key_Up)
                toIndex = fromIndex.sibling(fromIndex.row() - 1, fromIndex.column());
            else if (event->key() == Qt::Key_Down)
                toIndex = fromIndex.sibling(fromIndex.row() + 1, fromIndex.column());

            if (toIndex.isValid() && fromIndex != toIndex)
                std::swap(model()->screen(fromIndex), model()->screen(toIndex));
        }
        // In this case let base also handle the event, because it will proceed moving
        // the selection to target, update the view according to model changes etc.
        QTableView::keyPressEvent(event);
    }
    else
    {
        QTableView::keyPressEvent(event);
    }
}

void ScreenSetupView::mouseDoubleClickEvent(QMouseEvent* event)
{
    if (event->buttons() & Qt::LeftButton)
    {
        int col = columnAt(event->pos().x());
        int row = rowAt(event->pos().y());
        enter(model()->createIndex(row, col));
    }
    else
        event->ignore();
}

void ScreenSetupView::dragEnterEvent(QDragEnterEvent* event)
{
    // we accept anything that enters us by a drag as long as the
    // mime type is okay. anything else is dealt with in dragMoveEvent()
    if (event->mimeData()->hasFormat(ScreenSetupModel::mimeType()))
        event->accept();
    else
        event->ignore();
}

void ScreenSetupView::dragMoveEvent(QDragMoveEvent* event)
{
    if (event->mimeData()->hasFormat(ScreenSetupModel::mimeType()))
    {
        // where does the event come from? myself or someone else?
        if (event->source() == this)
        {
            // myself is ok, but then it must be a move action, never a copy
            event->setDropAction(Qt::MoveAction);
            event->accept();
        }
        else
        {
            int col = columnAt(event->pos().x());
            int row = rowAt(event->pos().y());

            // a drop from outside is not allowed if there's a screen already there.
            if (!model()->screen(col, row).isNull())
                event->ignore();
            else
                event->acceptProposedAction();
        }
    }
    else
        event->ignore();
}

// this is reimplemented from QAbstractItemView::startDrag()
void ScreenSetupView::startDrag(Qt::DropActions)
{
    QModelIndexList indexes = selectedIndexes();

    if (indexes.count() != 1)
        return;

    QMimeData* pData = model()->mimeData(indexes);
    if (pData == NULL)
        return;

    QPixmap pixmap = *model()->screen(indexes[0]).pixmap();
    QDrag* pDrag = new QDrag(this);
    pDrag->setPixmap(pixmap);
    pDrag->setMimeData(pData);
    pDrag->setHotSpot(QPoint(pixmap.width() / 2, pixmap.height() / 2));

    if (pDrag->exec(Qt::MoveAction, Qt::MoveAction) == Qt::MoveAction)
    {
        selectionModel()->clear();

        // make sure to only delete the drag source if screens weren't swapped
        // see ScreenSetupModel::dropMimeData
        if (!model()->screen(indexes[0]).swapped())
            model()->screen(indexes[0]) = Screen();
        else
            model()->screen(indexes[0]).setSwapped(false);
    }
}

QStyleOptionViewItem ScreenSetupView::viewOptions() const
{
    QStyleOptionViewItem option = QTableView::viewOptions();
    option.showDecorationSelected = true;
    option.decorationPosition = QStyleOptionViewItem::Top;
    option.displayAlignment = Qt::AlignCenter;
    option.textElideMode = Qt::ElideMiddle;
    return option;
}

