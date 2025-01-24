/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2012 - 2016 Symless Ltd.
 * SPDX-FileCopyrightText: (C) 2002 Chris Schoeneman
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#pragma once

#include "deskflow/IClient.h"

#include "HelloBack.h"
#include "base/EventTypes.h"
#include "deskflow/ClientArgs.h"
#include "deskflow/Clipboard.h"
#include "deskflow/DragInformation.h"
#include "deskflow/INode.h"
#include "mt/CondVar.h"
#include "net/NetworkAddress.h"

#include <memory>

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
class Client : public IClient, public INode
{
public:
  class FailInfo
  {
  public:
    FailInfo(const char *what) : m_retry(false), m_what(what)
    {
    }
    bool m_retry;
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
      deskflow::Screen *screen, deskflow::ClientArgs const &args
  );
  Client(Client const &) = delete;
  Client(Client &&) = delete;
  ~Client();

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

  //! Received drag information
  void dragInfoReceived(uint32_t fileNum, std::string data);

  //! Create a new thread and use it to send file to Server
  void sendFileToServer(const char *filename);

  //! Send dragging file information back to server
  void sendDragInfo(uint32_t fileCount, std::string &info, size_t size);

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

  //! Return true if received file size is valid
  bool isReceivedFileSizeValid();

  //! Return expected file size
  size_t &getExpectedFileSize()
  {
    return m_expectedFileSize;
  }

  //! Return received file data
  std::string &getReceivedFileData()
  {
    return m_receivedFileData;
  }

  //! Return drag file list
  DragFileList getDragFileList()
  {
    return m_dragFileList;
  }

  //! Return last resolved adresses count
  size_t getLastResolvedAddressesCount() const
  {
    return m_resolvedAddressesCount;
  }

  //@}

  // IScreen overrides
  virtual void *getEventTarget() const;
  virtual bool getClipboard(ClipboardID id, IClipboard *) const;
  virtual void getShape(int32_t &x, int32_t &y, int32_t &width, int32_t &height) const;
  virtual void getCursorPos(int32_t &x, int32_t &y) const;

  // IClient overrides
  virtual void enter(int32_t xAbs, int32_t yAbs, uint32_t seqNum, KeyModifierMask mask, bool forScreensaver);
  virtual bool leave();
  virtual void setClipboard(ClipboardID, const IClipboard *);
  virtual void grabClipboard(ClipboardID);
  virtual void setClipboardDirty(ClipboardID, bool);
  virtual void keyDown(KeyID, KeyModifierMask, KeyButton, const std::string &);
  virtual void keyRepeat(KeyID, KeyModifierMask, int32_t count, KeyButton, const std::string &lang);
  virtual void keyUp(KeyID, KeyModifierMask, KeyButton);
  virtual void mouseDown(ButtonID);
  virtual void mouseUp(ButtonID);
  virtual void mouseMove(int32_t xAbs, int32_t yAbs);
  virtual void mouseRelativeMove(int32_t xRel, int32_t yRel);
  virtual void mouseWheel(int32_t xDelta, int32_t yDelta);
  virtual void screensaver(bool activate);
  virtual void resetOptions();
  virtual void setOptions(const OptionsList &options);
  virtual std::string getName() const;

private:
  void sendClipboard(ClipboardID);
  void sendEvent(Event::Type, void *);
  void sendConnectionFailedEvent(const char *msg);
  void sendFileChunk(const void *data);
  void sendFileThread(void *);
  void writeToDropDirThread(void *);
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
  void handleConnected(const Event &, void *);
  void handleConnectionFailed(const Event &, void *);
  void handleConnectTimeout(const Event &, void *);
  void handleOutputError(const Event &, void *);
  void handleDisconnected(const Event &, void *);
  void handleShapeChanged(const Event &, void *);
  void handleClipboardGrabbed(const Event &, void *);
  void handleHello(const Event &, void *);
  void handleSuspend(const Event &event, void *);
  void handleResume(const Event &event, void *);
  void handleFileChunkSending(const Event &, void *);
  void handleFileRecieveCompleted(const Event &, void *);
  void handleStopRetry(const Event &, void *);
  void onFileRecieveCompleted();
  void sendClipboardThread(void *);
  void bindNetworkInterface(IDataSocket *socket) const;

public:
  bool m_mock;

private:
  std::string m_name;
  NetworkAddress m_serverAddress;
  ISocketFactory *m_socketFactory;
  deskflow::Screen *m_screen;
  deskflow::IStream *m_stream;
  EventQueueTimer *m_timer;
  ServerProxy *m_server;
  bool m_ready;
  bool m_active;
  bool m_suspended;
  bool m_connectOnResume;
  bool m_ownClipboard[kClipboardEnd];
  bool m_sentClipboard[kClipboardEnd];
  IClipboard::Time m_timeClipboard[kClipboardEnd];
  std::string m_dataClipboard[kClipboardEnd];
  IEventQueue *m_events;
  std::size_t m_expectedFileSize;
  std::string m_receivedFileData;
  DragFileList m_dragFileList;
  std::string m_dragFileExt;
  using AutoThread = std::unique_ptr<Thread>;
  AutoThread m_sendFileThread;
  AutoThread m_writeToDropDirThread;
  bool m_useSecureNetwork;
  bool m_enableClipboard;
  size_t m_maximumClipboardSize;
  deskflow::ClientArgs m_args;
  size_t m_resolvedAddressesCount = 0;
  std::unique_ptr<deskflow::client::HelloBack> m_pHelloBack;
};
