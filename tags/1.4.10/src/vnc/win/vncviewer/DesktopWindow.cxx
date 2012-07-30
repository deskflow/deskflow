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

#include <windows.h>
#include <commctrl.h>
#include <rfb/Configuration.h>
#include <rfb/LogWriter.h>
#include <rfb_win32/WMShatter.h>
#include <rfb_win32/LowLevelKeyEvents.h>
#include <rfb_win32/MonitorInfo.h>
#include <rfb_win32/DeviceContext.h>
#include <rfb_win32/Win32Util.h>
#include <vncviewer/DesktopWindow.h>
#include <vncviewer/resource.h>

using namespace rfb;
using namespace rfb::win32;


// - Statics & consts

static LogWriter vlog("DesktopWindow");

const int TIMER_BUMPSCROLL = 1;
const int TIMER_POINTER_INTERVAL = 2;
const int TIMER_POINTER_3BUTTON = 3;


//
// -=- DesktopWindowClass

//
// Window class used as the basis for all DesktopWindow instances
//

class DesktopWindowClass {
public:
  DesktopWindowClass();
  ~DesktopWindowClass();
  ATOM classAtom;
  HINSTANCE instance;
};

LRESULT CALLBACK DesktopWindowProc(HWND wnd, UINT msg, WPARAM wParam, LPARAM lParam) {
  LRESULT result;
  if (msg == WM_CREATE)
    SetWindowLong(wnd, GWL_USERDATA, (long)((CREATESTRUCT*)lParam)->lpCreateParams);
  else if (msg == WM_DESTROY)
    SetWindowLong(wnd, GWL_USERDATA, 0);
  DesktopWindow* _this = (DesktopWindow*) GetWindowLong(wnd, GWL_USERDATA);
  if (!_this) {
    vlog.info("null _this in %x, message %u", wnd, msg);
    return rfb::win32::SafeDefWindowProc(wnd, msg, wParam, lParam);
  }

  try {
    result = _this->processMessage(msg, wParam, lParam);
  } catch (rdr::Exception& e) {
    vlog.error("untrapped: %s", e.str());
  }

  return result;
};

static HCURSOR dotCursor = (HCURSOR)LoadImage(GetModuleHandle(0), MAKEINTRESOURCE(IDC_DOT_CURSOR), IMAGE_CURSOR, 0, 0, LR_SHARED);
static HCURSOR arrowCursor = (HCURSOR)LoadImage(NULL, IDC_ARROW, IMAGE_CURSOR, 0, 0, LR_SHARED); 

DesktopWindowClass::DesktopWindowClass() : classAtom(0) {
  WNDCLASS wndClass;
  wndClass.style = 0;
  wndClass.lpfnWndProc = DesktopWindowProc;
  wndClass.cbClsExtra = 0;
  wndClass.cbWndExtra = 0;
  wndClass.hInstance = instance = GetModuleHandle(0);
  wndClass.hIcon = (HICON)LoadImage(GetModuleHandle(0), MAKEINTRESOURCE(IDI_ICON), IMAGE_ICON, 0, 0, LR_SHARED);
  if (!wndClass.hIcon)
    printf("unable to load icon:%ld", GetLastError());
  wndClass.hCursor = NULL;
  wndClass.hbrBackground = NULL;
  wndClass.lpszMenuName = 0;
  wndClass.lpszClassName = _T("rfb::win32::DesktopWindowClass");
  classAtom = RegisterClass(&wndClass);
  if (!classAtom) {
    throw rdr::SystemException("unable to register DesktopWindow window class", GetLastError());
  }
}

DesktopWindowClass::~DesktopWindowClass() {
  if (classAtom) {
    UnregisterClass((const TCHAR*)classAtom, instance);
  }
}

DesktopWindowClass baseClass;


//
// -=- DesktopWindow instance implementation
//

