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
// ConnParams - structure containing the connection parameters.
//

#ifndef __RFB_CONNPARAMS_H__
#define __RFB_CONNPARAMS_H__

#include <rdr/types.h>
#include <rfb/PixelFormat.h>

namespace rdr { class InStream; }

namespace rfb {

  class ConnParams {
  public:
    ConnParams();
    ~ConnParams();

    bool readVersion(rdr::InStream* is, bool* done);
    void writeVersion(rdr::OutStream* os);

    int majorVersion;
    int minorVersion;

    void setVersion(int major, int minor) {
      majorVersion = major; minorVersion = minor;
    }
    bool isVersion(int major, int minor) {
      return majorVersion == major && minorVersion == minor;
    }
    bool beforeVersion(int major, int minor) {
      return (majorVersion < major ||
              (majorVersion == major && minorVersion < minor));
    }
    bool afterVersion(int major, int minor) {
      return !beforeVersion(major,minor+1);
    }

    int width;
    int height;

    const PixelFormat& pf() { return pf_; }
    void setPF(const PixelFormat& pf);

    const char* name() { return name_; }
    void setName(const char* name);

    rdr::U32 currentEncoding() { return currentEncoding_; }
    int nEncodings() { return nEncodings_; }
    const rdr::U32* encodings() { return encodings_; }
    void setEncodings(int nEncodings, const rdr::U32* encodings);
    bool useCopyRect;

    bool supportsLocalCursor;
    bool supportsDesktopResize;

  private:

    PixelFormat pf_;
    char* name_;
    int nEncodings_;
    rdr::U32* encodings_;
    int currentEncoding_;
    char verStr[13];
    int verStrPos;
  };
}
#endif
