#ifndef PROTOCOLTYPES_H
#define PROTOCOLTYPES_H

#include "BasicTypes.h"

// version number
static const SInt32		kMajorVersion = 0;
static const SInt32		kMinorVersion = 1;

// message codes (trailing NUL is not part of code).  codes are
// grouped into:
//   commands  -- request an action, no reply expected
//   queries   -- request info
//   data      -- send info
//   errors    -- notify of error
static const char		kMsgCClose[] 		= "CBYE";		// server
static const char		kMsgCEnter[] 		= "CINN%2i%2i";	// server
static const char		kMsgCLeave[] 		= "COUT";		// server
static const char		kMsgCClipboard[] 	= "CCLP";		// server
static const char		kMsgCScreenSaver[] 	= "CSEC%1i";	// server

static const char		kMsgDKeyDown[]		= "DKDN%2i%2i";	// server
static const char		kMsgDKeyRepeat[]	= "DKRP%2i%2i%2i";	// server
static const char		kMsgDKeyUp[]		= "DKUP%2i%2i";	// server
static const char		kMsgDMouseDown[]	= "DMDN%1i";	// server
static const char		kMsgDMouseUp[]		= "DMUP%1i";	// server
static const char		kMsgDMouseMove[]	= "DMMV%2i%2i";	// server
static const char		kMsgDMouseWheel[]	= "DMWM%2i";	// server
static const char		kMsgDClipboard[]	= "DCLP%s";		// server
static const char		kMsgDInfo[]			= "DINF%2i%2i%2i";	// client

static const char		kMsgQClipboard[]	= "QCLP";		// server
static const char		kMsgQInfo[]			= "QINF";		// server

static const char		kMsgEIncompatible[]	= "EICV";

#endif

