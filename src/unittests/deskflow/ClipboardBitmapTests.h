/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2026 Deskflow Developers
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#include "arch/Arch.h"
#include "base/Log.h"

#include <QTest>

class ClipboardBitmapTests : public QObject
{
  Q_OBJECT
private Q_SLOTS:
  void bmpToDib_bitfieldsHeader_keepsMasks();
  void bmpToDib_v5HeaderWithGap_dropsOnlyTheGap();
  void bmpToDib_notBmp_isRejected();
  void dibToBmp_v5Header_pointsPastHeader();
  void dibToBmp_palette_pointsPastPalette();
  void roundTrip_v5Header_isUnchanged();
  void roundTrip_plainHeader_isUnchanged();
  void dibPixelOffset_paletteLargerThanData_isInvalid();

private:
  Arch m_arch;
  Log m_log;
};
