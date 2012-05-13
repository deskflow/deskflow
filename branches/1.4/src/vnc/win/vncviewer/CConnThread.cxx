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

// -=- CConnThread.cxx

// A CConnThread instance is created for each new connection.
// The CConnThread creates the corresponding CConn instance
// and manages it.

#include <stdlib.h>
#include <rfb/LogWriter.h>
#include <rfb/Hostname.h>
#include <rfb_win32/MsgBox.h>
#include <network/TcpSocket.h>
#include <vncviewer/CConnThread.h>
#include <vncviewer/CConn.h>
#include <vncviewer/ConnectionDialog.h>
#include <vncviewer/ConnectingDialog.h>
#include <vncviewer/UserPasswdDialog.h>
#include <set>

using namespace rfb;
using namespace win32;

static LogWriter vlog("CConnThread");

static std::set<CConnThread*> threads;
static Mutex threadsLock;
static Handle noMoreThreads(CreateEvent(0, TRUE, FALSE, 0));


CConnThread::CConnThread() : Thread("CConnThread"), isConfig(false),
                             sock(0), reverse(false) {
  vlog.info("CConnThread (dialog)");
  setDeleteAfterRun();
  Lock l(threadsLock);
  threads.insert(this);
  start();
}

CConnThread::CConnThread(const char* hostOrConfig_, bool isConfig_)
 : Thread("CConnThread"), hostOrConfig(strDup(hostOrConfig_)),
   isConfig(isConfig_), sock(0), reverse(false) {
  vlog.info("CConnThread (host/port)");
  setDeleteAfterRun();
  Lock l(threadsLock);
  threads.insert(this);
  start();
}

CConnThread::CConnThread(network::Socket* sock_, bool reverse_)
 : Thread("CConnThread"), isConfig(false), sock(sock_), reverse(reverse_) {
  vlog.info("CConnThread (reverse connection)");
  setDeleteAfterRun();
  Lock l(threadsLock);
  threads.insert(this);
  start();
}

CConnThread::~CConnThread() {
  Lock l(threadsLock);
  threads.erase(this);
  if (threads.empty())
    SetEvent(noMoreThreads);
  delete sock;
}


void CConnThread::run() {
  CConnOptions options;
  bool reconnect;

  do {
    {
      CConn conn;
      reconnect = false;

      // If there is no socket object then set the host & port info
      if (!sock && !options.host.buf) {
        try {
          if (isConfig) {
            // A configuration file name was specified - load it
            CharArray filename = hostOrConfig.takeBuf();
            options.readFromFile(filename.buf);
          } else {
            // An actual hostname (and possibly port) was specified
            options.host.replaceBuf(hostOrConfig.takeBuf());
          }

          if (!options.host.buf) {
            // No host was specified - prompt for one
            ConnectionDialog connDlg(&conn);
            if (!connDlg.showDialog())
              return;
            options = conn.getOptions();
            options.setHost(CStr(connDlg.hostname.buf));
          }
        } catch (rdr::Exception& e) {
          MsgBox(0, TStr(e.str()), MB_ICONERROR | MB_OK);
          return;
        }
      }

      // Apply the connection options to the CConn
      conn.applyOptions(options);

      if (!sock) {
        // There is no existing connection - better make one
        const char* hostAndPort = conn.getOptions().host.buf;

        try {
          ConnectingDialog dlg;
          sock = dlg.connect(hostAndPort);

          // If the connection was cancelled by the user, just quit
          if (!sock)
            return;
        } catch(rdr::Exception& e) {
          MsgBox(NULL, TStr(e.str()), MB_ICONERROR | MB_OK);
          return;
        }

        // Try to add the caller to the MRU
        MRU::addToMRU(hostAndPort);
      }

      // Run the RFB protocol over the connected socket
      conn.initialise(sock, reverse);
      while (!conn.isClosed()) {
        try {
          conn.getInStream()->check(1,1);
          conn.processMsg();
        } catch (rdr::EndOfStream) {
          if (conn.state() == CConnection::RFBSTATE_NORMAL)
            conn.close();
          else
            conn.close("The connection closed unexpectedly");
        } catch (rfb::AuthCancelledException) {
          conn.close();
        } catch (rfb::AuthFailureException& e) {
          // Clear the password, in case we auto-reconnect
          options = conn.getOptions();
          options.password.replaceBuf(0);
          conn.applyOptions(options);
          conn.close(e.str());
        } catch (rdr::Exception& e) {
          conn.close(e.str());
        }
      }

      // If there is a cause for closing the connection logged then display it
      if (conn.closeReason()) {
        reconnect = !reverse && conn.getOptions().autoReconnect;
        if (!reconnect) {
          MsgBox(0, TStr(conn.closeReason()), MB_ICONINFORMATION | MB_OK);
        } else {
          options = conn.getOptions();
          const char* format = "%s\nDo you wish to attempt to reconnect to %s?";
          CharArray message(strlen(conn.closeReason()) + strlen(format) +
                            strlen(conn.getOptions().host.buf));
          sprintf(message.buf, format, conn.closeReason(), conn.getOptions().host.buf);
          if (MsgBox(0, TStr(message.buf), MB_ICONQUESTION | MB_YESNO | MB_DEFBUTTON2) != IDYES)
            reconnect = false;
        }
      }
    } // Exit the CConn's scope, implicitly destroying it & making it safe to delete the TcpSocket

    // Clean up the old socket, if any
    delete sock; sock = 0;
  } while (reconnect);
}


BOOL CConnThread::getMessage(MSG* msg, HWND hwnd, UINT minMsg, UINT maxMsg) {
  while (!PeekMessage(msg, hwnd, minMsg, maxMsg, PM_REMOVE)) {
    DWORD result = MsgWaitForMultipleObjects(1, &noMoreThreads.h, FALSE, INFINITE, QS_ALLINPUT);
    if (result == WAIT_OBJECT_0)
      return FALSE;
    else if (result == WAIT_FAILED)
      throw rdr::SystemException("CConnThread::getMessage wait failed", GetLastError());
  }
  return msg->message != WM_QUIT;
}
