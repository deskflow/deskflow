/* Copyright (C) 2002-2005 RealVNC Ltd.  All Rights Reserved.
 * 
 * This is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 * 
 * This software is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this software; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307,
 * USA.
 */

// -=- MsgWindow.cxx

#include <rfb_win32/MsgWindow.h>
#include <rfb_win32/WMShatter.h>
#include <rfb/LogWriter.h>
#include <rdr/Exception.h>
#include <malloc.h>
#include <tchar.h>

using namespace rfb;
using namespace rfb::win32;

static LogWriter vlog("MsgWindow");

//
// -=- MsgWindowClass
//

class MsgWindowClass {
public:
  MsgWindowClass();
  ~MsgWindowClass();
  ATOM classAtom;
  HINSTANCE instance;
};

LRESULT CALLBACK MsgWindowProc(HWND wnd, UINT msg, WPARAM wParam, LPARAM lParam) {
  LRESULT result;

  if (msg == WM_CREATE)
    SetWindowLong(wnd, GWL_USERDATA, (long)((CREATESTRUCT*)lParam)->lpCreateParams);
  else if (msg == WM_DESTROY)
    SetWindowLong(wnd, GWL_USERDATA, 0);
  MsgWindow* _this = (MsgWindow*) GetWindowLong(wnd, GWL_USERDATA);
  if (!_this) {
    vlog.info("null _this in %x, message %x", wnd, msg);
    return SafeDefWindowProc(wnd, msg, wParam, lParam);
  }

  try {
    result = _this->processMessage(msg, wParam, lParam);
  } catch (rdr::Exception& e) {
    vlog.error("untrapped: %s", e.str());
  }

  return result;
};

MsgWindowClass::MsgWindowClass() : classAtom(0) {
  WNDCLASS wndClass;
  wndClass.style = 0;
  wndClass.lpfnWndProc = MsgWindowProc;
  wndClass.cbClsExtra = 0;
  wndClass.cbWndExtra = 0;
  wndClass.hInstance = instance = GetModuleHandle(0);
  wndClass.hIcon = 0;
  wndClass.hCursor = 0;
  wndClass.hbrBackground = 0;
  wndClass.lpszMenuName = 0;
  wndClass.lpszClassName = _T("rfb::win32::MsgWindowClass");
  classAtom = RegisterClass(&wndClass);
  if (!classAtom) {
    throw rdr::SystemException("unable to register MsgWindow window class", GetLastError());
  }
}

MsgWindowClass::~MsgWindowClass() {
  if (classAtom) {
    UnregisterClass((const TCHAR*)classAtom, instance);
  }
}

MsgWindowClass baseClass;

//
// -=- MsgWindow
//

MsgWindow::MsgWindow(const TCHAR* name_) : name(tstrDup(name_)), handle(0) {
  vlog.debug("creating window \"%s\"", (const char*)CStr(name.buf));
  handle = CreateWindow((const TCHAR*)baseClass.classAtom, name.buf, WS_OVERLAPPED,
    0, 0, 10, 10, 0, 0, baseClass.instance, this);
  if (!handle) {
    throw rdr::SystemException("unable to create WMNotifier window instance", GetLastError());
  }
  vlog.debug("created window \"%s\" (%x)", (const char*)CStr(name.buf), handle);
}

MsgWindow::~MsgWindow() {
  if (handle)
    DestroyWindow(handle);
  vlog.debug("destroyed window \"%s\" (%x)", (const char*)CStr(name.buf), handle); 
}

LRESULT
MsgWindow::processMessage(UINT msg, WPARAM wParam, LPARAM lParam) {
  return SafeDefWindowProc(getHandle(), msg, wParam, lParam);
}