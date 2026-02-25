/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2025 Chris Rizzitello <sithlord48@gmail.com>
 * SPDX-FileCopyrightText: (C) 2024 Symless Ltd.
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#pragma once

#include <QWidget>

namespace deskflow::gui::core {

class WaylandWarnings
{
public:
  explicit WaylandWarnings() = default;

  void showOnce(QWidget *parent);

private:
  bool m_errorShown{false};

#if WINAPI_LIBEI
  const bool m_hasEi = true;
#else
  const bool m_hasEi = false;
#endif

#if WINAPI_LIBPORTAL
  const bool m_hasPortal = true;
#else
  const bool m_hasPortal = false;
#endif
};

} // namespace deskflow::gui::core
