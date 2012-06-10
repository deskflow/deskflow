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

// Security.h

// Wrapper classes for a few Windows NT security structures/functions
// that are used by VNC

#ifndef __RFB_WIN32_SECURITY_H__
#define __RFB_WIN32_SECURITY_H__

#include <rdr/types.h>
#include <rfb_win32/LocalMem.h>
#include <rfb_win32/TCharArray.h>
#include <aclapi.h>

namespace rfb {

  namespace win32 {

    struct Trustee : public TRUSTEE {
      Trustee(const TCHAR* name,
              TRUSTEE_FORM form=TRUSTEE_IS_NAME,
              TRUSTEE_TYPE type=TRUSTEE_IS_UNKNOWN);
    };

    struct ExplicitAccess : public EXPLICIT_ACCESS {
      ExplicitAccess(const TCHAR* name,
                     TRUSTEE_FORM type,
                     DWORD perms,
                     ACCESS_MODE mode,
                     DWORD inherit=0);
    };

    // Helper class for building access control lists
    struct AccessEntries {
      AccessEntries();
      ~AccessEntries();
      void allocMinEntries(int count);
      void addEntry(const TCHAR* trusteeName,
                    DWORD permissions,
                    ACCESS_MODE mode);
      void addEntry(const PSID sid,
                    DWORD permissions,
                    ACCESS_MODE mode);

      EXPLICIT_ACCESS* entries;
      int entry_count;
    };

    // Helper class for handling SIDs
    struct Sid : rdr::U8Array {
      Sid() {}
      operator PSID() const {return (PSID)buf;}
      PSID takePSID() {PSID r = (PSID)buf; buf = 0; return r;}

      static PSID copySID(const PSID sid);

      void setSID(const PSID sid);

      void getUserNameAndDomain(TCHAR** name, TCHAR** domain);

      struct Administrators;
      struct SYSTEM;
      struct FromToken;

    private:
      Sid(const Sid&);
      Sid& operator=(const Sid&);
    };
      
    struct Sid::Administrators : public Sid {
      Administrators();
    };
    struct Sid::SYSTEM : public Sid {
      SYSTEM();
    };
    struct Sid::FromToken : public Sid {
      FromToken(HANDLE h);
    };

    // Helper class for handling & freeing ACLs
    struct AccessControlList : public LocalMem {
      AccessControlList(int size) : LocalMem(size) {}
      AccessControlList(PACL acl_=0) : LocalMem(acl_) {}
      operator PACL() {return (PACL)ptr;}
    };

    // Create a new ACL based on supplied entries and, if supplied, existing ACL 
    PACL CreateACL(const AccessEntries& ae, PACL existing_acl=0);

    // Helper class for memory-management of self-relative SecurityDescriptors
    struct SecurityDescriptorPtr : LocalMem {
      SecurityDescriptorPtr(int size) : LocalMem(size) {}
      SecurityDescriptorPtr(PSECURITY_DESCRIPTOR sd_=0) : LocalMem(sd_) {}
      PSECURITY_DESCRIPTOR takeSD() {return takePtr();}
    };

    // Create a new self-relative Security Descriptor, owned by SYSTEM/Administrators,
    //   with the supplied DACL and no SACL.  The returned value can be assigned
    //   to a SecurityDescriptorPtr to be managed.
    PSECURITY_DESCRIPTOR CreateSdWithDacl(const PACL dacl);

  }

}

#endif
