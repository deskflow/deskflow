/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2025 Chris Rizzitello <sithlord48@gmail.com>
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */
#include "base/Log.h"

#include <QTest>

namespace deskflow {
class KeyMapTests : public QObject
{
  Q_OBJECT
private slots:
  void findBestKey_requiredDown_matchExactFirstItem();
  void findBestKey_requiredAndExtraSensitiveDown_matchExactFirstItem();
  void findBestKey_requiredAndExtraSensitiveDown_matchExactSecondItem();
  void findBestKey_extraSensitiveDown_matchExactSecondItem();
  void findBestKey_noRequiredDown_matchOneRequiredChangeItem();
  void findBestKey_onlyOneRequiredDown_matchTwoRequiredChangesItem();
  void findBestKey_noRequiredDown_cannotMatch();
  void isCommand();
  void mapkey();

private:
  Arch m_arch;
  Log m_log;
};
} // namespace deskflow
