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

//
// SSecurityFactoryStandard - implementation of the SSecurityFactory
// interface.
//
// Server implementations must define an instance of a
// VncAuthPasswdParameter-based class somewhere.  Any class based on
// VncAuthPasswdParameter will automatically register itself as the
// password parameter to be used by the Standard factory.
//
// Two implementations are provided here:
//
// VncAuthPasswdConfigParameter - reads the password from the Binary
//                                parameter "Password".
// VncAuthPasswdFileParameter   - reads the password from the file named
//                                in the String parameter "PasswordFile".
//
// This factory supports only the "None" and "VncAuth" security types.
//

#ifndef __RFB_SSECURITYFACTORY_STANDARD_H__
#define __RFB_SSECURITYFACTORY_STANDARD_H__

#include <rfb/SSecurityVncAuth.h>
#include <rfb/Configuration.h>
#include <rfb/util.h>

namespace rfb {

  class VncAuthPasswdParameter : public VncAuthPasswdGetter, BinaryParameter {
  public:
    VncAuthPasswdParameter(const char* name, const char* desc, StringParameter* passwdFile_);
    virtual char* getVncAuthPasswd();
  protected:
    StringParameter* passwdFile;
  };

  class SSecurityFactoryStandard : public SSecurityFactory {
  public:
    virtual SSecurity* getSSecurity(rdr::U8 secType, bool reverse);
    virtual void getSecTypes(std::list<rdr::U8>* secTypes, bool reverse);
    static StringParameter sec_types;
    static StringParameter rev_sec_types;
    static StringParameter vncAuthPasswdFile;
    static VncAuthPasswdParameter vncAuthPasswd;
  protected:
    virtual bool isSecTypeSupported(rdr::U8 secType);
  };

}
#endif
