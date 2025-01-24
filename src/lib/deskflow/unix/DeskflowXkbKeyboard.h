/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2012 - 2021 Symless Ltd.
 * SPDX-FileCopyrightText: (C) 2002 Chris Schoeneman
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#if WINAPI_XWINDOWS

#pragma once

#include <X11/XKBlib.h>
#include <X11/extensions/XKBrules.h>
#include <cstdio>

namespace deskflow {

namespace linux {

class DeskflowXkbKeyboard
{
  XkbRF_VarDefsRec m_data = {};

public:
  DeskflowXkbKeyboard();
  DeskflowXkbKeyboard(const DeskflowXkbKeyboard &) = delete;
  DeskflowXkbKeyboard &operator=(const DeskflowXkbKeyboard &) = delete;

  const char *getLayout() const;
  const char *getVariant() const;

  ~DeskflowXkbKeyboard();
};

} // namespace linux

} // namespace deskflow

#endif // WINAPI_XWINDOWS
