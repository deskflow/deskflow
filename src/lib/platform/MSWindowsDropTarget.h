/*
 * synergy -- mouse and keyboard sharing utility
 * Copyright (C) 2014-2016 Symless Ltd.
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

#pragma once

#include <string>
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include <oleidl.h>

class MSWindowsScreen;

class MSWindowsDropTarget : public IDropTarget {
public:
    MSWindowsDropTarget ();
    ~MSWindowsDropTarget ();

    // IUnknown implementation
    HRESULT __stdcall QueryInterface (REFIID iid, void** object);
    ULONG __stdcall AddRef (void);
    ULONG __stdcall Release (void);

    // IDropTarget implementation
    HRESULT __stdcall DragEnter (IDataObject* dataObject, DWORD keyState,
                                 POINTL point, DWORD* effect);
    HRESULT __stdcall DragOver (DWORD keyState, POINTL point, DWORD* effect);
    HRESULT __stdcall DragLeave (void);
    HRESULT __stdcall Drop (IDataObject* dataObject, DWORD keyState,
                            POINTL point, DWORD* effect);

    void setDraggingFilename (char* const);
    std::string getDraggingFilename ();
    void clearDraggingFilename ();

    static MSWindowsDropTarget& instance ();

private:
    bool queryDataObject (IDataObject* dataObject);

    long m_refCount;
    bool m_allowDrop;
    std::string m_dragFilename;

    static MSWindowsDropTarget* s_instance;
};
