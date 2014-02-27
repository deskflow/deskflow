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
#include "CDataHandlerExtension.h"
#include <Shlwapi.h>

extern LONG g_refCount;
extern void outputDebugStringF(const char *str, ...);

CClassFactory::CClassFactory() :
	m_refCount(1)
{
	outputDebugStringF("synwinxt: > CClassFactory::ctor, g_refCount=%d\n", g_refCount);
	InterlockedIncrement(&g_refCount);
	outputDebugStringF("synwinxt: < CClassFactory::ctor, g_refCount=%d\n", g_refCount);
}

CClassFactory::~CClassFactory()
{
	outputDebugStringF("synwinxt: > CClassFactory::dtor, g_refCount=%d\n", g_refCount);
	InterlockedDecrement(&g_refCount);
	outputDebugStringF("synwinxt: < CClassFactory::dtor, g_refCount=%d\n", g_refCount);
}

HRESULT STDMETHODCALLTYPE
CClassFactory::QueryInterface(REFIID riid, void **ppvObject)
{
	outputDebugStringF("synwinxt: > CClassFactory::QueryInterface\n");
	static const QITAB qit[] = 
	{
		QITABENT(CClassFactory, IClassFactory),
		{ 0 },
	};
	HRESULT hr = QISearch(this, qit, riid, ppvObject);
	
	outputDebugStringF("synwinxt: < CClassFactory::QueryInterface, hr=%d\n", hr);
	return hr;
}

ULONG STDMETHODCALLTYPE
CClassFactory::AddRef()
{
	outputDebugStringF("synwinxt: > CClassFactory::AddRef, m_refCount=%d\n", m_refCount);
	LONG r = InterlockedIncrement(&m_refCount);
	outputDebugStringF("synwinxt: < CClassFactory::AddRef, r=%d, m_refCount=%d\n", r, m_refCount);
	return r;
}

ULONG STDMETHODCALLTYPE
CClassFactory::Release()
{
	outputDebugStringF("synwinxt: > CClassFactory::Release, m_refCount=%d\n", m_refCount);
	LONG r = InterlockedDecrement(&m_refCount);
	if (r == 0) {
		delete this;
	}

	outputDebugStringF("synwinxt: < CClassFactory::Release, r=%d\n", r);
	return r;
}

HRESULT STDMETHODCALLTYPE
CClassFactory::CreateInstance(IUnknown *pUnkOuter, REFIID riid, void **ppvObject)
{
	outputDebugStringF("synwinxt: > CClassFactory::CreateInstance\n");
	if (pUnkOuter != NULL) {
		return CLASS_E_NOAGGREGATION;
	}

	CDataHandlerExtension* pExtension = new CDataHandlerExtension();
	HRESULT hr = pExtension->QueryInterface(riid, ppvObject);
	if (FAILED(hr)) {
		delete pExtension;
	}
	
	outputDebugStringF("synwinxt: < CClassFactory::CreateInstance, hr=%d\n", hr);
	return hr;
}

HRESULT STDMETHODCALLTYPE
CClassFactory::LockServer(BOOL fLock)
{
	outputDebugStringF("synwinxt: > CClassFactory::LockServer, g_refCount=%d\n", g_refCount);
	if (fLock) {
		InterlockedIncrement(&g_refCount);
	}
	else {
		InterlockedDecrement(&g_refCount);
	}
	outputDebugStringF("synwinxt: < CClassFactory::LockServer, g_refCount=%d\n", g_refCount);
	return S_OK;
}
