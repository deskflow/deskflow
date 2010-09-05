/*
 * synergy -- mouse and keyboard sharing utility
 * Copyright (C) 2002 Chris Schoeneman
 * 
 * This package is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * found in the file COPYING that should have accompanied this file.
 * 
 * This package is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */

#ifndef PROTOCOLTYPES_H
#define PROTOCOLTYPES_H

#include "BasicTypes.h"

// protocol version number
// 1.0:  initial protocol
// 1.1:  adds KeyCode to key press, release, and repeat
static const SInt16		kProtocolMajorVersion = 1;
static const SInt16		kProtocolMinorVersion = 2;

// default contact port number
static const UInt16		kDefaultPort = 24800;

// maximum total length for greeting returned by client
static const UInt32		kMaxHelloLength = 1024;

// time between heartbeats (in seconds).  negative value disables
// heartbeat.
static const double		kHeartRate = -1.0;

// number of skipped heartbeats that constitutes death
static const double		kHeartBeatsUntilDeath = 3.0;

// direction constants
enum EDirection {
	kNoDirection,
	kLeft,
	kRight,
	kTop,
	kBottom,
	kFirstDirection = kLeft,
	kLastDirection = kBottom,
	kNumDirections = kLastDirection - kFirstDirection + 1
};
enum EDirectionMask {
	kNoDirMask  = 0,
	kLeftMask   = 1 << kLeft,
	kRightMask  = 1 << kRight,
	kTopMask    = 1 << kTop,
	kBottomMask = 1 << kBottom
};


//
// message codes (trailing NUL is not part of code).  in comments, $n
// refers to the n'th argument (counting from one).  message codes are
// always 4 bytes optionally followed by message specific parameters
// except those for the greeting handshake.
//

//
// positions and sizes are signed 16 bit integers.
//

//
// greeting handshake messages
//

// say hello to client;  primary -> secondary
// $1 = protocol major version number supported by server.  $2 =
// protocol minor version number supported by server.
static const char		kMsgHello[]			= "Synergy%2i%2i";

// respond to hello from server;  secondary -> primary
// $1 = protocol major version number supported by client.  $2 =
// protocol minor version number supported by client.  $3 = client
// name.
static const char		kMsgHelloBack[]		= "Synergy%2i%2i%s";


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

// reset options:  primary -> secondary
// client should reset all of its options to their defaults.
static const char		kMsgCResetOptions[]	= "CROP";

// resolution change acknowledgment:  primary -> secondary
// sent by primary in response to a secondary screen's kMsgDInfo.
// this is sent for every kMsgDInfo, whether or not the primary
// had sent a kMsgQInfo.
static const char		kMsgCInfoAck[]		= "CIAK";


//
// data codes
//

// key pressed:  primary -> secondary
// $1 = KeyID, $2 = KeyModifierMask, $3 = KeyButton
// the KeyButton identifies the physical key on the primary used to
// generate this key.  the secondary should note the KeyButton along
// with the physical key it uses to generate the key press.  on
// release, the secondary can then use the primary's KeyButton to
// find its corresponding physical key and release it.  this is
// necessary because the KeyID on release may not be the KeyID of
// the press.  this can happen with combining (dead) keys or if
// the keyboard layouts are not identical and the user releases
// a modifier key before releasing the modified key.
static const char		kMsgDKeyDown[]		= "DKDN%2i%2i%2i";

// key pressed 1.0:  same as above but without KeyButton
static const char		kMsgDKeyDown1_0[]	= "DKDN%2i%2i";

// key auto-repeat:  primary -> secondary
// $1 = KeyID, $2 = KeyModifierMask, $3 = number of repeats, $4 = KeyButton
static const char		kMsgDKeyRepeat[]	= "DKRP%2i%2i%2i%2i";

// key auto-repeat 1.0:  same as above but without KeyButton
static const char		kMsgDKeyRepeat1_0[]	= "DKRP%2i%2i%2i";

// key released:  primary -> secondary
// $1 = KeyID, $2 = KeyModifierMask, $3 = KeyButton
static const char		kMsgDKeyUp[]		= "DKUP%2i%2i%2i";

// key released 1.0:  same as above but without KeyButton
static const char		kMsgDKeyUp1_0[]		= "DKUP%2i%2i";

// mouse button pressed:  primary -> secondary
// $1 = ButtonID
static const char		kMsgDMouseDown[]	= "DMDN%1i";

// mouse button released:  primary -> secondary
// $1 = ButtonID
static const char		kMsgDMouseUp[]		= "DMUP%1i";

// mouse moved:  primary -> secondary
// $1 = x, $2 = y.  x,y are absolute screen coordinates.
static const char		kMsgDMouseMove[]	= "DMMV%2i%2i";

// relative mouse move:  primary -> secondary
// $1 = dx, $2 = dy.  dx,dy are motion deltas.
static const char		kMsgDMouseRelMove[]	= "DMRM%2i%2i";

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
// $5 = size of warp zone, (obsolete)
// $6, $7 = the x,y position of the mouse on the secondary screen.
//
// the secondary screen must send this message in response to the
// kMsgQInfo message.  it must also send this message when the
// screen's resolution changes.  in this case, the secondary screen
// should ignore any kMsgDMouseMove messages until it receives a
// kMsgCInfoAck in order to prevent attempts to move the mouse off
// the new screen area.
static const char		kMsgDInfo[]			= "DINF%2i%2i%2i%2i%2i%2i%2i";

// set options:  primary -> secondary
// client should set the given option/value pairs.  $1 = option/value
// pairs.
static const char		kMsgDSetOptions[]	= "DSOP%4I";


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
static const char		kMsgEBusy[] 		= "EBSY";

// unknown client:  primary -> secondary
// name provided when connecting is not in primary's screen
// configuration map.
static const char		kMsgEUnknown[]		= "EUNK";

// protocol violation:  primary -> secondary
// primary should disconnect after sending this message.
static const char		kMsgEBad[]			= "EBAD";


//
// structures
//

//! Screen information
/*!
This class contains information about a screen.
*/
class CClientInfo {
public:
	//! Screen position
	/*!
	The position of the upper-left corner of the screen.  This is
	typically 0,0.
	*/
	SInt32				m_x, m_y;

	//! Screen size
	/*!
	The size of the screen in pixels.
	*/
	SInt32				m_w, m_h;

	//! Obsolete (jump zone size)
	SInt32				obsolete1;

	//! Mouse position
	/*!
	The current location of the mouse cursor.
	*/
	SInt32				m_mx, m_my;
};

#endif

