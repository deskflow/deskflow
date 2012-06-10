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

// -=- AboutDialog.h

#ifndef __RFB_WIN32_ABOUT_DIALOG_H__
#define __RFB_WIN32_ABOUT_DIALOG_H__

#include <rfb_win32/Dialog.h>
#include <rfb/util.h>

extern const char* buildTime;

namespace rfb {

  namespace win32 {

    class AboutDialog : Dialog {
    public:
      AboutDialog();
      virtual bool showDialog();
      virtual void initDialog();

      static AboutDialog instance;

      typedef WORD LabelId;
      static const LabelId DialogId;    // Resource ID of the About dialog
      static const LabelId BuildTime;   // Resource ID of the BuildTime label in the dialog
      static const LabelId Version;     // etc...
      static const LabelId Copyright;
      static const LabelId Description;
    protected:
      WORD dialogId;
    };

  };

};

#endif
