/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2021 Symless Ltd.
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#pragma once

namespace deskflow::gui::config {

/**
 * @brief Represents the state of the server config dialog.
 *
 * Future design consideration: Once moving the server config dialog to the GUI
 * lib, we can probably just pass a reference to that rather than needing an
 * object to track it's state.
 */
class ServerConfigDialogState
{
public:
  bool isVisible() const
  {
    return m_isVisible;
  }
  void setVisible(bool isVisible)
  {
    m_isVisible = isVisible;
  }

private:
  bool m_isVisible = false;
};

} // namespace deskflow::gui::config
