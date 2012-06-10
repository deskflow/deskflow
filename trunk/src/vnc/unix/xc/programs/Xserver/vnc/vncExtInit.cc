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

#include <stdio.h>

extern "C" {
#define class c_class
#define NEED_EVENTS
#include "X.h"
#include "Xproto.h"
#include "misc.h"
#include "os.h"
#include "dixstruct.h"
#include "extnsionst.h"
#include "scrnintstr.h"
#include "selection.h"
#define _VNCEXT_SERVER_
#define _VNCEXT_PROTO_
#include "vncExt.h"
#undef class
#undef xalloc
}

#include <rfb/Configuration.h>
#include <rfb/Logger_stdio.h>
#include <rfb/LogWriter.h>
#include <rfb/util.h>
#include <rfb/ServerCore.h>
#include <rfb/SSecurityFactoryStandard.h>
#include <rdr/HexOutStream.h>
#include <rfb/LogWriter.h>
#undef max
#undef min
#include <network/TcpSocket.h>

#include "XserverDesktop.h"
#include "vncHooks.h"
#include "vncExtInit.h"

extern "C" {

  extern void vncExtensionInit();
  static void vncResetProc(ExtensionEntry* extEntry);
  static void vncBlockHandler(pointer data, OSTimePtr t, pointer readmask);
  static void vncWakeupHandler(pointer data, int nfds, pointer readmask);
  static void vncClientStateChange(CallbackListPtr*, pointer, pointer);
  static void SendSelectionChangeEvent(Atom selection);
  static int ProcVncExtDispatch(ClientPtr client);
  static int SProcVncExtDispatch(ClientPtr client);

  extern char *display;

  extern Selection *CurrentSelections;
  extern int NumCurrentSelections;
}

using namespace rfb;

static rfb::LogWriter vlog("vncext");

static unsigned long vncExtGeneration = 0;
static bool initialised = false;
static XserverDesktop* desktop[MAXSCREENS] = { 0, };
void* vncFbptr[MAXSCREENS] = { 0, };

static char* clientCutText = 0;
static int clientCutTextLen = 0;

static XserverDesktop* queryConnectDesktop = 0;
static void* queryConnectId = 0;
static int queryConnectTimeout = 0;
static OsTimerPtr queryConnectTimer = 0;

static struct VncInputSelect* vncInputSelectHead = 0;
struct VncInputSelect {
  VncInputSelect(ClientPtr c, Window w, int m) : client(c), window(w), mask(m)
  {
    next = vncInputSelectHead;
    vncInputSelectHead = this;
  }
  ClientPtr client;
  Window window;
  int mask;
  VncInputSelect* next;
};

static int nPrevSelections = 0;
static TimeStamp* prevSelectionTimes = 0;

static int vncErrorBase = 0;
static int vncEventBase = 0;
static char* vncPasswdFile = 0;
int vncInetdSock = -1;

rfb::AliasParameter rfbauth("rfbauth", "Alias for PasswordFile",
                            &SSecurityFactoryStandard::vncAuthPasswdFile);
rfb::StringParameter httpDir("httpd",
                             "Directory containing files to serve via HTTP",
                             "");
rfb::IntParameter httpPort("httpPort", "TCP port to listen for HTTP",0);
rfb::AliasParameter rfbwait("rfbwait", "Alias for ClientWaitTimeMillis",
                            &rfb::Server::clientWaitTimeMillis);
rfb::IntParameter rfbport("rfbport", "TCP port to listen for RFB protocol",0);
rfb::StringParameter desktopName("desktop", "Name of VNC desktop","x11");
rfb::BoolParameter localhostOnly("localhost",
                                 "Only allow connections from localhost",
                                 false);

