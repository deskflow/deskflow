/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2015 Symless Ltd.
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#pragma once

#include <QString>

class CoreTool
{
public:
  QString getProfileDir() const;
  QString getInstalledDir() const;
  QString getArch() const;
};
