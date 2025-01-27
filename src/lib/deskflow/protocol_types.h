/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2012 - 2016 Symless Ltd.
 * SPDX-FileCopyrightText: (C) 2002 Chris Schoeneman
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#pragma once

#include "base/EventTypes.h"

#include <cstdint>

// protocol version number
// 1.0:  initial protocol
// 1.1:  adds KeyCode to key press, release, and repeat
// 1.2:  adds mouse relative motion
// 1.3:  adds keep alive and deprecates heartbeats,
//       adds horizontal mouse scrolling
// 1.4:  adds crypto support
// 1.5:  adds file transfer and removes home brew crypto
// 1.6:  adds clipboard streaming
// 1.7   adds security input notifications
// 1.8   adds language synchronization functionality
// NOTE: with new version, deskflow minor version should increment
static const int16_t kProtocolMajorVersion = 1;
static const int16_t kProtocolMinorVersion = 8;

// default contact port number
static const uint16_t kDefaultPort = 24800;

// maximum total length for greeting returned by client
static const uint32_t kMaxHelloLength = 1024;

// time between kMsgCKeepAlive (in seconds).  a non-positive value disables
// keep alives.  this is the default rate that can be overridden using an
// option.
static const double kKeepAliveRate = 3.0;

// number of skipped kMsgCKeepAlive messages that indicates a problem
static const double kKeepAlivesUntilDeath = 3.0;

// obsolete heartbeat stuff
static const double kHeartRate = -1.0;
static const double kHeartBeatsUntilDeath = 3.0;

// Messages of very large size indicate a likely protocol error. We don't parse such messages and
// drop connection instead. Note that e.g. the clipboard messages are already limited to 32kB.
static constexpr uint32_t PROTOCOL_MAX_MESSAGE_LENGTH = 4 * 1024 * 1024;
static constexpr uint32_t PROTOCOL_MAX_LIST_LENGTH = 1024 * 1024;
static constexpr uint32_t PROTOCOL_MAX_STRING_LENGTH = 1024 * 1024;

// direction constants
enum EDirection
{
  kNoDirection,
  kLeft,
  kRight,
  kTop,
  kBottom,
  kFirstDirection = kLeft,
  kLastDirection = kBottom,
  kNumDirections = kLastDirection - kFirstDirection + 1
};
enum EDirectionMask
{
  kNoDirMask = 0,
  kLeftMask = 1 << kLeft,
  kRightMask = 1 << kRight,
  kTopMask = 1 << kTop,
  kBottomMask = 1 << kBottom
};

// Data transfer constants
enum EDataTransfer
{
  kDataStart = 1,
  kDataChunk = 2,
  kDataEnd = 3
};

