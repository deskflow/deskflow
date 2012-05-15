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

// -=- CConn.h

// Windows-specific implementation of CConnection

#ifndef __RFB_WIN32_CCONN_H__
#define __RFB_WIN32_CCONN_H__

#include <network/Socket.h>
#include <rfb/CConnection.h>
#include <rfb/Cursor.h>
#include <rfb/UserPasswdGetter.h>
#include <rfb_win32/Registry.h>
#include <rfb_win32/Handle.h>
#include <vncviewer/InfoDialog.h>
#include <vncviewer/OptionsDialog.h>
#include <vncviewer/CConnOptions.h>
#include <vncviewer/DesktopWindow.h>
#include <list>


namespace rfb {

  namespace win32 {

    class CConn : public CConnection,
                  UserPasswdGetter,
                  DesktopWindow::Callback,
                  rdr::FdInStreamBlockCallback
    {
    public:
      CConn();
      ~CConn();

      // - Start the VNC session on the supplied socket
      //   The socket must already be connected to a host
      bool initialise(network::Socket* s, bool reverse=false);

      // - Set/get the session options
      void applyOptions(CConnOptions& opt);
      const CConnOptions& getOptions() const { return options; };

      // - Show the options dialog for the connection
      void showOptionsDialog();

      // - Close the socket & set the reason for closure
      void close(const char* reason=0);
      bool isClosed() const { return isClosed_; }
      const char* closeReason() const { return closeReason_.buf; }

      // - Last received encoding, for the Info dialog
      int lastUsedEncoding() const { return lastUsedEncoding_; }

      // - Get at the DesktopWindow, if any
      DesktopWindow* getWindow() { return window; }

      // - Get at the underlying Socket
      network::Socket* getSocket() { return sock; }

      // - Get the server's preferred format
      const PixelFormat& getServerDefaultPF() const { return serverDefaultPF; }

      // Global user-config registry key
	  static RegKey userConfigKey;

	  // shows the desktop window
	  void showViewer();

	  // hides the desktop window
	  void hideViewer();

    protected:
      // InputHandler interface (via DesktopWindow::Callback)
      void keyEvent(rdr::U32 key, bool down);
      void pointerEvent(const Point& pos, int buttonMask);
      void clientCutText(const char* str, int len);

      // DesktopWindow::Callback interface
      void displayChanged();
      void paintCompleted();
      bool sysCommand(WPARAM wParam, LPARAM lParam);
      void closeWindow();
      void refreshMenu(bool enableSysCommands);

      // CConnection interface
      CSecurity* getCSecurity(int secType);
      void setColourMapEntries(int firstColour, int nColours, rdr::U16* rgbs);
      void bell();
      void framebufferUpdateEnd();
      void setDesktopSize(int w, int h);
      void setCursor(int w, int h, const Point& hotspot, void* data, void* mask);
      void setName(const char* name);
      void serverInit();
      void serverCutText(const char* str, int len);
      void beginRect(const Rect& r, unsigned int encoding);
      void endRect(const Rect& r, unsigned int encoding);
      void fillRect(const Rect& r, Pixel pix);
      void imageRect(const Rect& r, void* pixels);
      void copyRect(const Rect& r, int srcX, int srcY);

      // rdr::FdInStreamBlockCallback interface
      void blockCallback();

      // UserPasswdGetter interface
      // (overridden to allow a pre-supplied username & password)
      void getUserPasswd(char** user, char** password);

      // CConn-specific internal interface
      void autoSelectFormatAndEncoding();
      void requestNewUpdate();
      void calculateFullColourPF();

      // The desktop window
      DesktopWindow* window;

      // Info and Options dialogs
      OptionsDialog optionsDialog;
      InfoDialog infoDialog;

      // VNC Viewer options
      CConnOptions options;

      // Pixel format and encoding
      PixelFormat serverDefaultPF;
      PixelFormat fullColourPF;
      bool sameMachine;
      bool encodingChange;
      bool formatChange;
      int lastUsedEncoding_;

      // Networking and RFB protocol
      network::Socket* sock;
      Handle sockEvent;
      bool reverseConnection;
      bool requestUpdate;

      // Debugging/logging
      std::list<Rect> debugRects;
      CharArray closeReason_;
      bool isClosed_;
    };

  };

};

#endif