DesktopWindow::DesktopWindow(Callback* cb) 
  : buffer(0),
    client_size(0, 0, 16, 16), window_size(0, 0, 32, 32),
    cursorVisible(false), cursorAvailable(false), cursorInBuffer(false),
    systemCursorVisible(true), trackingMouseLeave(false),
    handle(0), has_focus(false), palette_changed(false),
    fullscreenActive(false), fullscreenRestore(false),
    bumpScroll(false), callback(cb) {

  // Create the window
  const char* name = "DesktopWindow";
  handle = CreateWindow((const TCHAR*)baseClass.classAtom, TStr(name),
	WS_VISIBLE | WS_POPUP |	WS_CLIPSIBLINGS | WS_CLIPCHILDREN,
    0, 0, 10, 10, 0, 0, baseClass.instance, this);
  if (!handle)
    throw rdr::SystemException("unable to create WMNotifier window instance", GetLastError());
  vlog.debug("created window \"%s\" (%x)", name, handle);

  // Initialise the CPointer pointer handler
  ptr.setHWND(handle);
  ptr.setIntervalTimerId(TIMER_POINTER_INTERVAL);
  ptr.set3ButtonTimerId(TIMER_POINTER_3BUTTON);

  // Initialise the bumpscroll timer
  bumpScrollTimer.setHWND(handle);
  bumpScrollTimer.setId(TIMER_BUMPSCROLL);

  // Hook the clipboard
  clipboard.setNotifier(this);

  // Create the backing buffer
  buffer = new win32::DIBSectionBuffer(handle);

  // Show the window
  centerWindow(handle, 0);
  ShowWindow(handle, SW_HIDE);
}

DesktopWindow::~DesktopWindow() {
  vlog.debug("~DesktopWindow");
  showSystemCursor();
  if (handle) {
    disableLowLevelKeyEvents(handle);
    DestroyWindow(handle);
    handle = 0;
  }
  delete buffer;
  vlog.debug("~DesktopWindow done");
}


void DesktopWindow::setFullscreen(bool fs) {
  if (fs && !fullscreenActive) {
    fullscreenActive = bumpScroll = true;

    // Un-minimize the window if required
    if (GetWindowLong(handle, GWL_STYLE) & WS_MINIMIZE)
      ShowWindow(handle, SW_RESTORE);

    // Save the current window position
    GetWindowRect(handle, &fullscreenOldRect);

    // Find the size of the display the window is on
    MonitorInfo mi(handle);

    // Set the window full-screen
    DWORD flags = GetWindowLong(handle, GWL_STYLE);
    fullscreenOldFlags = flags;
    flags = flags & ~(WS_CAPTION | WS_THICKFRAME | WS_MAXIMIZE | WS_MINIMIZE);
    vlog.debug("flags=%x", flags);

    SetWindowLong(handle, GWL_STYLE, flags);
    SetWindowPos(handle, HWND_TOP, mi.rcMonitor.left, mi.rcMonitor.top,
      mi.rcMonitor.right-mi.rcMonitor.left,
      mi.rcMonitor.bottom-mi.rcMonitor.top,
      SWP_FRAMECHANGED);
  } else if (!fs && fullscreenActive) {
    fullscreenActive = bumpScroll = false;

    // Set the window non-fullscreen
    SetWindowLong(handle, GWL_STYLE, fullscreenOldFlags);

    // Set the window position
    SetWindowPos(handle, HWND_NOTOPMOST,
      fullscreenOldRect.left, fullscreenOldRect.top,
      fullscreenOldRect.right - fullscreenOldRect.left, 
      fullscreenOldRect.bottom - fullscreenOldRect.top,
      SWP_FRAMECHANGED);
  }

  // Adjust the viewport offset to cope with change in size between FS
  // and previous window state.
  setViewportOffset(scrolloffset);
}

void DesktopWindow::setDisableWinKeys(bool dwk) {
  // Enable low-level event hooking, so we get special keys directly
  if (dwk)
    enableLowLevelKeyEvents(handle);
  else
    disableLowLevelKeyEvents(handle);
}


void DesktopWindow::setMonitor(const char* monitor) {
  MonitorInfo mi(monitor);
  mi.moveTo(handle);
}

char* DesktopWindow::getMonitor() const {
  MonitorInfo mi(handle);
  return strDup(mi.szDevice);
}


