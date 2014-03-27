/*
 * synergy -- mouse and keyboard sharing utility
 * Copyright (C) 2013 Bolton Software Ltd.
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

#include "synwinxt.h"
#include "CClassFactory.h"
#include "CArchMiscWindows.h"
#include <strsafe.h>
#include <Windows.h>

#pragma comment(lib, "Shlwapi.lib")

#if defined(_MSC_VER)
#pragma comment(linker, "-section:sharedData,rws")
#pragma data_seg("sharedData")
#endif

static BYTE g_draggingFilename[MAX_PATH] = { 0 };

#if defined(_MSC_VER)
#pragma data_seg()
#endif

// {1BE208B1-BC21-4E39-8BB6-A5DC3F51479E}
GUID g_CLSID = {0x1be208b1, 0xbc21, 0x4e39, {0x8b, 0xb6, 0xa5, 0xdc, 0x3f, 0x51, 0x47, 0x9e}};
LONG g_refCount = 0;
HINSTANCE g_instance = NULL;

HRESULT registerInprocServer(CHAR* module, const CLSID& clsid, CHAR* threadModel);
HRESULT registerShellExtDataHandler(CHAR* fileType, const CLSID& clsid);
HRESULT unregisterShellExtDataHandler(CHAR* fileType, const CLSID& clsid);
HRESULT unregisterInprocServer(const CLSID& clsid);

void
outputDebugStringF(const char* str, ...)
{
	char buf[2048];

	va_list ptr;
	va_start(ptr,str);
	vsprintf_s(buf,str,ptr);

	OutputDebugStringA(buf);
}

BOOL APIENTRY
DllMain(HMODULE module, DWORD reason, LPVOID reserved)
{
	outputDebugStringF("synwinxt: > DllMain, reason=%d\n", reason);

	switch (reason) {
	case DLL_PROCESS_ATTACH:
		g_instance = module;
		DisableThreadLibraryCalls(module);
		break;

	case DLL_THREAD_ATTACH:
		break;
	case DLL_THREAD_DETACH:
		break;
	case DLL_PROCESS_DETACH:
		break;
	}

	outputDebugStringF("synwinxt: < DllMain\n");
	return TRUE;
}

STDAPI
DllGetClassObject(REFCLSID rclsid, REFIID riid, LPVOID* ppvObj) 
{
	outputDebugStringF("synwinxt: > DllGetClassObject\n");

	HRESULT hr = E_OUTOFMEMORY; 
	*ppvObj = NULL; 
 
	CClassFactory *classFactory = new CClassFactory();
	if (classFactory != NULL) { 
		hr = classFactory->QueryInterface(riid, ppvObj); 
		classFactory->Release(); 
	}

	outputDebugStringF("synwinxt: < DllGetClassObject, hr=%d\n", hr);
	return hr;
}

STDAPI
DllCanUnloadNow()
{
	outputDebugStringF("synwinxt: > DllCanUnloadNow, g_refCount=%d\n", g_refCount);
	int r = g_refCount > 0 ? S_FALSE : S_OK;
	outputDebugStringF("synwinxt: < DllCanUnloadNow, g_refCount=%d, r=%d\n", g_refCount, r);
	return r;
}
STDAPI
DllRegisterServer()
{
	outputDebugStringF("synwinxt: > DllRegisterServer\n");

	HRESULT hr;

	CHAR module[MAX_PATH];
	if (GetModuleFileName(g_instance, module, ARRAYSIZE(module)) == 0) {
		hr = HRESULT_FROM_WIN32(GetLastError());
		outputDebugStringF("synwinxt: < DllRegisterServer, hr=%d\n", hr);
		return hr;
	}

	hr = registerInprocServer(
		module,
		g_CLSID,
		"Apartment");

	if (SUCCEEDED(hr)) {
		hr = registerShellExtDataHandler(
			"*", 
			g_CLSID);
	}
	
	outputDebugStringF("synwinxt: < DllRegisterServer, hr=%d\n", hr);
	return hr;
}

STDAPI
DllUnregisterServer()
{
	outputDebugStringF("synwinxt: > DllUnregisterServer\n");

	HRESULT hr = S_OK;

	CHAR module[MAX_PATH];
	if (GetModuleFileName(g_instance, module, ARRAYSIZE(module)) == 0) {
		hr = HRESULT_FROM_WIN32(GetLastError());
		outputDebugStringF("synwinxt: < DllRegisterServer, hr=%d\n", hr);
		return hr;
	}

	hr = unregisterInprocServer(g_CLSID);
	if (SUCCEEDED(hr)) {
		hr = unregisterShellExtDataHandler("*", g_CLSID);
	}
	
	outputDebugStringF("synwinxt: < DllUnregisterServer, hr=%d\n", hr);
	return hr;
}

HRESULT
registerInprocServer(CHAR* module, const CLSID& clsid, CHAR* threadModel)
{
	outputDebugStringF("synwinxt: > registerInprocServer\n");

	if (module == NULL || threadModel == NULL) {
		return E_INVALIDARG;
	}

	HRESULT hr;

	WCHAR CLASSID[MAX_PATH];
	CHAR szCLSID[MAX_PATH];
	StringFromGUID2(clsid, CLASSID, ARRAYSIZE(CLASSID));
	WideCharToMultiByte(CP_ACP, 0, CLASSID, -1, szCLSID, MAX_PATH, NULL, NULL);

	CHAR subkey[MAX_PATH];

	// create the HKCR\CLSID\{<CLSID>}\InprocServer32 key.
	hr = StringCchPrintf(subkey, ARRAYSIZE(subkey), "CLSID\\%s\\InprocServer32", szCLSID);

	if (SUCCEEDED(hr)) {
		// set the default value of the InprocServer32 key to the 
		// path of the COM module.
		HKEY key = CArchMiscWindows::addKey(HKEY_CLASSES_ROOT, subkey);
		CArchMiscWindows::setValue(key, NULL, module);

		if (SUCCEEDED(hr)) {
			// set the threading model of the component.
			CArchMiscWindows::setValue(key, "ThreadingModel", threadModel);
		}
	}
	
	outputDebugStringF("synwinxt: < registerInprocServer, hr=%d\n", hr);
	return hr;
}

HRESULT
unregisterInprocServer(const CLSID& clsid)
{
	outputDebugStringF("synwinxt: > unregisterInprocServer\n");

	HRESULT hr = S_OK;

	WCHAR CLASSID[MAX_PATH];
	CHAR szCLSID[MAX_PATH];
	StringFromGUID2(clsid, CLASSID, ARRAYSIZE(CLASSID));
	WideCharToMultiByte(CP_ACP, 0, CLASSID, -1, szCLSID, MAX_PATH, NULL, NULL);

	CHAR subkey[MAX_PATH];

	// delete the HKCR\CLSID\{<CLSID>} key.
	hr = StringCchPrintf(subkey, ARRAYSIZE(subkey), "CLSID\\%s", szCLSID);
	if (SUCCEEDED(hr)) {
		hr = HRESULT_FROM_WIN32(RegDeleteTree(HKEY_CLASSES_ROOT, subkey));
	}
	
	if (FAILED(hr)) {
		outputDebugStringF("synwinxt: < unregisterInprocServer, hr=FAILED\n");
	}
	else {
		outputDebugStringF("synwinxt: < unregisterInprocServer, hr=%d\n", hr);
	}
	return hr;
}

HRESULT
registerShellExtDataHandler(CHAR* fileType, const CLSID& clsid)
{
	outputDebugStringF("synwinxt: > registerShellExtDataHandler\n");

	if (fileType == NULL) {
		return E_INVALIDARG;
	}

	HRESULT hr;

	WCHAR szCLSID[MAX_PATH];
	CHAR CLASSID[MAX_PATH];
	StringFromGUID2(clsid, szCLSID, ARRAYSIZE(szCLSID));
	WideCharToMultiByte(CP_ACP, 0, szCLSID, -1, CLASSID, MAX_PATH, NULL, NULL);

	CHAR subkey[MAX_PATH];

	// create the key HKCR\<File Type>\shellex\DataHandler
	hr = StringCchPrintf(subkey, ARRAYSIZE(subkey), "%s\\shellex\\DataHandler", fileType);
	if (SUCCEEDED(hr)) {
		// set the default value of the key.
		HKEY key = CArchMiscWindows::addKey(HKEY_CLASSES_ROOT, subkey);
		CArchMiscWindows::setValue(key, NULL, CLASSID);
	}
	
	outputDebugStringF("synwinxt: < registerShellExtDataHandler, hr=%d\n", hr);
	return hr;
}

HRESULT
unregisterShellExtDataHandler(CHAR* fileType, const CLSID& clsid)
{
	outputDebugStringF("synwinxt: > unregisterShellExtDataHandler\n");

	if (fileType == NULL) {
		return E_INVALIDARG;
	}

	HRESULT hr;

	WCHAR CLASSID[MAX_PATH];
	CHAR szCLSID[MAX_PATH];
	StringFromGUID2(clsid, CLASSID, ARRAYSIZE(CLASSID));
	WideCharToMultiByte(CP_ACP, 0, CLASSID, -1, szCLSID, MAX_PATH, NULL, NULL);

	CHAR subkey[MAX_PATH];

	// remove the HKCR\<File Type>\shellex\DataHandler key.
	hr = StringCchPrintf(subkey, ARRAYSIZE(subkey), "%s\\shellex\\DataHandler", fileType);
	if (SUCCEEDED(hr)) {
		hr = HRESULT_FROM_WIN32(RegDeleteTree(HKEY_CLASSES_ROOT, subkey));
	}
	
	outputDebugStringF("synwinxt: < unregisterShellExtDataHandler, hr=%d\n", hr);
	return hr;
}

std::string
getFileExt(const char* filenameCStr)
{
	std::string filename = filenameCStr;
	size_t findExt = filename.find_last_of(".", filename.size());
	if (findExt != std::string::npos) {
		return filename.substr(findExt + 1, filename.size() - findExt - 1);
	}
	return "";
}

void
setDraggingFilename(const char* filename)
{
	outputDebugStringF("synwinxt: > setDraggingFilename, filename=%s\n", filename);
	
	// HACK: only handle files that are not .exe or .lnk
	// dragging anything, including a selection marquee, from a program
	// (e.g. explorer.exe) will cause this function to be called with the
	// path of that program. currently we don't know how to test for this
	// situation, so just ignore exe and lnk files.
	std::string ext = getFileExt(filename);
	if ((ext != "exe") && (ext != "lnk")) {
		memcpy(g_draggingFilename, filename, MAX_PATH);
	}
	else {
		outputDebugStringF(
			"synwinxt: ignoring filename=%s, ext=%s\n",
			filename, ext.c_str());
	}

	outputDebugStringF("synwinxt: < setDraggingFilename, g_draggingFilename=%s\n", g_draggingFilename);
}

void
getDraggingFilename(char* filename)
{
	outputDebugStringF("synwinxt: > getDraggingFilename\n");
	memcpy(filename, g_draggingFilename, MAX_PATH);
	
	// mark string as empty once used, so we can't accidentally copy
	// the same file more than once unless the user does this on purpose.
	g_draggingFilename[0] = NULL;

	outputDebugStringF("synwinxt: < getDraggingFilename, filename=%s\n", filename);
}
