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
#include <memory>

#include "DeskflowXkbKeyboard.h"
#include "base/Log.h"

namespace deskflow {

namespace linux {

DeskflowXkbKeyboard::DeskflowXkbKeyboard()
{
  using XkbDisplay = std::unique_ptr<Display, decltype(&XCloseDisplay)>;
  XkbDisplay display(XkbOpenDisplay(nullptr, nullptr, nullptr, nullptr, nullptr, nullptr), &XCloseDisplay);

  if (display) {
    if (!XkbRF_GetNamesProp(display.get(), nullptr, &m_data)) {
      LOG((CLOG_WARN "error reading keyboard layouts"));
    }
  } else {
    LOG((CLOG_WARN "can't open xkb display during reading languages"));
  }
}

const char *DeskflowXkbKeyboard::getLayout() const
{
  return m_data.layout ? m_data.layout : "us";
}

const char *DeskflowXkbKeyboard::getVariant() const
{
  return m_data.variant ? m_data.variant : "";
}

DeskflowXkbKeyboard::~DeskflowXkbKeyboard()
{
  std::free(m_data.model);
  std::free(m_data.layout);
  std::free(m_data.variant);
  std::free(m_data.options);
}

} // namespace linux

} // namespace deskflow

#endif // WINAPI_XWINDOWS