bool DesktopWindow::setViewportOffset(const Point& tl) {
  Point np = Point(max(0, min(tl.x, buffer->width()-client_size.width())),
    max(0, min(tl.y, buffer->height()-client_size.height())));
  Point delta = np.translate(scrolloffset.negate());
  if (!np.equals(scrolloffset)) {
    scrolloffset = np;
    ScrollWindowEx(handle, -delta.x, -delta.y, 0, 0, 0, 0, SW_INVALIDATE);
    UpdateWindow(handle);
    return true;
  }
  return false;
}


bool DesktopWindow::processBumpScroll(const Point& pos)
{
  if (!bumpScroll) return false;
  int bumpScrollPixels = 20;
  bumpScrollDelta = Point();

  if (pos.x == client_size.width()-1)
    bumpScrollDelta.x = bumpScrollPixels;
  else if (pos.x == 0)
    bumpScrollDelta.x = -bumpScrollPixels;
  if (pos.y == client_size.height()-1)
    bumpScrollDelta.y = bumpScrollPixels;
  else if (pos.y == 0)
    bumpScrollDelta.y = -bumpScrollPixels;

  if (bumpScrollDelta.x || bumpScrollDelta.y) {
    if (bumpScrollTimer.isActive()) return true;
    if (setViewportOffset(scrolloffset.translate(bumpScrollDelta))) {
      bumpScrollTimer.start(25);
      return true;
    }
  }

  bumpScrollTimer.stop();
  return false;
}


