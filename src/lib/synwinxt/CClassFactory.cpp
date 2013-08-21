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

CClassFactory::CClassFactory()
{
	InterlockedIncrement(&g_refCount);
}

CClassFactory::~CClassFactory()
{
	InterlockedDecrement(&g_refCount);
}

HRESULT STDMETHODCALLTYPE
CClassFactory::QueryInterface(REFIID riid, void **ppvObject)
{
	static const QITAB qit[] = 
	{
		QITABENT(CClassFactory, IClassFactory),
		{ 0 },
	};
	return QISearch(this, qit, riid, ppvObject);
}

ULONG STDMETHODCALLTYPE
CClassFactory::AddRef()
{
	return InterlockedIncrement(&m_refCount);
}

ULONG STDMETHODCALLTYPE
CClassFactory::Release()
{
	LONG count = InterlockedDecrement(&m_refCount);
	if (count == 0) {
		delete this;
	}
	return count;
}

HRESULT STDMETHODCALLTYPE
CClassFactory::CreateInstance(IUnknown *pUnkOuter, REFIID riid, void **ppvObject)
{
	if (pUnkOuter != NULL) {
		return CLASS_E_NOAGGREGATION;
	}

	CDataHandlerExtension* pExtension = new CDataHandlerExtension();
	HRESULT hr = pExtension->QueryInterface(riid, ppvObject);
	if (FAILED(hr)) {
		delete pExtension;
	}

	return hr;
}

HRESULT STDMETHODCALLTYPE
CClassFactory::LockServer(BOOL fLock)
{
	if (fLock) {
		InterlockedIncrement(&g_refCount);
	}
	else {
		InterlockedDecrement(&g_refCount);
	}
	return S_OK;
}
