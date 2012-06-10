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
//
// TXDialog.h
//
// A TXDialog is a pop-up dialog window.  The dialog can be made visible by
// calling its show() method.  Dialogs can be modal or non-modal.  For a modal
// dialog box, the show() method only returns when the dialog box has been
// dismissed.  For a non-modal dialog box, the show() method returns
// immediately.
//

#ifndef __TXDIALOG_H__
#define __TXDIALOG_H__

#include "TXWindow.h"
#include <errno.h>

class TXDialog : public TXWindow, public TXDeleteWindowCallback {
public:
  TXDialog(Display* dpy, int width, int height, const char* name,
           bool modal_=false)
    : TXWindow(dpy, width, height), done(false), ok(false), modal(modal_)
  {
    toplevel(name, this);
    resize(width, height);
  }

  virtual ~TXDialog() {}

  // show() makes the dialog visible.  For a modal dialog box, this processes X
  // events until the done flag has been set, after which it returns the value
  // of the ok flag.  For a non-modal dialog box it always returns true
  // immediately.
  bool show() {
    ok = false;
    done = false;
    initDialog();
    raise();
    map();
    if (modal) {
      while (true) {
        TXWindow::handleXEvents(dpy);
        if (done) {
          return ok;
        }
        fd_set rfds;
        FD_ZERO(&rfds);
        FD_SET(ConnectionNumber(dpy), &rfds);
        int n = select(FD_SETSIZE, &rfds, 0, 0, 0);
        if (n < 0) throw rdr::SystemException("select",errno);
      }
    }
    return true;
  }

  // initDialog() can be overridden in a derived class.  Typically it is used
  // to make sure that checkboxes have the right state, etc.
  virtual void initDialog() {}

  // resize() is overidden here to re-center the dialog
  void resize(int w, int h) {
    TXWindow::resize(w,h);
    int dpyWidth = WidthOfScreen(DefaultScreenOfDisplay(dpy));
    int dpyHeight = HeightOfScreen(DefaultScreenOfDisplay(dpy));
    setUSPosition((dpyWidth - width() - 10) / 2, (dpyHeight - height() - 30) / 2);
  }    

protected:
  virtual void deleteWindow(TXWindow* w) {
    ok = false;
    done = true;
    unmap();
  }

  bool done;
  bool ok;
  bool modal;
};

#endif
