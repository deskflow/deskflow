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
#ifndef __RFB_USERPASSWDGETTER_H__
#define __RFB_USERPASSWDGETTER_H__
namespace rfb {
  class UserPasswdGetter {
  public:
    // getUserPasswd gets the username and password.  This might involve a
    // dialog, getpass(), etc.  The user buffer pointer can be null, in which
    // case no user name will be retrieved.  The caller MUST delete [] the
    // result(s).
    virtual void getUserPasswd(char** user, char** password)=0;
  };
}
#endif
