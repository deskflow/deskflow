/*
 * synergy -- mouse and keyboard sharing utility
 * Copyright (C) 2024 Symless Ltd.
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

#include <stdexcept>

namespace synergy::gui {

/**
 * @brief Useful for terminating a code path when using `qFatal`.
 *
 * Using `qFatal` indirectly causes an abort.
 */
class QtFatalNotThrown : public std::runtime_error {
public:
  explicit QtFatalNotThrown()
      : std::runtime_error("this error should not be thrown") {}
};

} // namespace synergy::gui
