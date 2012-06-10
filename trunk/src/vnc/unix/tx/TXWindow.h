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
// TXWindow.h
//
// A TXWindow is the base class for all tx windows (widgets).  In addition it
// contains a number of static methods and members which are used throughout
// tx.
//
// Before calling any other tx methods, TXWindow::init() must be called with
// the X display to use.

#ifndef __TXWINDOW_H__
#define __TXWINDOW_H__

#include <rdr/types.h>
#include <X11/X.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <map>


// TXDeleteWindowCallback's deleteWindow() method is called when a top-level
// window is "deleted" (closed) by the user using the window manager.
class TXWindow;
class TXDeleteWindowCallback {
public:
  virtual void deleteWindow(TXWindow* w) = 0;
};

// TXEventHandler is an interface implemented by classes wanting to handle X
// events on a window.  Most derived classes of window are their own event
// handlers.
class TXEventHandler {
public:
  virtual void handleEvent(TXWindow* w, XEvent* ev) = 0;
};

class TXWindow {
public:

  // Constructor - creates a window of the given size, with the default
  // background (currently grey).  It is mapped by default if it has a parent.
  // If no parent is specified its parent is the root window and it will remain
  // unmapped.
  TXWindow(Display* dpy_, int width=1, int height=1, TXWindow* parent_=0,
           int borderWidth=0);
  virtual ~TXWindow();

  // toplevel() declares that this is a top-level window.  Various
  // window-manager-related properties are set on the window.  The given
  // TXDeleteWindowCallback is notified when the window is "deleted" (cloesd)
  // by the user.
  void toplevel(const char* name, TXDeleteWindowCallback* dwc=0,
                int argc=0, char** argv=0, const char* windowClass=0,
                bool iconic=false);

  // setMaxSize() tells the window manager the maximum size to allow a
  // top-level window.  It has no effect on a non-top-level window.
  void setMaxSize(int w, int h);

  // setUSPosition() tells the window manager the position which the "user" has
  // asked for a top-level window.  Most window managers ignore requests by a
  // program for position, so you have to tell it that the "user" asked for the
  // position.  This has no effect on a non-top-level window.
  void setUSPosition(int x, int y);

  void setGeometry(const char* geom, int x, int y, int w, int h);

  // setTransientFor() tells the window manager that this window is "owned" by
  // the given window.  The window manager can use this information as it sees
  // fit.
  void setTransientFor(Window w) { XSetTransientForHint(dpy, win(), w); }

  // setEventHandler() sets the TXEventHandler to handle X events for this
  // window.  It returns the previous event handler, so that handlers can chain
  // themselves.
  TXEventHandler* setEventHandler(TXEventHandler* h);

  // Accessor methods
  Window win() { return win_; }
  int width() { return width_; }
  int height() { return height_; }

  // selectionOwner() returns true if this window owns the given selection.
  bool selectionOwner(Atom selection) { return selectionOwner_[selection]; }

  // Wrappers around common Xlib calls
  void addEventMask(long mask);
  void removeEventMask(long mask);
  void map()                   { XMapWindow(dpy, win()); }
  void unmap();
  void setBg(unsigned long bg) { XSetWindowBackground(dpy, win(), bg); }
  void move(int x, int y)      { XMoveWindow(dpy, win(), x, y); }
  void resize(int w, int h);
  void raise()                 { XRaiseWindow(dpy, win()); }
  void setBorderWidth(int bw);
  void invalidate(int x=0, int y=0, int w=0, int h=0) { XClearArea(dpy, win(), x, y, w, h, True); }

  // ownSelection requests that the window owns the given selection from the
  // given time (the time should be taken from an X event).
  void ownSelection(Atom selection, Time time);


  // drawBevel draws a rectangular or circular bevel filling the given
  // rectangle, using the given colours for the middle, the top/left and the
  // bottom/right.
  void drawBevel(GC gc, int x, int y, int w, int h, int b,
                 unsigned long middle, unsigned long tl, unsigned long br,
                 bool round=false);

  // Methods to be overridden in a derived class

  // resizeNotify() is called whenever the window's dimensions may have
  // changed.
  virtual void resizeNotify() {}

  // takeFocus() is called when the window has received keyboard focus from the
  // window manager.
  virtual void takeFocus(Time time) {}

  // selectionNotify() is called when the selection owner has replied to a
  // request for information about a selection from the selection owner.
  virtual void selectionNotify(XSelectionEvent* ev, Atom type, int format,
                               int nitems, void* data) {}

  // selectionRequest() is called when this window is the selection owner and
  // another X client has requested the selection.  It should set the given
  // property on the given window to the value of the given selection,
  // returning true if successful, false otherwise.
  virtual bool selectionRequest(Window requestor,
                                Atom selection, Atom property) { return false;}

  // Static methods

  // init() must be called before any other tx methods.
  static void init(Display* dpy, const char* defaultWindowClass);

  // getColours() sets the pixel values in the cols array to the best available
  // for the given rgb values, even in the case of a full colormap.
  static void getColours(Display* dpy, XColor* cols, int nCols);

  // handleXEvents() should be called whenever there are events to handle on
  // the connection to the X display.  It process all available events, then
  // returns when there are no more events to process.
  static void handleXEvents(Display* dpy);

  // windowWithName() locates a window with a given name on a display.
  static Window windowWithName(Display* dpy, Window top, const char* name);

  // strEmptyToNull() returns the string it's given but turns an empty string
  // into null, which can be useful for passing rfb parameters to Xlib calls.
  static char* strEmptyToNull(char* s) { return s && s[0] ? s : 0; }

  // The following are default values for various things.
  static unsigned long black, white;
  static unsigned long defaultFg, defaultBg, lightBg, darkBg;
  static unsigned long disabledFg, disabledBg, enabledBg;
  static unsigned long scrollbarBg;
  static GC defaultGC;
  static Colormap cmap;
  static Font defaultFont;
  static XFontStruct* defaultFS;
  static Time cutBufferTime;
  static Pixmap dot, tick;
  static const int dotSize, tickSize;
  static char* defaultWindowClass;

  Display* const dpy;

  int xPad, yPad, bevel;

private:

  // handleXEvent() is called from handleXEvents() when an event for this
  // window arrives.  It does general event processing before calling on to the
  // event handler.
  void handleXEvent(XEvent* ev);

  TXWindow* parent;
  Window win_;
  int width_, height_;
  TXEventHandler* eventHandler;
  TXDeleteWindowCallback* dwc;
  long eventMask;
  XSizeHints sizeHints;
  std::map<Atom,Time> selectionOwnTime;
  std::map<Atom,bool> selectionOwner_;
  bool toplevel_;
};

extern Atom wmProtocols, wmDeleteWindow, wmTakeFocus;
extern Atom xaTIMESTAMP, xaTARGETS, xaSELECTION_TIME, xaSELECTION_STRING;
extern Atom xaCLIPBOARD;

#endif
