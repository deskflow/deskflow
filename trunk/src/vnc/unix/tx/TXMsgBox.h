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
// TXMsgBox.h
//
// A TXMsgBox is a specialised pop-up dialog window, designed to present
// the user with a small amount of textual information, and potentially to
// obtain their response.
// TXMsgBoxes are always modal, and may have an Ok button, Ok+Cancel buttons,
// or Yes+No buttons.
// The MsgBox helper function creates a TXMsgBox on the fly, runs it, and
// returns the result.
//

#ifndef __TXMSGBOX_H__
#define __TXMSGBOX_H__

#include "TXDialog.h"
#include "TXLabel.h"
#include "TXButton.h"

enum TXMsgBoxFlags {
  MB_OK = 0,
  MB_OKCANCEL = 1,
  MB_YESNO = 4,
  MB_ICONERROR = 0x10,
  MB_ICONQUESTION = 0x20,
  MB_ICONWARNING = 0x30,
  MB_ICONINFORMATION = 0x40,
  MB_DEFBUTTON1 = 0,
  MB_DEFBUTTON2 = 0x100
};

class TXMsgBox : public TXDialog, public TXButtonCallback {
public:
  TXMsgBox(Display* dpy, const char* text, unsigned int flags, const char* title=0)
    : TXDialog(dpy, 1, 1, "Message", true),
      textLabel(dpy, "", this),
    okButton(dpy, "OK", this, this, 60),
    cancelButton(dpy, "Cancel", this, this, 60)
  {
    textLabel.xPad = 8;
    textLabel.move(0, yPad*4);
    textLabel.setText(text);
    resize(textLabel.width(),
           textLabel.height() + okButton.height() + yPad*12);

    switch (flags & 0x30) {
    case MB_ICONERROR:
      toplevel("Error", this); break;
    case MB_ICONQUESTION:
      toplevel("Question", this); break;
    case MB_ICONWARNING:
      toplevel("Warning", this); break;
    case MB_ICONINFORMATION:
      toplevel("Information", this); break;
    default:
      if (title)
	toplevel(title, this);
      break;
    };

    switch (flags & 0x7) {
    default:
      okButton.move((width() - okButton.width()) / 2,
		    height() - yPad*4 - okButton.height());
      cancelButton.unmap();
      break;
    case MB_OKCANCEL:
    case MB_YESNO:
      
      okButton.move(((width()/2) - okButton.width()) / 2,
		    height() - yPad*4 - okButton.height());
      cancelButton.move(((width()*3/2) - cancelButton.width()) / 2,
			height() - yPad*4 - cancelButton.height());
      if ((flags & 0x7) == MB_YESNO) {
	okButton.setText("Yes");
	cancelButton.setText("No");
      }
      break;
    };

    setBorderWidth(1);
  }

  virtual void buttonActivate(TXButton* b) {
    ok = (b == &okButton);
    done = true; 
    unmap();
  }

  TXLabel textLabel;
  TXButton okButton;
  TXButton cancelButton;
};

#endif
