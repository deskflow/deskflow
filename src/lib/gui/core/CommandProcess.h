/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2014 Symless Ltd.
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#pragma once

#include <QObject>
#include <QStringList>

class CommandProcess : public QObject
{
  Q_OBJECT

public:
  CommandProcess(QString cmd, QStringList arguments, QString input = "");

signals:
  void finished();

public slots:
  QString run();

private:
  QString m_Command;
  QStringList m_Arguments;
  QString m_Input;
};
