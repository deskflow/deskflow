/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2012 - 2016 Symless Ltd.
 * SPDX-FileCopyrightText: (C) 2002 Chris Schoeneman
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#pragma once

#include "common/IInterface.h"
#include "deskflow/IApp.h"

#include <string>
#include <vector>

class IAppUtil : public IInterface
{
public:
  virtual void adoptApp(IApp *app) = 0;
  virtual IApp &app() const = 0;
  virtual int run(int argc, char **argv) = 0;
  virtual void startNode() = 0;
  virtual std::vector<std::string> getKeyboardLayoutList() = 0;
  virtual std::string getCurrentLanguageCode() = 0;
  virtual void showNotification(const std::string &title, const std::string &text) const = 0;
};
