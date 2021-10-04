/*
 * synergy -- mouse and keyboard sharing utility
 * Copyright (C) 2014-2021 Symless Ltd.
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
#ifndef SYNERGY_PATH_H
#define SYNERGY_PATH_H

#include "String.h"

namespace synergy {

namespace filesystem {

#ifdef SYSAPI_WIN32
    std::wstring path(const String& filePath);
#else
    std::string path(const String& filePath);
#endif


} //namespace filesystem

} //namespace synergy

#endif // SYNERGY_PATH_H
