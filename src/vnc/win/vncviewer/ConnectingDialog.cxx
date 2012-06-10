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

// -=- ConnectingDialog.cxx

#include <stdlib.h>
#include <vncviewer/ConnectingDialog.h>
#include <vncviewer/resource.h>
#include <network/TcpSocket.h>
#include <rfb/Threading.h>
#include <rfb/Hostname.h>
#include <map>

using namespace rfb;
using namespace rfb::win32;


// ConnectingDialog callback
static BOOL CALLBACK ConnectingDlgProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam ) {
  bool* activePtr = (bool*)GetWindowLong(hwnd, GWL_USERDATA);
  switch (uMsg) {
  case WM_INITDIALOG:
    SetWindowLong(hwnd, GWL_USERDATA, lParam);
    return TRUE;
	case WM_COMMAND:
	  switch (LOWORD(wParam)) {
	  case IDCANCEL:
      if (activePtr)
        *activePtr = false;
		  return TRUE;
		}
		break;
	case WM_DESTROY:
    if (activePtr)
      *activePtr = false;
	  return TRUE;
	}
	return 0;
}


// Global map, used by ConnectingDialog::Threads to call back to their owning
// ConnectingDialogs, while coping with the fact that the owner may already have quit.
static std::map<int, ConnectingDialog*> dialogs;
static int nextDialogId = 0;
static Mutex dialogsLock;


// ConnectingDialog::Thread
//   Attempts to connect to the specified host.  If the connection succeeds, the
//   socket is saved in the owning ConnectingDialog, if still available, and the
//   event is signalled.  If the connection fails, the Exception text is returned
//   to the dialog.  If the dialog is already gone, the Exception/socket are discarded.
//   NB: This thread class cleans itself up on exit - DO NOT join()!
class ConnectingDialog::Thread : public rfb::Thread {
public:
  Thread(int dialogId_, const char* hostAndPort) : dialogId(dialogId_) {
    setDeleteAfterRun();
    getHostAndPort(hostAndPort, &host.buf, &port);
  }
  virtual void run() {
    try {
      returnSock(new network::TcpSocket(host.buf, port));
    } catch (rdr::Exception& e) {
      returnException(e);
    }
  }
  void returnSock(network::Socket* s) {
    Lock l(dialogsLock);
    if (dialogs.count(dialogId)) {
      dialogs[dialogId]->newSocket = s;
      SetEvent(dialogs[dialogId]->readyEvent);
    } else {
      delete s;
    }
  }
  void returnException(const rdr::Exception& e) {
    Lock l(dialogsLock);
    if (dialogs.count(dialogId)) {
      dialogs[dialogId]->errMsg.replaceBuf(strDup(e.str()));
      SetEvent(dialogs[dialogId]->readyEvent);
    }
  };
  CharArray host;
  int port;
  int dialogId;
};


ConnectingDialog::ConnectingDialog() : dialog(0), readyEvent(CreateEvent(0, TRUE, FALSE, 0)),
                                       newSocket(0), dialogId(0) {
}

network::Socket* ConnectingDialog::connect(const char* hostAndPort) {
  Thread* connectThread = 0;
  bool active = true;
  errMsg.replaceBuf(0);
  newSocket = 0;

  // Get a unique dialog identifier and create the dialog window
  {
    Lock l(dialogsLock);
    dialogId = ++nextDialogId;
    dialogs[dialogId] = this;
    dialog = CreateDialogParam(GetModuleHandle(0),
      MAKEINTRESOURCE(IDD_CONNECTING_DLG), 0, &ConnectingDlgProc, (long)&active);
    ShowWindow(dialog, SW_SHOW);
    ResetEvent(readyEvent);
  }

  // Create and start the connection thread
  try {
    connectThread = new Thread(dialogId, hostAndPort);
    connectThread->start();
  } catch (rdr::Exception& e) {
    errMsg.replaceBuf(strDup(e.str()));
    active = false;
  }

  // Process window messages until the connection thread signals readyEvent, or the dialog is cancelled
  while (active && (MsgWaitForMultipleObjects(1, &readyEvent.h, FALSE, INFINITE, QS_ALLINPUT) == WAIT_OBJECT_0 + 1)) {
    MSG msg;
    while (PeekMessage(&msg, 0, 0, 0, PM_REMOVE))
      DispatchMessage(&msg);
  }

  // Remove this dialog from the table
  //   NB: If the dialog was cancelled then the thread is still running, and will only
  //       discover that we're gone when it looks up our unique Id in the dialog table.
  {
    Lock l(dialogsLock);
    dialogs.erase(dialogId);
  }

  // Close the dialog window
  DestroyWindow(dialog); dialog=0;

  // Throw the exception, if there was one
  if (errMsg.buf)
    throw rdr::Exception(errMsg.buf);

  // Otherwise, return the socket
  //   NB: The socket will be null if the dialog was cancelled
  return newSocket;
}
