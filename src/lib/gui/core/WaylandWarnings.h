/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2024 Symless Ltd.
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#pragma once

#include <QWidget>
#include <memory>

#include "CoreProcess.h"
#include "platform/wayland.h"

namespace deskflow::gui::core {

class WaylandWarnings
{
public:
  struct Deps
  {
    virtual ~Deps() = default;
    virtual void showWaylandLibraryError(QWidget *parent);
  };

  explicit WaylandWarnings(std::shared_ptr<Deps> deps = std::make_shared<Deps>()) : m_pDeps(deps)
  {
  }

  void showOnce(
      QWidget *parent, CoreProcess::Mode mode, bool hasEi = platform::kHasEi, bool hasPortal = platform::kHasPortal,
      bool hasPortalInputCapture = platform::kHasPortalInputCapture
  );

private:
  bool m_errorShown{false};
  std::shared_ptr<Deps> m_pDeps;
};

} // namespace deskflow::gui::core
