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
// PasswdDialog.h
//

#ifndef __PASSWDDIALOG_H__
#define __PASSWDDIALOG_H__

#include "TXDialog.h"
#include "TXLabel.h"
#include "TXEntry.h"

class PasswdDialog : public TXDialog, public TXEntryCallback {
public:
  PasswdDialog(Display* dpy, const char* title, bool userDisabled)
    : TXDialog(dpy, 320, 100, title, true),
      userLabel(dpy, "Username:", this, 120),
      userEntry(dpy, this, this, false, 180),
      passwdLabel(dpy, "Password:", this, 120),
      passwdEntry(dpy, this, this, true, 180)
  {
    userLabel.move(0, 20);
    userEntry.move(userLabel.width(), 18);
    userEntry.disabled(userDisabled);
    passwdLabel.move(0, 60);
    passwdEntry.move(passwdLabel.width(), 58);
  }

  void takeFocus(Time time) {
    if (!userEntry.disabled())
      XSetInputFocus(dpy, userEntry.win(), RevertToParent, time);
    else
      XSetInputFocus(dpy, passwdEntry.win(), RevertToParent, time);
  }

  void entryCallback(TXEntry* e, Detail detail, Time time) {
    if (e == &userEntry) {
      if (detail == ENTER || detail == NEXT_FOCUS || detail == PREV_FOCUS)
        XSetInputFocus(dpy, passwdEntry.win(), RevertToParent, time);
    } else if (e == &passwdEntry) {
      if (detail == ENTER) {
        ok = true;
        done = true;
      } else if (detail == NEXT_FOCUS || detail == PREV_FOCUS) {
        XSetInputFocus(dpy, userEntry.win(), RevertToParent, time);
      }
    }
  }

  TXLabel userLabel;
  TXEntry userEntry;
  TXLabel passwdLabel;
  TXEntry passwdEntry;
};

#endif
