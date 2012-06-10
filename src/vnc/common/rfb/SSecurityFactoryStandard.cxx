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
// SSecurityFactoryStandard
//

#include <rfb/secTypes.h>
#include <rfb/SSecurityNone.h>
#include <rfb/Configuration.h>
#include <rfb/LogWriter.h>
#include <rfb/Exception.h>
#include <rfb/SSecurityFactoryStandard.h>
#include <rfb/Password.h>

using namespace rfb;

static LogWriter vlog("SSecurityFactoryStandard");

StringParameter SSecurityFactoryStandard::sec_types
("SecurityTypes",
 "Specify which security scheme to use for incoming connections (None, VncAuth)",
 "VncAuth");

StringParameter SSecurityFactoryStandard::rev_sec_types
("ReverseSecurityTypes",
 "Specify encryption scheme to use for reverse connections (None)",
 "None");


StringParameter SSecurityFactoryStandard::vncAuthPasswdFile
("PasswordFile", "Password file for VNC authentication", "");
VncAuthPasswdParameter SSecurityFactoryStandard::vncAuthPasswd
("Password", "Obfuscated binary encoding of the password which clients must supply to "
 "access the server", &SSecurityFactoryStandard::vncAuthPasswdFile);


SSecurity* SSecurityFactoryStandard::getSSecurity(rdr::U8 secType, bool reverseConnection) {
  switch (secType) {
  case secTypeNone: return new SSecurityNone();
  case secTypeVncAuth:
    return new SSecurityVncAuth(&vncAuthPasswd);
  default:
    throw Exception("Security type not supported");
  }
}

void SSecurityFactoryStandard::getSecTypes(std::list<rdr::U8>* secTypes, bool reverseConnection) {
  CharArray secTypesStr;
  if (reverseConnection)
    secTypesStr.buf = rev_sec_types.getData();
  else
    secTypesStr.buf = sec_types.getData();
  std::list<int> configured = parseSecTypes(secTypesStr.buf);
  std::list<int>::iterator i;
  for (i=configured.begin(); i!=configured.end(); i++) {
    if (isSecTypeSupported(*i))
      secTypes->push_back(*i);
  }
}

bool SSecurityFactoryStandard::isSecTypeSupported(rdr::U8 secType) {
  switch (secType) {
  case secTypeNone:
  case secTypeVncAuth:
    return true;
  default:
    return false;
  }
}


VncAuthPasswdParameter::VncAuthPasswdParameter(const char* name,
                                               const char* desc,
                                               StringParameter* passwdFile_)
: BinaryParameter(name, desc, 0, 0), passwdFile(passwdFile_) {
}

char* VncAuthPasswdParameter::getVncAuthPasswd() {
  ObfuscatedPasswd obfuscated;
  getData((void**)&obfuscated.buf, &obfuscated.length);

  if (obfuscated.length == 0) {
    if (passwdFile) {
      CharArray fname(passwdFile->getData());
      if (!fname.buf[0]) {
        vlog.info("neither %s nor %s params set", getName(), passwdFile->getName());
        return 0;
      }

      FILE* fp = fopen(fname.buf, "r");
      if (!fp) {
        vlog.error("opening password file '%s' failed",fname.buf);
        return 0;
      }

      vlog.debug("reading password file");
      obfuscated.buf = new char[128];
      obfuscated.length = fread(obfuscated.buf, 1, 128, fp);
      fclose(fp);
    } else {
      vlog.info("%s parameter not set", getName());
    }
  }

  try {
    PlainPasswd password(obfuscated);
    return password.takeBuf();
  } catch (...) {
    return 0;
  }
}


