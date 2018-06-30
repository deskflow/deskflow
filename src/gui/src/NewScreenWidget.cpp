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

#include "NewScreenWidget.h"
#include "ScreenSetupModel.h"

#include <QtCore>
#include <QtGui>

NewScreenWidget::NewScreenWidget(QWidget* parent) :
    QLabel(parent)
{
}

void NewScreenWidget::mousePressEvent(QMouseEvent* event)
{
    Screen newScreen(tr("Unnamed"));

    QByteArray itemData;
    QDataStream dataStream(&itemData, QIODevice::WriteOnly);
    dataStream << -1 << -1 << newScreen;

    QMimeData* pMimeData = new QMimeData;
    pMimeData->setData(ScreenSetupModel::mimeType(), itemData);

    QDrag* pDrag = new QDrag(this);
    pDrag->setMimeData(pMimeData);
    pDrag->setPixmap(*pixmap());
    pDrag->setHotSpot(event->pos());

    pDrag->exec(Qt::CopyAction, Qt::CopyAction);
}