void vncExtensionInit()
{
  if (vncExtGeneration == serverGeneration) {
    vlog.error("vncExtensionInit: called twice in same generation?");
    return;
  }
  vncExtGeneration = serverGeneration;

  ExtensionEntry* extEntry
    = AddExtension(VNCEXTNAME, VncExtNumberEvents, VncExtNumberErrors,
                   ProcVncExtDispatch, SProcVncExtDispatch, vncResetProc,
                   StandardMinorOpcode);
  if (!extEntry) {
    ErrorF("vncExtInit: AddExtension failed\n");
    return;
  }

  vncErrorBase = extEntry->errorBase;
  vncEventBase = extEntry->eventBase;

  vlog.info("VNC extension running!");

  if (!AddCallback(&ClientStateCallback, vncClientStateChange, 0)) {
    FatalError("AddCallback failed\n");
  }

  try {
    if (!initialised) {
      rfb::initStdIOLoggers();
      initialised = true;
    }

    for (int scr = 0; scr < screenInfo.numScreens; scr++) {

      if (!desktop[scr]) {
        network::TcpListener* listener = 0;
        network::TcpListener* httpListener = 0;
        if (scr == 0 && vncInetdSock != -1) {
          if (network::TcpSocket::isSocket(vncInetdSock) &&
              !network::TcpSocket::isConnected(vncInetdSock))
          {
            listener = new network::TcpListener(0, 0, vncInetdSock, true);
            vlog.info("inetd wait");
          }
        } else {
          int port = rfbport;
          if (port == 0) port = 5900 + atoi(display);
          port += 1000 * scr;
          listener = new network::TcpListener(port, localhostOnly);
          vlog.info("Listening for VNC connections on port %d",port);
          CharArray httpDirStr(httpDir.getData());
          if (httpDirStr.buf[0]) {
            port = httpPort;
            if (port == 0) port = 5800 + atoi(display);
            port += 1000 * scr;
            httpListener = new network::TcpListener(port, localhostOnly);
            vlog.info("Listening for HTTP connections on port %d",port);
          }
        }

        CharArray desktopNameStr(desktopName.getData());
        desktop[scr] = new XserverDesktop(screenInfo.screens[scr], listener,
                                          httpListener,
                                          desktopNameStr.buf,
                                          vncFbptr[scr]);
        vlog.info("created VNC server for screen %d", scr);

        if (scr == 0 && vncInetdSock != -1 && !listener) {
          network::Socket* sock = new network::TcpSocket(vncInetdSock);
          desktop[scr]->addClient(sock, false);
          vlog.info("added inetd sock");
        }

      } else {
        desktop[scr]->serverReset(screenInfo.screens[scr]);
      }

      vncHooksInit(screenInfo.screens[scr], desktop[scr]);
    }

    RegisterBlockAndWakeupHandlers(vncBlockHandler, vncWakeupHandler, 0);

  } catch (rdr::Exception& e) {
    vlog.error("vncExtInit: %s",e.str());
  }
}

static void vncResetProc(ExtensionEntry* extEntry)
{
}

//
// vncBlockHandler - called just before the X server goes into select().  Call
// on to the block handler for each desktop.  Then check whether any of the
// selections have changed, and if so, notify any interested X clients.
//

static void vncBlockHandler(pointer data, OSTimePtr timeout, pointer readmask)
{
  fd_set* fds = (fd_set*)readmask;

  for (int scr = 0; scr < screenInfo.numScreens; scr++) {
    if (desktop[scr]) {
      desktop[scr]->blockHandler(fds);
    }
  }

  if (nPrevSelections != NumCurrentSelections) {
    prevSelectionTimes
      = (TimeStamp*)xnfrealloc(prevSelectionTimes,
                               NumCurrentSelections * sizeof(TimeStamp));
    for (int i = nPrevSelections; i < NumCurrentSelections; i++) {
      prevSelectionTimes[i].months = 0;
      prevSelectionTimes[i].milliseconds = 0;
    }
    nPrevSelections = NumCurrentSelections;
  }
  for (int i = 0; i < NumCurrentSelections; i++) {
    if (CurrentSelections[i].lastTimeChanged.months
        != prevSelectionTimes[i].months ||
        CurrentSelections[i].lastTimeChanged.milliseconds
        != prevSelectionTimes[i].milliseconds)
    {
      SendSelectionChangeEvent(CurrentSelections[i].selection);
      prevSelectionTimes[i] = CurrentSelections[i].lastTimeChanged;
    }
  }
}

static void vncWakeupHandler(pointer data, int nfds, pointer readmask)
{
  fd_set* fds = (fd_set*)readmask;

  for (int scr = 0; scr < screenInfo.numScreens; scr++) {
    if (desktop[scr]) {
      desktop[scr]->wakeupHandler(fds, nfds);
    }
  }
}

