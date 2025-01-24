/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2012 - 2016 Symless Ltd.
 * SPDX-FileCopyrightText: (C) 2008 Volker Lanz <vl@fidra.de>
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#include "TrashScreenWidget.h"

#include <QDragEnterEvent>
#include <QDropEvent>
#include <QMimeData>

#include "ScreenSetupModel.h"

void TrashScreenWidget::dragEnterEvent(QDragEnterEvent *event)
{
  if (event->mimeData()->hasFormat(ScreenSetupModel::mimeType())) {
    event->setDropAction(Qt::MoveAction);
    event->accept();
  } else
    event->ignore();
}

void TrashScreenWidget::dropEvent(QDropEvent *event)
{
  if (event->mimeData()->hasFormat(ScreenSetupModel::mimeType())) {
    event->acceptProposedAction();
    Q_EMIT screenRemoved();
  } else {
    event->ignore();
  }
}
