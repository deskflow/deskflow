/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2025 Deskflow Developers
 * SPDX-FileCopyrightText: (C) 2012 - 2016 Synergy App Ltd
 * SPDX-FileCopyrightText: (C) 2008 Volker Lanz <vl@fidra.de>
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#include "NewComputerWidget.h"
#include "ComputerSetupModel.h"

#include <QDrag>
#include <QLabel>
#include <QMimeData>
#include <QMouseEvent>

NewComputerWidget::NewComputerWidget(QWidget *parent) : QLabel(parent)
{
  // do nothing
}

void NewComputerWidget::mousePressEvent(QMouseEvent *)
{
  //: Used as the hostname. Translation may not contain spaces
  Screen newScreen(tr("Unnamed"));

  QByteArray itemData;
  QDataStream dataStream(&itemData, QIODevice::WriteOnly);
  dataStream << -1 << -1 << newScreen;

  auto *pMimeData = new QMimeData;
  pMimeData->setData(ComputerSetupModel::mimeType(), itemData);

  auto *pDrag = new QDrag(this);
  pDrag->setMimeData(pMimeData);
  pDrag->setPixmap(pixmap());
  pDrag->setHotSpot(QPoint(width() / 2, height() / 2));
  pDrag->exec(Qt::CopyAction, Qt::CopyAction);
}
