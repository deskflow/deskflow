#include "stdpre.h"
#if !defined(CONFIG_PLATFORM_LINUX)
#include <ostream>
#else
// some versions of libstdc++ don't have <ostream>
// FIXME -- only include iostream for versions that don't have ostream
#include <iostream>
#endif
#include "stdpost.h"