// Data received constants
enum EDataReceived
{
  kStart,
  kNotFinish,
  kFinish,
  kError
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

// used to say hello back to the server as Synergy
extern const char *const kSynergyProtocolName;

// used to say hello back to the server as Barrier
extern const char *const kBarrierProtocolName;

// say hello to client;  primary -> secondary
// $1 = protocol major version number supported by server.  $2 =
// protocol minor version number supported by server. $3 = server
// keyboard layout list.
extern const char *const kMsgHello;

// args part of kMsgHello.
// used as part of a dynamic hello message.
// this can be superseded by kMsgHello once `ProtocolUtil::writef`
// supports fixed length strings (e.g. %7s).
extern const char *const kMsgHelloArgs;

// respond to hello from server;  secondary -> primary
// $1 = protocol major version number supported by client.  $2 =
// protocol minor version number supported by client.  $3 = client
// name.
extern const char *const kMsgHelloBack;

// args part of kMsgHelloBack.
// used as part of a dynamic hello message.
// this can be superseded by kMsgHelloBack once `ProtocolUtil::writef`
// supports fixed length strings (e.g. %7s).
extern const char *const kMsgHelloBackArgs;

//
// command codes
//

// no operation;  secondary -> primary
extern const char *const kMsgCNoop;

// close connection;  primary -> secondary
extern const char *const kMsgCClose;

// enter screen:  primary -> secondary
// entering screen at screen position $1 = x, $2 = y.  x,y are
// absolute screen coordinates.  $3 = sequence number, which is
// used to order messages between screens.  the secondary screen
// must return this number with some messages.  $4 = modifier key
// mask.  this will have bits set for each toggle modifier key
// that is activated on entry to the screen.  the secondary screen
// should adjust its toggle modifiers to reflect that state.
extern const char *const kMsgCEnter;

// leave screen:  primary -> secondary
// leaving screen.  the secondary screen should send clipboard
// data in response to this message for those clipboards that
// it has grabbed (i.e. has sent a kMsgCClipboard for and has
// not received a kMsgCClipboard for with a greater sequence
// number) and that were grabbed or have changed since the
// last leave.
extern const char *const kMsgCLeave;

// grab clipboard:  primary <-> secondary
// sent by screen when some other app on that screen grabs a
// clipboard.  $1 = the clipboard identifier, $2 = sequence number.
// secondary screens must use the sequence number passed in the
// most recent kMsgCEnter.  the primary always sends 0.
extern const char *const kMsgCClipboard;

// screensaver change:  primary -> secondary
// screensaver on primary has started ($1 == 1) or closed ($1 == 0)
extern const char *const kMsgCScreenSaver;

// reset options:  primary -> secondary
// client should reset all of its options to their defaults.
extern const char *const kMsgCResetOptions;

// resolution change acknowledgment:  primary -> secondary
// sent by primary in response to a secondary screen's kMsgDInfo.
// this is sent for every kMsgDInfo, whether or not the primary
// had sent a kMsgQInfo.
extern const char *const kMsgCInfoAck;

// keep connection alive:  primary <-> secondary
// sent by the server periodically to verify that connections are still
// up and running.  clients must reply in kind on receipt.  if the server
// gets an error sending the message or does not receive a reply within
// a reasonable time then the server disconnects the client.  if the
// client doesn't receive these (or any message) periodically then it
// should disconnect from the server.  the appropriate interval is
// defined by an option.
extern const char *const kMsgCKeepAlive;

//
// data codes
//
// The same as kMsgDKeyDown but with languageCode
// $4 = languageCode
extern const char *const kMsgDKeyDownLang;

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
// languageCode is parameter which helps client to react on unknwon
// language letters
extern const char *const kMsgDKeyDown;

// key pressed 1.0:  same as above but without KeyButton
extern const char *const kMsgDKeyDown1_0;

// key auto-repeat:  primary -> secondary
// $1 = KeyID, $2 = KeyModifierMask, $3 = number of repeats, $4 = KeyButton
// $5 =language code
extern const char *const kMsgDKeyRepeat;

// key auto-repeat 1.0:  same as above but without KeyButton
extern const char *const kMsgDKeyRepeat1_0;

// key released:  primary -> secondary
// $1 = KeyID, $2 = KeyModifierMask, $3 = KeyButton
extern const char *const kMsgDKeyUp;

// key released 1.0:  same as above but without KeyButton
extern const char *const kMsgDKeyUp1_0;

// mouse button pressed:  primary -> secondary
// $1 = ButtonID
extern const char *const kMsgDMouseDown;

// mouse button released:  primary -> secondary
// $1 = ButtonID
extern const char *const kMsgDMouseUp;

// mouse moved:  primary -> secondary
// $1 = x, $2 = y.  x,y are absolute screen coordinates.
extern const char *const kMsgDMouseMove;

// relative mouse move:  primary -> secondary
// $1 = dx, $2 = dy.  dx,dy are motion deltas.
extern const char *const kMsgDMouseRelMove;

// mouse scroll:  primary -> secondary
// $1 = xDelta, $2 = yDelta.  the delta should be +120 for one tick forward
// (away from the user) or right and -120 for one tick backward (toward
// the user) or left.
extern const char *const kMsgDMouseWheel;

// mouse vertical scroll:  primary -> secondary
// like as kMsgDMouseWheel except only sends $1 = yDelta.
extern const char *const kMsgDMouseWheel1_0;

// clipboard data:  primary <-> secondary
// $2 = sequence number, $3 = mark $4 = clipboard data.  the sequence number
// is 0 when sent by the primary.  secondary screens should use the
// sequence number from the most recent kMsgCEnter.  $1 = clipboard
// identifier.
extern const char *const kMsgDClipboard;

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
extern const char *const kMsgDInfo;

// set options:  primary -> secondary
// client should set the given option/value pairs.  $1 = option/value
// pairs.
extern const char *const kMsgDSetOptions;

// file data:  primary <-> secondary
// transfer file data. A mark is used in the first byte.
// 0 means the content followed is the file size.
// 1 means the content followed is the chunk data.
// 2 means the file transfer is finished.
extern const char *const kMsgDFileTransfer;

// drag infomation:  primary <-> secondary
// transfer drag infomation. The first 2 bytes are used for storing
// the number of dragging objects. Then the following string consists
// of each object's directory.
extern const char *const kMsgDDragInfo;

// secure input notification:  primary -> secondary
// $1 = app. app only obtainable on MacOS since that's the only
// platform facing secure input problems
extern const char *const kMsgDSecureInputNotification;

// language synchronization:  primary -> secondary
// $1 = List of server languages
extern const char *const kMsgDLanguageSynchronisation;

//
// query codes
//

// query screen info:  primary -> secondary
// client should reply with a kMsgDInfo.
extern const char *const kMsgQInfo;

//
// error codes
//

// incompatible versions:  primary -> secondary
// $1 = major version of primary, $2 = minor version of primary.
extern const char *const kMsgEIncompatible;

// name provided when connecting is already in use:  primary -> secondary
extern const char *const kMsgEBusy;

// unknown client:  primary -> secondary
// name provided when connecting is not in primary's screen
// configuration map.
extern const char *const kMsgEUnknown;

// protocol violation:  primary -> secondary
// primary should disconnect after sending this message.
extern const char *const kMsgEBad;

//
// structures
//

//! Screen information
/*!
This class contains information about a screen.
*/
class ClientInfo
{
public:
  //! Screen position
  /*!
  The position of the upper-left corner of the screen.  This is
  typically 0,0.
  */
  int32_t m_x, m_y;

  //! Screen size
  /*!
  The size of the screen in pixels.
  */
  int32_t m_w, m_h;

  //! Obsolete (jump zone size)
  int32_t obsolete1;

  //! Mouse position
  /*!
  The current location of the mouse cursor.
  */
  int32_t m_mx, m_my;
};
