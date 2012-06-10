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

// -=- Dialog.cxx

// Base-class for any Dialog classes we might require

#include <rfb_win32/Dialog.h>
#include <rfb_win32/TCharArray.h>
#include <rfb/LogWriter.h>
#include <rdr/Exception.h>
#include <rfb_win32/Win32Util.h>

#ifdef _DIALOG_CAPTURE
#ifdef PropSheet_IndexToId
#include <rfb_win32/DeviceFrameBuffer.h>
#include <extra/LoadBMP.cxx>
#else
#undef _DIALOG_CAPTURE
#pragma message("  NOTE: Not building Dialog Capture support.")
#endif
#endif

using namespace rfb;
using namespace rfb::win32;

static LogWriter dlog("Dialog");
static LogWriter plog("PropSheet");


Dialog::Dialog(HINSTANCE inst_)
: inst(inst_), alreadyShowing(false), handle(0)
{
}

Dialog::~Dialog()
{
}


bool Dialog::showDialog(const TCHAR* resource, HWND owner)
{
  if (alreadyShowing) return false;
  handle = 0;
  alreadyShowing = true;
  INT_PTR result = DialogBoxParam(inst, resource, owner,
                                  staticDialogProc, (LPARAM)this);
  if (result<0)
    throw rdr::SystemException("DialogBoxParam failed", GetLastError());
  alreadyShowing = false;
  return (result == 1);
}


bool Dialog::isItemChecked(int id) {
  return SendMessage(GetDlgItem(handle, id), BM_GETCHECK, 0, 0) == BST_CHECKED;
}
int Dialog::getItemInt(int id) {
  BOOL trans;
  int result = GetDlgItemInt(handle, id, &trans, TRUE);
  if (!trans)
    throw rdr::Exception("unable to read dialog Int");
  return result;
}
TCHAR* Dialog::getItemString(int id) {
  TCharArray tmp(256);
  if (!GetDlgItemText(handle, id, tmp.buf, 256))
    tmp.buf[0] = 0;
  return tmp.takeBuf();
}

void Dialog::setItemChecked(int id, bool state) {
  dlog.debug("bool[%d]=%d", id, (int)state);
  SendMessage(GetDlgItem(handle, id), BM_SETCHECK, state ? BST_CHECKED : BST_UNCHECKED, 0);
}
void Dialog::setItemInt(int id, int value) {
  dlog.debug("int[%d]=%d", id, value);
  SetDlgItemInt(handle, id, value, TRUE);
}
void Dialog::setItemString(int id, const TCHAR* s) {
  dlog.debug("string[%d]=%s", id, (const char*)CStr(s));
  SetDlgItemText(handle, id, s);
}


void Dialog::enableItem(int id, bool state) {
  dlog.debug("enable[%d]=%d", id, (int)state);
  EnableWindow(GetDlgItem(handle, id), state);
}




BOOL CALLBACK Dialog::staticDialogProc(HWND hwnd, UINT msg,
				       WPARAM wParam, LPARAM lParam)
{
  if (msg == WM_INITDIALOG)
    SetWindowLong(hwnd, GWL_USERDATA, (LONG)lParam);

  LONG self = GetWindowLong(hwnd, GWL_USERDATA);
  if (!self) return FALSE;

  return ((Dialog*)self)->dialogProc(hwnd, msg, wParam, lParam);
}

BOOL Dialog::dialogProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
  switch (msg) {

  case WM_INITDIALOG:
    handle = hwnd;
    initDialog();
    return TRUE;

  case WM_COMMAND:
    switch (LOWORD(wParam)) {
    case IDOK:
      if (onOk()) {
        EndDialog(hwnd, 1);
        return TRUE;
      }
      return FALSE;
    case IDCANCEL:
      EndDialog(hwnd, 0);
      return TRUE;
    default:
      return onCommand(LOWORD(wParam), HIWORD(wParam));
    };

  case WM_HELP:
    return onHelp(((HELPINFO*)lParam)->iCtrlId);

  }

  return FALSE;
}


