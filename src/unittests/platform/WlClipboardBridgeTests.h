/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2026 Deskflow Developers
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#pragma once

#include "arch/Arch.h"
#include "base/Log.h"

#include <QTest>

class WlClipboardBridgeTests : public QObject
{
  Q_OBJECT

private Q_SLOTS:
  void initTestCase();
  void readData_noClipboard_returnsEmpty();
  void localChange_htmlAndText_callbackReceivesBothOnce();
  void setData_remoteHtml_writesClipboardWithoutCallback();
  void toClipboard_textHtmlPng_addsAllFormats();
  void toClipboard_utf16Html_convertsToUtf8();
  void fromClipboard_bitmapAndText_prefersPng();
  void fromClipboard_textAndHtml_prefersText();
  void fromClipboard_htmlOnly_returnsHtml();

private:
  Arch m_arch;
  Log m_log;
};