static void vncClientStateChange(CallbackListPtr*, pointer, pointer p)
{
  ClientPtr client = ((NewClientInfoRec*)p)->client;
  if (client->clientState == ClientStateGone) {
    VncInputSelect** nextPtr = &vncInputSelectHead;
    for (VncInputSelect* cur = vncInputSelectHead; cur; cur = *nextPtr) {
      if (cur->client == client) {
        *nextPtr = cur->next;
        delete cur;
        continue;
      }
      nextPtr = &cur->next;
    }
  }
}

void vncBell()
{
  for (int scr = 0; scr < screenInfo.numScreens; scr++) {
    if (desktop[scr]) {
      desktop[scr]->bell();
    }
  }
}

void vncClientGone(int fd)
{
  if (fd == vncInetdSock) {
    fprintf(stderr,"inetdSock client gone\n");
    GiveUp(0);
  }
}

void vncClientCutText(const char* str, int len)
{
  delete [] clientCutText;
  clientCutText = new char[len];
  memcpy(clientCutText, str, len);
  clientCutTextLen = len;
  xVncExtClientCutTextNotifyEvent ev;
  ev.type = vncEventBase + VncExtClientCutTextNotify;
  for (VncInputSelect* cur = vncInputSelectHead; cur; cur = cur->next) {
    if (cur->mask & VncExtClientCutTextMask) {
      ev.sequenceNumber = cur->client->sequence;
      ev.window = cur->window;
      ev.time = GetTimeInMillis();
      if (cur->client->swapped) {
        int n;
        swaps(&ev.sequenceNumber, n);
        swapl(&ev.window, n);
        swapl(&ev.time, n);
      }
      WriteToClient(cur->client, sizeof(xVncExtClientCutTextNotifyEvent),
                    (char *)&ev);
    }
  }
}


static CARD32 queryConnectTimerCallback(OsTimerPtr timer,
                                        CARD32 now, pointer arg)
{
  if (queryConnectTimeout)
    queryConnectDesktop->approveConnection(queryConnectId, false, "The attempt to prompt the user to accept the connection failed");
  // Re-notify clients, causing them to discover that we're done
  vncQueryConnect(queryConnectDesktop, queryConnectId);
  return 0;
}

void vncQueryConnect(XserverDesktop* desktop, void* opaqueId)
{
  // Only one query can be processed at any one time
  if (queryConnectTimeout && ((desktop != queryConnectDesktop) ||
                              (opaqueId != queryConnectId))) {
    desktop->approveConnection(opaqueId, false,
                               "Another connection is currently being queried.");
    return;
  }

  // Get the query timeout.  If it's zero, there is no query.
  queryConnectTimeout = desktop->getQueryTimeout(opaqueId);
  queryConnectId = queryConnectTimeout ? opaqueId : 0;
  queryConnectDesktop = queryConnectTimeout ? desktop : 0;

  // Notify clients
  bool notified = false;
  xVncExtQueryConnectNotifyEvent ev;
  ev.type = vncEventBase + VncExtQueryConnectNotify;
  for (VncInputSelect* cur = vncInputSelectHead; cur; cur = cur->next) {
    if (cur->mask & VncExtQueryConnectMask) {
      ev.sequenceNumber = cur->client->sequence;
      ev.window = cur->window;
      if (cur->client->swapped) {
        int n;
        swaps(&ev.sequenceNumber, n);
        swapl(&ev.window, n);
      }
      WriteToClient(cur->client, sizeof(xVncExtQueryConnectNotifyEvent),
                    (char *)&ev);
      notified = true;
    }
  }

  // If we're being asked to query a connection (rather than to cancel
  //   a query), and haven't been able to notify clients then reject it.
  if (queryConnectTimeout && !notified) {
    queryConnectTimeout = 0;
    queryConnectId = 0;
    queryConnectDesktop = 0;
    desktop->approveConnection(opaqueId, false,
                               "Unable to query the local user to accept the connection.");
    return;
  }    

  // Set a timer so that if no-one ever responds, we will eventually 
  //   reject the connection
  //   NB: We don't set a timer if sock is null, since that indicates
  //       that pending queries should be cancelled.
  if (queryConnectDesktop)
    queryConnectTimer = TimerSet(queryConnectTimer, 0,
                                 queryConnectTimeout*2000,
                                 queryConnectTimerCallback, 0);
  else
    TimerCancel(queryConnectTimer);
}

