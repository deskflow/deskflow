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
// ServerDialog.h
//

#ifndef __SERVERDIALOG_H__
#define __SERVERDIALOG_H__

#include "TXDialog.h"
#include "TXLabel.h"
#include "TXEntry.h"
#include "TXButton.h"
#include "OptionsDialog.h"
#include "AboutDialog.h"

class ServerDialog : public TXDialog, public TXEntryCallback,
                     public TXButtonCallback {
public:
  ServerDialog(Display* dpy, OptionsDialog* options_, AboutDialog* about_)
    : TXDialog(dpy, 332, 120, "VNC Viewer: Connection Details", true),
      label(dpy, "VNC server:", this, 100),
      entry(dpy, this, this, false, 180),
      aboutButton(dpy, "About...", this, this, 60),
      optionsButton(dpy, "Options...", this, this, 60),
      okButton(dpy, "OK", this, this, 60),
      cancelButton(dpy, "Cancel", this, this, 60),
      options(options_), about(about_)
  {
    label.move(0, 30);
    entry.move(label.width(), 28);
    int x = width();
    int y = height() - yPad*4 - cancelButton.height();
    x -= cancelButton.width() + xPad*6;
    cancelButton.move(x, y);
    x -= okButton.width() + xPad*6;
    okButton.move(x, y);
    x -= optionsButton.width() + xPad*6;
    optionsButton.move(x, y);
    x -= aboutButton.width() + xPad*6;
    aboutButton.move(x, y);
  }

  virtual void takeFocus(Time time) {
    XSetInputFocus(dpy, entry.win(), RevertToParent, time);
  }

  virtual void entryCallback(TXEntry* e, Detail detail, Time time) {
    if (detail == ENTER) {
      ok = true;
      done = true;
    }
  }

  virtual void buttonActivate(TXButton* b) {
    if (b == &okButton) {
      ok = true;
      done = true;
    } else if (b == &cancelButton) {
      ok = false;
      done = true;
    } else if (b == &optionsButton) {
      options->show();
    } else if (b == &aboutButton) {
      about->show();
    }
  }

  TXLabel label;
  TXEntry entry;
  TXButton aboutButton, optionsButton, okButton, cancelButton;
  OptionsDialog* options;
  AboutDialog* about;
};

#endif
