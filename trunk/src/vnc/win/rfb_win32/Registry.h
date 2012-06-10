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
// -=- Registry.h

// C++ wrappers around the Win32 Registry APIs

#ifndef __RFB_WIN32_REGISTRY_H__
#define __RFB_WIN32_REGISTRY_H__

#include <windows.h>
#include <rfb_win32/Security.h>
#include <rfb/util.h>

namespace rfb {

  namespace win32 {

    class RegKey {
    public:
      // No key open
      RegKey();

      // Duplicate the specified existing key
      RegKey(const HKEY k);
      RegKey(const RegKey& k);

      // Calls close() internally
      ~RegKey();

      void setHKEY(HKEY key, bool freeKey);
    private:
      RegKey& operator=(const RegKey& k);
      HKEY& operator=(const HKEY& k);
    public:

      // Returns true if key was created, false if already existed
      bool createKey(const RegKey& root, const TCHAR* name);

      // Opens key if it exists, or raises an exception if not
      void openKey(const RegKey& root, const TCHAR* name, bool readOnly=false);

      // Set the (discretionary) access control list for the key
      void setDACL(const PACL acl, bool inheritFromParent=true);

      // Closes current key, if required
      void close();

      // Delete a subkey/value
      void deleteKey(const TCHAR* name) const;
      void deleteValue(const TCHAR* name) const;


      // Block waiting for a registry change, OR return immediately and notify the
      // event when there is a change, if specified
      void awaitChange(bool watchSubTree, DWORD filter, HANDLE event=0) const;

      void setExpandString(const TCHAR* valname, const TCHAR* s) const;
      void setString(const TCHAR* valname, const TCHAR* s) const;
      void setBinary(const TCHAR* valname, const void* data, int length) const;
      void setInt(const TCHAR* valname, int i) const;
      void setBool(const TCHAR* valname, bool b) const;

      TCHAR* getString(const TCHAR* valname) const;
      TCHAR* getString(const TCHAR* valname, const TCHAR* def) const;

      void getBinary(const TCHAR* valname, void** data, int* length) const;
      void getBinary(const TCHAR* valname, void** data, int* length, void* def, int deflength) const;

      int getInt(const TCHAR* valname) const;
      int getInt(const TCHAR* valname, int def) const;

      bool getBool(const TCHAR* valname) const;
      bool getBool(const TCHAR* valname, bool def) const;

      TCHAR* getRepresentation(const TCHAR* valname) const;

      bool isValue(const TCHAR* valname) const;

      // Get the name of value/key number "i"
      // If there are fewer than "i" values then return 0
      // NAME IS OWNED BY RegKey OBJECT!
      const TCHAR* getValueName(int i);
      const TCHAR* getKeyName(int i);

      operator HKEY() const;
    protected:
      HKEY key;
      bool freeKey;
      TCharArray valueName;
      DWORD valueNameBufLen;
    };

  };

};

#endif // __RFB_WIN32_REG_CONFIG_H__
