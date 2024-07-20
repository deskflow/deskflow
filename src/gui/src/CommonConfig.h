/*
 * synergy -- mouse and keyboard sharing utility
 * Copyright (C) 2020 Symless Ltd.
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

namespace synergy::gui {

/// @brief Common configuration interface
class CommonConfig {
public:
  CommonConfig() = default;
  virtual ~CommonConfig() = default;
  virtual void loadSettings() = 0;
  virtual void saveSettings() = 0;
  bool modified() const { return m_modified; }
  void setModified(bool modified) { m_modified = modified; }

private:
  bool m_modified = false;
};

} // namespace synergy::gui
