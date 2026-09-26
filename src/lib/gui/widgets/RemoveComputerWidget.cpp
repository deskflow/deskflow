/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2012 - 2016 Synergy App Ltd
 * SPDX-FileCopyrightText: (C) 2008 Volker Lanz <vl@fidra.de>
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#include "RemoveComputerWidget.h"

#include <QDragEnterEvent>
#include <QDropEvent>
#include <QMimeData>

#include "ComputerSetupModel.h"

void RemoveComputerWidget::dragEnterEvent(QDragEnterEvent *event)
{
  if (event->mimeData()->hasFormat(ComputerSetupModel::mimeType())) {
    event->setDropAction(Qt::MoveAction);
    event->accept();
  } else
    event->ignore();
}

void RemoveComputerWidget::dropEvent(QDropEvent *event)
{
  if (event->mimeData()->hasFormat(ComputerSetupModel::mimeType())) {
    event->acceptProposedAction();
    Q_EMIT computerRemoved();
  } else {
    event->ignore();
  }
}
