/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2012 Symless Ltd.
 * SPDX-FileCopyrightText: (C) 2002 Chris Schoeneman
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#pragma once

#include "base/Event.h"
#include "base/EventTypes.h"
#include "base/Stopwatch.h"
#include "common/stdmap.h"
#include "common/stdset.h"
#include "common/stdvector.h"
#include "deskflow/Clipboard.h"
#include "deskflow/DragInformation.h"
#include "deskflow/INode.h"
#include "deskflow/ServerArgs.h"
#include "deskflow/clipboard_types.h"
#include "deskflow/key_types.h"
#include "deskflow/mouse_types.h"
#include "deskflow/option_types.h"
#include "server/Config.h"
#include <memory>

class BaseClientProxy;
class EventQueueTimer;
class PrimaryClient;
class InputFilter;
namespace deskflow {
class Screen;
}
class IEventQueue;
class Thread;
class ClientListener;

//! Deskflow server
/*!
This class implements the top-level server algorithms for deskflow.
*/
class Server : public INode
{
  using ServerConfig = deskflow::server::Config;

public:
  //! Lock cursor to screen data
  class LockCursorToScreenInfo
  {
  public:
    enum State
    {
      kOff,
      kOn,
      kToggle
    };

    static LockCursorToScreenInfo *alloc(State state = kToggle);

  public:
    State m_state;
  };

  //! Switch to screen data
  class SwitchToScreenInfo
  {
  public:
    static SwitchToScreenInfo *alloc(const std::string &screen);

  public:
    // this is a C-string;  this type is a variable size structure
    char m_screen[1];
  };

  //! Switch in direction data
  class SwitchInDirectionInfo
  {
  public:
    static SwitchInDirectionInfo *alloc(EDirection direction);

  public:
    EDirection m_direction;
  };

  //! Screen connected data
  class ScreenConnectedInfo
  {
  public:
    ScreenConnectedInfo(std::string screen) : m_screen(screen)
    {
    }

  public:
    std::string m_screen; // was char[1]
  };

  //! Keyboard broadcast data
  class KeyboardBroadcastInfo
  {
  public:
    enum State
    {
      kOff,
      kOn,
      kToggle
    };

    static KeyboardBroadcastInfo *alloc(State state = kToggle);
    static KeyboardBroadcastInfo *alloc(State state, const std::string &screens);

  public:
    State m_state;
    char m_screens[1];
  };

  /*!
  Start the server with the configuration \p config and the primary
  client (local screen) \p primaryClient.  The client retains
  ownership of \p primaryClient.
  */
  Server(
      ServerConfig &config, PrimaryClient *primaryClient, deskflow::Screen *screen, IEventQueue *events,
      deskflow::ServerArgs const &args
  );
  Server(Server const &) = delete;
  Server(Server &&) = delete;
  ~Server();

  Server &operator=(Server const &) = delete;
  Server &operator=(Server &&) = delete;

#ifdef TEST_ENV
  Server() : m_mock(true), m_config(NULL)
  {
  }
  void setActive(BaseClientProxy *active)
  {
    m_active = active;
  }
#endif

  //! @name manipulators
  //@{

  //! Set configuration
  /*!
  Change the server's configuration.  Returns true iff the new
  configuration was accepted (it must include the server's name).
  This will disconnect any clients no longer in the configuration.
  */
  bool setConfig(const ServerConfig &);

  //! Add a client
  /*!
  Adds \p client to the server.  The client is adopted and will be
  destroyed when the client disconnects or is disconnected.
  */
  void adoptClient(BaseClientProxy *client);

  //! Disconnect clients
  /*!
  Disconnect clients.  This tells them to disconnect but does not wait
  for them to actually do so.  The server sends the disconnected event
  when they're all disconnected (or immediately if none are connected).
  The caller can also just destroy this object to force the disconnection.
  */
  void disconnect();

  //! Create a new thread and use it to send file to client
  void sendFileToClient(const char *filename);

