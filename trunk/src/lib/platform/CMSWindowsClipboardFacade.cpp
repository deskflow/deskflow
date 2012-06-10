/*
 * synergy -- mouse and keyboard sharing utility
 * Copyright (C) 2002 Chris Schoeneman, Nick Bolton, Sorin Sbarnea
 * 
 * This package is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * found in the file COPYING that should have accompanied this file.
 * 
 * This package is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "CMSWindowsClipboard.h"
#include "CMSWindowsClipboardFacade.h"

void CMSWindowsClipboardFacade::write(HANDLE win32Data, UINT win32Format)
{
	if (SetClipboardData(win32Format, win32Data) == NULL) {
		// free converted data if we couldn't put it on
		// the clipboard.
		// nb: couldn't cause this in integ tests.
		GlobalFree(win32Data);
	}
}