static void SendSelectionChangeEvent(Atom selection)
{
  xVncExtSelectionChangeNotifyEvent ev;
  ev.type = vncEventBase + VncExtSelectionChangeNotify;
  for (VncInputSelect* cur = vncInputSelectHead; cur; cur = cur->next) {
    if (cur->mask & VncExtSelectionChangeMask) {
      ev.sequenceNumber = cur->client->sequence;
      ev.window = cur->window;
      ev.selection = selection;
      if (cur->client->swapped) {
        int n;
        swaps(&ev.sequenceNumber, n);
        swapl(&ev.window, n);
        swapl(&ev.selection, n);
      }
      WriteToClient(cur->client, sizeof(xVncExtSelectionChangeNotifyEvent),
                    (char *)&ev);
    }
  }
}

static int ProcVncExtSetParam(ClientPtr client)
{
  REQUEST(xVncExtSetParamReq);
  REQUEST_FIXED_SIZE(xVncExtSetParamReq, stuff->paramLen);
  CharArray param(stuff->paramLen+1);
  strncpy(param.buf, (char*)&stuff[1], stuff->paramLen);
  param.buf[stuff->paramLen] = 0;

  xVncExtSetParamReply rep;
  int n;
  rep.type = X_Reply;
  rep.length = 0;
  rep.sequenceNumber = client->sequence;
  rep.success = rfb::Configuration::setParam(param.buf);
  if (client->swapped) {
    swaps(&rep.sequenceNumber, n);
    swapl(&rep.length, n);
  }
  WriteToClient(client, sizeof(xVncExtSetParamReply), (char *)&rep);
  return (client->noClientException);
}

static int SProcVncExtSetParam(ClientPtr client)
{
  register char n;
  REQUEST(xVncExtSetParamReq);
  swaps(&stuff->length, n);
  REQUEST_AT_LEAST_SIZE(xVncExtSetParamReq);
  return ProcVncExtSetParam(client);
}

static int ProcVncExtGetParam(ClientPtr client)
{
  REQUEST(xVncExtGetParamReq);
  REQUEST_FIXED_SIZE(xVncExtGetParamReq, stuff->paramLen);
  CharArray param(stuff->paramLen+1);
  strncpy(param.buf, (char*)&stuff[1], stuff->paramLen);
  param.buf[stuff->paramLen] = 0;

  xVncExtGetParamReply rep;
  int n;
  rep.type = X_Reply;
  rep.sequenceNumber = client->sequence;
  rep.success = 0;
  int len = 0;
  char* value = 0;
  rfb::VoidParameter* p = rfb::Configuration::getParam(param.buf);
  // Hack to avoid exposing password!
  if (strcasecmp(param.buf, "Password") == 0)
    p = 0;
  if (p) {
    value = p->getValueStr();
    rep.success = 1;
    len = value ? strlen(value) : 0;
  }
  rep.length = (len + 3) >> 2;
  rep.valueLen = len;
  if (client->swapped) {
    swaps(&rep.sequenceNumber, n);
    swapl(&rep.length, n);
    swaps(&rep.valueLen, n);
  }
  WriteToClient(client, sizeof(xVncExtGetParamReply), (char *)&rep);
  if (value)
    WriteToClient(client, len, value);
  delete [] value;
  return (client->noClientException);
}

static int SProcVncExtGetParam(ClientPtr client)
{
  register char n;
  REQUEST(xVncExtGetParamReq);
  swaps(&stuff->length, n);
  REQUEST_AT_LEAST_SIZE(xVncExtGetParamReq);
  return ProcVncExtGetParam(client);
}

