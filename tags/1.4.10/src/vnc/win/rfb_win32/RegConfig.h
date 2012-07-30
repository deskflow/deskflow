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

// -=- RegConfig.h

// Class which monitors the registry and reads in the registry settings
// whenever they change, or are added or removed.

#ifndef __RFB_WIN32_REG_CONFIG_H__
#define __RFB_WIN32_REG_CONFIG_H__

#include <rfb/Threading.h>
#include <rfb/Configuration.h>
#include <rfb_win32/Registry.h>
#include <rfb_win32/EventManager.h>
#include <rfb_win32/Handle.h>

namespace rfb {

  namespace win32 {

    class RegConfig : EventHandler {
    public:
      RegConfig(EventManager* em);
      ~RegConfig();

      // Specify the registry key to read Configuration items from
      bool setKey(const HKEY rootkey, const TCHAR* keyname);

      // Support for a callback, run in the RegConfig host thread whenever
      // the registry configuration changes
      class Callback {
      public:
        virtual ~Callback() {}
        virtual void regConfigChanged() = 0;
      };
      void setCallback(Callback* cb) { callback = cb; }

      // Read entries from the specified key into the Configuration
      static void loadRegistryConfig(RegKey& key);
    protected:
      // EventHandler interface and trigger event
      virtual void processEvent(HANDLE event);

      EventManager* eventMgr;
      Handle event;
      Callback* callback;
      RegKey key;
    };

    class RegConfigThread : Thread {
    public:
      RegConfigThread();
      ~RegConfigThread();

      // Start the thread, reading from the specified key
      bool start(const HKEY rootkey, const TCHAR* keyname);
    protected:
      void run();
      Thread* join();
      EventManager eventMgr;
      RegConfig config;
    };

  };

};

#endif // __RFB_WIN32_REG_CONFIG_H__
