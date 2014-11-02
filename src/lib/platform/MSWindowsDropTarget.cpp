/*
 * synergy -- mouse and keyboard sharing utility
 * Copyright (C) 2014 Synergy Si Ltd.
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

#include "platform/MSWindowsDropTarget.h"

#include "base/Log.h"
#include "common/common.h"

#include <stdio.h>
#include <Shlobj.h>

void getDropData(IDataObject *pDataObject);

CMSWindowsDropTarget* CMSWindowsDropTarget::s_instance = NULL;

CMSWindowsDropTarget::CMSWindowsDropTarget() :
	m_refCount(1),
	m_allowDrop(false)
{
	s_instance = this;
}

CMSWindowsDropTarget::~CMSWindowsDropTarget()
{
}

CMSWindowsDropTarget&
CMSWindowsDropTarget::instance()
{
	assert(s_instance != NULL);
	return *s_instance;
}

HRESULT
CMSWindowsDropTarget::DragEnter(IDataObject* dataObject, DWORD keyState, POINTL point, DWORD* effect)
{
	// check if data object contain drop
	m_allowDrop = queryDataObject(dataObject);
	if (m_allowDrop) {
		getDropData(dataObject);
	}
	
	*effect = DROPEFFECT_NONE;

	return S_OK;
}

HRESULT
CMSWindowsDropTarget::DragOver(DWORD keyState, POINTL point, DWORD* effect)
{
	*effect = DROPEFFECT_NONE;

	return S_OK;
}

HRESULT
CMSWindowsDropTarget::DragLeave(void)
{
	return S_OK;
}

HRESULT
CMSWindowsDropTarget::Drop(IDataObject* dataObject, DWORD keyState, POINTL point, DWORD* effect)
{
	*effect = DROPEFFECT_NONE;

	return S_OK;
}

bool
CMSWindowsDropTarget::queryDataObject(IDataObject* dataObject)
{
	// check if it supports CF_HDROP using a HGLOBAL
	FORMATETC fmtetc = { CF_HDROP, 0, DVASPECT_CONTENT, -1, TYMED_HGLOBAL };

	return dataObject->QueryGetData(&fmtetc) == S_OK ? true : false;
}

void
CMSWindowsDropTarget::setDraggingFilename(char* const filename)
{
	m_dragFilename = filename;
}

std::string
CMSWindowsDropTarget::getDraggingFilename()
{
	return m_dragFilename;
}

void
CMSWindowsDropTarget::clearDraggingFilename()
{
	m_dragFilename.clear();
}

void
getDropData(IDataObject* dataObject)
{
	// construct a FORMATETC object
	FORMATETC fmtEtc = { CF_HDROP, 0, DVASPECT_CONTENT, -1, TYMED_HGLOBAL };
	STGMEDIUM stgMed;

	// See if the dataobject contains any DROP stored as a HGLOBAL
	if(dataObject->QueryGetData(&fmtEtc) == S_OK) {
		if(dataObject->GetData(&fmtEtc, &stgMed) == S_OK) {
			// get data here
			PVOID data = GlobalLock(stgMed.hGlobal);

			// data object global handler contains:
			// DROPFILESfilename1 filename2 two spaces as the end
			// TODO: get multiple filenames
			wchar_t* wcData = (wchar_t*)((LPBYTE)data + sizeof(DROPFILES));

			// convert wchar to char
			char* filename = new char[wcslen(wcData) + 1];
			filename[wcslen(wcData)] = '\0';
			wcstombs(filename, wcData, wcslen(wcData));

			CMSWindowsDropTarget::instance().setDraggingFilename(filename);
			
			GlobalUnlock(stgMed.hGlobal);

			// release the data using the COM API
			ReleaseStgMedium(&stgMed);

			delete[] filename;
		}
	}
}

HRESULT __stdcall
CMSWindowsDropTarget::QueryInterface (REFIID iid, void ** object)
{
	if (iid == IID_IDropTarget || iid == IID_IUnknown) {
		AddRef();
		*object = this;
		return S_OK;
	}
	else {
		*object = 0;
		return E_NOINTERFACE;
	}
}

ULONG __stdcall
CMSWindowsDropTarget::AddRef(void)
{
	return InterlockedIncrement(&m_refCount);
}

ULONG __stdcall
CMSWindowsDropTarget::Release(void)
{
	LONG count = InterlockedDecrement(&m_refCount);
		
	if (count == 0) {
		delete this;
		return 0;
	}
	else {
		return count;
	}
}