  //! Received dragging information from client
  void dragInfoReceived(uint32_t fileNum, std::string content);

  //! Store ClientListener pointer
  void setListener(ClientListener *p)
  {
    m_clientListener = p;
  }

  //@}
  //! @name accessors
  //@{

  //! Get the network protocol
  /*!
  Returns the network protocol used by the server.
  */
  std::string protocolString() const;

  //! Get number of connected clients
  /*!
  Returns the number of connected clients, including the server itself.
  */
  uint32_t getNumClients() const;

  //! Get the list of connected clients
  /*!
  Set the \c list to the names of the currently connected clients.
  */
  void getClients(std::vector<std::string> &list) const;

  //! Return true if recieved file size is valid
  bool isReceivedFileSizeValid();

  //! Return expected file data size
  size_t &getExpectedFileSize()
  {
    return m_expectedFileSize;
  }

  //! Return received file data
  std::string &getReceivedFileData()
  {
    return m_receivedFileData;
  }

  //! Return fake drag file list
  DragFileList getFakeDragFileList()
  {
    return m_fakeDragFileList;
  }

  //! Returns true if it's client mode and server initiates connection
  bool isClientMode() const;

  //@}

private:
  // get canonical name of client
  std::string getName(const BaseClientProxy *) const;

  // get the sides of the primary screen that have neighbors
  uint32_t getActivePrimarySides() const;

  // returns true iff mouse should be locked to the current screen
  // according to this object only, ignoring what the primary client
  // says.
  bool isLockedToScreenServer() const;

  // returns true iff mouse should be locked to the current screen
  // according to this object or the primary client.
  bool isLockedToScreen() const;

  // returns the jump zone of the client
  int32_t getJumpZoneSize(BaseClientProxy *) const;

  // change the active screen
  void switchScreen(BaseClientProxy *, int32_t x, int32_t y, bool forScreenSaver);

  // jump to screen
  void jumpToScreen(BaseClientProxy *);

  // convert pixel position to fraction, using x or y depending on the
  // direction.
  float mapToFraction(BaseClientProxy *, EDirection, int32_t x, int32_t y) const;

  // convert fraction to pixel position, writing only x or y depending
  // on the direction.
  void mapToPixel(BaseClientProxy *, EDirection, float f, int32_t &x, int32_t &y) const;

  // returns true if the client has a neighbor anywhere along the edge
  // indicated by the direction.
  bool hasAnyNeighbor(BaseClientProxy *, EDirection) const;

  // lookup neighboring screen, mapping the coordinate independent of
  // the direction to the neighbor's coordinate space.
  BaseClientProxy *getNeighbor(BaseClientProxy *, EDirection, int32_t &x, int32_t &y) const;

  // lookup neighboring screen.  given a position relative to the
  // source screen, find the screen we should move onto and where.
  // if the position is sufficiently far from the source then we
  // cross multiple screens.  if there is no suitable screen then
  // return NULL and x,y are not modified.
  BaseClientProxy *mapToNeighbor(BaseClientProxy *, EDirection, int32_t &x, int32_t &y) const;

  // adjusts x and y or neither to avoid ending up in a jump zone
  // after entering the client in the given direction.
  void avoidJumpZone(BaseClientProxy *, EDirection, int32_t &x, int32_t &y) const;

  // test if a switch is permitted.  this includes testing user
  // options like switch delay and tracking any state required to
  // implement them.  returns true iff a switch is permitted.
  bool isSwitchOkay(BaseClientProxy *dst, EDirection, int32_t x, int32_t y, int32_t xActive, int32_t yActive);

  // update switch state due to a mouse move at \p x, \p y that
  // doesn't switch screens.
  void noSwitch(int32_t x, int32_t y);

  // stop switch timers
  void stopSwitch();

  // start two tap switch timer
  void startSwitchTwoTap();

