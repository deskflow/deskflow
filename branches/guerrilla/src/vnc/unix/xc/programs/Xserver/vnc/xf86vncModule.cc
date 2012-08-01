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
/*  This is the xf86 module code for the vnc extension.
 */

#include <rfb/Configuration.h>
#include <rfb/Logger_stdio.h>
#include <rfb/LogWriter.h>

extern "C" {
#define class c_class
#define private c_private
#define bool c_bool
#define new c_new
#include "xf86.h"
#include "xf86Module.h"
#undef class
#undef private
#undef bool
#undef new

using namespace rfb;

extern void vncExtensionInit();
static void vncExtensionInitWithParams(INITARGS);

#ifdef XFree86LOADER

static MODULESETUPPROTO(vncSetup);

ExtensionModule vncExt =
{
    vncExtensionInitWithParams,
    "VNC",
    NULL,
    NULL,
    NULL
};

static XF86ModuleVersionInfo vncVersRec =
{
    "vnc",
    "RealVNC Ltd",
    MODINFOSTRING1,
    MODINFOSTRING2,
    XF86_VERSION_CURRENT,
    1, 0, 0,
    ABI_CLASS_EXTENSION,         /* needs the server extension ABI */
    ABI_EXTENSION_VERSION,
    MOD_CLASS_EXTENSION,
    {0,0,0,0}
};

XF86ModuleData vncModuleData = { &vncVersRec, vncSetup, NULL };

static pointer
vncSetup(pointer module, pointer opts, int *errmaj, int *errmin) {
    LoadExtension(&vncExt, FALSE);
    /* Need a non-NULL return value to indicate success */
    return (pointer)1;
}

static void vncExtensionInitWithParams(INITARGS)
{
  rfb::initStdIOLoggers();
  rfb::LogWriter::setLogParams("*:stderr:30");

  for (int scr = 0; scr < screenInfo.numScreens; scr++) {
    ScrnInfoPtr pScrn = xf86Screens[scr];

    for (ParameterIterator i(Configuration::global()); i.param; i.next()) {
      char* val = xf86FindOptionValue(pScrn->options, i.param->getName());
      if (val)
        i.param->setParam(val);
    }
  }

  vncExtensionInit();
}

#endif /* XFree86LOADER */
}
