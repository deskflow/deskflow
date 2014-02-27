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

#include "CDataHandlerExtension.h"
#include <Shlwapi.h>
#include <comutil.h>
#include <string>

extern LONG g_refCount;
extern GUID g_CLSID;
extern void setDraggingFilename(const char* str);
extern void outputDebugStringF(const char* str, ...);

CDataHandlerExtension::CDataHandlerExtension() :
	m_refCount(1)
{
	outputDebugStringF("synwinxt: > CDataHandlerExtension::ctor, g_refCount=%d\n", g_refCount);
	InterlockedIncrement(&g_refCount);
	outputDebugStringF("synwinxt: < CDataHandlerExtension::ctor, g_refCount=%d\n", g_refCount);
}

CDataHandlerExtension::~CDataHandlerExtension()
{
	outputDebugStringF("synwinxt: > CDataHandlerExtension::dtor, g_refCount=%d\n", g_refCount);
	InterlockedDecrement(&g_refCount);
	outputDebugStringF("synwinxt: < CDataHandlerExtension::dtor, g_refCount=%d\n", g_refCount);
}

HRESULT STDMETHODCALLTYPE
CDataHandlerExtension::QueryInterface(REFIID riid, void **ppvObject)
{
	outputDebugStringF("synwinxt: > CDataHandlerExtension::QueryInterface\n");
	static const QITAB qit[] = 
	{
		QITABENT(CDataHandlerExtension, IPersistFile),
		QITABENT(CDataHandlerExtension, IDataObject), 
		{ 0 },
	};
	HRESULT hr = QISearch(this, qit, riid, ppvObject);
	
	if (FAILED(hr)) {
		outputDebugStringF("synwinxt: < CDataHandlerExtension::QueryInterface, hr=FAILED\n");
	}
	else {
		outputDebugStringF("synwinxt: < CDataHandlerExtension::QueryInterface, hr=%d\n", hr);
	}
	return hr;
}

ULONG STDMETHODCALLTYPE
CDataHandlerExtension::AddRef()
{
	outputDebugStringF("synwinxt: > CDataHandlerExtension::AddRef, m_refCount=%d\n", m_refCount);
	LONG r = InterlockedIncrement(&m_refCount);
	outputDebugStringF("synwinxt: < CDataHandlerExtension::AddRef, r=%d, m_refCount=%d\n", r, m_refCount);
	return r;
}

ULONG STDMETHODCALLTYPE
CDataHandlerExtension::Release()
{
	outputDebugStringF("synwinxt: > CDataHandlerExtension::Release, m_refCount=%d\n", m_refCount);
	LONG r = InterlockedDecrement(&m_refCount);
	if (r == 0) {
		delete this;
	}
	outputDebugStringF("synwinxt: < CDataHandlerExtension::Release, r=%d\n", r);
	return r;
}

HRESULT STDMETHODCALLTYPE
CDataHandlerExtension::Load(__RPC__in LPCOLESTR pszFileName, DWORD dwMode)
{
	outputDebugStringF("synwinxt: > CDataHandlerExtension::Load\n");
	std::string fileName = _bstr_t(pszFileName);
	setDraggingFilename(fileName.c_str());
	outputDebugStringF("synwinxt: < CDataHandlerExtension::Load\n");
	return S_OK;
}

HRESULT STDMETHODCALLTYPE
CDataHandlerExtension::GetClassID(__RPC__out CLSID *pClassID)
{
	outputDebugStringF("synwinxt: > CDataHandlerExtension::GetClassID\n");
	*pClassID = g_CLSID;
	outputDebugStringF("synwinxt: < CDataHandlerExtension::GetClassID\n");
	return S_OK;
}
