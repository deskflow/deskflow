/*
 * synergy -- mouse and keyboard sharing utility
 * Copyright (C) 2012 Synergy Si Ltd.
 * Copyright (C) 2004 Chris Schoeneman
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

#include "arch/win32/ArchSystemWindows.h"
#include "arch/win32/ArchMiscWindows.h"
#include "arch/win32/XArchWindows.h"

#include "tchar.h"
#include <string>

#include <windows.h>
#include <psapi.h>

static const char* s_settingsKeyNames[] = {
	_T("SOFTWARE"),
	_T("Synergy"),
	NULL
};

//
// ArchSystemWindows
//

ArchSystemWindows::ArchSystemWindows()
{
	// do nothing
}

ArchSystemWindows::~ArchSystemWindows()
{
	// do nothing
}

std::string
ArchSystemWindows::getOSName() const
{
#if WINVER >= _WIN32_WINNT_WIN2K
	OSVERSIONINFOEX info;
#else
	OSVERSIONINFO info;
#endif

	info.dwOSVersionInfoSize = sizeof(info);
	if (GetVersionEx((OSVERSIONINFO*) &info)) {
		switch (info.dwPlatformId) {
		case VER_PLATFORM_WIN32_NT:
			#if WINVER >= _WIN32_WINNT_WIN2K
			if (info.dwMajorVersion == 6) {
				if (info.dwMinorVersion == 0) {
					if (info.wProductType == VER_NT_WORKSTATION) {
						return "Microsoft Windows Vista";
					}
					else {
						return "Microsoft Windows Server 2008";
					}
				}
				else if (info.dwMinorVersion == 1) {
					if (info.wProductType == VER_NT_WORKSTATION) {
						return "Microsoft Windows 7";
					}
					else {
						return "Microsoft Windows Server 2008 R2";
					}
				}
			}
			#endif

			if (info.dwMajorVersion == 5 && info.dwMinorVersion == 2) {
				return "Microsoft Windows Server 2003";
			}
			if (info.dwMajorVersion == 5 && info.dwMinorVersion == 1) {
				return "Microsoft Windows XP";
			}
			if (info.dwMajorVersion == 5 && info.dwMinorVersion == 0) {
				return "Microsoft Windows Server 2000";
			}
			char buffer[100];
			sprintf(buffer, "Microsoft Windows %d.%d",
							info.dwMajorVersion, info.dwMinorVersion);
			return buffer;

		default:
			break;
		}
	}
	return "Microsoft Windows <unknown>";
}

std::string
ArchSystemWindows::getPlatformName() const
{
#ifdef _X86_
	if (isWOW64())
		return "x86 (WOW64)";
	else
		return "x86";
#else
#ifdef _AMD64_
	return "x64";
#else
	return "Unknown";
#endif
#endif
}

std::string
ArchSystemWindows::setting(const std::string& valueName) const
{
	HKEY key = ArchMiscWindows::openKey(HKEY_LOCAL_MACHINE, s_settingsKeyNames);
	if (key == NULL)
		return "";

	return ArchMiscWindows::readValueString(key, valueName.c_str());
}

void
ArchSystemWindows::setting(const std::string& valueName, const std::string& valueString) const
{
	HKEY key = ArchMiscWindows::addKey(HKEY_LOCAL_MACHINE, s_settingsKeyNames);
	if (key == NULL)
		throw XArch(std::string("could not access registry key: ") + valueName);
	ArchMiscWindows::setValue(key, valueName.c_str(), valueString.c_str());
}

bool
ArchSystemWindows::isWOW64() const
{
#if WINVER >= _WIN32_WINNT_WINXP
	typedef BOOL (WINAPI *LPFN_ISWOW64PROCESS) (HANDLE, PBOOL);
	HMODULE hModule = GetModuleHandle(TEXT("kernel32"));
	if (!hModule) return FALSE;

	LPFN_ISWOW64PROCESS fnIsWow64Process =
		(LPFN_ISWOW64PROCESS) GetProcAddress(hModule, "IsWow64Process");

	BOOL bIsWow64 = FALSE;
	if (NULL != fnIsWow64Process &&
		fnIsWow64Process(GetCurrentProcess(), &bIsWow64) &&
		bIsWow64)
	{
		return true;
	}
#endif
	return false;
}
#pragma comment(lib, "psapi")

std::string
ArchSystemWindows::getLibsUsed(void) const
{
    HMODULE hMods[1024];
    HANDLE hProcess;
    DWORD cbNeeded;
    unsigned int i;
	char hex[16];

	DWORD pid = GetCurrentProcessId();

	std::string msg = "pid:" + std::to_string((_ULonglong)pid) + "\n";

    hProcess = OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_READ, FALSE, pid);

    if (NULL == hProcess) {
        return msg;
	}

    if (EnumProcessModules(hProcess, hMods, sizeof(hMods), &cbNeeded)) {
        for (i = 0; i < (cbNeeded / sizeof(HMODULE)); i++) {
            TCHAR szModName[MAX_PATH];
            if (GetModuleFileNameEx(hProcess, hMods[i], szModName, sizeof(szModName) / sizeof(TCHAR))) {
				sprintf(hex,"(0x%08X)",hMods[i]);
				msg += szModName;
				msg.append(hex);
				msg.append("\n");
            }
        }
    }

    CloseHandle(hProcess);
    return msg;
}
