#include "common.h"

#if defined(CONFIG_PLATFORM_WIN32)

#include "CWin32Platform.cpp"

#elif defined(CONFIG_PLATFORM_UNIX)

#include "CUnixPlatform.cpp"

#endif
