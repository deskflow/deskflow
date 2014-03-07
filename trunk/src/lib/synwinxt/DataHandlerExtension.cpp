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

#include "DataHandlerExtension.h"

#include <Shlwapi.h>
#include <comutil.h>
#include <string>

extern LONG g_refCount;
extern GUID g_CLSID;
extern void setDraggingFilename(const char* str);
extern void log(const char* str, ...);

CDataHandlerExtension::CDataHandlerExtension() :
	m_refCount(1)
{
	log("> CDataHandlerExtension::ctor, g_refCount=%d", g_refCount);
	InterlockedIncrement(&g_refCount);
	log("< CDataHandlerExtension::ctor, g_refCount=%d", g_refCount);
}

CDataHandlerExtension::~CDataHandlerExtension()
{
	log("> CDataHandlerExtension::dtor, g_refCount=%d", g_refCount);
	InterlockedDecrement(&g_refCount);
	log("< CDataHandlerExtension::dtor, g_refCount=%d", g_refCount);
}

HRESULT STDMETHODCALLTYPE
CDataHandlerExtension::QueryInterface(REFIID riid, void **ppvObject)
{
	log("> CDataHandlerExtension::QueryInterface");
	static const QITAB qit[] = 
	{
		QITABENT(CDataHandlerExtension, IPersistFile),
		QITABENT(CDataHandlerExtension, IDataObject), 
		{ 0 },
	};
	HRESULT hr = QISearch(this, qit, riid, ppvObject);
	
	if (FAILED(hr)) {
		log("< CDataHandlerExtension::QueryInterface, hr=FAILED");
	}
	else {
		log("< CDataHandlerExtension::QueryInterface, hr=%d", hr);
	}
	return hr;
}

ULONG STDMETHODCALLTYPE
CDataHandlerExtension::AddRef()
{
	log("> CDataHandlerExtension::AddRef, m_refCount=%d", m_refCount);
	LONG r = InterlockedIncrement(&m_refCount);
	log("< CDataHandlerExtension::AddRef, r=%d, m_refCount=%d", r, m_refCount);
	return r;
}

ULONG STDMETHODCALLTYPE
CDataHandlerExtension::Release()
{
	log("> CDataHandlerExtension::Release, m_refCount=%d", m_refCount);
	LONG r = InterlockedDecrement(&m_refCount);
	if (r == 0) {
		delete this;
	}
	log("< CDataHandlerExtension::Release, r=%d", r);
	return r;
}

HRESULT STDMETHODCALLTYPE
CDataHandlerExtension::Load(__RPC__in LPCOLESTR pszFileName, DWORD dwMode)
{
	log("> CDataHandlerExtension::Load");
	std::string fileName = _bstr_t(pszFileName);
	setDraggingFilename(fileName.c_str());
	log("< CDataHandlerExtension::Load");
	return S_OK;
}

HRESULT STDMETHODCALLTYPE
CDataHandlerExtension::GetClassID(__RPC__out CLSID *pClassID)
{
	log("> CDataHandlerExtension::GetClassID");
	*pClassID = g_CLSID;
	log("< CDataHandlerExtension::GetClassID");
	return S_OK;
}
