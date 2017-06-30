/*
 * synergy -- mouse and keyboard sharing utility
 * Copyright (C) 2012-2016 Symless Ltd.
 * Copyright (C) 2008 Volker Lanz (vl@fidra.de)
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

#include "BaseConfig.h"

const char* BaseConfig::m_ModifierNames[] = {
    "shift", "ctrl", "alt", "meta", "super", "none"};

const char* BaseConfig::m_FixNames[] = {"halfDuplexCapsLock",
                                        "halfDuplexNumLock",
                                        "halfDuplexScrollLock",
                                        "xtestIsXineramaUnaware"};

const char* BaseConfig::m_SwitchCornerNames[] = {
    "top-left", "top-right", "bottom-left", "bottom-right"};
