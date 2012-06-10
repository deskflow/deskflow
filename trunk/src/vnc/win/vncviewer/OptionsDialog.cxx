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

#include <vncviewer/OptionsDialog.h>
#include <vncviewer/CConn.h>
#include <vncviewer/resource.h>
#include <rfb_win32/Registry.h>
#include <rfb_win32/MsgBox.h>
#include <rfb_win32/OSVersion.h>
#include <rfb/encodings.h>
#include <rfb/CConnection.h>
#include <commdlg.h>
#include <rfb/LogWriter.h>

using namespace rfb;
using namespace rfb::win32;

static LogWriter vlog("Options");


struct OptionsInfo {
  CConn* view;
  CConnOptions options;
};


OptionsDialog rfb::win32::OptionsDialog::global;


class ViewerOptions : public PropSheet {
public:
  ViewerOptions(OptionsInfo& info_, std::list<PropSheetPage*> pages)
    : PropSheet(GetModuleHandle(0), 
    info_.view ? _T("VNC Viewer Options") : _T("VNC Viewer Defaults"), pages),
    info(info_), changed(false) {
  }
  ~ViewerOptions() {
    if (changed) {
      if (info.view)
        // Apply the settings to the supplied session object
        info.view->applyOptions(info.options);
      else {
        // Commit the settings to the user's registry area
        info.options.writeDefaults();
      }
    }
  }

  void setChanged() {changed = true;}

  bool changed;
  OptionsInfo& info;
};
      

class FormatPage : public PropSheetPage {
public:
  FormatPage(OptionsInfo* dlg_)
    : PropSheetPage(GetModuleHandle(0), MAKEINTRESOURCE(IDD_FORMAT)), dlg(dlg_) {
  }
  virtual void initDialog() {
    setItemChecked(IDC_ENCODING_AUTO, dlg->options.autoSelect);
    setItemChecked(IDC_FORMAT_FULLCOLOUR, dlg->options.fullColour);
    if (!dlg->options.fullColour) {
      switch (dlg->options.lowColourLevel) {
      case 0: setItemChecked(IDC_FORMAT_VERYLOWCOLOUR, true); break;
      case 1: setItemChecked(IDC_FORMAT_LOWCOLOUR, true); break;
      case 2: setItemChecked(IDC_FORMAT_MEDIUMCOLOUR, true); break;
      }
    }
    switch (dlg->options.preferredEncoding) {
    case encodingZRLE: setItemChecked(IDC_ENCODING_ZRLE, true); break;
    case encodingHextile: setItemChecked(IDC_ENCODING_HEXTILE, true); break;
    case encodingRaw: setItemChecked(IDC_ENCODING_RAW, true); break;
    }
    onCommand(IDC_ENCODING_AUTO, 0 /* ? */); // Force enableItem status to refresh
  }
  virtual bool onOk() {
    dlg->options.autoSelect = isItemChecked(IDC_ENCODING_AUTO);
    dlg->options.fullColour = isItemChecked(IDC_FORMAT_FULLCOLOUR);
    if (isItemChecked(IDC_FORMAT_VERYLOWCOLOUR))
      dlg->options.lowColourLevel = 0;
    if (isItemChecked(IDC_FORMAT_LOWCOLOUR))
      dlg->options.lowColourLevel = 1;
    if (isItemChecked(IDC_FORMAT_MEDIUMCOLOUR))
      dlg->options.lowColourLevel = 2;
    dlg->options.preferredEncoding = encodingZRLE;
    if (isItemChecked(IDC_ENCODING_HEXTILE))
      dlg->options.preferredEncoding = encodingHextile;
    if (isItemChecked(IDC_ENCODING_RAW))
      dlg->options.preferredEncoding = encodingRaw;
    ((ViewerOptions*)propSheet)->setChanged();
    return true;
  }
  virtual bool onCommand(int id, int cmd) {
    if (id == IDC_ENCODING_AUTO) {
      bool ok = !isItemChecked(IDC_ENCODING_AUTO);
      enableItem(IDC_ENCODING_ZRLE, ok);
      enableItem(IDC_ENCODING_HEXTILE, ok);
      enableItem(IDC_ENCODING_RAW, ok);
      return true;
    }
    return false;
  }
protected:
  OptionsInfo* dlg;
};