PropSheetPage::PropSheetPage(HINSTANCE inst, const TCHAR* id) : Dialog(inst), propSheet(0) {
  page.dwSize = sizeof(page);
  page.dwFlags = 0; // PSP_USECALLBACK;
  page.hInstance = inst;
  page.pszTemplate = id;
  page.pfnDlgProc = staticPageProc;
  page.lParam = (LPARAM)this;
  page.pfnCallback = 0; // staticPageProc;
}

PropSheetPage::~PropSheetPage() {
}


BOOL CALLBACK PropSheetPage::staticPageProc(HWND hwnd, UINT msg,
				       WPARAM wParam, LPARAM lParam)
{
  if (msg == WM_INITDIALOG)
    SetWindowLong(hwnd, GWL_USERDATA, ((PROPSHEETPAGE*)lParam)->lParam);

  LONG self = GetWindowLong(hwnd, GWL_USERDATA);
  if (!self) return FALSE;

  return ((PropSheetPage*)self)->dialogProc(hwnd, msg, wParam, lParam);
}

BOOL PropSheetPage::dialogProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
  switch (msg) {

  case WM_INITDIALOG:
    handle = hwnd;
    initDialog();
    return TRUE;

  case WM_NOTIFY:
    switch (((NMHDR*)lParam)->code) {
    case PSN_APPLY:
      onOk();
      return FALSE;
    };
    return FALSE;

  case WM_COMMAND:
    return onCommand(LOWORD(wParam), HIWORD(wParam));

  case WM_HELP:
    return onHelp(((HELPINFO*)lParam)->iCtrlId);

  }

  return FALSE;
}


PropSheet::PropSheet(HINSTANCE inst_, const TCHAR* title_, std::list<PropSheetPage*> pages_, HICON icon_)
: title(tstrDup(title_)), inst(inst_), pages(pages_), alreadyShowing(0), handle(0), icon(icon_) {
}

PropSheet::~PropSheet() {
}


// For some reason, DLGTEMPLATEEX isn't defined in the Windows headers - go figure...
struct DLGTEMPLATEEX {
  WORD dlgVer;
  WORD signature;
  DWORD helpID;
  DWORD exStyle;
  DWORD style;
  WORD cDlgItems;
  short x;
  short y;
  short cx;
  short cy;
};

static int CALLBACK removeCtxtHelp(HWND hwnd, UINT message, LPARAM lParam) {
  if (message == PSCB_PRECREATE) {
    // Remove the context-help style, to remove the titlebar ? button
    // *** Nasty hack to cope with new & old dialog template formats...
    if (((DLGTEMPLATEEX*)lParam)->signature == 0xffff)
      ((DLGTEMPLATEEX*)lParam)->style &= ~DS_CONTEXTHELP;
    else
      ((LPDLGTEMPLATE)lParam)->style &= ~DS_CONTEXTHELP;
  }
  return TRUE;
}


