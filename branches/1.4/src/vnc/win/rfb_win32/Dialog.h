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

// -=- RegConfig.h

// Class which monitors the registry and reads in the registry settings
// whenever they change, or are added or removed.

#ifndef __RFB_WIN32_DIALOG_H__
#define __RFB_WIN32_DIALOG_H__

#include <windows.h>
#include <prsht.h>
#include <list>
#include <rfb_win32/TCharArray.h>

namespace rfb {

  namespace win32 {

    // Dialog - A simple Win32 Dialog box.  A derived class of Dialog overrides the
    // initDialog(), command() and ok() methods to take appropriate action.  A
    // simple dialog box can be displayed by creating a Dialog object and calling
    // show().

    class Dialog {
    public:

      Dialog(HINSTANCE inst);
      virtual ~Dialog();

      // showDialog() displays the dialog box.  It returns when it has been dismissed,
      // returning true if "OK" was pressed, false otherwise.  The resource
      // argument identifies the dialog resource (often a MAKEINTRESOURCE macro
      // expansion), and owner is an optional window handle - the corresponding
      // window is disabled while the dialog box is displayed.

      bool showDialog(const TCHAR* resource, HWND owner=0);

      // initDialog() is called upon receipt of the WM_INITDIALOG message.

      virtual void initDialog() {}

      // onCommand() is called upon receipt of a WM_COMMAND message item other than IDOK
      // or IDCANCEL.  It should return true if the command has been handled.

      virtual bool onCommand(int item, int cmd) { return false; }

      // onHelp() is called upon receipt of a WM_MENU message.  This indicates that
      // context-specific help should be displayed, for a dialog control, for example.
      // It should return true if the command has been handled.

      virtual bool onHelp(int item) { return false; }

      // onOk() is called when the OK button is pressed.  The hwnd argument is the
      // dialog box's window handle.

      virtual bool onOk() { return true; }

      // Read the states of items
      bool isItemChecked(int id);
      int getItemInt(int id);
      TCHAR* getItemString(int id); // Recipient owns string storage
      
      // Set the states of items
      void setItemChecked(int id, bool state);
      void setItemInt(int id, int value);
      void setItemString(int id, const TCHAR* s);

      // enableItem is used to grey out an item, making it inaccessible, or to
      // re-enable it.
      void enableItem(int id, bool state);

    protected:
      static BOOL CALLBACK staticDialogProc(HWND hwnd, UINT msg,
			      WPARAM wParam, LPARAM lParam);
      virtual BOOL dialogProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
      HINSTANCE inst;
      HWND handle;
      bool alreadyShowing;
    };

    // PropertySheetPage 
    // Class used to define property pages within a PropertySheet.
    // Each page is associated with a particular dialog resource, indicated by
    // the "id" parameter supplied to the constructor.

    class PropSheetPage;

    class PropSheet {
    public:
      PropSheet(HINSTANCE inst, const TCHAR* title, std::list<PropSheetPage*> pages, HICON icon=0);
      virtual ~PropSheet();

      // Display the PropertySheet
      bool showPropSheet(HWND owner, bool showApply = false, bool showCtxtHelp = false, bool capture=false);
      
      // Calls initDialog again for each page that has already had it called.
      // Note: If a page hasn't been seen yet, it won't have been called.
      // Note: This must only be called while the property sheet is visible.
      void reInitPages();

      // Calls onOk for each page that has had initDialog called, and returns
      // false if any one of them returns false, or true otherwise.  ALL the
      // onOk() methods will be called, even if one of them fails.
      // Note: If a page hasn't been seen yet, it won't have been called.
      // Note: This must only be called while the property sheet is visible.
      bool commitPages();

      friend class PropSheetPage;

    protected:
      HWND owner;
      HICON icon;
      std::list<PropSheetPage*> pages;
      HINSTANCE inst;
      TCharArray title;
      HWND handle;
      bool alreadyShowing;
    };

    class PropSheetPage : public Dialog {
    public:
      PropSheetPage(HINSTANCE inst, const TCHAR* id);
      virtual ~PropSheetPage();

      void setChanged(bool changed);

      friend class PropSheet;

    protected:
      void setPropSheet(PropSheet* ps) {propSheet = ps;};
      static BOOL CALLBACK staticPageProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
      virtual BOOL dialogProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
      PROPSHEETPAGE page;
      PropSheet* propSheet;
    };

  };

};

#endif // __RFB_WIN32_DIALOG_H__
