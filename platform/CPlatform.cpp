#include "common.h"

#if WINDOWS_LIKE

#include "CWin32Platform.cpp"

#elif UNIX_LIKE

#include "CUnixPlatform.cpp"

#else

#error Unsupported platform

#endif
