/*
 * synergy -- mouse and keyboard sharing utility
 * Copyright (C) 2015 Synergy Si Ltd
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

#if defined _WIN32
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>

#if defined(ns_EXPORTS)
#define NS_API __declspec(dllexport)
#else
#define NS_API __declspec(dllimport)
#endif

#else
#define NS_API
#endif

extern "C" {

NS_API void				init(void* log, void* arch);
NS_API int				initEvent(void (*sendEvent)(const char*, void*));
NS_API void*			invoke(const char* command, void** args);
NS_API void				cleanup();

}