  // arm the two tap switch timer if \p x, \p y is outside the tap zone
  void armSwitchTwoTap(int32_t x, int32_t y);

  // stop the two tap switch timer
  void stopSwitchTwoTap();

  // returns true iff the two tap switch timer is started
  bool isSwitchTwoTapStarted() const;

  // returns true iff should switch because of two tap
  bool shouldSwitchTwoTap() const;

  // start delay switch timer
  void startSwitchWait(int32_t x, int32_t y);

  // stop delay switch timer
  void stopSwitchWait();

  // returns true iff the delay switch timer is started
  bool isSwitchWaitStarted() const;

  // returns the corner (EScreenSwitchCornerMasks) where x,y is on the
  // given client.  corners have the given size.
  uint32_t getCorner(BaseClientProxy *, int32_t x, int32_t y, int32_t size) const;

  // stop relative mouse moves
  void stopRelativeMoves();

  // send screen options to \c client
  void sendOptions(BaseClientProxy *client) const;

  // process options from configuration
  void processOptions();

  // event handlers
  void handleShapeChanged(const Event &, void *);
  void handleClipboardGrabbed(const Event &, void *);
  void handleClipboardChanged(const Event &, void *);
  void handleKeyDownEvent(const Event &, void *);
  void handleKeyUpEvent(const Event &, void *);
  void handleKeyRepeatEvent(const Event &, void *);
  void handleButtonDownEvent(const Event &, void *);
  void handleButtonUpEvent(const Event &, void *);
  void handleMotionPrimaryEvent(const Event &, void *);
  void handleMotionSecondaryEvent(const Event &, void *);
  void handleWheelEvent(const Event &, void *);
  void handleScreensaverActivatedEvent(const Event &, void *);
  void handleScreensaverDeactivatedEvent(const Event &, void *);
  void handleSwitchWaitTimeout(const Event &, void *);
  void handleClientDisconnected(const Event &, void *);
  void handleClientCloseTimeout(const Event &, void *);
  void handleSwitchToScreenEvent(const Event &, void *);
  void handleSwitchInDirectionEvent(const Event &, void *);
  void handleKeyboardBroadcastEvent(const Event &, void *);
  void handleLockCursorToScreenEvent(const Event &, void *);
  void handleFakeInputBeginEvent(const Event &, void *);
  void handleFakeInputEndEvent(const Event &, void *);
  void handleFileChunkSendingEvent(const Event &, void *);
  void handleFileRecieveCompletedEvent(const Event &, void *);

  // event processing
  void onClipboardChanged(BaseClientProxy *sender, ClipboardID id, uint32_t seqNum);
  void onScreensaver(bool activated);
  void onKeyDown(KeyID, KeyModifierMask, KeyButton, const std::string &, const char *screens);
  void onKeyUp(KeyID, KeyModifierMask, KeyButton, const char *screens);
  void onKeyRepeat(KeyID, KeyModifierMask, int32_t, KeyButton, const std::string &);
  void onMouseDown(ButtonID);
  void onMouseUp(ButtonID);
  bool onMouseMovePrimary(int32_t x, int32_t y);
  void onMouseMoveSecondary(int32_t dx, int32_t dy);
  void onMouseWheel(int32_t xDelta, int32_t yDelta);
  void onFileChunkSending(const void *data);
  void onFileRecieveCompleted();

  // add client to list and attach event handlers for client
  bool addClient(BaseClientProxy *);

  // remove client from list and detach event handlers for client
  bool removeClient(BaseClientProxy *);

  // close a client
  void closeClient(BaseClientProxy *, const char *msg);

  // close clients not in \p config
  void closeClients(const ServerConfig &config);

  // close all clients whether they've completed the handshake or not,
  // except the primary client
  void closeAllClients();

  // remove clients from internal state
  void removeActiveClient(BaseClientProxy *);
  void removeOldClient(BaseClientProxy *);

  // force the cursor off of \p client
  void forceLeaveClient(BaseClientProxy *client);

