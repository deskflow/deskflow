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
// TXMenu.h
//
// A TXMenu consists of multiple entries which can be added one at a time.
// Each entry consists of some text, and has an associated integer identifier.
// A callback is made when a menu entry is selected.
//

#ifndef __TXMENU_H__
#define __TXMENU_H__

#include "TXWindow.h"

// TXMenuCallback's menuSelect() method is called when a particular menu entry
// is selected.  The id argument identifies the menu entry.
class TXMenu;
class TXMenuCallback {
public:
  virtual void menuSelect(long id, TXMenu* menu)=0;
};

class TXMenu : public TXWindow, public TXEventHandler {
public:
  TXMenu(Display* dpy_, TXMenuCallback* cb=0, int width=1, int height=1,
         TXWindow* parent_=0);
  virtual ~TXMenu();

  // addEntry() adds an entry to the end of the menu with the given text and
  // identifier.
  void addEntry(const char* text, long id);

  // check() sets whether the given menu entry should have a tick next to it.
  void check(long id, bool checked);

private:
  int entryHeight(int i);
  virtual void handleEvent(TXWindow* w, XEvent* ev);
  void paint();

  GC gc;
  TXMenuCallback* cb;
  enum { maxEntries = 64 };
  char* text[maxEntries];
  long id[maxEntries];
  bool checked[maxEntries];
  int nEntries;
  int highlight;
};

#endif
