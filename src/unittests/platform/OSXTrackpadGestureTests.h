/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2026 Lucas Magotteaux
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#include "base/Log.h"

#include <QTest>

class OSXTrackpadGestureTests : public QObject
{
  Q_OBJECT
private Q_SLOTS:
  void trackpadScrollsAreForwarded_data();
  void trackpadScrollsAreForwarded();
  void forwardedScrollKeepsItsPhases();
  void scaleScrollInvertsAndScales();

private:
  Log m_log;
};
