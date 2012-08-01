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
#include <rfb/Blacklist.h>
#include <rfb/Configuration.h>

using namespace rfb;

IntParameter Blacklist::threshold("BlacklistThreshold",
                              "The number of unauthenticated connection attempts allowed from any "
                              "individual host before that host is black-listed",
                              5);
IntParameter Blacklist::initialTimeout("BlacklistTimeout",
                              "The initial timeout applied when a host is first black-listed.  "
                              "The host cannot re-attempt a connection until the timeout expires.",
                              10);


Blacklist::Blacklist() {
}

Blacklist::~Blacklist() {
  // Free the map keys
  BlacklistMap::iterator i;
  for (i=blm.begin(); i!=blm.end(); i++) {
    strFree((char*)(*i).first);
  }
}

bool Blacklist::isBlackmarked(const char* name) {
  BlacklistMap::iterator i = blm.find(name);
  if (i == blm.end()) {
    // Entry is not already black-marked.
    // Create the entry unmarked, unblocked,
    // with suitable defaults set.
    BlacklistInfo bi;
    bi.marks = 1;
    bi.blockUntil = 0;
    bi.blockTimeout = initialTimeout;
    blm[strDup(name)] = bi;
    i = blm.find(name);
  }

  // Entry exists - has it reached the threshold yet?
  if ((*i).second.marks >= threshold) {
    // Yes - entry is blocked - has the timeout expired?        
    time_t now = time(0);
    if (now >= (*i).second.blockUntil) {
      // Timeout has expired.  Reset timeout and allow
      // a re-try.
      (*i).second.blockUntil = now + (*i).second.blockTimeout;
      (*i).second.blockTimeout = (*i).second.blockTimeout * 2;
      return false;
    }
    // Blocked and timeout still in effect - reject!
    return true;
  }

  // We haven't reached the threshold yet.
  // Increment the black-mark counter but allow
  // the entry to pass.
  (*i).second.marks++;
  return false;
}

void Blacklist::clearBlackmark(const char* name) {
  BlacklistMap::iterator i = blm.find(name);
  if (i != blm.end()) {
    strFree((char*)(*i).first);
    blm.erase(i);
  }
}