static int ProcVncExtGetParamDesc(ClientPtr client)
{
  REQUEST(xVncExtGetParamDescReq);
  REQUEST_FIXED_SIZE(xVncExtGetParamDescReq, stuff->paramLen);
  CharArray param(stuff->paramLen+1);
  strncpy(param.buf, (char*)&stuff[1], stuff->paramLen);
  param.buf[stuff->paramLen] = 0;

  xVncExtGetParamDescReply rep;
  int n;
  rep.type = X_Reply;
  rep.sequenceNumber = client->sequence;
  rep.success = 0;
  int len = 0;
  const char* desc = 0;
  rfb::VoidParameter* p = rfb::Configuration::getParam(param.buf);
  if (p) {
    desc = p->getDescription();
    rep.success = 1;
    len = desc ? strlen(desc) : 0;
  }
  rep.length = (len + 3) >> 2;
  rep.descLen = len;
  if (client->swapped) {
    swaps(&rep.sequenceNumber, n);
    swapl(&rep.length, n);
    swaps(&rep.descLen, n);
  }
  WriteToClient(client, sizeof(xVncExtGetParamDescReply), (char *)&rep);
  if (desc)
    WriteToClient(client, len, (char*)desc);
  return (client->noClientException);
}

static int SProcVncExtGetParamDesc(ClientPtr client)
{
  register char n;
  REQUEST(xVncExtGetParamDescReq);
  swaps(&stuff->length, n);
  REQUEST_AT_LEAST_SIZE(xVncExtGetParamDescReq);
  return ProcVncExtGetParamDesc(client);
}

static int ProcVncExtListParams(ClientPtr client)
{
  REQUEST(xVncExtListParamsReq);
  REQUEST_SIZE_MATCH(xVncExtListParamsReq);

  xVncExtListParamsReply rep;
  int n;
  rep.type = X_Reply;
  rep.sequenceNumber = client->sequence;

  int nParams = 0;
  int len = 0;
  for (ParameterIterator i(Configuration::global()); i.param; i.next()) {
    int l = strlen(i.param->getName());
    if (l <= 255) {
      nParams++;
      len += l + 1;
    }
  }
  rep.length = (len + 3) >> 2;
  rep.nParams = nParams;
  if (client->swapped) {
    swaps(&rep.sequenceNumber, n);
    swapl(&rep.length, n);
    swaps(&rep.nParams, n);
  }
  WriteToClient(client, sizeof(xVncExtListParamsReply), (char *)&rep);
  rdr::U8* data = new rdr::U8[len];
  rdr::U8* ptr = data;
  for (ParameterIterator i(Configuration::global()); i.param; i.next()) {
    int l = strlen(i.param->getName());
    if (l <= 255) {
      *ptr++ = l;
      memcpy(ptr, i.param->getName(), l);
      ptr += l;
    }
  }
  WriteToClient(client, len, (char*)data);
  delete [] data;
  return (client->noClientException);
}

static int SProcVncExtListParams(ClientPtr client)
{
  register char n;
  REQUEST(xVncExtListParamsReq);
  swaps(&stuff->length, n);
  REQUEST_SIZE_MATCH(xVncExtListParamsReq);
  return ProcVncExtListParams(client);
}

static int ProcVncExtSetServerCutText(ClientPtr client)
{
  REQUEST(xVncExtSetServerCutTextReq);
  REQUEST_FIXED_SIZE(xVncExtSetServerCutTextReq, stuff->textLen);
  char* str = new char[stuff->textLen+1];
  strncpy(str, (char*)&stuff[1], stuff->textLen);
  str[stuff->textLen] = 0;
  for (int scr = 0; scr < screenInfo.numScreens; scr++) {
    if (desktop[scr]) {
      desktop[scr]->serverCutText(str, stuff->textLen);
    }
  }
  delete [] str;
  return (client->noClientException);
}

static int SProcVncExtSetServerCutText(ClientPtr client)
{
  register char n;
  REQUEST(xVncExtSetServerCutTextReq);
  swaps(&stuff->length, n);
  REQUEST_AT_LEAST_SIZE(xVncExtSetServerCutTextReq);
  swapl(&stuff->textLen, n);
  return ProcVncExtSetServerCutText(client);
}

static int ProcVncExtGetClientCutText(ClientPtr client)
{
  REQUEST(xVncExtGetClientCutTextReq);
  REQUEST_SIZE_MATCH(xVncExtGetClientCutTextReq);

  xVncExtGetClientCutTextReply rep;
  int n;
  rep.type = X_Reply;
  rep.length = (clientCutTextLen + 3) >> 2;
  rep.sequenceNumber = client->sequence;
  rep.textLen = clientCutTextLen;
  if (client->swapped) {
    swaps(&rep.sequenceNumber, n);
    swapl(&rep.length, n);
    swapl(&rep.textLen, n);
  }
  WriteToClient(client, sizeof(xVncExtGetClientCutTextReply), (char *)&rep);
  if (clientCutText)
    WriteToClient(client, clientCutTextLen, clientCutText);
  return (client->noClientException);
}

