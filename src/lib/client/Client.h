/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2025 Deskflow Developers
 * SPDX-FileCopyrightText: (C) 2012 - 2016 Symless Ltd.
 * SPDX-FileCopyrightText: (C) 2002 Chris Schoeneman
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#pragma once

#include "deskflow/IClient.h"

#include "HelloBack.h"
#include "base/EventTypes.h"
#include "deskflow/IClipboard.h"
#include "net/NetworkAddress.h"

#include <climits>
#include <memory>

class Event;
class EventQueueTimer;
namespace deskflow {
class Screen;
}
class ServerProxy;
class IDataSocket;
class ISocketFactory;
namespace deskflow {
class IStream;
}
class IEventQueue;
class Thread;
class TCPSocket;

//! Deskflow client
/*!
This class implements the top-level client algorithms for deskflow.
*/
class Client : public IClient
{
public:
  class FailInfo
  {
  public:
    explicit FailInfo(const char *what) : m_what(what)
    {
      // do nothing
    }
    bool m_retry = false;
    std::string m_what;
  };

public:
  /*!
  This client will attempt to connect to the server using \p name
  as its name and \p address as the server's address and \p factory
  to create the socket.  \p screen is    the local screen.
  */
  Client(
      IEventQueue *events, const std::string &name, const NetworkAddress &address, ISocketFactory *socketFactory,
      deskflow::Screen *screen
  );
  Client(Client const &) = delete;
  Client(Client &&) = delete;
  ~Client() override;

  Client &operator=(Client const &) = delete;
  Client &operator=(Client &&) = delete;

  //! @name manipulators
  //@{

  //! Connect to server
  /*!
  Starts an attempt to connect to the server.  This is ignored if
  the client is trying to connect or is already connected.
  */
  void connect(size_t addressIndex = 0);

  //! Disconnect
  /*!
  Disconnects from the server with an optional error message.
  */
  void disconnect(const char *msg);

  //! Refuse connection
  /*!
  Disconnects from the server with an optional error message.
  Unlike disconnect this function doesn't try to use other ip addresses
  */
  void refuseConnection(const char *msg);

  //! Notify of handshake complete
  /*!
  Notifies the client that the connection handshake has completed.
  */
  virtual void handshakeComplete();

  //@}
  //! @name accessors
  //@{

  //! Test if connected
  /*!
  Returns true iff the client is successfully connected to the server.
  */
  bool isConnected() const;

  //! Test if connecting
  /*!
  Returns true iff the client is currently attempting to connect to
  the server.
  */
  bool isConnecting() const;

  //! Get address of server
  /*!
  Returns the address of the server the client is connected (or wants
  to connect) to.
  */
  NetworkAddress getServerAddress() const;

  //! Return last resolved adresses count
  size_t getLastResolvedAddressesCount() const
  {
    return m_resolvedAddressesCount;
  }

  //@}

  // IScreen overrides
  void *getEventTarget() const final;
  bool getClipboard(ClipboardID id, IClipboard *) const override;
  void getShape(int32_t &x, int32_t &y, int32_t &width, int32_t &height) const override;
  void getCursorPos(int32_t &x, int32_t &y) const override;

  // IClient overrides
  void enter(int32_t xAbs, int32_t yAbs, uint32_t seqNum, KeyModifierMask mask, bool forScreensaver) override;
  bool leave() override;
  void setClipboard(ClipboardID, const IClipboard *) override;
  void grabClipboard(ClipboardID) override;
  void setClipboardDirty(ClipboardID, bool) override;
  void keyDown(KeyID, KeyModifierMask, KeyButton, const std::string &) override;
  void keyRepeat(KeyID, KeyModifierMask, int32_t count, KeyButton, const std::string &lang) override;
  void keyUp(KeyID, KeyModifierMask, KeyButton) override;
  void mouseDown(ButtonID) override;
  void mouseUp(ButtonID) override;
  void mouseMove(int32_t xAbs, int32_t yAbs) override;
  void mouseRelativeMove(int32_t xRel, int32_t yRel) override;
  void mouseWheel(int32_t xDelta, int32_t yDelta) override;
  void screensaver(bool activate) override;
  void resetOptions() override;
  void setOptions(const OptionsList &options) override;
  std::string getName() const override;

private:
  void sendClipboard(ClipboardID);
  void sendEvent(deskflow::EventTypes);
  void sendConnectionFailedEvent(const char *msg);
  void setupConnecting();
  void setupConnection();
  void setupScreen();
  void setupTimer();
  void cleanup();
  void cleanupConnecting();
  void cleanupConnection();
  void cleanupScreen();
  void cleanupTimer();
  void cleanupStream();
  void handleConnected();
  void handleConnectionFailed(const Event &event);
  void handleConnectTimeout();
  void handleOutputError();
  void handleDisconnected();
  void handleShapeChanged();
  void handleClipboardGrabbed(const Event &event);
  void handleHello();
  void handleSuspend();
  void handleResume();
  void sendClipboardThread(void *);
  void bindNetworkInterface(IDataSocket *socket) const;

private:
  std::string m_name;
  NetworkAddress m_serverAddress;
  ISocketFactory *m_socketFactory = nullptr;
  deskflow::Screen *m_screen = nullptr;
  deskflow::IStream *m_stream = nullptr;
  EventQueueTimer *m_timer = nullptr;
  ServerProxy *m_server = nullptr;
  bool m_ready = false;
  bool m_active = false;
  bool m_suspended = false;
  bool m_connectOnResume = false;
  bool m_ownClipboard[kClipboardEnd];
  bool m_sentClipboard[kClipboardEnd];
  IClipboard::Time m_timeClipboard[kClipboardEnd];
  std::string m_dataClipboard[kClipboardEnd];
  IEventQueue *m_events = nullptr;
  bool m_useSecureNetwork = false;
  bool m_enableClipboard = true;
  size_t m_maximumClipboardSize = INT_MAX;
  size_t m_resolvedAddressesCount = 0;
  std::unique_ptr<deskflow::client::HelloBack> m_pHelloBack;
};
