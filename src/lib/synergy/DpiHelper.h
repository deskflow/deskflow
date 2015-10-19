/*
 * synergy -- mouse and keyboard sharing utility
 * Copyright (C) 2015 Synergy Seamless Inc.
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

#include "common/common.h"

class DpiHelper {
public:
	enum EDpi {
		kDefaultDpi = 100
	};

	static void calculateDpi(size_t width, size_t height);
	static float getDpi() { return (float)(s_dpi / 100.0f); }

public:
	static size_t s_dpi;
	static bool s_dpiScaled;
	static size_t s_resolutionWidth;
	static size_t s_resolutionHeight;
	static size_t s_primaryWidthCenter;
	static size_t s_primaryHeightCenter;
};
