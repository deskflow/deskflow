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
// absolute screen coordinates.  $3 = sequence number, which is
// used to order messages between screens.  the secondary screen
// must return this number with some messages.  $4 = modifier key
// mask.  this will have bits set for each toggle modifier key
// that is activated on entry to the screen.  the secondary screen
// should adjust its toggle modifiers to reflect that state.
static const char		kMsgCEnter[] 		= "CINN%2i%2i%4i%2i";

// leave screen:  primary -> secondary
// leaving screen.  the secondary screen should send clipboard
// data in response to this message for those clipboards that
// it has grabbed (i.e. has sent a kMsgCClipboard for and has
// not received a kMsgCClipboard for with a greater sequence
// number) and that were grabbed or have changed since the
// last leave.
static const char		kMsgCLeave[] 		= "COUT";

// grab clipboard:  primary <-> secondary
// sent by screen when some other app on that screen grabs a
// clipboard.  $1 = the clipboard identifier, $2 = sequence number.
// secondary screens must use the sequence number passed in the
// most recent kMsgCEnter.  the primary always sends 0.
static const char		kMsgCClipboard[] 	= "CCLP%1i%4i";

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
// is 0 when sent by the primary.  secondary screens should use the
// sequence number from the most recent kMsgCEnter.  $1 = clipboard
// identifier.
static const char		kMsgDClipboard[]	= "DCLP%1i%4i%s";

// client data:  secondary -> primary
// $1 = seconary screen width in pixels, $2 = screen height, $3 =
// size of warp zone.
static const char		kMsgDInfo[]			= "DINF%2i%2i%2i";


//
// query codes
//

// query screen info:  primary -> secondary
// client should reply with a kMsgDInfo.
static const char		kMsgQInfo[]			= "QINF";


//
// error codes
//

// incompatible versions:  primary -> secondary
// $1 = major version of primary, $2 = minor version of primary.
static const char		kMsgEIncompatible[]	= "EICV%2i%2i";

// name provided when connecting is already in use:  primary -> secondary
static const char		kMsgEBusy[] = "EBSY";

// protocol violation:  primary -> secondary
// primary should disconnect after sending this message.
static const char		kMsgEBad[] = "EBAD";

#endif

