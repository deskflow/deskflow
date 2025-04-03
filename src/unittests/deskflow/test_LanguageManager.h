/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2025 Chris Rizzitello <sithlord48@gmail.com>
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#include "../../lib/deskflow/languages/LanguageManager.h"

#include <base/Log.h>

#include <QObject>

class LanguageManager_Test : public QObject
{
  Q_OBJECT
private slots:
  void initTestCase();
  // Test are run in order top to bottom
  void remoteLanguages();
  void localLanguage();
  void missedLanguage();
  void serializeLocalLanguages();
  void languageInstall();

private:
  Arch m_arch;
  Log m_log;
};
