#pragma once

#include "XBase.h"

//! Generic platform exception
XBASE_SUBCLASS(XPlatform, XBase);

//! XInput not supported exception
/*!
The XInput DLL was not supported.
*/
XBASE_SUBCLASS_WHAT(XXInputNotSupported, XPlatform);
