#ifndef CPLATFORM_H
#define CPLATFORM_H

#include "common.h"

#if WINDOWS_LIKE

#include "CWin32Platform.h"
typedef CWin32Platform CPlatform;

#elif UNIX_LIKE

#include "CUnixPlatform.h"
typedef CUnixPlatform CPlatform;

#else

#error Unsupported platform

#endif

#endif
