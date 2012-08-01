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

// -=- RegConfig.cxx

#include <malloc.h>

#include <rfb_win32/RegConfig.h>
#include <rfb/LogWriter.h>
#include <rfb/util.h>
//#include <rdr/HexOutStream.h>

using namespace rfb;
using namespace rfb::win32;


static LogWriter vlog("RegConfig");


RegConfig::RegConfig(EventManager* em) : eventMgr(em), event(CreateEvent(0, TRUE, FALSE, 0)), callback(0) {
  if (em->addEvent(event, this))
    eventMgr = em;
}

RegConfig::~RegConfig() {
  if (eventMgr)
    eventMgr->removeEvent(event);
}

bool RegConfig::setKey(const HKEY rootkey, const TCHAR* keyname) {
  try {
    key.createKey(rootkey, keyname);
    processEvent(event);
    return true;
  } catch (rdr::Exception& e) {
    vlog.debug(e.str());
    return false;
  }
}

void RegConfig::loadRegistryConfig(RegKey& key) {
  DWORD i = 0;
  try {
    while (1) {
      TCharArray name = tstrDup(key.getValueName(i++));
      if (!name.buf) break;
      TCharArray value = key.getRepresentation(name.buf);
      if (!value.buf || !Configuration::setParam(CStr(name.buf), CStr(value.buf)))
        vlog.info("unable to process %s", CStr(name.buf));
    }
  } catch (rdr::SystemException& e) {
    if (e.err != 6)
      vlog.error(e.str());
  }
}

void RegConfig::processEvent(HANDLE event_) {
  vlog.info("registry changed");

  // Reinstate the registry change notifications
  ResetEvent(event);
  key.awaitChange(true, REG_NOTIFY_CHANGE_NAME | REG_NOTIFY_CHANGE_LAST_SET, event);

  // Load settings
  loadRegistryConfig(key);

  // Notify the callback, if supplied
  if (callback)
    callback->regConfigChanged();
}


RegConfigThread::RegConfigThread() : Thread("RegConfigThread"), config(&eventMgr) {
}

RegConfigThread::~RegConfigThread() {
  join();
}

bool RegConfigThread::start(const HKEY rootKey, const TCHAR* keyname) {
  if (config.setKey(rootKey, keyname)) {
    Thread::start();
    return true;
  }
  return false;
}

void RegConfigThread::run() {
  DWORD result = 0;
  MSG msg;
  while ((result = eventMgr.getMessage(&msg, 0, 0, 0)) > 0) {}
  if (result < 0)
    throw rdr::SystemException("RegConfigThread failed", GetLastError());
}

Thread* RegConfigThread::join() {
  PostThreadMessage(getThreadId(), WM_QUIT, 0, 0);
  return Thread::join();
}