LRESULT
DesktopWindow::processMessage(UINT msg, WPARAM wParam, LPARAM lParam) {
  switch (msg) {

    // -=- Process standard window messages

  case WM_DISPLAYCHANGE:
    // Display format has changed - notify callback
    callback->displayChanged();
    break;

  case WM_PAINT:
    {
      PAINTSTRUCT ps;
      HDC paintDC = BeginPaint(handle, &ps);
      if (!paintDC)
        throw rdr::SystemException("unable to BeginPaint", GetLastError());
      Rect pr = Rect(ps.rcPaint.left, ps.rcPaint.top, ps.rcPaint.right, ps.rcPaint.bottom);

      if (!pr.is_empty()) {

        // Draw using the correct palette
        PaletteSelector pSel(paintDC, windowPalette.getHandle());

        if (buffer->bitmap) {
          // Update the bitmap's palette
          if (palette_changed) {
            palette_changed = false;
            buffer->refreshPalette();
          }

          // Get device context
          BitmapDC bitmapDC(paintDC, buffer->bitmap);

          // Blit the border if required
          Rect bufpos = desktopToClient(buffer->getRect());
          if (!pr.enclosed_by(bufpos)) {
            vlog.debug("draw border");
            HBRUSH black = (HBRUSH) GetStockObject(BLACK_BRUSH);
            RECT r;
            SetRect(&r, 0, 0, bufpos.tl.x, client_size.height()); FillRect(paintDC, &r, black);
            SetRect(&r, bufpos.tl.x, 0, bufpos.br.x, bufpos.tl.y); FillRect(paintDC, &r, black);
            SetRect(&r, bufpos.br.x, 0, client_size.width(), client_size.height()); FillRect(paintDC, &r, black);
            SetRect(&r, bufpos.tl.x, bufpos.br.y, bufpos.br.x, client_size.height()); FillRect(paintDC, &r, black);
          }

          // Do the blit
          Point buf_pos = clientToDesktop(pr.tl);

          if (!BitBlt(paintDC, pr.tl.x, pr.tl.y, pr.width(), pr.height(),
                      bitmapDC, buf_pos.x, buf_pos.y, SRCCOPY))
            throw rdr::SystemException("unable to BitBlt to window", GetLastError());
        }
      }

      EndPaint(handle, &ps);

      // - Notify the callback that a paint message has finished processing
      callback->paintCompleted();
    }
    return 0;

    // -=- Palette management

  case WM_PALETTECHANGED:
    vlog.debug("WM_PALETTECHANGED");
    if ((HWND)wParam == handle) {
      vlog.debug("ignoring");
      break;
    }
  case WM_QUERYNEWPALETTE:
    vlog.debug("re-selecting palette");
    {
      WindowDC wdc(handle);
      PaletteSelector pSel(wdc, windowPalette.getHandle());
      if (pSel.isRedrawRequired()) {
        InvalidateRect(handle, 0, FALSE);
        UpdateWindow(handle);
      }
    }
    return TRUE;

    // -=- Window position

    // Prevent the window from being resized to be too large if in normal mode.
    // If maximized or fullscreen the allow oversized windows.

  case WM_WINDOWPOSCHANGING:
    {
      WINDOWPOS* wpos = (WINDOWPOS*)lParam;
      if (wpos->flags & SWP_NOSIZE)
        break;

      // Work out how big the window should ideally be
      DWORD current_style = GetWindowLong(handle, GWL_STYLE);
      DWORD style = current_style & ~(WS_VSCROLL | WS_HSCROLL);
      RECT r;
      SetRect(&r, 0, 0, buffer->width(), buffer->height());
      AdjustWindowRect(&r, style, FALSE);
      Rect reqd_size = Rect(r.left, r.top, r.right, r.bottom);

      if (current_style & WS_VSCROLL)
        reqd_size.br.x += GetSystemMetrics(SM_CXVSCROLL);
      if (current_style & WS_HSCROLL)
        reqd_size.br.y += GetSystemMetrics(SM_CXHSCROLL);
      RECT current;
      GetWindowRect(handle, &current);

      if (!(GetWindowLong(handle, GWL_STYLE) & WS_MAXIMIZE) && !fullscreenActive) {
        // Ensure that the window isn't resized too large
        if (wpos->cx > reqd_size.width()) {
          wpos->cx = reqd_size.width();
          wpos->x = current.left;
        }
        if (wpos->cy > reqd_size.height()) {
          wpos->cy = reqd_size.height();
          wpos->y = current.top;
        }
      }
    }
    break;

    // Add scrollbars if required and update window size info we have cached.

  case WM_SIZE:
    {
      Point old_offset = desktopToClient(Point(0, 0));

      // Update the cached sizing information
      RECT r;
      GetWindowRect(handle, &r);
      window_size = Rect(r.left, r.top, r.right, r.bottom);
      GetClientRect(handle, &r);
      client_size = Rect(r.left, r.top, r.right, r.bottom);

      // Determine whether scrollbars are required
      calculateScrollBars();

      // Redraw if required
      if ((!old_offset.equals(desktopToClient(Point(0, 0)))))
        InvalidateRect(handle, 0, TRUE);
    }
    break;

  case WM_VSCROLL:
  case WM_HSCROLL: 
    {
      Point delta;
      int newpos = (msg == WM_VSCROLL) ? scrolloffset.y : scrolloffset.x;

      switch (LOWORD(wParam)) {
      case SB_PAGEUP: newpos -= 50; break;
      case SB_PAGEDOWN: newpos += 50; break;
      case SB_LINEUP: newpos -= 5; break;
      case SB_LINEDOWN: newpos += 5; break;
      case SB_THUMBTRACK:
      case SB_THUMBPOSITION: newpos = HIWORD(wParam); break;
      default: vlog.info("received unknown scroll message");
      };

      if (msg == WM_HSCROLL)
        setViewportOffset(Point(newpos, scrolloffset.y));
      else
        setViewportOffset(Point(scrolloffset.x, newpos));
  
      SCROLLINFO si;
      si.cbSize = sizeof(si); 
      si.fMask  = SIF_POS; 
      si.nPos   = newpos; 
      SetScrollInfo(handle, (msg == WM_VSCROLL) ? SB_VERT : SB_HORZ, &si, TRUE); 
    }
    break;

    // -=- Bump-scrolling

  case WM_TIMER:
    switch (wParam) {
    case TIMER_BUMPSCROLL:
      if (!setViewportOffset(scrolloffset.translate(bumpScrollDelta)))
        bumpScrollTimer.stop();
      break;
    case TIMER_POINTER_INTERVAL:
    case TIMER_POINTER_3BUTTON:
      ptr.handleTimer(callback, wParam);
      break;
    }
    break;

    // -=- Cursor shape/visibility handling

  case WM_SETCURSOR:
    if (LOWORD(lParam) != HTCLIENT)
      break;
    SetCursor(cursorInBuffer ? dotCursor : arrowCursor);
    return TRUE;

  case WM_MOUSELEAVE:
    trackingMouseLeave = false;
    cursorOutsideBuffer();
    return 0;

    // -=- Mouse input handling

  case WM_MOUSEMOVE:
  case WM_LBUTTONUP:
  case WM_MBUTTONUP:
  case WM_RBUTTONUP:
  case WM_LBUTTONDOWN:
  case WM_MBUTTONDOWN:
  case WM_RBUTTONDOWN:
#ifdef WM_MOUSEWHEEL
  case WM_MOUSEWHEEL:
#endif
    if (has_focus)
    {
      if (!trackingMouseLeave) {
        TRACKMOUSEEVENT tme;
        tme.cbSize = sizeof(TRACKMOUSEEVENT);
        tme.dwFlags = TME_LEAVE;
        tme.hwndTrack = handle;
        _TrackMouseEvent(&tme);
        trackingMouseLeave = true;
      }
      int mask = 0;
      if (LOWORD(wParam) & MK_LBUTTON) mask |= 1;
      if (LOWORD(wParam) & MK_MBUTTON) mask |= 2;
      if (LOWORD(wParam) & MK_RBUTTON) mask |= 4;

#ifdef WM_MOUSEWHEEL
      if (msg == WM_MOUSEWHEEL) {
        int delta = (short)HIWORD(wParam);
        int repeats = (abs(delta)+119) / 120;
        int wheelMask = (delta > 0) ? 8 : 16;
        vlog.debug("repeats %d, mask %d\n",repeats,wheelMask);
        for (int i=0; i<repeats; i++) {
          ptr.pointerEvent(callback, oldpos, mask | wheelMask);
          ptr.pointerEvent(callback, oldpos, mask);
        }
      } else {
#endif
        Point clientPos = Point(LOWORD(lParam), HIWORD(lParam));
        Point p = clientToDesktop(clientPos);

        // If the mouse is not within the server buffer area, do nothing
        cursorInBuffer = buffer->getRect().contains(p);
        if (!cursorInBuffer) {
          cursorOutsideBuffer();
          break;
        }

        // If we're locally rendering the cursor then redraw it
        if (cursorAvailable) {
          // - Render the cursor!
          if (!p.equals(cursorPos)) {
            hideLocalCursor();
            cursorPos = p;
            showLocalCursor();
            if (cursorVisible)
              hideSystemCursor();
          }
        }

        // If we are doing bump-scrolling then try that first...
        if (processBumpScroll(clientPos))
          break;

        // Send a pointer event to the server
        ptr.pointerEvent(callback, p, mask);
        oldpos = p;
#ifdef WM_MOUSEWHEEL
      }
#endif
    } else {
      cursorOutsideBuffer();
    }
    break;

    // -=- Track whether or not the window has focus

  case WM_SETFOCUS:
    has_focus = true;
    break;
  case WM_KILLFOCUS:
    has_focus = false;
    cursorOutsideBuffer();
    // Restore the keyboard to a consistent state
    kbd.releaseAllKeys(callback);
    break;

    // -=- Handle the extra window menu items

    // Pass system menu messages to the callback and only attempt
    // to process them ourselves if the callback returns false.
  case WM_SYSCOMMAND:
    // Call the supplied callback
    if (callback->sysCommand(wParam, lParam))
      break;

    // - Not processed by the callback, so process it as a system message
    switch (wParam & 0xfff0) {

      // When restored, ensure that full-screen mode is re-enabled if required.
    case SC_RESTORE:
      {
      if (GetWindowLong(handle, GWL_STYLE) & WS_MINIMIZE) {
        rfb::win32::SafeDefWindowProc(handle, msg, wParam, lParam);
        setFullscreen(fullscreenRestore);
      }
      else if (fullscreenActive)
        setFullscreen(false);
      else
        rfb::win32::SafeDefWindowProc(handle, msg, wParam, lParam);

      return 0;
      }

      // If we are maximized or minimized then that cancels full-screen mode.
    case SC_MINIMIZE:
    case SC_MAXIMIZE:
      fullscreenRestore = fullscreenActive;
      setFullscreen(false);
      break;

      // If the menu is about to be shown, make sure it's up to date
    case SC_KEYMENU:
    case SC_MOUSEMENU:
      callback->refreshMenu(true);
      break;

    };
    break;

    // Treat all menu commands as system menu commands
  case WM_COMMAND:
    SendMessage(handle, WM_SYSCOMMAND, wParam, lParam);
    return 0;

    // -=- Handle keyboard input

  case WM_KEYUP:
  case WM_KEYDOWN:
    // Hook the MenuKey to pop-up the window menu
    if (menuKey && (wParam == menuKey)) {

      bool ctrlDown = (GetAsyncKeyState(VK_CONTROL) & 0x8000) != 0;
      bool altDown = (GetAsyncKeyState(VK_MENU) & 0x8000) != 0;
      bool shiftDown = (GetAsyncKeyState(VK_SHIFT) & 0x8000) != 0;
      if (!(ctrlDown || altDown || shiftDown)) {

        // If MenuKey is being released then pop-up the menu
        if ((msg == WM_KEYDOWN)) {
          // Make sure it's up to date
          callback->refreshMenu(false);

          // Show it under the pointer
          POINT pt;
          GetCursorPos(&pt);
          cursorInBuffer = false;
          TrackPopupMenu(GetSystemMenu(handle, FALSE),
            TPM_CENTERALIGN | TPM_VCENTERALIGN, pt.x, pt.y, 0, handle, 0);
        }

        // Ignore the MenuKey keypress for both press & release events
        return 0;
      }
    }
	case WM_SYSKEYDOWN:
	case WM_SYSKEYUP:
    kbd.keyEvent(callback, wParam, lParam, (msg == WM_KEYDOWN) || (msg == WM_SYSKEYDOWN));
    return 0;

    // -=- Handle the window closing

  case WM_CLOSE:
    vlog.debug("WM_CLOSE %x", handle);
    callback->closeWindow();
    break;

  };

  return rfb::win32::SafeDefWindowProc(handle, msg, wParam, lParam);
}


