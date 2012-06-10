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

// -=- Clipboard.cxx

#include <rfb_win32/Clipboard.h>
#include <rfb_win32/WMShatter.h>
#include <rfb/util.h>

#include <rfb/LogWriter.h>

using namespace rfb;
using namespace rfb::win32;

static LogWriter vlog("Clipboard");


//
// -=- CR/LF handlers
//

char*
dos2unix(const char* text) {
  int len = strlen(text)+1;
  char* unix = new char[strlen(text)+1];
  int i, j=0;
  for (i=0; i<len; i++) {
    if (text[i] != '\x0d')
      unix[j++] = text[i];
  }
  return unix;
}

char*
unix2dos(const char* text) {
  int len = strlen(text)+1;
  char* dos = new char[strlen(text)*2+1];
  int i, j=0;
  for (i=0; i<len; i++) {
    if (text[i] == '\x0a')
      dos[j++] = '\x0d';
    dos[j++] = text[i];
  }
  return dos;
}


//
// -=- ISO-8859-1 (Latin 1) filter (in-place)
//

void
removeNonISOLatin1Chars(char* text) {
  int len = strlen(text);
  int i=0, j=0;
  for (; i<len; i++) {
    if (((text[i] >= 1) && (text[i] <= 127)) ||
        ((text[i] >= 160) && (text[i] <= 255)))
      text[j++] = text[i];
  }
  text[j] = 0;
}

//
// -=- Clipboard object
//

Clipboard::Clipboard()
  : MsgWindow(_T("Clipboard")), notifier(0), next_window(0) {
  next_window = SetClipboardViewer(getHandle());
  vlog.debug("registered clipboard handler");
}

Clipboard::~Clipboard() {
  vlog.debug("removing %x from chain (next is %x)", getHandle(), next_window);
  ChangeClipboardChain(getHandle(), next_window);
}

LRESULT
Clipboard::processMessage(UINT msg, WPARAM wParam, LPARAM lParam) {
  switch (msg) {

  case WM_CHANGECBCHAIN:
    vlog.debug("change clipboard chain (%x, %x)", wParam, lParam);
    if ((HWND) wParam == next_window)
      next_window = (HWND) lParam;
    else if (next_window != 0)
      SendMessage(next_window, msg, wParam, lParam);
    else
      vlog.error("bad clipboard chain change!");
    break;

  case WM_DRAWCLIPBOARD:
    {
      HWND owner = GetClipboardOwner();
      if (owner == getHandle()) {
        vlog.debug("local clipboard changed by me");
      } else {
        vlog.debug("local clipboard changed by %x", owner);

			  // Open the clipboard
			  if (OpenClipboard(getHandle())) {
				  // Get the clipboard data
				  HGLOBAL cliphandle = GetClipboardData(CF_TEXT);
				  if (cliphandle) {
					  char* clipdata = (char*) GlobalLock(cliphandle);

            // Notify clients
            if (notifier) {
              if (!clipdata) {
                notifier->notifyClipboardChanged(0, 0);
              } else {
                CharArray unix_text;
                unix_text.buf = dos2unix(clipdata);
                removeNonISOLatin1Chars(unix_text.buf);
                notifier->notifyClipboardChanged(unix_text.buf, strlen(unix_text.buf));
              }
            } else {
              vlog.debug("no clipboard notifier registered");
            }

					  // Release the buffer and close the clipboard
					  GlobalUnlock(cliphandle);
				  }

				  CloseClipboard();
        }
			}
    }
    if (next_window)
		  SendMessage(next_window, msg, wParam, lParam);
    return 0;

  };
  return MsgWindow::processMessage(msg, wParam, lParam);
};

void
Clipboard::setClipText(const char* text) {
  HANDLE clip_handle = 0;

  try {

    // - Firstly, we must open the clipboard
    if (!OpenClipboard(getHandle()))
      throw rdr::SystemException("unable to open Win32 clipboard", GetLastError());

    // - Pre-process the supplied clipboard text into DOS format
    CharArray dos_text;
    dos_text.buf = unix2dos(text);
    removeNonISOLatin1Chars(dos_text.buf);
    int dos_text_len = strlen(dos_text.buf);

    // - Allocate global memory for the data
    clip_handle = ::GlobalAlloc(GMEM_MOVEABLE, dos_text_len+1);

    char* data = (char*) GlobalLock(clip_handle);
    memcpy(data, dos_text.buf, dos_text_len+1);
    data[dos_text_len] = 0;
    GlobalUnlock(clip_handle);

    // - Next, we must clear out any existing data
    if (!EmptyClipboard())
      throw rdr::SystemException("unable to empty Win32 clipboard", GetLastError());

    // - Set the new clipboard data
    if (!SetClipboardData(CF_TEXT, clip_handle))
      throw rdr::SystemException("unable to set Win32 clipboard", GetLastError());
    clip_handle = 0;

    vlog.debug("set clipboard");
  } catch (rdr::Exception& e) {
    vlog.debug(e.str());
  }

  // - Close the clipboard
  if (!CloseClipboard())
    vlog.debug("unable to close Win32 clipboard: %u", GetLastError());
  else
    vlog.debug("closed clipboard");
  if (clip_handle) {
    vlog.debug("freeing clipboard handle");
    GlobalFree(clip_handle);
  }
}