class MiscPage : public PropSheetPage {
public:
  MiscPage(OptionsInfo* dlg_)
    : PropSheetPage(GetModuleHandle(0), MAKEINTRESOURCE(IDD_MISC)), dlg(dlg_) {
  }
  virtual void initDialog() {
    setItemChecked(IDC_CONN_SHARED, dlg->options.shared);
    enableItem(IDC_CONN_SHARED, (!dlg->view) || (dlg->view->state() != CConnection::RFBSTATE_NORMAL));
    setItemChecked(IDC_FULL_SCREEN, dlg->options.fullScreen);
    setItemChecked(IDC_LOCAL_CURSOR, dlg->options.useLocalCursor);
    setItemChecked(IDC_DESKTOP_RESIZE, dlg->options.useDesktopResize);
    enableItem(IDC_PROTOCOL_3_3, (!dlg->view) || (dlg->view->state() != CConnection::RFBSTATE_NORMAL));
    setItemChecked(IDC_PROTOCOL_3_3, dlg->options.protocol3_3);
    setItemChecked(IDC_ACCEPT_BELL, dlg->options.acceptBell);
    setItemChecked(IDC_AUTO_RECONNECT, dlg->options.autoReconnect);
  }
  virtual bool onOk() {
    dlg->options.shared = isItemChecked(IDC_CONN_SHARED);
    dlg->options.fullScreen = isItemChecked(IDC_FULL_SCREEN);
    dlg->options.useLocalCursor = isItemChecked(IDC_LOCAL_CURSOR);
    dlg->options.useDesktopResize = isItemChecked(IDC_DESKTOP_RESIZE);
    dlg->options.protocol3_3 = isItemChecked(IDC_PROTOCOL_3_3);
    dlg->options.acceptBell = isItemChecked(IDC_ACCEPT_BELL);
    dlg->options.autoReconnect = isItemChecked(IDC_AUTO_RECONNECT);
    ((ViewerOptions*)propSheet)->setChanged();
    return true;
  }
protected:
  OptionsInfo* dlg;
};

class InputsPage : public PropSheetPage {
public:
  InputsPage(OptionsInfo* dlg_)
    : PropSheetPage(GetModuleHandle(0), MAKEINTRESOURCE(IDD_INPUTS)), dlg(dlg_) {
  }
  virtual void initDialog() {
    setItemChecked(IDC_SEND_POINTER, dlg->options.sendPtrEvents);
    setItemChecked(IDC_SEND_KEYS, dlg->options.sendKeyEvents);
    setItemChecked(IDC_CLIENT_CUTTEXT, dlg->options.clientCutText);
    setItemChecked(IDC_SERVER_CUTTEXT, dlg->options.serverCutText);
    setItemChecked(IDC_DISABLE_WINKEYS, dlg->options.disableWinKeys && !osVersion.isPlatformWindows);
    enableItem(IDC_DISABLE_WINKEYS, !osVersion.isPlatformWindows);
    setItemChecked(IDC_EMULATE3, dlg->options.emulate3);
    setItemChecked(IDC_POINTER_INTERVAL, dlg->options.pointerEventInterval != 0);

    // Populate the Menu Key tab
    HWND menuKey = GetDlgItem(handle, IDC_MENU_KEY);
    SendMessage(menuKey, CB_RESETCONTENT, 0, 0);
    SendMessage(menuKey, CB_ADDSTRING, 0, (LPARAM)_T("none"));
    if (!dlg->options.menuKey)
      SendMessage(menuKey, CB_SETCURSEL, 0, 0);
    for (int i=0; i<12; i++) {
      TCHAR buf[4];
      _stprintf(buf, _T("F%d"), i+1);
      int index = SendMessage(menuKey, CB_ADDSTRING, 0, (LPARAM)buf);
      if (i == (dlg->options.menuKey - VK_F1))
        SendMessage(menuKey, CB_SETCURSEL, index, 0);
    }
  }
  virtual bool onOk() {
    dlg->options.sendPtrEvents = isItemChecked(IDC_SEND_POINTER);
    dlg->options.sendKeyEvents = isItemChecked(IDC_SEND_KEYS);
    dlg->options.clientCutText = isItemChecked(IDC_CLIENT_CUTTEXT);
    dlg->options.serverCutText = isItemChecked(IDC_SERVER_CUTTEXT);
    dlg->options.disableWinKeys = isItemChecked(IDC_DISABLE_WINKEYS);
    dlg->options.emulate3 = isItemChecked(IDC_EMULATE3);
    dlg->options.pointerEventInterval = 
      isItemChecked(IDC_POINTER_INTERVAL) ? 200 : 0;

    HWND mkHwnd = GetDlgItem(handle, IDC_MENU_KEY);
    int index = SendMessage(mkHwnd, CB_GETCURSEL, 0, 0);
    TCharArray keyName(SendMessage(mkHwnd, CB_GETLBTEXTLEN, index, 0)+1);
    SendMessage(mkHwnd, CB_GETLBTEXT, index, (LPARAM)keyName.buf);
    if (_tcscmp(keyName.buf, _T("none")) == 0)
      dlg->options.setMenuKey("");
    else
      dlg->options.setMenuKey(CStr(keyName.buf));

    ((ViewerOptions*)propSheet)->setChanged();
    return true;
  }
protected:
  OptionsInfo* dlg;
};

