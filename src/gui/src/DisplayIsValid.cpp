#ifdef WINAPI_XWINDOWS

#include "DisplayIsValid.h"
#include <X11/Xlib.h>

bool display_is_valid()
{
    auto dsp = XOpenDisplay(NULL);
    if (dsp != NULL)
        XCloseDisplay(dsp);
    return dsp != NULL;
}

#endif
