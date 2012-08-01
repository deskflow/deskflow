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
// InfoDialog.h
//

#ifndef __INFODIALOG_H__
#define __INFODIALOG_H__

#include "TXDialog.h"
#include "TXLabel.h"
#include "TXButton.h"

extern char buildtime[];

class InfoDialog : public TXDialog, public TXButtonCallback {
public:
  InfoDialog(Display* dpy)
    : TXDialog(dpy, 1, 1, "VNC connection info"),
      infoLabel(dpy, "", this, 1, 1, TXLabel::left),
      okButton(dpy, "OK", this, this, 60)
  {
    infoLabel.xPad = 8;
    infoLabel.move(0, yPad*4);
    setBorderWidth(1);
  }

  void setText(char* infoText) {
    infoLabel.setText(infoText);
    resize(infoLabel.width(),
           infoLabel.height() + okButton.height() + yPad*12);

    okButton.move((width() - okButton.width()) / 2,
                  height() - yPad*4 - okButton.height());
  }

  virtual void buttonActivate(TXButton* b) {
    unmap();
  }

  TXLabel infoLabel;
  TXButton okButton;
};

#endif