class DefaultsPage : public PropSheetPage {
public:
  DefaultsPage(OptionsInfo* dlg_)
    : PropSheetPage(GetModuleHandle(0), MAKEINTRESOURCE(IDD_DEFAULTS)), dlg(dlg_) {
  }
  virtual void initDialog() {
    enableItem(IDC_LOAD_CONFIG, dlg->options.configFileName.buf);
    enableItem(IDC_SAVE_CONFIG, dlg->options.configFileName.buf);
  }
  virtual bool onCommand(int id, int cmd) {
    switch (id) {
    case IDC_LOAD_DEFAULTS:
      dlg->options = CConnOptions();
      break;
    case IDC_SAVE_DEFAULTS:
      propSheet->commitPages();
      dlg->options.writeDefaults();
      break;
    case IDC_LOAD_CONFIG:
      dlg->options.readFromFile(dlg->options.configFileName.buf);
      break;
    case IDC_SAVE_CONFIG:
      propSheet->commitPages();
      dlg->options.writeToFile(dlg->options.configFileName.buf);
      MsgBox(handle, _T("Options saved successfully"),
             MB_OK | MB_ICONINFORMATION);
      return 0;
    case IDC_SAVE_CONFIG_AS:
      propSheet->commitPages();
      // Get a filename to save to
      TCHAR newFilename[4096];
      TCHAR currentDir[4096];
      if (dlg->options.configFileName.buf)
        _tcscpy(newFilename, TStr(dlg->options.configFileName.buf));
      else
        newFilename[0] = 0;
      OPENFILENAME ofn;
      memset(&ofn, 0, sizeof(ofn));
#ifdef OPENFILENAME_SIZE_VERSION_400
      ofn.lStructSize = OPENFILENAME_SIZE_VERSION_400;
#else
      ofn.lStructSize = sizeof(ofn);
#endif
      ofn.hwndOwner = handle;
      ofn.lpstrFilter = _T("VNC Connection Options\000*.vnc\000");
      ofn.lpstrFile = newFilename;
      currentDir[0] = 0;
      GetCurrentDirectory(4096, currentDir);
      ofn.lpstrInitialDir = currentDir;
      ofn.nMaxFile = 4096;
      ofn.lpstrDefExt = _T(".vnc");
      ofn.Flags = OFN_NOREADONLYRETURN | OFN_OVERWRITEPROMPT | OFN_PATHMUSTEXIST;
      if (!GetSaveFileName(&ofn)) {
        if (CommDlgExtendedError())
          throw rdr::Exception("GetSaveFileName failed");
        return 0;
      }

      // Save the Options
      dlg->options.writeToFile(CStr(newFilename));
      MsgBox(handle, _T("Options saved successfully"),
             MB_OK | MB_ICONINFORMATION);
      return 0;
    };
    propSheet->reInitPages();
    return true;
  }
protected:
  OptionsInfo* dlg;
};


OptionsDialog::OptionsDialog() : visible(false) {
}

bool OptionsDialog::showDialog(CConn* view, bool capture) {
  if (visible) return false;
  visible = true;

  // Grab the current properties
  OptionsInfo info;
  if (view)
    info.options = view->getOptions();
  info.view = view;

  // Build a list of pages to display
  std::list<PropSheetPage*> pages;
  FormatPage formatPage(&info); pages.push_back(&formatPage);
  InputsPage inputsPage(&info); pages.push_back(&inputsPage);
  MiscPage miscPage(&info); pages.push_back(&miscPage);
  DefaultsPage defPage(&info); if (view) pages.push_back(&defPage);

  // Show the property sheet
  ViewerOptions dialog(info, pages);
  dialog.showPropSheet(view && view->getWindow() ? view->getWindow()->getHandle() : 0,
                       false, false, capture);

  visible = false;
  return dialog.changed;
}
