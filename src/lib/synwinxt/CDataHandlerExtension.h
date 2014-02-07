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

#pragma once

#include <shlobj.h>

class CDataHandlerExtension : public IDataObject, public IPersistFile
{
public:
	CDataHandlerExtension();
	~CDataHandlerExtension();
	
	// IUnknown overrides
	HRESULT STDMETHODCALLTYPE QueryInterface(REFIID riid, void **ppvObject);
	ULONG STDMETHODCALLTYPE AddRef();
	ULONG STDMETHODCALLTYPE Release();
	
	// IDataObject overrides
	HRESULT STDMETHODCALLTYPE GetData(FORMATETC *pformatetcIn, STGMEDIUM *pmedium) { return E_NOTIMPL; }
	HRESULT STDMETHODCALLTYPE GetDataHere(FORMATETC *pformatetc, STGMEDIUM *pmedium) { return E_NOTIMPL; }
	HRESULT STDMETHODCALLTYPE QueryGetData(__RPC__in_opt FORMATETC *pformatetc) { return E_NOTIMPL; }
	HRESULT STDMETHODCALLTYPE GetCanonicalFormatEtc(__RPC__in_opt FORMATETC *pformatectIn, __RPC__out FORMATETC *pformatetcOut) { return E_NOTIMPL; }
	HRESULT STDMETHODCALLTYPE SetData(FORMATETC *pformatetc, STGMEDIUM *pmedium, BOOL fRelease) { return E_NOTIMPL; }
	HRESULT STDMETHODCALLTYPE EnumFormatEtc(DWORD dwDirection, __RPC__deref_out_opt IEnumFORMATETC **ppenumFormatEtc) { return E_NOTIMPL; }
	HRESULT STDMETHODCALLTYPE DAdvise(__RPC__in FORMATETC *pformatetc, DWORD advf, __RPC__in_opt IAdviseSink *pAdvSink, __RPC__out DWORD *pdwConnection) { return E_NOTIMPL; }
	HRESULT STDMETHODCALLTYPE DUnadvise(DWORD dwConnection) { return E_NOTIMPL; }
	HRESULT STDMETHODCALLTYPE EnumDAdvise(__RPC__deref_out_opt IEnumSTATDATA **ppenumAdvise) { return E_NOTIMPL; }

	// IPersistFile overrides
	HRESULT STDMETHODCALLTYPE IsDirty() { return E_NOTIMPL; };
	HRESULT STDMETHODCALLTYPE Load(__RPC__in LPCOLESTR pszFileName, DWORD dwMode);
	HRESULT STDMETHODCALLTYPE Save(__RPC__in_opt LPCOLESTR pszFileName, BOOL fRemember) { return E_NOTIMPL; }
	HRESULT STDMETHODCALLTYPE SaveCompleted(__RPC__in_opt LPCOLESTR pszFileName) { return E_NOTIMPL; }
	HRESULT STDMETHODCALLTYPE GetCurFile(__RPC__deref_out_opt LPOLESTR *ppszFileName) { return E_NOTIMPL; }

	// IPersist overrides
	HRESULT STDMETHODCALLTYPE GetClassID(__RPC__out CLSID *pClassID);

private:
	LONG m_refCount;
};