void
DesktopWindow::hideLocalCursor() {
  // - Blit the cursor backing store over the cursor
  // *** ALWAYS call this BEFORE changing buffer PF!!!
  if (cursorVisible) {
    cursorVisible = false;
    buffer->imageRect(cursorBackingRect, cursorBacking.data);
    invalidateDesktopRect(cursorBackingRect);
  }
}

void
DesktopWindow::showLocalCursor() {
  if (cursorAvailable && !cursorVisible && cursorInBuffer) {
    if (!buffer->getPF().equal(cursor.getPF()) ||
      cursor.getRect().is_empty()) {
      vlog.info("attempting to render invalid local cursor");
      cursorAvailable = false;
      showSystemCursor();
      return;
    }
    cursorVisible = true;
    
    cursorBackingRect = cursor.getRect().translate(cursorPos).translate(cursor.hotspot.negate());
    cursorBackingRect = cursorBackingRect.intersect(buffer->getRect());
    buffer->getImage(cursorBacking.data, cursorBackingRect);

    renderLocalCursor();

    invalidateDesktopRect(cursorBackingRect);
  }
}

void DesktopWindow::cursorOutsideBuffer()
{
  cursorInBuffer = false;
  hideLocalCursor();
  showSystemCursor();
}

void
DesktopWindow::renderLocalCursor()
{
  Rect r = cursor.getRect();
  r = r.translate(cursorPos).translate(cursor.hotspot.negate());
  buffer->maskRect(r, cursor.data, cursor.mask.buf);
}

