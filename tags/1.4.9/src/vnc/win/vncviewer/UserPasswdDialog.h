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

// -=- UserPasswdDialog.h

// Username and password dialog for VNC Viewer 4.0
// Note that the password and username fields are only freed
// when the dialog instance is deleted - it is important to
// ensure that the instance is deleted as early as possible, to
// avoid the password being retained in memory for too long.

#ifndef __RFB_WIN32_USERPASSWD_DIALOG_H__
#define __RFB_WIN32_USERPASSWD_DIALOG_H__

#include <rfb_win32/Dialog.h>
#include <rfb_win32/TCharArray.h>
#include <rfb/CSecurity.h>
#include <rfb/UserPasswdGetter.h>

namespace rfb {

  namespace win32 {

    class UserPasswdDialog : Dialog, public UserPasswdGetter {
    public:
      UserPasswdDialog();
      virtual bool showDialog();
      virtual void initDialog();
      virtual bool onOk();
      virtual void getUserPasswd(char** user, char** passwd);
      void setCSecurity(const CSecurity* cs);
    protected:
      TCharArray username;
      TPlainPasswd password;
      bool showUsername, showPassword;
      TCharArray description;
    };

  };

};

#endif
