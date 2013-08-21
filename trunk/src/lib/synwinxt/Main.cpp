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

#include "CClassFactory.h"
#include "CArchMiscWindows.h"
#include <strsafe.h>
#include <Windows.h>

#pragma comment(lib, "Shlwapi.lib")

// {1BE208B1-BC21-4E39-8BB6-A5DC3F51479E}
GUID g_CLSID = {0x1be208b1, 0xbc21, 0x4e39, {0x8b, 0xb6, 0xa5, 0xdc, 0x3f, 0x51, 0x47, 0x9e}};
LONG g_refCount = 0;
HINSTANCE g_instance = NULL;

HRESULT registerInprocServer(CHAR* module, const CLSID& clsid, CHAR* threadModel);
HRESULT registerShellExtDataHandler(CHAR* fileType, const CLSID& clsid);
HRESULT unregisterShellExtDataHandler(CHAR* fileType, const CLSID& clsid);
HRESULT unregisterInprocServer(const CLSID& clsid);
void outputDebugStringF(const char *str, ...);

BOOL APIENTRY
DllMain(HMODULE module, DWORD reason, LPVOID reserved)
{
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
	return TRUE;
}

STDAPI
DllGetClassObject(REFCLSID rclsid, REFIID riid, LPVOID* ppvObj) 
{
	HRESULT hr = E_OUTOFMEMORY; 
	*ppvObj = NULL; 
 
	CClassFactory *classFactory = new CClassFactory();
	if (classFactory != NULL) { 
		hr = classFactory->QueryInterface(riid, ppvObj); 
		classFactory->Release(); 
	} 
	return hr;
}

STDAPI
DllCanUnloadNow()
{
	return g_refCount > 0 ? S_FALSE : S_OK;
}
STDAPI
DllRegisterServer()
{
	HRESULT hr;

	CHAR module[MAX_PATH];
	if (GetModuleFileName(g_instance, module, ARRAYSIZE(module)) == 0) {
		hr = HRESULT_FROM_WIN32(GetLastError());
		return hr;
	}

	// Register the component.
	hr = registerInprocServer(
			module,
			g_CLSID,
			"Apartment");

	if (SUCCEEDED(hr)) {
		hr = registerShellExtDataHandler(
				"*", 
				g_CLSID);
	}

	return hr;
}

STDAPI
DllUnregisterServer()
{
	HRESULT hr = S_OK;

	CHAR module[MAX_PATH];
	if (GetModuleFileName(g_instance, module, ARRAYSIZE(module)) == 0) {
		hr = HRESULT_FROM_WIN32(GetLastError());
		return hr;
	}

	// Unregister the component.
	hr = unregisterInprocServer(g_CLSID);
	if (SUCCEEDED(hr)) {
		// Unregister the context menu handler.
		hr = unregisterShellExtDataHandler("*", g_CLSID);
	}
	
	return hr;
}

HRESULT
registerInprocServer(CHAR* module, const CLSID& clsid, CHAR* threadModel)
{
	if (module == NULL || threadModel == NULL) {
		return E_INVALIDARG;
	}

	HRESULT hr;

	WCHAR CLASSID[MAX_PATH];
	CHAR szCLSID[MAX_PATH];
	StringFromGUID2(clsid, CLASSID, ARRAYSIZE(CLASSID));
	WideCharToMultiByte(CP_ACP, 0, CLASSID, -1, szCLSID, MAX_PATH, NULL, NULL);

	CHAR subkey[MAX_PATH];

	// Create the HKCR\CLSID\{<CLSID>}\InprocServer32 key.
	hr = StringCchPrintf(subkey, ARRAYSIZE(subkey), "CLSID\\%s\\InprocServer32", szCLSID);

	if (SUCCEEDED(hr)) {
		// Set the default value of the InprocServer32 key to the 
		// path of the COM module.
		HKEY key = CArchMiscWindows::addKey(HKEY_CLASSES_ROOT, subkey);
		CArchMiscWindows::setValue(key, NULL, module);

		if (SUCCEEDED(hr)) {
			// Set the threading model of the component.
			CArchMiscWindows::setValue(key, "ThreadingModel", threadModel);
		}
	}

	return hr;
}

HRESULT
unregisterInprocServer(const CLSID& clsid)
{
	HRESULT hr = S_OK;

	WCHAR CLASSID[MAX_PATH];
	CHAR szCLSID[MAX_PATH];
	StringFromGUID2(clsid, CLASSID, ARRAYSIZE(CLASSID));
	WideCharToMultiByte(CP_ACP, 0, CLASSID, -1, szCLSID, MAX_PATH, NULL, NULL);

	CHAR subkey[MAX_PATH];

	// Delete the HKCR\CLSID\{<CLSID>} key.
	hr = StringCchPrintf(subkey, ARRAYSIZE(subkey), "CLSID\\%s", szCLSID);
	if (SUCCEEDED(hr)) {
		hr = HRESULT_FROM_WIN32(RegDeleteTree(HKEY_CLASSES_ROOT, subkey));
	}

	return hr;
}

HRESULT
registerShellExtDataHandler(CHAR* fileType, const CLSID& clsid)
{
	if (fileType == NULL) {
		return E_INVALIDARG;
	}

	HRESULT hr;

	WCHAR szCLSID[MAX_PATH];
	CHAR CLASSID[MAX_PATH];
	StringFromGUID2(clsid, szCLSID, ARRAYSIZE(szCLSID));
	WideCharToMultiByte(CP_ACP, 0, szCLSID, -1, CLASSID, MAX_PATH, NULL, NULL);

	CHAR subkey[MAX_PATH];

	// Create the key HKCR\<File Type>\shellex\DataHandler
	hr = StringCchPrintf(subkey, ARRAYSIZE(subkey), "%s\\shellex\\DataHandler", fileType);
	if (SUCCEEDED(hr)) {
		// Set the default value of the key.
		HKEY key = CArchMiscWindows::addKey(HKEY_CLASSES_ROOT, subkey);
		CArchMiscWindows::setValue(key, NULL, CLASSID);
	}

	return hr;
}

HRESULT
unregisterShellExtDataHandler(CHAR* fileType, const CLSID& clsid)
{
	if (fileType == NULL) {
		return E_INVALIDARG;
	}

	HRESULT hr;

	WCHAR CLASSID[MAX_PATH];
	CHAR szCLSID[MAX_PATH];
	StringFromGUID2(clsid, CLASSID, ARRAYSIZE(CLASSID));
	WideCharToMultiByte(CP_ACP, 0, CLASSID, -1, szCLSID, MAX_PATH, NULL, NULL);

	CHAR subkey[MAX_PATH];

	// Remove the HKCR\<File Type>\shellex\DataHandler key.
	hr = StringCchPrintf(subkey, ARRAYSIZE(subkey), 
		"%s\\shellex\\DataHandler", fileType);
	if (SUCCEEDED(hr)) {
		hr = HRESULT_FROM_WIN32(RegDeleteTree(HKEY_CLASSES_ROOT, subkey));
	}

	return hr;
}

void
outputDebugStringF(const char *str, ...)
{
	char buf[2048];

	va_list ptr;
	va_start(ptr,str);
	vsprintf_s(buf,str,ptr);

	OutputDebugStringA(buf);
}