void
DesktopWindow::hideSystemCursor() {
  if (systemCursorVisible) {
    vlog.debug("hide system cursor");
    systemCursorVisible = false;
    ShowCursor(FALSE);
  }
}

void
DesktopWindow::showSystemCursor() {
  if (!systemCursorVisible) {
    vlog.debug("show system cursor");
    systemCursorVisible = true;
    ShowCursor(TRUE);
  }
}


bool
DesktopWindow::invalidateDesktopRect(const Rect& crect) {
  Rect rect = desktopToClient(crect);
  if (rect.intersect(client_size).is_empty()) return false;
  RECT invalid = {rect.tl.x, rect.tl.y, rect.br.x, rect.br.y};
  InvalidateRect(handle, &invalid, FALSE);
  return true;
}


void
DesktopWindow::notifyClipboardChanged(const char* text, int len) {
  callback->clientCutText(text, len);
}


void
DesktopWindow::setPF(const PixelFormat& pf) {
  // If the cursor is the wrong format then clear it
  if (!pf.equal(buffer->getPF()))
    setCursor(0, 0, Point(), 0, 0);

  // Update the desktop buffer
  buffer->setPF(pf);
  
  // Redraw the window
  InvalidateRect(handle, 0, 0);
}

void
DesktopWindow::setSize(int w, int h) {
  vlog.debug("setSize %dx%d", w, h);

  // If the locally-rendered cursor is visible then remove it
  hideLocalCursor();

  // Resize the backing buffer
  buffer->setSize(w, h);

  // If the window is not maximised or full-screen then resize it
  if (!(GetWindowLong(handle, GWL_STYLE) & WS_MAXIMIZE) && !fullscreenActive) {
    // Resize the window to the required size
    RECT r = {0, 0, w, h};
    AdjustWindowRect(&r, GetWindowLong(handle, GWL_STYLE), FALSE);

    // Resize about the center of the window, and clip to current monitor
    MonitorInfo mi(handle);
    resizeWindow(handle, r.right-r.left, r.bottom-r.top);
    mi.clipTo(handle);
  } else {
    // Ensure the screen contents are consistent
    InvalidateRect(handle, 0, FALSE);
  }

  // Enable/disable scrollbars as appropriate
  calculateScrollBars();
}

