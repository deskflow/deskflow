#ifndef CPLATFORM_H
#define CPLATFORM_H

#include "common.h"

#if defined(CONFIG_PLATFORM_WIN32)

#include "CWin32Platform.h"
typedef CWin32Platform CPlatform;

#elif defined(CONFIG_PLATFORM_UNIX)

#include "CUnixPlatform.h"
typedef CUnixPlatform CPlatform;

#endif

#endif
