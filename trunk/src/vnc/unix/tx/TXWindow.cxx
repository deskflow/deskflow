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
// TXWindow.cxx
//

#include <X11/Xatom.h>
#include "TXWindow.h"
#include <list>
#include <stdio.h>
#include <stdlib.h>
#include <rfb/util.h>

std::list<TXWindow*> windows;

Atom wmProtocols, wmDeleteWindow, wmTakeFocus;
Atom xaTIMESTAMP, xaTARGETS, xaSELECTION_TIME, xaSELECTION_STRING;
Atom xaCLIPBOARD;
unsigned long TXWindow::black, TXWindow::white;
unsigned long TXWindow::defaultFg, TXWindow::defaultBg;
unsigned long TXWindow::lightBg, TXWindow::darkBg;
unsigned long TXWindow::disabledFg, TXWindow::disabledBg;
unsigned long TXWindow::enabledBg;
unsigned long TXWindow::scrollbarBg;
Colormap TXWindow::cmap = 0;
GC TXWindow::defaultGC = 0;
Font TXWindow::defaultFont = 0;
XFontStruct* TXWindow::defaultFS = 0;
Time TXWindow::cutBufferTime = 0;
Pixmap TXWindow::dot = 0, TXWindow::tick = 0;
const int TXWindow::dotSize = 4, TXWindow::tickSize = 8;
char* TXWindow::defaultWindowClass;

void TXWindow::init(Display* dpy, const char* defaultWindowClass_)
{
  cmap = DefaultColormap(dpy,DefaultScreen(dpy));
  wmProtocols = XInternAtom(dpy, "WM_PROTOCOLS", False);
  wmDeleteWindow = XInternAtom(dpy, "WM_DELETE_WINDOW", False);
  wmTakeFocus = XInternAtom(dpy, "WM_TAKE_FOCUS", False);
  xaTIMESTAMP = XInternAtom(dpy, "TIMESTAMP", False);
  xaTARGETS = XInternAtom(dpy, "TARGETS", False);
  xaSELECTION_TIME = XInternAtom(dpy, "SELECTION_TIME", False);
  xaSELECTION_STRING = XInternAtom(dpy, "SELECTION_STRING", False);
  xaCLIPBOARD = XInternAtom(dpy, "CLIPBOARD", False);
  XColor cols[6];
  cols[0].red = cols[0].green = cols[0].blue = 0x0000;
  cols[1].red = cols[1].green = cols[1].blue = 0xbbbb;
  cols[2].red = cols[2].green = cols[2].blue = 0xeeee;
  cols[3].red = cols[3].green = cols[3].blue = 0x5555;
  cols[4].red = cols[4].green = cols[4].blue = 0x8888;
  cols[5].red = cols[5].green = cols[5].blue = 0xffff;
  getColours(dpy, cols, 6);
  black = defaultFg = cols[0].pixel;
  defaultBg = disabledBg = cols[1].pixel;
  lightBg = cols[2].pixel;
  darkBg = disabledFg = cols[3].pixel;
  scrollbarBg = cols[4].pixel;
  white = enabledBg = cols[5].pixel;
  defaultGC = XCreateGC(dpy, DefaultRootWindow(dpy), 0, 0);
  defaultFS
    = XLoadQueryFont(dpy, "-*-helvetica-medium-r-*-*-12-*-*-*-*-*-*-*");
  if (!defaultFS) {
    defaultFS = XLoadQueryFont(dpy, "fixed");
    if (!defaultFS) {
      fprintf(stderr,"Failed to load any font\n");
      exit(1);
    }
  }
  defaultFont = defaultFS->fid;
  XSetForeground(dpy, defaultGC, defaultFg);
  XSetBackground(dpy, defaultGC, defaultBg);
  XSetFont(dpy, defaultGC, defaultFont);
  XSelectInput(dpy, DefaultRootWindow(dpy), PropertyChangeMask);

  static char dotBits[] = { 0x06, 0x0f, 0x0f, 0x06};
  dot = XCreateBitmapFromData(dpy, DefaultRootWindow(dpy), dotBits,
                              dotSize, dotSize);
  static char tickBits[] = { 0x80, 0xc0, 0xe2, 0x76, 0x3e, 0x1c, 0x08, 0x00};
  tick = XCreateBitmapFromData(dpy, DefaultRootWindow(dpy), tickBits,
                               tickSize, tickSize);
  defaultWindowClass = rfb::strDup(defaultWindowClass_);
}

