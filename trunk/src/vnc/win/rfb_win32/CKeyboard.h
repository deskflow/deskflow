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

// -=- CKeyboard.h
//
// Client-side keyboard handling for Win32

#ifndef __RFB_WIN32_CKEYBOARD_H__
#define __RFB_WIN32_CKEYBOARD_H__

#include <rfb/InputHandler.h>
#include <map>

namespace rfb {

  namespace win32 {

    class CKeyboard {
    public:
      void keyEvent(InputHandler* writer, rdr::U8 vkey, rdr::U32 flags,
                    bool down);
      void releaseAllKeys(InputHandler* writer);
      const std::map<int,rdr::U32>& pressedKeys() const {return downKeysym;};
      bool keyPressed(int k) const {return downKeysym.find(k)!=downKeysym.end();}
    private:
      void win32::CKeyboard::releaseKey(InputHandler* writer, int extendedVkey);
      void win32::CKeyboard::pressKey(InputHandler* writer, int extendedVkey,
                                      rdr::U32 keysym);
      std::map<int,rdr::U32> downKeysym;
    };

  }; // win32

}; // rfb

#endif // __RFB_WIN32_CKEYBOARD_H__
