#ifndef PROTOCOLTYPES_H
#define PROTOCOLTYPES_H

#include "BasicTypes.h"

// version number
static const SInt16		kProtocolMajorVersion = 0;
static const SInt16		kProtocolMinorVersion = 1;

// contact port number
static const UInt16		kDefaultPort = 24800;

// time between heartbeats (in seconds)
static const double		kHeartRate = 2.0;

// time without a heartbeat that we call death
static const double		kHeartDeath = 3.0 * kHeartRate;

//
// message codes (trailing NUL is not part of code).  in comments, $n
// refers to the n'th argument (counting from one).  message codes are
// always 4 bytes optionally followed by message specific parameters.
//

//
// positions and sizes are signed 16 bit integers.
//

//
// command codes
//

// no operation;  secondary -> primary
static const char		kMsgCNoop[] 		= "CNOP";

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

// resolution change acknowledgment:  primary -> secondary
// sent by primary in response to a secondary screen's kMsgDInfo.
// this is sent for every kMsgDInfo, whether or not the primary
// had sent a kMsgQInfo.
static const char		kMsgCInfoAck[] = "CIAK";


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
// $1 = delta.  the delta should be +120 for one tick forward (away
// from the user) and -120 for one tick backward (toward the user).
static const char		kMsgDMouseWheel[]	= "DMWM%2i";

// clipboard data:  primary <-> secondary
// $2 = sequence number, $3 = clipboard data.  the sequence number
// is 0 when sent by the primary.  secondary screens should use the
// sequence number from the most recent kMsgCEnter.  $1 = clipboard
// identifier.
static const char		kMsgDClipboard[]	= "DCLP%1i%4i%s";

// client data:  secondary -> primary
// $1 = coordinate of leftmost pixel on secondary screen,
// $2 = coordinate of topmost pixel on secondary screen,
// $3 = width of secondary screen in pixels,
// $4 = height of secondary screen in pixels,
// $5 = size of warp zone,
// $6, $7 = the x,y position of the mouse on the secondary screen.
//
// the secondary screen must send this message in response to the
// kMsgQInfo message.  it must also send this message when the
// screen's resolution changes.  in this case, the secondary screen
// should ignore any kMsgDMouseMove messages until it receives a
// kMsgCInfoAck in order to prevent attempts to move the mouse off
// the new screen area.
static const char		kMsgDInfo[]			= "DINF%2i%2i%2i%2i%2i%2i%2i";


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

// unknown client:  primary -> secondary
// name provided when connecting is not in primary's screen
// configuration map.
static const char		kMsgEUnknown[] = "EUNK";

// protocol violation:  primary -> secondary
// primary should disconnect after sending this message.
static const char		kMsgEBad[] = "EBAD";

#endif