void TXWindow::handleXEvents(Display* dpy)
{
  while (XPending(dpy)) {
    XEvent ev;
    XNextEvent(dpy, &ev);
    if (ev.type == MappingNotify) {
      XRefreshKeyboardMapping(&ev.xmapping);
    } else if (ev.type == PropertyNotify &&
               ev.xproperty.window == DefaultRootWindow(dpy) &&
               ev.xproperty.atom == XA_CUT_BUFFER0) {
      cutBufferTime = ev.xproperty.time;
    } else {
      std::list<TXWindow*>::iterator i;
      for (i = windows.begin(); i != windows.end(); i++) {
        if ((*i)->win() == ev.xany.window)
          (*i)->handleXEvent(&ev);
      }
    }
  }
}

void TXWindow::getColours(Display* dpy, XColor* cols, int nCols)
{
  bool* got = new bool[nCols];
  bool failed = false;
  int i;
  for (i = 0; i < nCols; i++) {
    if (XAllocColor(dpy, cmap, &cols[i])) {
      got[i] = true;
    } else {
      got[i] = false;
      failed = true;
    }
  }

  if (!failed) {
    delete [] got;
    return;
  }

  // AllocColor has failed.  This is because the colormap is full.  So the
  // only thing we can do is use the "shared" pixels in the colormap.  The
  // code below is designed to work even when the colormap isn't full so is
  // more complex than it needs to be in this case.  However it would be
  // useful one day to be able to restrict the number of colours allocated by
  // an application so I'm leaving it in here.

  // For each pixel in the colormap, try to allocate exactly its RGB values.
  // If this returns a different pixel then it must be a private or
  // unallocated pixel, so we can't use it.  If it returns us the same pixel
  // again, it's almost certainly a shared colour, so we can use it (actually
  // it is possible that it was an unallocated pixel which we've now
  // allocated - by going through the pixels in reverse order we make this
  // unlikely except for the lowest unallocated pixel - this works because of
  // the way the X server allocates new pixels).

  int cmapSize = DisplayCells(dpy,DefaultScreen(dpy));

  XColor* cm = new XColor[cmapSize];
  bool* shared = new bool[cmapSize];
  bool* usedAsNearest = new bool[cmapSize];

  for (i = 0; i < cmapSize; i++) {
    cm[i].pixel = i;
    shared[i] = usedAsNearest[i] = false;
  }

  XQueryColors(dpy, cmap, cm, cmapSize);

  for (i = cmapSize-1; i >= 0; i--) {
    if (XAllocColor(dpy, cmap, &cm[i])) {
      if (cm[i].pixel == (unsigned long)i) {
        shared[i] = true;
      } else {
        XFreeColors(dpy, cmap, &cm[i].pixel, 1, 0);
      }
    }
  }

  for (int j = 0; j < nCols; j++) {
    unsigned long minDistance = ULONG_MAX;
    unsigned long nearestPixel = 0;
    if (!got[j]) {
      for (i = 0; i < cmapSize; i++) {
        if (shared[i]) {
          unsigned long rd = (cm[i].red - cols[j].red)/2;
          unsigned long gd = (cm[i].green - cols[j].green)/2;
          unsigned long bd = (cm[i].blue - cols[j].blue)/2;
          unsigned long distance = (rd*rd + gd*gd + bd*bd);

          if (distance < minDistance) {
            minDistance = distance;
            nearestPixel = i;
          }
        }
      }

      cols[j].pixel = nearestPixel;
      usedAsNearest[nearestPixel] = true;
    }
  }

  for (i = 0; i < cmapSize; i++) {
    if (shared[i] && !usedAsNearest[i]) {
      unsigned long p = i;
      XFreeColors(dpy, cmap, &p, 1, 0);
    }
  }
}

Window TXWindow::windowWithName(Display* dpy, Window top, const char* name)
{
  char* windowName;
  if (XFetchName(dpy, top, &windowName)) {
    if (strcmp(windowName, name) == 0) {
      XFree(windowName);
      return top;
    }
    XFree(windowName);
  }

  Window* children;
  Window dummy;
  unsigned int nchildren;
  if (!XQueryTree(dpy, top, &dummy, &dummy, &children,&nchildren) || !children)
    return 0;

  for (int i = 0; i < (int)nchildren; i++) {
    Window w = windowWithName(dpy, children[i], name);
    if (w) {
      XFree((char*)children);
      return w;
    }
  }
  XFree((char*)children);
  return 0;
}


