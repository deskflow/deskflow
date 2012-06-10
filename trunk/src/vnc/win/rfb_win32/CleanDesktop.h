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

// -=- CleanDesktop.h

#ifndef __RFB_WIN32_CLEANDESKTOP_H__
#define __RFB_WIN32_CLEANDESKTOP_H__

#include <rfb_win32/TCharArray.h>

namespace rfb {

  namespace win32 {

    class CleanDesktop {
    public:
      CleanDesktop();
      ~CleanDesktop();

      void disableWallpaper();
      void enableWallpaper();

      void disablePattern();
      void enablePattern();

      void disableEffects();
      void enableEffects();

    private:
      bool restoreActiveDesktop;
      bool restoreWallpaper;
      bool restorePattern;
      bool restoreEffects;
      BOOL uiEffects;
      BOOL comboBoxAnim, gradientCaptions, hotTracking, listBoxSmoothScroll, menuAnim;
    };

  }; // win32

}; // rfb

#endif // __RFB_WIN32_CLEANDESKTOP_H__
