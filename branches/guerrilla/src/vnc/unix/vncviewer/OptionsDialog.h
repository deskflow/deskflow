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
// OptionsDialog.h
//

#ifndef __OPTIONSDIALOG_H__
#define __OPTIONSDIALOG_H__

#include "TXDialog.h"
#include "TXLabel.h"
#include "TXEntry.h"
#include "TXButton.h"
#include "TXCheckbox.h"
#include "parameters.h"

class OptionsDialogCallback {
public:
  virtual void setOptions() = 0;
  virtual void getOptions() = 0;
};

class OptionsDialog : public TXDialog, public TXButtonCallback,
                      public TXCheckboxCallback {
public:
  OptionsDialog(Display* dpy, OptionsDialogCallback* cb_)
    : TXDialog(dpy, 400, 400, "VNC Viewer: Connection Options"), cb(cb_),
      formatAndEnc(dpy, "Encoding and Colour Level:", this),
      inputs(dpy, "Inputs:", this),
      misc(dpy, "Misc:", this),
      autoSelect(dpy, "Auto select", this, false, this),
      fullColour(dpy, "Full (all available colours)", this, true, this),
      mediumColour(dpy, "Medium (256 colours)", this, true, this),
      lowColour(dpy, "Low (64 colours)", this, true, this),
      veryLowColour(dpy, "Very low (8 colours)", this, true, this),
      zrle(dpy, "ZRLE", this, true, this),
      hextile(dpy, "Hextile", this, true, this),
      raw(dpy, "Raw", this, true, this),
      viewOnly(dpy, "View only (ignore mouse & keyboard)", this, false, this),
      acceptClipboard(dpy, "Accept clipboard from server", this, false, this),
      sendClipboard(dpy, "Send clipboard to server", this, false, this),
      sendPrimary(dpy, "Send primary selection & cut buffer as clipboard",
                  this, false, this),
      shared(dpy, "Shared (don't disconnect other viewers)", this, false,this),
      fullScreen(dpy, "Full-screen mode", this, false, this),
      useLocalCursor(dpy, "Render cursor locally", this, false, this),
      dotWhenNoCursor(dpy, "Show dot when no cursor", this, false, this),
      okButton(dpy, "OK", this, this, 60),
      cancelButton(dpy, "Cancel", this, this, 60)
  {
    int y = yPad;
    formatAndEnc.move(xPad, y);
    y += formatAndEnc.height();
    autoSelect.move(xPad, y);
    int x2 = xPad + autoSelect.width() + xPad*5;
    fullColour.move(x2, y);
    y += autoSelect.height();
    zrle.move(xPad, y);
    mediumColour.move(x2, y);
    y += zrle.height();
    hextile.move(xPad, y);
    lowColour.move(x2, y);
    y += hextile.height();
    raw.move(xPad, y);
    veryLowColour.move(x2, y);
    y += raw.height();

    y += yPad*4;
    inputs.move(xPad, y);
    y += inputs.height();
    viewOnly.move(xPad, y);
    y += viewOnly.height();
    acceptClipboard.move(xPad, y);
    y += acceptClipboard.height();
    sendClipboard.move(xPad, y);
    y += sendClipboard.height();
    sendPrimary.move(xPad, y);
    y += sendPrimary.height();

    y += yPad*4;
    misc.move(xPad, y);
    y += misc.height();
    shared.move(xPad, y);
    y += shared.height();
    fullScreen.move(xPad, y);
    y += fullScreen.height();
    useLocalCursor.move(xPad, y);
    y += useLocalCursor.height();
    dotWhenNoCursor.move(xPad, y);
    y += dotWhenNoCursor.height();

    okButton.move(width() - xPad*12 - cancelButton.width() - okButton.width(),
                  height() - yPad*4 - okButton.height());
    cancelButton.move(width() - xPad*6 - cancelButton.width(),
                      height() - yPad*4 - cancelButton.height());
    setBorderWidth(1);
  }

  virtual void initDialog() {
    if (cb) cb->setOptions();
    zrle.disabled(autoSelect.checked());
    hextile.disabled(autoSelect.checked());
    raw.disabled(autoSelect.checked());
    sendPrimary.disabled(!sendClipboard.checked());
    dotWhenNoCursor.disabled(!useLocalCursor.checked());
  }

  virtual void takeFocus(Time time) {
    //XSetInputFocus(dpy, entry.win, RevertToParent, time);
  }

  virtual void buttonActivate(TXButton* b) {
    if (b == &okButton) {
      if (cb) cb->getOptions();
      unmap();
    } else if (b == &cancelButton) {
      unmap();
    }
  }

  virtual void checkboxSelect(TXCheckbox* checkbox) {
    if (checkbox == &autoSelect) {
      zrle.disabled(autoSelect.checked());
      hextile.disabled(autoSelect.checked());
      raw.disabled(autoSelect.checked());
    } else if (checkbox == &fullColour || checkbox == &mediumColour ||
               checkbox == &lowColour || checkbox == &veryLowColour) {
      fullColour.checked(checkbox == &fullColour);
      mediumColour.checked(checkbox == &mediumColour);
      lowColour.checked(checkbox == &lowColour);
      veryLowColour.checked(checkbox == &veryLowColour);
    } else if (checkbox == &zrle || checkbox == &hextile || checkbox == &raw) {
      zrle.checked(checkbox == &zrle);
      hextile.checked(checkbox == &hextile);
      raw.checked(checkbox == &raw);
    } else if (checkbox == &sendClipboard) {
      sendPrimary.disabled(!sendClipboard.checked());
    } else if (checkbox == &useLocalCursor) {
      dotWhenNoCursor.disabled(!useLocalCursor.checked());
    }
  }

  OptionsDialogCallback* cb;
  TXLabel formatAndEnc, inputs, misc;
  TXCheckbox autoSelect;
  TXCheckbox fullColour, mediumColour, lowColour, veryLowColour;
  TXCheckbox zrle, hextile, raw;
  TXCheckbox viewOnly, acceptClipboard, sendClipboard, sendPrimary;
  TXCheckbox shared, fullScreen, useLocalCursor, dotWhenNoCursor;
  TXButton okButton, cancelButton;
};

#endif
