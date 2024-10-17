/*
 * Deskflow -- mouse and keyboard sharing utility
 * Copyright (C) 2021 Symless Ltd.
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
