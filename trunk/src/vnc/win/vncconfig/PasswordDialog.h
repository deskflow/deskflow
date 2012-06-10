/* Copyright (C) 2004-2005 RealVNC Ltd.  All Rights Reserved.
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
#ifndef WINVNCCONF_PASSWORD_DIALOG
#define WINVNCCONF_PASSWORD_DIALOG

#include <rfb_win32/Registry.h>
#include <rfb_win32/Dialog.h>

namespace rfb {
  namespace win32 {

    class PasswordDialog : public Dialog {
    public:
      PasswordDialog(const RegKey& rk, bool registryInsecure_);
      bool showDialog(HWND owner=0);
      bool onOk();
    protected:
      const RegKey& regKey;
      bool registryInsecure;
    };

  };
};

#endif