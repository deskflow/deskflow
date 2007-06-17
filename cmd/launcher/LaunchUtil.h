/*
 * synergy -- mouse and keyboard sharing utility
 * Copyright (C) 2002 Chris Schoeneman
 * 
 * This package is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * found in the file COPYING that should have accompanied this file.
 * 
 * This package is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */

#ifndef LAUNCHUTIL_H
#define LAUNCHUTIL_H

#include "CString.h"

#define WINDOWS_LEAN_AND_MEAN
#include <windows.h>
#include <sys/types.h>
#include <sys/stat.h>

#define CLIENT_APP "synergyc.exe"
#define SERVER_APP "synergys.exe"
#define CONFIG_NAME "synergy.sgc"

class CConfig;

// client must define this and set it before calling any function here
extern HINSTANCE s_instance;

CString					getString(DWORD id);
CString					getErrorString(DWORD error);

void					showError(HWND hwnd, const CString& msg);
void					askOkay(HWND hwnd, const CString& title,
							const CString& msg);
bool					askVerify(HWND hwnd, const CString& msg);
bool					isShowingDialog();

void					setWindowText(HWND hwnd, const CString& msg);
CString					getWindowText(HWND hwnd);

HWND					getItem(HWND hwnd, int id);
void					enableItem(HWND hwnd, int id, bool enabled);

void					setItemChecked(HWND, bool);
bool					isItemChecked(HWND);

CString					getAppPath(const CString& appName);

bool					isConfigNewer(time_t&, bool userConfig);
bool					loadConfig(CConfig& config, time_t&, bool& userConfig);
bool					saveConfig(const CConfig& config,
							bool sysOnly, time_t&);

const TCHAR* const*		getSettingsPath();

#endif
