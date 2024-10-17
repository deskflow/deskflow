/*
 * Deskflow -- mouse and keyboard sharing utility
 * Copyright (C) 2012-2021 Symless Ltd.
 * Copyright (C) 2002 Chris Schoeneman
 *
 * This package is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * found in the file LICENSE that should have accompanied this file.
 *
 * This package is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
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
