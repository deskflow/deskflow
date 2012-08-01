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
// TXMenu.cxx
//

#include "TXMenu.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <rfb/util.h>
#include <X11/keysym.h>

TXMenu::TXMenu(Display* dpy_, TXMenuCallback* cb_, int w, int h,
               TXWindow* parent_)
  : TXWindow(dpy_, w, h, parent_), cb(cb_), nEntries(0),
    highlight(-1)
{
  setEventHandler(this);
  gc = XCreateGC(dpy, win(), 0, 0);
  addEventMask(ExposureMask | ButtonPressMask | ButtonReleaseMask |
               PointerMotionMask | EnterWindowMask | LeaveWindowMask);
}

TXMenu::~TXMenu()
{
  XFreeGC(dpy, gc);
  for (int i = 0; i < nEntries; i++)
    delete [] text[i];
}

inline int TXMenu::entryHeight(int i)
{
  if (text[i])
    return defaultFS->ascent + defaultFS->descent + bevel*2 + yPad*2;
  else
    return yPad*2 + 1;
}

void TXMenu::addEntry(const char* text_, long id_)
{
  assert(nEntries < maxEntries);
  text[nEntries] = rfb::strDup(text_);
  checked[nEntries] = false;
  id[nEntries++] = id_;
  int tw = 0;
  if (text_)
    tw = XTextWidth(defaultFS, text_, strlen(text_));
  int newWidth = width();
  if (tw + bevel*2 + xPad*5 + tickSize > width())
    newWidth = tw + bevel*2 + xPad*5 + tickSize;
  int newHeight = 0;
  for (int i = 0; i < nEntries; i++)
    newHeight += entryHeight(i);
  resize(newWidth, newHeight);
}

void TXMenu::check(long id_, bool checked_)
{
  for (int i = 0; i < nEntries; i++) {
    if (id[i] == id_) {
      checked[i] = checked_;
      break;
    }
  }
}

void TXMenu::paint()
{
  int y = 0;
  for (int i = 0; i < nEntries; i++) {
    if (text[i]) {
      if (i == highlight)
        drawBevel(gc, 0, y, width(), entryHeight(i), bevel,
                  defaultBg, darkBg, lightBg);
      else
        XClearArea(dpy, win(), 0, y, width(), entryHeight(i), false);
      if (checked[i])
        XCopyPlane(dpy, tick, win(), defaultGC, 0, 0, tickSize, tickSize,
                   bevel + xPad,
                   y + bevel + yPad + defaultFS->ascent - tickSize, 1);

      XDrawImageString(dpy, win(), defaultGC, bevel + xPad*2 + tickSize,
                       y + bevel + yPad + defaultFS->ascent,
                       text[i], strlen(text[i]));
    } else {
      XDrawLine(dpy, win(), defaultGC, bevel + xPad, y + entryHeight(i) / 2,
                width() - bevel - xPad, y + entryHeight(i) / 2);
    }
    y += entryHeight(i);
  }
}

void TXMenu::handleEvent(TXWindow* w, XEvent* ev)
{
  switch (ev->type) {
  case Expose:
    paint();
    break;

  case ButtonRelease:
    {
      int y = ev->xmotion.y;
      int entryY = 0;
      for (int i = 0; i < nEntries; i++) {
        if (y >= entryY && y <= entryY + entryHeight(i)) {
          if (cb && text[i])
            cb->menuSelect(id[i], this);
          break;
        }
        entryY += entryHeight(i);
      }
      highlight = -1;
      paint();
      break;
    }

  case ButtonPress:
  case MotionNotify:
    {
      int y = ev->xmotion.y;
      int entryY = 0;
      for (int i = 0; i < nEntries; i++) {
        if (y >= entryY && y <= entryY + entryHeight(i)) {
          if (highlight != i) {
            highlight = i;
            paint();
          }
          break;
        }
        entryY += entryHeight(i);
      }
      break;
    }

  case KeyPress:
    {
      KeySym ks;
      char str[256];
      XLookupString(&ev->xkey, str, 256, &ks, NULL);
      if (ks == XK_Escape) {
        highlight = -1;
        unmap();
      } else if (ks == XK_Down || ks == XK_Up) {
        if (nEntries < 1) break;
        if (highlight < 0)
          highlight = (ks == XK_Down ? nEntries-1 : 0);
        int start = highlight;
        int inc = (ks == XK_Down ? 1 : nEntries-1);
        do {
          highlight = (highlight + inc) % nEntries;
        } while (highlight != start && !text[highlight]);
        paint();
      } else if (ks == XK_space || ks == XK_KP_Space ||
                 ks == XK_Return || ks == XK_KP_Enter) {
        if (cb && highlight >= 0 && text[highlight])
          cb->menuSelect(id[highlight], this);
        highlight = -1;
        paint();
      }
      break;
    }

  case EnterNotify:
  case LeaveNotify:
    highlight = -1;
    paint();
    break;
  }
}
