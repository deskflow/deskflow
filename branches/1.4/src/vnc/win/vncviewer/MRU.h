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
#ifndef __VIEWER_MRU_H__
#define __VIEWER_MRU_H__

#include <windows.h>
#include <list>
#include <set>
#include <rfb_win32/Registry.h>
#include <rfb/util.h>
#include <rdr/HexOutStream.h>

namespace rfb {

  namespace win32 {

    namespace MRU {

      static const RegKey RegRoot = HKEY_CURRENT_USER;
      static const TCHAR* RegPath = _T("Software\\RealVNC\\VNCViewer4\\MRU");
      static const int MaxMRUEntries = 256;
      static const int MRUEntries = 10;

      static std::list<char*> getEntries() {
        std::list<char*> mru;

        try {
          RegKey key;
          key.openKey(RegRoot, RegPath);

          CharArray order;
          int length;
          key.getBinary(_T("Order"), (void**)&order.buf, &length);

          for (int i=0; i<length; i++) {
            TCharArray keyname = rdr::HexOutStream::binToHexStr(&order.buf[i], 1);
            try {
              TCharArray entry = key.getString(keyname.buf);
              mru.push_back(strDup(entry.buf));
            } catch (rdr::Exception) {
            }
          }
        } catch (rdr::Exception) {
        }

        return mru;
      }

      static void addToMRU(const char* name) {
        RegKey key;
        key.createKey(RegRoot, RegPath);

        BYTE keycode;
        CharArray old_order;
        char order[MaxMRUEntries];
        int orderlen;
        
        try {
          key.getBinary(_T("Order"), (void**)&old_order.buf, &orderlen);
          if (orderlen)
            memcpy(order, old_order.buf, orderlen);

          std::set<int> ordercodes;
          keycode = 0;
          bool found = false;
          for (int i=0; i<orderlen; i++) {
            TCharArray keyname = rdr::HexOutStream::binToHexStr(&order[i], 1);
            try {
              TCharArray hostname = key.getString(keyname.buf);
              if (stricmp(name, CStr(hostname.buf)) == 0) {
                keycode = order[i];
                found = true;
                break;
              }
            } catch (rdr::Exception) {
            }
            ordercodes.insert(order[i]);
          }

          if (!found) {
            if (orderlen <= MRUEntries) {
              while (ordercodes.find(keycode) != ordercodes.end()) keycode++;
            } else {
              keycode = order[orderlen-1];
              orderlen--;
            }
          }

        } catch (rdr::Exception) {
          keycode = 0;
          orderlen = 0;
        }

        orderlen++;
        int i, j=orderlen-1;
        for (i=0; i<orderlen-1; i++) {
          if (order[i] == keycode) {
            j = i;
            orderlen--;
            break;
          }
        }
        for (i=j; i>0; i--)
          order[i] = order[i-1];
        order[0] = keycode;

        TCharArray keyname = rdr::HexOutStream::binToHexStr((char*)&keycode, 1);
        key.setString(keyname.buf, TStr(name));
        key.setBinary(_T("Order"), order, orderlen);
      }

    };

  };

};

#endif