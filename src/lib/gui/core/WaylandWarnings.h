/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2024 Symless Ltd.
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#pragma once

#include <QWidget>

#include "platform/Wayland.h"

namespace deskflow::gui::core {

class WaylandWarnings
{
public:
  explicit WaylandWarnings() = default;

  void showOnce(
      QWidget *parent, bool hasEi = platform::kHasEi, bool hasPortal = platform::kHasPortal,
      bool hasPortalInputCapture = platform::kHasPortalInputCapture
  );

private:
  bool m_errorShown{false};
};

} // namespace deskflow::gui::core
