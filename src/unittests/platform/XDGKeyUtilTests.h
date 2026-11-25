/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2026 Deskflow Developers
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#pragma once

#include <QTest>

class XDGKeyUtilTests : public QObject
{
  Q_OBJECT

private Q_SLOTS:
  void mapKeySymToKeyID_unicodeKeySym_mapsToCodePoint_data();
  void mapKeySymToKeyID_unicodeKeySym_mapsToCodePoint();
};