  // thread funciton for sending file
  void sendFileThread(void *);

  // thread function for writing file to drop directory
  void writeToDropDirThread(void *);

  // thread function for sending drag information
  void sendDragInfoThread(void *);

  // send drag info to new client screen
  void sendDragInfo(BaseClientProxy *newScreen);

public:
  bool m_mock;

private:
  class ClipboardInfo
  {
  public:
    ClipboardInfo();

  public:
    Clipboard m_clipboard;
    std::string m_clipboardData;
    std::string m_clipboardOwner;
    uint32_t m_clipboardSeqNum;
  };

  // used in hello message sent to the client
  ENetworkProtocol m_protocol;

  // the primary screen client
  PrimaryClient *m_primaryClient;

  // all clients (including the primary client) indexed by name
  using ClientList = std::map<std::string, BaseClientProxy *>;
  using ClientSet = std::set<BaseClientProxy *>;
  ClientList m_clients;
  ClientSet m_clientSet;

  // all old connections that we're waiting to hangup
  using OldClients = std::map<BaseClientProxy *, EventQueueTimer *>;
  OldClients m_oldClients;

  // the client with focus
  BaseClientProxy *m_active;

  // the sequence number of enter messages
  uint32_t m_seqNum;

  // current mouse position (in absolute screen coordinates) on
  // whichever screen is active
  int32_t m_x, m_y;

  // last mouse deltas.  this is needed to smooth out double tap
  // on win32 which reports bogus mouse motion at the edge of
  // the screen when using low level hooks, synthesizing motion
  // in the opposite direction the mouse actually moved.
  int32_t m_xDelta, m_yDelta;
  int32_t m_xDelta2, m_yDelta2;

  // current configuration
  ServerConfig *m_config;

  // input filter (from m_config);
  InputFilter *m_inputFilter;

  // clipboard cache
  ClipboardInfo m_clipboards[kClipboardEnd];

  // state saved when screen saver activates
  BaseClientProxy *m_activeSaver;
  int32_t m_xSaver, m_ySaver;

  // common state for screen switch tests.  all tests are always
  // trying to reach the same screen in the same direction.
  EDirection m_switchDir;
  BaseClientProxy *m_switchScreen;

  // state for delayed screen switching
  double m_switchWaitDelay;
  EventQueueTimer *m_switchWaitTimer;
  int32_t m_switchWaitX, m_switchWaitY;

  // state for double-tap screen switching
  double m_switchTwoTapDelay;
  Stopwatch m_switchTwoTapTimer;
  bool m_switchTwoTapEngaged;
  bool m_switchTwoTapArmed;
  int32_t m_switchTwoTapZone;

  // modifiers needed before switching
  bool m_switchNeedsShift;
  bool m_switchNeedsControl;
  bool m_switchNeedsAlt;

  // relative mouse move option
  bool m_relativeMoves;

  // flag whether or not we have broadcasting enabled and the screens to
  // which we should send broadcasted keys.
  bool m_keyboardBroadcasting;
  std::string m_keyboardBroadcastingScreens;

  // screen locking (former scroll lock)
  bool m_lockedToScreen;

  // server screen
  deskflow::Screen *m_screen;

  IEventQueue *m_events;

  // file transfer
  using AutoThread = std::unique_ptr<Thread>;
  size_t m_expectedFileSize;
  std::string m_receivedFileData;
  DragFileList m_dragFileList;
  DragFileList m_fakeDragFileList;
  AutoThread m_sendFileThread;
  AutoThread m_writeToDropDirThread;
  std::string m_dragFileExt;
  bool m_ignoreFileTransfer;
  bool m_disableLockToScreen;
  bool m_enableClipboard;
  size_t m_maximumClipboardSize;

  AutoThread m_sendDragInfoThread;
  bool m_waitDragInfoThread;

  ClientListener *m_clientListener;
  deskflow::ServerArgs m_args;
};
