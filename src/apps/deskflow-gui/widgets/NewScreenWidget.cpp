/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2025 Deskflow Developers
 * SPDX-FileCopyrightText: (C) 2012 - 2016 Symless Ltd.
 * SPDX-FileCopyrightText: (C) 2008 Volker Lanz <vl@fidra.de>
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#include "NewScreenWidget.h"
#include "ScreenSetupModel.h"

#include <QDrag>
#include <QLabel>
#include <QMimeData>
#include <QMouseEvent>

NewScreenWidget::NewScreenWidget(QWidget *parent) : QLabel(parent)
{
}

void NewScreenWidget::mousePressEvent(QMouseEvent *event)
{
  Screen newScreen(tr("Unnamed"));

  QByteArray itemData;
  QDataStream dataStream(&itemData, QIODevice::WriteOnly);
  dataStream << -1 << -1 << newScreen;

  QMimeData *pMimeData = new QMimeData;
  pMimeData->setData(ScreenSetupModel::mimeType(), itemData);

  QDrag *pDrag = new QDrag(this);
  pDrag->setMimeData(pMimeData);
  pDrag->setPixmap(pixmap());
  pDrag->setHotSpot(event->pos());
  pDrag->exec(Qt::CopyAction, Qt::CopyAction);
}