bool PropSheet::showPropSheet(HWND owner, bool showApply, bool showCtxtHelp, bool capture) {
  if (alreadyShowing) return false;
  alreadyShowing = true;
  int count = pages.size();

  HPROPSHEETPAGE* hpages = new HPROPSHEETPAGE[count];
  try {
    // Create the PropertSheet page GDI objects.
    std::list<PropSheetPage*>::iterator pspi;
    int i = 0;
    for (pspi=pages.begin(); pspi!=pages.end(); pspi++) {
      hpages[i] = CreatePropertySheetPage(&((*pspi)->page));
      (*pspi)->setPropSheet(this);
      i++;
    }
 
    // Initialise and create the PropertySheet itself
    PROPSHEETHEADER header;
    header.dwSize = PROPSHEETHEADER_V1_SIZE;
    header.dwFlags = PSH_MODELESS | (showApply ? 0 : PSH_NOAPPLYNOW) | (showCtxtHelp ? 0 : PSH_USECALLBACK);
    header.pfnCallback = removeCtxtHelp;
    header.hwndParent = owner;
    header.hInstance = inst;
    header.pszCaption = title.buf;
    header.nPages = count;
    header.nStartPage = 0;
    header.phpage = hpages;
    if (icon) {
      header.hIcon = icon;
      header.dwFlags |= PSH_USEHICON;
    }

    handle = (HWND)PropertySheet(&header);
    if ((handle == 0) || (handle == (HWND)-1))
      throw rdr::SystemException("PropertySheet failed", GetLastError());
    centerWindow(handle, owner);
    plog.info("created %lx", handle);

#ifdef _DIALOG_CAPTURE
    if (capture) {
      plog.info("capturing \"%s\"", (const char*)CStr(title.buf));
      char* tmpdir = getenv("TEMP");
      HDC dc = GetWindowDC(handle);
      DeviceFrameBuffer fb(dc);
      int i=0;
      while (true) {
        int id = PropSheet_IndexToId(handle, i);
        if (!id) break;
        PropSheet_SetCurSelByID(handle, id);
        MSG msg;
        while (PeekMessage(&msg, handle, 0, 0, PM_REMOVE)) {
          if (!PropSheet_IsDialogMessage(handle, &msg))
            DispatchMessage(&msg);
        }
        fb.grabRect(fb.getRect());
        TCHAR title[128];
        if (!GetWindowText(PropSheet_GetCurrentPageHwnd(handle), title, sizeof(title)))
          _stprintf(title, _T("capture%d"), i);
        CharArray pageTitle(strDup(title));
        for (int j=0; j<strlen(pageTitle.buf); j++) {
          if (pageTitle.buf[j] == '/' || pageTitle.buf[j] == '\\' || pageTitle.buf[j] == ':')
            pageTitle.buf[j] = '-';
        }
        char filename[256];
        sprintf(filename, "%s\\%s.bmp", tmpdir, pageTitle.buf);
        vlog.debug("writing to %s", filename);
        saveBMP(filename, &fb);
        i++;
      }
      ReleaseDC(handle, dc);
    } else {
#endif
      try {
        if (owner)
          EnableWindow(owner, FALSE);
        // Run the PropertySheet
        MSG msg;
        while (GetMessage(&msg, 0, 0, 0)) {
          if (!PropSheet_IsDialogMessage(handle, &msg))
            DispatchMessage(&msg);
          if (!PropSheet_GetCurrentPageHwnd(handle))
            break;
        }
        if (owner)
          EnableWindow(owner, TRUE);
      } catch (...) {
        if (owner)
          EnableWindow(owner, TRUE);
        throw;
      }
#ifdef _DIALOG_CAPTURE
    }
#endif

    plog.info("finished %lx", handle);

    DestroyWindow(handle);
    handle = 0;
    alreadyShowing = false;

    // Clear up the pages' GDI objects
    for (pspi=pages.begin(); pspi!=pages.end(); pspi++)
      (*pspi)->setPropSheet(0);
    delete [] hpages; hpages = 0;

    return true;
  } catch (rdr::Exception) {
    alreadyShowing = false;

    std::list<PropSheetPage*>::iterator pspi;
    for (pspi=pages.begin(); pspi!=pages.end(); pspi++)
      (*pspi)->setPropSheet(0);
    delete [] hpages; hpages = 0;

    throw;
  }
}

void PropSheet::reInitPages() {
  plog.debug("reInitPages %lx", handle);
  std::list<PropSheetPage*>::iterator pspi;
  for (pspi=pages.begin(); pspi!=pages.end(); pspi++) {
    if ((*pspi)->handle)
      (*pspi)->initDialog();
  }
}

bool PropSheet::commitPages() {
  plog.debug("commitPages %lx", handle);
  bool result = true;
  std::list<PropSheetPage*>::iterator pspi;
  for (pspi=pages.begin(); pspi!=pages.end(); pspi++) {
    if ((*pspi)->handle)
      result = result && (*pspi)->onOk();
  }
  return result;
}


void PropSheetPage::setChanged(bool changed) {
  if (propSheet) {
    plog.debug("setChanged[%lx(%lx)]=%d", handle, propSheet->handle, (int)changed);
    if (changed)
      PropSheet_Changed(propSheet->handle, handle);
    else
      PropSheet_UnChanged(propSheet->handle, handle);
  }
}
