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

// -=- WinVNC Version 4.0 Main Routine

#include <winvnc/VNCServerWin32.h>
#include <winvnc/resource.h>
#include <winvnc/STrayIcon.h>
#include <rfb_win32/ComputerName.h>
#include <rfb_win32/CurrentUser.h>
#include <rfb_win32/Service.h>
#include <rfb/SSecurityFactoryStandard.h>
#include <rfb/Hostname.h>
#include <rfb/LogWriter.h>

using namespace rfb;
using namespace win32;
using namespace winvnc;
using namespace network;

static LogWriter vlog("VNCServerWin32");


const TCHAR* winvnc::VNCServerWin32::RegConfigPath = _T("Software\\RealVNC\\WinVNC4");


static IntParameter http_port("HTTPPortNumber",
  "TCP/IP port on which the server will serve the Java applet VNC Viewer ", 5800);
static IntParameter port_number("PortNumber",
  "TCP/IP port on which the server will accept connections", 5900);
static StringParameter hosts("Hosts",
  "Filter describing which hosts are allowed access to this server", "+0.0.0.0/0.0.0.0");
static BoolParameter localHost("LocalHost",
  "Only accept connections from via the local loop-back network interface", false);
static BoolParameter queryOnlyIfLoggedOn("QueryOnlyIfLoggedOn",
  "Only prompt for a local user to accept incoming connections if there is a user logged on", false);


VNCServerWin32::VNCServerWin32()
  : command(NoCommand), commandSig(commandLock),
    commandEvent(CreateEvent(0, TRUE, FALSE, 0)),
    vncServer(CStr(ComputerName().buf), &desktop),
    hostThread(0), runServer(false), isDesktopStarted(false),
    httpServer(&vncServer), config(&sockMgr), trayIcon(0),
    rfbSock(&sockMgr), httpSock(&sockMgr),
    queryConnectDialog(0)
{
  // Initialise the desktop
  desktop.setStatusLocation(&isDesktopStarted);

  // Initialise the VNC server
  vncServer.setQueryConnectionHandler(this);

  // Register the desktop's event to be handled
  sockMgr.addEvent(desktop.getUpdateEvent(), &desktop);

  // Register the queued command event to be handled
  sockMgr.addEvent(commandEvent, this);
}

VNCServerWin32::~VNCServerWin32() {
  delete trayIcon;

  // Stop the SDisplay from updating our state
  desktop.setStatusLocation(0);

  // Join the Accept/Reject dialog thread
  if (queryConnectDialog)
    delete queryConnectDialog->join();
}


void VNCServerWin32::processAddressChange(network::SocketListener* sock_) {
  if (!trayIcon || (sock_ != rfbSock.sock))
    return;

  // Tool-tip prefix depends on server mode
  const TCHAR* prefix = _T("VNC Server (User):");
  if (isServiceProcess())
    prefix = _T("VNC Server (Service):");

  // Fetch the list of addresses
  std::list<char*> addrs;
  if (rfbSock.sock)
    rfbSock.sock->getMyAddresses(&addrs);
  else
    addrs.push_front(strDup("Not accepting connections"));

  // Allocate space for the new tip
  std::list<char*>::iterator i, next_i;
  int length = _tcslen(prefix)+1;
  for (i=addrs.begin(); i!= addrs.end(); i++)
    length += strlen(*i) + 1;

  // Build the new tip
  TCharArray toolTip(length);
  _tcscpy(toolTip.buf, prefix);
  for (i=addrs.begin(); i!= addrs.end(); i=next_i) {
    next_i = i; next_i ++;
    TCharArray addr = *i;    // Assumes ownership of string
    _tcscat(toolTip.buf, addr.buf);
    if (next_i != addrs.end())
      _tcscat(toolTip.buf, _T(","));
  }
  
  // Pass the new tip to the tray icon
  vlog.info("Refreshing tray icon");
  trayIcon->setToolTip(toolTip.buf);
}

void VNCServerWin32::regConfigChanged() {
  // -=- Make sure we're listening on the right ports.
  rfbSock.setServer(&vncServer);
  rfbSock.setPort(port_number, localHost);
  httpSock.setServer(&httpServer);
  httpSock.setPort(http_port, localHost);

  // -=- Update the Java viewer's web page port number.
  httpServer.setRFBport(rfbSock.sock ? port_number : 0);

  // -=- Update the TCP address filter for both ports, if open.
  CharArray pattern(hosts.getData());
  rfbSock.setFilter(pattern.buf);
  httpSock.setFilter(pattern.buf);

  // -=- Update the tray icon tooltip text with IP addresses
  processAddressChange(rfbSock.sock);
}


