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

#include "synergy/DpiHelper.h"
#include "base/Log.h"

#include <assert.h> 

size_t DpiHelper::s_dpi = kDefaultDpi;
bool DpiHelper::s_dpiScaled = false;
size_t DpiHelper::s_resolutionWidth = 0;
size_t DpiHelper::s_resolutionHeight = 0;
size_t DpiHelper::s_primaryWidthCenter = 0;
size_t DpiHelper::s_primaryHeightCenter = 0;

void DpiHelper::calculateDpi(size_t width, size_t height)
{
	if (s_resolutionWidth == 0 || 
		s_resolutionHeight == 0 || 
		s_primaryWidthCenter == 0 ||
		s_primaryHeightCenter == 0) {
		return;
	}

	size_t dpiTest1 = s_resolutionWidth * 100 / width;
	size_t dpiTest2 = s_resolutionHeight * 100 / height;

    if (dpiTest1 == dpiTest2) {
        s_dpi = dpiTest1;

        if (s_dpi != kDefaultDpi) {
            s_dpiScaled = true;

            LOG((CLOG_DEBUG "DPI: %d%%", s_dpi));
            LOG((CLOG_DEBUG "physical resolution: %d, %d scaled resolution: %d, %d", s_resolutionWidth, s_resolutionHeight, width, height));
        }
    }
}