TXWindow::TXWindow(Display* dpy_, int w, int h, TXWindow* parent_,
                   int borderWidth)
  : dpy(dpy_), xPad(3), yPad(3), bevel(2), parent(parent_), width_(w),
    height_(h), eventHandler(0), dwc(0), eventMask(0), toplevel_(false)
{
  sizeHints.flags = 0;
  XSetWindowAttributes attr;
  attr.background_pixel = defaultBg;
  attr.border_pixel = 0;
  Window par = parent ? parent->win() : DefaultRootWindow(dpy);
  win_ = XCreateWindow(dpy, par, 0, 0, width_, height_, borderWidth,
                      CopyFromParent, CopyFromParent, CopyFromParent,
                      CWBackPixel | CWBorderPixel, &attr);
  if (parent) map();

  windows.push_back(this);
}

TXWindow::~TXWindow()
{
  windows.remove(this);
  XDestroyWindow(dpy, win());
}

void TXWindow::toplevel(const char* name, TXDeleteWindowCallback* dwc_,
                        int argc, char** argv, const char* windowClass,
                        bool iconic)
{
  toplevel_ = true;
  XWMHints wmHints;
  wmHints.flags = InputHint|StateHint;
  wmHints.input = True;
  wmHints.initial_state = iconic ? IconicState : NormalState;
  XClassHint classHint;
  if (!windowClass) windowClass = defaultWindowClass;
  classHint.res_name = (char*)name;
  classHint.res_class = (char*)windowClass;
  XSetWMProperties(dpy, win(), 0, 0, argv, argc,
                   &sizeHints, &wmHints, &classHint);
  XStoreName(dpy, win(), name);
  XSetIconName(dpy, win(), name);
  Atom protocols[10];
  int nProtocols = 0;
  protocols[nProtocols++] = wmTakeFocus;
  dwc = dwc_;
  if (dwc)
    protocols[nProtocols++] = wmDeleteWindow;
  XSetWMProtocols(dpy, win(), protocols, nProtocols);
  addEventMask(StructureNotifyMask);
}

void TXWindow::setMaxSize(int w, int h)
{
  sizeHints.flags |= PMaxSize;
  sizeHints.max_width = w;
  sizeHints.max_height = h;
  XSetWMNormalHints(dpy, win(), &sizeHints);
}

void TXWindow::setUSPosition(int x, int y)
{
  sizeHints.flags |= USPosition;
  sizeHints.x = x;
  sizeHints.y = y;
  XSetWMNormalHints(dpy, win(), &sizeHints);
  move(x, y);
}

void TXWindow::setGeometry(const char* geom, int x, int y, int w, int h)
{
  char defGeom[256];
  sprintf(defGeom,"%dx%d+%d+%d",w,h,x,y);
  XWMGeometry(dpy, DefaultScreen(dpy), strEmptyToNull((char*)geom), defGeom,
              0, &sizeHints, &x, &y, &w, &h, &sizeHints.win_gravity);
  sizeHints.flags |= PWinGravity;
  setUSPosition(x, y);
  resize(w, h);
}

TXEventHandler* TXWindow::setEventHandler(TXEventHandler* h)
{
  TXEventHandler* old = eventHandler;
  eventHandler = h;
  return old;
}

void TXWindow::addEventMask(long mask)
{
  eventMask |= mask;
  XSelectInput(dpy, win(), eventMask);
}

void TXWindow::removeEventMask(long mask)
{
  eventMask &= ~mask;
  XSelectInput(dpy, win(), eventMask);
}

void TXWindow::unmap()
{
  XUnmapWindow(dpy, win());
  if (toplevel_) {
    XUnmapEvent ue;
    ue.type = UnmapNotify;
    ue.display = dpy;
    ue.event = DefaultRootWindow(dpy);
    ue.window = win();
    ue.from_configure = False;
    XSendEvent(dpy, DefaultRootWindow(dpy), False,
               (SubstructureRedirectMask|SubstructureNotifyMask),
               (XEvent*)&ue);
  }
}

void TXWindow::resize(int w, int h)
{
  //if (w == width_ && h == height_) return;
  XResizeWindow(dpy, win(), w, h);
  width_ = w;
  height_ = h;
  resizeNotify();
}

void TXWindow::setBorderWidth(int bw)
{
  XWindowChanges c;
  c.border_width = bw;
  XConfigureWindow(dpy, win(), CWBorderWidth, &c);
}

void TXWindow::ownSelection(Atom selection, Time time)
{
  XSetSelectionOwner(dpy, selection, win(), time);
  if (XGetSelectionOwner(dpy, selection) == win()) {
    selectionOwner_[selection] = true;
    selectionOwnTime[selection] = time;
  }
}

