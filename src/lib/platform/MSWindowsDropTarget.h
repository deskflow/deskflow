/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2014 - 2016 Symless Ltd.
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#pragma once

#include <string>
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include <oleidl.h>

class MSWindowsScreen;

class MSWindowsDropTarget : public IDropTarget
{
public:
  MSWindowsDropTarget();
  ~MSWindowsDropTarget();

  // IUnknown implementation
  HRESULT __stdcall QueryInterface(REFIID iid, void **object);
  ULONG __stdcall AddRef(void);
  ULONG __stdcall Release(void);

  // IDropTarget implementation
  HRESULT __stdcall DragEnter(IDataObject *dataObject, DWORD keyState, POINTL point, DWORD *effect);
  HRESULT __stdcall DragOver(DWORD keyState, POINTL point, DWORD *effect);
  HRESULT __stdcall DragLeave(void);
  HRESULT __stdcall Drop(IDataObject *dataObject, DWORD keyState, POINTL point, DWORD *effect);

  void setDraggingFilename(char *const);
  std::string getDraggingFilename();
  void clearDraggingFilename();

  static MSWindowsDropTarget &instance();

private:
  bool queryDataObject(IDataObject *dataObject);

  long m_refCount;
  bool m_allowDrop;
  std::string m_dragFilename;

  static MSWindowsDropTarget *s_instance;
};