int VNCServerWin32::run() {
  { Lock l(runLock);
    hostThread = Thread::self();
    runServer = true;
  }

  // - Create the tray icon (if possible)
  trayIcon = new STrayIconThread(*this, IDI_ICON, IDI_CONNECTED, IDR_TRAY);

  // - Register for notification of configuration changes
  config.setCallback(this);
  if (isServiceProcess())
    config.setKey(HKEY_LOCAL_MACHINE, RegConfigPath);
  else
    config.setKey(HKEY_CURRENT_USER, RegConfigPath);

  // - Set the address-changed handler for the RFB socket
  rfbSock.setAddressChangeNotifier(this);

  DWORD result = 0;
  try {
    vlog.debug("Entering message loop");

    // - Run the server until we're told to quit
    MSG msg;
    int result = 0;
    while (runServer) {
      result = sockMgr.getMessage(&msg, NULL, 0, 0);
      if (result < 0)
        throw rdr::SystemException("getMessage", GetLastError());
      if (!isServiceProcess() && (result == 0))
        break;
      TranslateMessage(&msg);
      DispatchMessage(&msg);
    }

    vlog.debug("Server exited cleanly");
  } catch (rdr::SystemException &s) {
    vlog.error(s.str());
    result = s.err;
  } catch (rdr::Exception &e) {
    vlog.error(e.str());
  }

  { Lock l(runLock);
    runServer = false;
    hostThread = 0;
  }

  return result;
}

void VNCServerWin32::stop() {
  Lock l(runLock);
  runServer = false;
  if (hostThread)
    PostThreadMessage(hostThread->getThreadId(), WM_QUIT, 0, 0);
}


bool VNCServerWin32::disconnectClients(const char* reason) {
  return queueCommand(DisconnectClients, reason, 0);
}

bool VNCServerWin32::addNewClient(const char* client) {
  TcpSocket* sock = 0;
  try {
    CharArray hostname;
    int port;
    getHostAndPort(client, &hostname.buf, &port, 5500);
    vlog.error("port=%d", port);
    sock = new TcpSocket(hostname.buf, port);
    if (queueCommand(AddClient, sock, 0))
      return true;
    delete sock;
  } catch (...) {
    delete sock;
  }
  return false;
}


VNCServerST::queryResult VNCServerWin32::queryConnection(network::Socket* sock,
                                            const char* userName,
                                            char** reason)
{
  if (queryOnlyIfLoggedOn && CurrentUserToken().noUserLoggedOn())
    return VNCServerST::ACCEPT;
  if (queryConnectDialog) {
    *reason = rfb::strDup("Another connection is currently being queried.");
    return VNCServerST::REJECT;
  }
  queryConnectDialog = new QueryConnectDialog(sock, userName, this);
  queryConnectDialog->startDialog();
  return VNCServerST::PENDING;
}

void VNCServerWin32::queryConnectionComplete() {
  queueCommand(QueryConnectionComplete, 0, 0, false);
}


bool VNCServerWin32::queueCommand(Command cmd, const void* data, int len, bool wait) {
  Lock l(commandLock);
  while (command != NoCommand)
    commandSig.wait();
  command = cmd;
  commandData = data;
  commandDataLen = len;
  SetEvent(commandEvent);
  if (wait) {
    while (command != NoCommand)
      commandSig.wait();
    commandSig.signal();
  }
  return true;
}

void VNCServerWin32::processEvent(HANDLE event_) {
  ResetEvent(event_);

  if (event_ == commandEvent.h) {
    // If there is no command queued then return immediately
    {
      Lock l(commandLock);
      if (command == NoCommand)
        return;
    }

    // Perform the required command
    switch (command) {

    case DisconnectClients:
      // Disconnect all currently active VNC Viewers
      vncServer.closeClients((const char*)commandData);
      break;

    case AddClient:
      // Make a reverse connection to a VNC Viewer
      sockMgr.addSocket((network::Socket*)commandData, &vncServer);
      break;

    case QueryConnectionComplete:
      // The Accept/Reject dialog has completed
      // Get the result, then clean it up
      vncServer.approveConnection(queryConnectDialog->getSock(),
                                  queryConnectDialog->isAccepted(),
                                  "Connection rejected by user");
      delete queryConnectDialog->join();
      queryConnectDialog = 0;
      break;

    default:
      vlog.error("unknown command %d queued", command);
    };

    // Clear the command and signal completion
    {
      Lock l(commandLock);
      command = NoCommand;
      commandSig.signal();
    }
  }
}