void
DesktopWindow::setCursor(int w, int h, const Point& hotspot, void* data, void* mask) {
  hideLocalCursor();

  cursor.hotspot = hotspot;

  cursor.setSize(w, h);
  cursor.setPF(buffer->getPF());
  cursor.imageRect(cursor.getRect(), data);
  memcpy(cursor.mask.buf, mask, cursor.maskLen());
  cursor.crop();

  cursorBacking.setSize(w, h);
  cursorBacking.setPF(buffer->getPF());

  cursorAvailable = true;

  showLocalCursor();
}

PixelFormat
DesktopWindow::getNativePF() const {
  vlog.debug("getNativePF()");
  return WindowDC(handle).getPF();
}


void
DesktopWindow::refreshWindowPalette(int start, int count) {
  vlog.debug("refreshWindowPalette(%d, %d)", start, count);

  Colour colours[256];
  if (count > 256) {
    vlog.debug("%d palette entries", count);
    throw rdr::Exception("too many palette entries");
  }

  // Copy the palette from the DIBSectionBuffer
  ColourMap* cm = buffer->getColourMap();
  if (!cm) return;
  for (int i=0; i<count; i++) {
    int r, g, b;
    cm->lookup(i, &r, &g, &b);
    colours[i].r = r;
    colours[i].g = g;
    colours[i].b = b;
  }

  // Set the window palette
  windowPalette.setEntries(start, count, colours);

  // Cause the window to be redrawn
  palette_changed = true;
  InvalidateRect(handle, 0, FALSE);
}


