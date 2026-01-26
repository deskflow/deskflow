/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2023 Input-Leap Developers
 * SPDX-FileCopyrightText: (C) 2013 - 2016 Symless Ltd.
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#pragma once

#include <stdint.h>
namespace deskflow {
enum class EventTypes : uint32_t
{
  /** An unknown event type. This type is used as a placeholder for unknown events when
      filtering events.
  */
  Unknown,

  /// Exit has been requested.
  Quit,

  /// This event is sent when system event occurs. The data is a pointer to system event type
  System,

  /// This event is sent when a timer event occurs. The data is pointer to TimerInfo.
  Timer,

  /// This event is sent when the client has successfully connected to the server.
  ClientConnected,

  /** This event is sent when the server refuses the client */
  ClientConnectionRefused,

  /** This event is sent when the server fails for some reason.
       The event data is a pointer to FailInfo.
  */
  ClientConnectionFailed,

  /** This event is sent when the client has disconnected from the server (and only after having
      successfully connected).
  */
  ClientDisconnected,

  /// A stream sends this event when \c read() will return with data.
  StreamInputReady,

  /** A stream sends this event when the output buffer has been flushed. If there have been no
      writes since the event was posted, calling \c shutdownOutput() or close() will not discard
      any data and \c flush() will return immediately.
  */
  StreamOutputFlushed,

  /// A stream sends this event when a write has failed.
  StreamOutputError,

  /** This event is sent when the input side of the stream has shutdown. When the input has
      shutdown, no more data will ever be available to read.
  */
  StreamInputShutdown,

  /** This event is sent when the output side of the stream has shutdown. When the output has
      shutdown, no more data can ever be written to the stream. Any attempt to do so will
      generate a output error event.
  */
  StreamOutputShutdown,

  /// This event is sent when a stream receives an irrecoverable input format error.
  StreamInputFormatError,

  /// A socket sends this event when a remote connection has been established.
  DataSocketConnected,

  /// A secure socket sends this event when a remote connection has been established.
  DataSocketSecureConnected,

  /** A socket sends this event when an attempt to connect to a remote port has failed.
      The data is a pointer to a ConnectionFailedInfo.
  */
  DataSocketConnectionFailed,

  /// A socket sends this event when a remote connection is waiting to be accepted.
  ListenSocketConnecting,

  /** A socket sends this event when the remote side of the socket has disconnected or
      shutdown both input and output.
  */
  SocketDisconnected,

  OsxScreenConfirmSleep,

  /// This event is sent whenever a server accepts a client.
  ClientListenerAccepted,

  /** This event is sent when the client has completed the initial handshake.  Until it is sent,
      the client is not fully connected.
  */
  ClientProxyReady,

  /** This event is sent when the client disconnects or is disconnected. The target is
      getEventTarget().
  */
  ClientProxyDisconnected,

  /** This event is sent when the client has correctly responded to the hello message.
      The target is this.
  */
  ClientProxyUnknownSuccess,

  /** This event is sent when a client fails to correctly respond to the hello message.
      The target is this.
  */
  ClientProxyUnknownFailure,

  /** This event is sent when a client screen has connected.
      The event data is a pointer to ScreenConnectedInfo that indicates the connected screen.
  */
  ServerConnected,

  /// This is event sent when all the clients have disconnected.
  ServerDisconnected,

  /** This event is sent to inform the server to switch screens.
      The event data is a pointer to SwitchToScreenInfo that indicates the target screen.
  */
  ServerSwitchToScreen,

  /// This event is sent to inform the server to toggle screens.  These is no event data.
  ServerToggleScreen,

  /** This event is sent to inform the server to switch screens.
      The event data is a pointer to SwitchInDirectionInfo that indicates the target direction.
  */
  ServerSwitchInDirection,

  /** This event is sent to inform the server to turn keyboard broadcasting on or off.
      The event data is a pointer to KeyboardBroadcastInfo.
  */
  ServerKeyboardBroadcast,

  /** This event is sent to inform the server to lock the cursor to the active screen or to
      unlock it. The event data is a pointer to LockCursorToScreenInfo.
  */
  ServerLockCursorToScreen,

  /// This event is sent when the screen has been switched to a client.
  ServerScreenSwitched,

  ServerAppReloadConfig,
  ServerAppForceReconnect,
  ServerAppResetServer,

  /// This event is sent when key is down. Event data is a pointer to KeyInfo (count == 1)
  KeyStateKeyDown,
  /// This event is sent when key is up. Event data is a pointer to KeyInfo (count == 1)
  KeyStateKeyUp,
  /// This event is sent when key is repeated. Event data is a pointer to KeyInfo.
  KeyStateKeyRepeat,

  /// This event is sent when button is down. Event data is a pointer to ButtonInfo
  PrimaryScreenButtonDown,

  /// This event is sent when button is up. Event data is a pointer to ButtonInfo
  PrimaryScreenButtonUp,

  /** This event is sent when mouse moves on primary screen.
      Event data is a pointer to MotionInfo, the values are absolute position.
  */
  PrimaryScreenMotionOnPrimary,

  /** This event is sent when mouse moves on secondary screen.
      Event data is a pointer to MotionInfo, the values are relative motion deltas.
  */
  PrimaryScreenMotionOnSecondary,

  /// This event is sent when mouse wheel is rotated. Event data is a pointer to WheelInfo.
  PrimaryScreenWheel,

  /// This event is sent when screensaver is activated.
  PrimaryScreenSaverActivated,

  /// This event is sent when screensaver is deactivated.
  PrimaryScreenSaverDeactivated,

  /// This event is sent when hotkey is down. Event data is a pointer to HotKeyInfo.
  PrimaryScreenHotkeyDown,

  /// This event is sent when hotkey is up. Event data is a pointer to HotKeyInfo.
  PrimaryScreenHotkeyUp,

  /// This event is sent when fake input begins.
  PrimaryScreenFakeInputBegin,

  /// This event is sent when fake input ends.
  PrimaryScreenFakeInputEnd,

  /** This event is sent whenever the screen has failed for some reason (e.g. the X Windows
      server died).
  */
  ScreenError,

  /// This event is sent whenever the screen's shape changes.
  ScreenShapeChanged,

  /** This event is sent whenever the system goes to sleep or a user session is deactivated (fast
      user switching).
  */
  ScreenSuspend,

  /** This event is sent whenever the system wakes up or a user session is activated (fast user
      switching).
  */
  ScreenResume,

  /** This event is sent whenever the clipboard is grabbed by some other application so we
      don't own it anymore. The data is a pointer to a ClipboardInfo.
  */
  ClipboardGrabbed,

  /** This event is sent whenever the contents of the clipboard has changed.
      The data is a pointer to a ClipboardInfo.
  */
  ClipboardChanged,

  /// This event is sent whenever a clipboard chunk is transferred.
  ClipboardSending,

  /// Start libei
  EIConnected,

  /// Stop libei
  EISessionClosed,
};
} // namespace deskflow