static int SProcVncExtGetClientCutText(ClientPtr client)
{
  register char n;
  REQUEST(xVncExtGetClientCutTextReq);
  swaps(&stuff->length, n);
  REQUEST_SIZE_MATCH(xVncExtGetClientCutTextReq);
  return ProcVncExtGetClientCutText(client);
}

static int ProcVncExtSelectInput(ClientPtr client)
{
  REQUEST(xVncExtSelectInputReq);
  REQUEST_SIZE_MATCH(xVncExtSelectInputReq);
  VncInputSelect** nextPtr = &vncInputSelectHead;
  VncInputSelect* cur;
  for (cur = vncInputSelectHead; cur; cur = *nextPtr) {
    if (cur->client == client && cur->window == stuff->window) {
      cur->mask = stuff->mask;
      if (!cur->mask) {
        *nextPtr = cur->next;
        delete cur;
      }
      break;
    }
    nextPtr = &cur->next;
  }
  if (!cur) {
    cur = new VncInputSelect(client, stuff->window, stuff->mask);
  }
  return (client->noClientException);
}

static int SProcVncExtSelectInput(ClientPtr client)
{
  register char n;
  REQUEST(xVncExtSelectInputReq);
  swaps(&stuff->length, n);
  REQUEST_SIZE_MATCH(xVncExtSelectInputReq);
  swapl(&stuff->window, n);
  swapl(&stuff->mask, n);
  return ProcVncExtSelectInput(client);
}

static int ProcVncExtConnect(ClientPtr client)
{
  REQUEST(xVncExtConnectReq);
  REQUEST_FIXED_SIZE(xVncExtConnectReq, stuff->strLen);
  CharArray str(stuff->strLen+1);
  strncpy(str.buf, (char*)&stuff[1], stuff->strLen);
  str.buf[stuff->strLen] = 0;

  xVncExtConnectReply rep;
  rep.success = 0;
  if (desktop[0]) {
    if (stuff->strLen == 0) {
      try {
        desktop[0]->disconnectClients();
        rep.success = 1;
      } catch (rdr::Exception& e) {
        vlog.error("Disconnecting all clients: %s",e.str());
      }
    } else {
      int port = 5500;
      for (int i = 0; i < stuff->strLen; i++) {
        if (str.buf[i] == ':') {
          port = atoi(&str.buf[i+1]);
          str.buf[i] = 0;
          break;
        }
      }

      try {
        network::Socket* sock = new network::TcpSocket(str.buf, port);
        desktop[0]->addClient(sock, true);
	rep.success = 1;
      } catch (rdr::Exception& e) {
        vlog.error("Reverse connection: %s",e.str());
      }
    }
  }

  rep.type = X_Reply;
  rep.length = 0;
  rep.sequenceNumber = client->sequence;
  if (client->swapped) {
    int n;
    swaps(&rep.sequenceNumber, n);
    swapl(&rep.length, n);
  }
  WriteToClient(client, sizeof(xVncExtConnectReply), (char *)&rep);
  return (client->noClientException);
}

static int SProcVncExtConnect(ClientPtr client)
{
  register char n;
  REQUEST(xVncExtConnectReq);
  swaps(&stuff->length, n);
  REQUEST_AT_LEAST_SIZE(xVncExtConnectReq);
  return ProcVncExtConnect(client);
}


