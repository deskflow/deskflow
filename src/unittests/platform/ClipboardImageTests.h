/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2026 Synergy App Ltd
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#pragma once

#include "arch/Arch.h"
#include "base/Log.h"

#include <QTest>

class ClipboardImageTests : public QObject
{
  Q_OBJECT

private Q_SLOTS:
  void initTestCase();
  void imageToDib_png_returnsDibWithImageSize();
  void imageToDib_bmp_returnsDibWithImageSize();
  void imageToDib_jpeg_returnsDibWithImageSize();
  void dibToImage_dibFromPng_keepsPixels();
  void toPng_jpeg_returnsPngWithImageSize();
  void imageToDib_notAnImage_returnsEmpty();

private:
  Arch m_arch;
  Log m_log;
};