void DesktopWindow::calculateScrollBars() {
  // Calculate the required size of window
  DWORD current_style = GetWindowLong(handle, GWL_STYLE);
  DWORD style = current_style & ~(WS_VSCROLL | WS_HSCROLL);
  DWORD old_style;
  RECT r;
  SetRect(&r, 0, 0, buffer->width(), buffer->height());
  AdjustWindowRect(&r, style, FALSE);
  Rect reqd_size = Rect(r.left, r.top, r.right, r.bottom);

  if (!bumpScroll) {
    // We only enable scrollbars if bump-scrolling is not active.
    // Effectively, this means if full-screen is not active,
    // but I think it's better to make these things explicit.
    
    // Work out whether scroll bars are required
    do {
      old_style = style;

      if (!(style & WS_HSCROLL) && (reqd_size.width() > window_size.width())) {
        style |= WS_HSCROLL;
        reqd_size.br.y += GetSystemMetrics(SM_CXHSCROLL);
      }
      if (!(style & WS_VSCROLL) && (reqd_size.height() > window_size.height())) {
        style |= WS_VSCROLL;
        reqd_size.br.x += GetSystemMetrics(SM_CXVSCROLL);
      }
    } while (style != old_style);
  }

  // Tell Windows to update the window style & cached settings
  if (style != current_style) {
    SetWindowLong(handle, GWL_STYLE, style);
    SetWindowPos(handle, NULL, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER | SWP_FRAMECHANGED);
  }

  // Update the scroll settings
  SCROLLINFO si;
  if (style & WS_VSCROLL) {
    si.cbSize = sizeof(si); 
    si.fMask  = SIF_RANGE | SIF_PAGE | SIF_POS; 
    si.nMin   = 0; 
    si.nMax   = buffer->height(); 
    si.nPage  = buffer->height() - (reqd_size.height() - window_size.height()); 
    maxscrolloffset.y = max(0, si.nMax-si.nPage);
    scrolloffset.y = min(maxscrolloffset.y, scrolloffset.y);
    si.nPos   = scrolloffset.y;
    SetScrollInfo(handle, SB_VERT, &si, TRUE);
  }
  if (style & WS_HSCROLL) {
    si.cbSize = sizeof(si); 
    si.fMask  = SIF_RANGE | SIF_PAGE | SIF_POS; 
    si.nMin   = 0;
    si.nMax   = buffer->width(); 
    si.nPage  = buffer->width() - (reqd_size.width() - window_size.width()); 
    maxscrolloffset.x = max(0, si.nMax-si.nPage);
    scrolloffset.x = min(maxscrolloffset.x, scrolloffset.x);
    si.nPos   = scrolloffset.x;
    SetScrollInfo(handle, SB_HORZ, &si, TRUE);
  }
}


void
DesktopWindow::setName(const char* name) {
  SetWindowText(handle, TStr(name));
}


void
DesktopWindow::serverCutText(const char* str, int len) {
  CharArray t(len+1);
  memcpy(t.buf, str, len);
  t.buf[len] = 0;
  clipboard.setClipText(t.buf);
}


void DesktopWindow::fillRect(const Rect& r, Pixel pix) {
  if (cursorBackingRect.overlaps(r)) hideLocalCursor();
  buffer->fillRect(r, pix);
  invalidateDesktopRect(r);
}
void DesktopWindow::imageRect(const Rect& r, void* pixels) {
  if (cursorBackingRect.overlaps(r)) hideLocalCursor();
  buffer->imageRect(r, pixels);
  invalidateDesktopRect(r);
}
void DesktopWindow::copyRect(const Rect& r, int srcX, int srcY) {
  if (cursorBackingRect.overlaps(r) ||
      cursorBackingRect.overlaps(Rect(srcX, srcY, srcX+r.width(), srcY+r.height())))
    hideLocalCursor();
  buffer->copyRect(r, Point(r.tl.x-srcX, r.tl.y-srcY));
  invalidateDesktopRect(r);
}

void DesktopWindow::invertRect(const Rect& r) {
  int stride;
  rdr::U8* p = buffer->getPixelsRW(r, &stride);
  for (int y = 0; y < r.height(); y++) {
    for (int x = 0; x < r.width(); x++) {
      switch (buffer->getPF().bpp) {
      case 8:  ((rdr::U8* )p)[x+y*stride] ^= 0xff;       break;
      case 16: ((rdr::U16*)p)[x+y*stride] ^= 0xffff;     break;
      case 32: ((rdr::U32*)p)[x+y*stride] ^= 0xffffffff; break;
      }
    }
  }
  invalidateDesktopRect(r);
}