static int ProcVncExtGetQueryConnect(ClientPtr client)
{
  REQUEST(xVncExtGetQueryConnectReq);
  REQUEST_SIZE_MATCH(xVncExtGetQueryConnectReq);

  const char *qcAddress=0, *qcUsername=0;
  int qcTimeout;
  if (queryConnectDesktop)
    qcTimeout = queryConnectDesktop->getQueryTimeout(queryConnectId,
                                                     &qcAddress, &qcUsername);
  else
    qcTimeout = 0;

  xVncExtGetQueryConnectReply rep;
  int n;
  rep.type = X_Reply;
  rep.sequenceNumber = client->sequence;
  rep.timeout = qcTimeout;
  rep.addrLen = qcTimeout ? strlen(qcAddress) : 0;
  rep.userLen = qcTimeout ? strlen(qcUsername) : 0;
  rep.opaqueId = (CARD32)queryConnectId;
  rep.length = (rep.userLen + rep.addrLen + 3) >> 2;
  if (client->swapped) {
    swaps(&rep.sequenceNumber, n);
    swapl(&rep.userLen, n);
    swapl(&rep.addrLen, n);
    swapl(&rep.timeout, n);
    swapl(&rep.opaqueId, n);
  }
  WriteToClient(client, sizeof(xVncExtGetQueryConnectReply), (char *)&rep);
  if (qcTimeout)
    WriteToClient(client, strlen(qcAddress), (char*)qcAddress);
  if (qcTimeout)
    WriteToClient(client, strlen(qcUsername), (char*)qcUsername);
  return (client->noClientException);
}

static int SProcVncExtGetQueryConnect(ClientPtr client)
{
  register char n;
  REQUEST(xVncExtGetQueryConnectReq);
  swaps(&stuff->length, n);
  REQUEST_SIZE_MATCH(xVncExtGetQueryConnectReq);
  return ProcVncExtGetQueryConnect(client);
}


static int ProcVncExtApproveConnect(ClientPtr client)
{
  REQUEST(xVncExtApproveConnectReq);
  REQUEST_SIZE_MATCH(xVncExtApproveConnectReq);
  if (queryConnectId == (void*)stuff->opaqueId) {
    for (int scr = 0; scr < screenInfo.numScreens; scr++) {
      if (desktop[scr]) {
        desktop[scr]->approveConnection(queryConnectId, stuff->approve,
                                        "Connection rejected by local user");
      }
    }
    // Inform other clients of the event and tidy up
    vncQueryConnect(queryConnectDesktop, queryConnectId);
  }
  return (client->noClientException);
}

static int SProcVncExtApproveConnect(ClientPtr client)
{
  register char n;
  REQUEST(xVncExtApproveConnectReq);
  swaps(&stuff->length, n);
  swapl(&stuff->opaqueId, n);
  REQUEST_SIZE_MATCH(xVncExtApproveConnectReq);
  return ProcVncExtApproveConnect(client);
}


static int ProcVncExtDispatch(ClientPtr client)
{
  REQUEST(xReq);
  switch (stuff->data) {
  case X_VncExtSetParam:
    return ProcVncExtSetParam(client);
  case X_VncExtGetParam:
    return ProcVncExtGetParam(client);
  case X_VncExtGetParamDesc:
    return ProcVncExtGetParamDesc(client);
  case X_VncExtListParams:
    return ProcVncExtListParams(client);
  case X_VncExtSetServerCutText:
    return ProcVncExtSetServerCutText(client);
  case X_VncExtGetClientCutText:
    return ProcVncExtGetClientCutText(client);
  case X_VncExtSelectInput:
    return ProcVncExtSelectInput(client);
  case X_VncExtConnect:
    return ProcVncExtConnect(client);
  case X_VncExtGetQueryConnect:
    return ProcVncExtGetQueryConnect(client);
  case X_VncExtApproveConnect:
    return ProcVncExtApproveConnect(client);
  default:
    return BadRequest;
  }
}

static int SProcVncExtDispatch(ClientPtr client)
{
  REQUEST(xReq);
  switch (stuff->data) {
  case X_VncExtSetParam:
    return SProcVncExtSetParam(client);
  case X_VncExtGetParam:
    return SProcVncExtGetParam(client);
  case X_VncExtGetParamDesc:
    return SProcVncExtGetParamDesc(client);
  case X_VncExtListParams:
    return SProcVncExtListParams(client);
  case X_VncExtSetServerCutText:
    return SProcVncExtSetServerCutText(client);
  case X_VncExtGetClientCutText:
    return SProcVncExtGetClientCutText(client);
  case X_VncExtSelectInput:
    return SProcVncExtSelectInput(client);
  case X_VncExtConnect:
    return SProcVncExtConnect(client);
  case X_VncExtGetQueryConnect:
    return SProcVncExtGetQueryConnect(client);
  case X_VncExtApproveConnect:
    return SProcVncExtApproveConnect(client);
  default:
    return BadRequest;
  }
}

