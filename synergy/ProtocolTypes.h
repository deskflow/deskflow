#ifndef PROTOCOLTYPES_H
#define PROTOCOLTYPES_H

#include "BasicTypes.h"

// version number
static const SInt32		kMajorVersion = 0;
static const SInt32		kMinorVersion = 1;

//
// message codes (trailing NUL is not part of code).  in comments, $n
// refers to the n'th argument (counting from one).  message codes are
// always 4 bytes optionally followed by message specific parameters.
//

//
// command codes
//

// close connection;  primary -> secondary
static const char		kMsgCClose[] 		= "CBYE";

// enter screen:  primary -> secondary
// entering screen at screen position $1 = x, $2 = y.  x,y are
// absolute screen coordinates.
static const char		kMsgCEnter[] 		= "CINN%2i%2i";

// leave screen:  primary -> secondary
// leaving screen
static const char		kMsgCLeave[] 		= "COUT";

// grab clipboard:  primary <-> secondary
// sent by screen when some other app on that screen grabs a
// clipboard.  $1 = the clipboard identifier.
static const char		kMsgCClipboard[] 	= "CCLP%1i";

// screensaver change:  primary -> secondary
// screensaver on primary has started ($1 == 1) or closed ($1 == 0)
static const char		kMsgCScreenSaver[] 	= "CSEC%1i";


//
// data codes
//

// key pressed:  primary -> secondary
// $1 = KeyID, $2 = KeyModifierMask
static const char		kMsgDKeyDown[]		= "DKDN%2i%2i";

// key auto-repeat:  primary -> secondary
// $1 = KeyID, $2 = KeyModifierMask, $3 = number of repeats
static const char		kMsgDKeyRepeat[]	= "DKRP%2i%2i%2i";

// key released:  primary -> secondary
// $1 = KeyID, $2 = KeyModifierMask
static const char		kMsgDKeyUp[]		= "DKUP%2i%2i";

// mouse button pressed:  primary -> secondary
// $1 = ButtonID
static const char		kMsgDMouseDown[]	= "DMDN%1i";

// mouse button released:  primary -> secondary
// $1 = ButtonID
static const char		kMsgDMouseUp[]		= "DMUP%1i";

// mouse moved:  primary -> secondary
// $1 = x, $2 = y.  x,y are absolute screen coordinates.
static const char		kMsgDMouseMove[]	= "DMMV%2i%2i";

// mouse button pressed:  primary -> secondary
// $1 = delta
static const char		kMsgDMouseWheel[]	= "DMWM%2i";

// clipboard data:  primary <-> secondary
// $2 = sequence number, $3 = clipboard data.  the sequence number
// is 0 when sent by the primary.  the secondary sends this message
// in response to a kMsgQClipboard and uses the sequence number from
// that message.  $1 = clipboard identifier.
static const char		kMsgDClipboard[]	= "DCLP%1i%4i%s";

// client data:  seconary -> primary
// $1 = seconary screen width in pixels, $2 = screen height, $3 =
// size of warp zone.
static const char		kMsgDInfo[]			= "DINF%2i%2i%2i";


//
// query codes
//

// query clipboard:  primary -> secondary
// $2 = sequence number.  the sequence number is an arbitrary value
// used by primary to identify the kMsgDClipboard response to a
// query.  $1 = clipboard identifier.
static const char		kMsgQClipboard[]	= "QCLP%1i%4i";

// query screen info:  primary -> secondary
// client should reply with a kMsgDInfo.
static const char		kMsgQInfo[]			= "QINF";


//
// error codes
//

static const char		kMsgEIncompatible[]	= "EICV";

#endif