void TXWindow::handleXEvent(XEvent* ev)
{
  switch (ev->type) {

  case ClientMessage:
    if (ev->xclient.message_type == wmProtocols) {
      if ((Atom)ev->xclient.data.l[0] == wmDeleteWindow) {
        if (dwc) dwc->deleteWindow(this);
      } else if ((Atom)ev->xclient.data.l[0] == wmTakeFocus) {
        takeFocus(ev->xclient.data.l[1]);
      }
    }
    break;

  case ConfigureNotify:
    if (ev->xconfigure.width != width_ || ev->xconfigure.height != height_) {
      width_ = ev->xconfigure.width;
      height_ = ev->xconfigure.height;
      resizeNotify();
    }
    break;

  case SelectionNotify:
    if (ev->xselection.property != None) {
      Atom type;
      int format;
      unsigned long nitems, after;
      unsigned char *data;
      XGetWindowProperty(dpy, win(), ev->xselection.property, 0, 16384, True,
                         AnyPropertyType, &type, &format,
                         &nitems, &after, &data);
      if (type != None) {
        selectionNotify(&ev->xselection, type, format, nitems, data);
        XFree(data);
        break;
      }
    }
    selectionNotify(&ev->xselection, 0, 0, 0, 0);
    break;

  case SelectionRequest:
    {
      XSelectionEvent se;
      se.type = SelectionNotify;
      se.display = ev->xselectionrequest.display;
      se.requestor = ev->xselectionrequest.requestor;
      se.selection = ev->xselectionrequest.selection;
      se.time = ev->xselectionrequest.time;
      se.target = ev->xselectionrequest.target;
      if (ev->xselectionrequest.property == None)
        ev->xselectionrequest.property = ev->xselectionrequest.target;
      if (!selectionOwner_[se.selection]) {
        se.property = None;
      } else {
        se.property = ev->xselectionrequest.property;
        if (se.target == xaTARGETS) {
          Atom targets[2];
          targets[0] = xaTIMESTAMP;
          targets[1] = XA_STRING;
          XChangeProperty(dpy, se.requestor, se.property, XA_ATOM, 32,
                          PropModeReplace, (unsigned char*)targets, 2);
        } else if (se.target == xaTIMESTAMP) {
          rdr::U32 t = selectionOwnTime[se.selection];
          XChangeProperty(dpy, se.requestor, se.property, XA_INTEGER, 32,
                          PropModeReplace, (unsigned char*)&t, 1);
        } else if (se.target == XA_STRING) {
          if (!selectionRequest(se.requestor, se.selection, se.property))
            se.property = None;
        }
      }
      XSendEvent(dpy, se.requestor, False, 0, (XEvent*)&se);
      break;
    }

  case SelectionClear:
    selectionOwner_[ev->xselectionclear.selection] = false;
    break;
  }

  if (eventHandler) eventHandler->handleEvent(this, ev);
}

void TXWindow::drawBevel(GC gc, int x, int y, int w, int h, int b,
                         unsigned long middle, unsigned long tl,
                         unsigned long br, bool round)
{
  if (round) {
    XGCValues gcv;
    gcv.line_width = b;
    XChangeGC(dpy, gc, GCLineWidth, &gcv);
    XSetForeground(dpy, gc, middle);
    XFillArc(dpy, win(), gc,  x, y, w-b/2, h-b/2, 0, 360*64);
    XSetForeground(dpy, gc, tl);
    XDrawArc(dpy, win(), gc,  x, y, w-b/2, h-b/2, 45*64, 180*64);
    XSetForeground(dpy, gc, br);
    XDrawArc(dpy, win(), gc,  x, y, w-b/2, h-b/2, 225*64, 180*64);
  } else {
    XSetForeground(dpy, gc, middle);
    if (w-2*b > 0 && h-2*b > 0)
      XFillRectangle(dpy, win(), gc, x+b, y+b, w-2*b, h-2*b);
    XSetForeground(dpy, gc, tl);
    XFillRectangle(dpy, win(), gc, x, y, w, b);
    XFillRectangle(dpy, win(), gc, x, y, b, h);
    XSetForeground(dpy, gc, br);
    for (int i = 0; i < b; i++) {
      if (w-i > 0) XFillRectangle(dpy, win(), gc, x+i, y+h-1-i, w-i, 1); 
      if (h-1-i > 0) XFillRectangle(dpy, win(), gc, x+w-1-i, y+i+1, 1, h-1-i);
    }
  }
}
