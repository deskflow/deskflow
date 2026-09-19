/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2026 Deskflow Developers
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#pragma once

#include <QTest>

class KeySequenceWidgetTests : public QObject
{
  Q_OBJECT

private Q_SLOTS:
  void focusPolicy_default_strongFocus();
  void keyClick_stoppedWidget_recordsKey();
};
