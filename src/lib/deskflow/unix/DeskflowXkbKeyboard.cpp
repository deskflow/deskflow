/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2012 - 2021 Symless Ltd.
 * SPDX-FileCopyrightText: (C) 2002 Chris Schoeneman
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
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
