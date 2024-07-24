/*
 * synergy -- mouse and keyboard sharing utility
 * Copyright (C) 2015 Symless Ltd.
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

enum ProductEdition {
  kBasic = 0,
  kPro = 1,
  kValueNotUsed = 2,
  kUnregistered = 3,
  kBusiness = 4,
  kBasicChina = 5,
  kProChina = 6,
  kLite = 7,
  kUltimate = 8,
};

/// @deprecated Use `ProductEdition` instead
using Edition = ProductEdition;
