/*
 * synergy -- mouse and keyboard sharing utility
 * Copyright (C) 2011 Chris Schoeneman, Nick Bolton, Sorin Sbarnea
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

#pragma once

#ifdef sygpadhk_EXPORTS
#define sygpadhk_API __declspec(dllexport)
#else
#define sygpadhk_API __declspec(dllimport)
#endif

sygpadhk_API LRESULT CALLBACK HookProc(int nCode, WPARAM wParam, LPARAM lParam);

sygpadhk_API BOOL InstallGamepadHook();
sygpadhk_API void RemoveGamepadHook();
