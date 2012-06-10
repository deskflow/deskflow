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
// Blacklist.h - Handling of black-listed entities.
// Just keeps a table mapping strings to timing information, including
// how many times the entry has been black-listed and when to next
// put it on probation (e.g. allow a connection in from the host, and
// re-blacklist it if that fails). 
//

#ifndef __RFB_BLACKLIST_H__
#define __RFB_BLACKLIST_H__

#include <string.h>
#include <time.h>
#include <map>

#include <rfb/Configuration.h>
#include <rfb/util.h>

namespace rfb {

  //
  // -=- Blacklist handler
  //
  // Parameters include a threshold after which to blacklist the named
  // host, and a timeout after which to re-consider them.
  //
  // Threshold means that isBlackmarked can be called that number of times
  // before it will return true.
  //
  // Timeout means that after that many seconds, the next call to isBlackmarked
  // will return false.  At the same time, the timeout is doubled, so that the
  // next calls will fail, until the timeout expires again or clearBlackmark is
  // called.
  //
  // When clearBlackMark is called, the corresponding entry is completely
  // removed, causing the next isBlackmarked call to return false.

  // KNOWN BUG:  Client can keep making rejected requests, thus increasing
  // their timeout.  If client does this for 30 years, timeout may wrap round
  // to a very small value again.

  // THIS CLASS IS NOT THREAD-SAFE!

  class Blacklist {
  public:
    Blacklist();
    ~Blacklist();

    bool isBlackmarked(const char* name);
    void clearBlackmark(const char* name);

    static IntParameter threshold;
    static IntParameter initialTimeout;

  protected:
    struct ltStr {
      bool operator()(const char* s1, const char* s2) const {
        return strcmp(s1, s2) < 0;
      };
    };
    struct BlacklistInfo {
      int marks;
      time_t blockUntil;
      unsigned int blockTimeout;
    };
    typedef std::map<const char*,BlacklistInfo,ltStr> BlacklistMap;
    BlacklistMap blm;
  };

}

#endif

