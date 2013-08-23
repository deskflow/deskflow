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
#include <strsafe.h>

extern LONG g_refCount;
extern GUID g_CLSID;
extern void updateDraggingDir(char*);
extern void outputDebugStringF(const char *str, ...);

CDataHandlerExtension::CDataHandlerExtension()
{
	InterlockedIncrement(&g_refCount);
}

CDataHandlerExtension::~CDataHandlerExtension()
{
	InterlockedDecrement(&g_refCount);
}

HRESULT STDMETHODCALLTYPE
CDataHandlerExtension::QueryInterface(REFIID riid, void **ppvObject)
{
	static const QITAB qit[] = 
	{
		QITABENT(CDataHandlerExtension, IPersistFile),
		QITABENT(CDataHandlerExtension, IDataObject), 
		{ 0 },
	};
	return QISearch(this, qit, riid, ppvObject);
}

ULONG STDMETHODCALLTYPE
CDataHandlerExtension::AddRef()
{
	return InterlockedIncrement(&m_refCount);
}

ULONG STDMETHODCALLTYPE
CDataHandlerExtension::Release()
{
	LONG count = InterlockedDecrement(&m_refCount);
	if (count == 0) {
		delete this;
	}
	return count;
}

HRESULT STDMETHODCALLTYPE
CDataHandlerExtension::Load(__RPC__in LPCOLESTR pszFileName, DWORD dwMode)
{
	char selectedFileDir[MAX_PATH];
	StringCchCopyW(m_selectedFileDir, ARRAYSIZE(m_selectedFileDir), pszFileName);
	WideCharToMultiByte(CP_ACP, 0, m_selectedFileDir, -1, selectedFileDir, MAX_PATH, NULL, NULL);
	updateDraggingDir(selectedFileDir);
	
	return S_OK;
}

HRESULT STDMETHODCALLTYPE
CDataHandlerExtension::GetClassID(__RPC__out CLSID *pClassID)
{
	*pClassID = g_CLSID;
	return S_OK;
}
