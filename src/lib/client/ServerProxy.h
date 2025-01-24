/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2012 - 2016 Symless Ltd.
 * SPDX-FileCopyrightText: (C) 2002 Chris Schoeneman
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#pragma once

#include "base/Event.h"
#include "base/Stopwatch.h"
#include "deskflow/clipboard_types.h"
#include "deskflow/key_types.h"
#include "deskflow/languages/LanguageManager.h"

class Client;
class ClientInfo;
class EventQueueTimer;
class IClipboard;
namespace deskflow {
class IStream;
}
class IEventQueue;

//! Proxy for server
/*!
This class acts a proxy for the server, converting calls into messages
to the server and messages from the server to calls on the client.
*/
class ServerProxy
{
public:
  /*!
  Process messages from the server on \p stream and forward to
  \p client.
  */
  ServerProxy(Client *client, deskflow::IStream *stream, IEventQueue *events);
  ServerProxy(ServerProxy const &) = delete;
  ServerProxy(ServerProxy &&) = delete;
  ~ServerProxy();

  ServerProxy &operator=(ServerProxy const &) = delete;
  ServerProxy &operator=(ServerProxy &&) = delete;

  //! @name manipulators
  //@{

  void onInfoChanged();
  bool onGrabClipboard(ClipboardID);
  void onClipboardChanged(ClipboardID, const IClipboard *);

  //@}

  // sending file chunk to server
  void fileChunkSending(uint8_t mark, char *data, size_t dataSize);

  // sending dragging information to server
  void sendDragInfo(uint32_t fileCount, const char *info, size_t size);

#ifdef TEST_ENV
  void handleDataForTest()
  {
    handleData(Event(), NULL);
  }
#endif

protected:
  enum EResult
  {
    kOkay,
    kUnknown,
    kDisconnect
  };
  EResult parseHandshakeMessage(const uint8_t *code);
  EResult parseMessage(const uint8_t *code);

private:
  // if compressing mouse motion then send the last motion now
  void flushCompressedMouse();

  void sendInfo(const ClientInfo &);

  void resetKeepAliveAlarm();
  void setKeepAliveRate(double);

  // modifier key translation
  KeyID translateKey(KeyID) const;
  KeyModifierMask translateModifierMask(KeyModifierMask) const;

  // event handlers
  void handleData(const Event &, void *);
  void handleKeepAliveAlarm(const Event &, void *);

  // message handlers
  void enter();
  void leave();
  void setClipboard();
  void grabClipboard();
  void keyDown(uint16_t id, uint16_t mask, uint16_t button, const std::string &lang);
  void keyRepeat();
  void keyUp();
  void mouseDown();
  void mouseUp();
  void mouseMove();
  void mouseRelativeMove();
  void mouseWheel();
  void screensaver();
  void resetOptions();
  void setOptions();
  void queryInfo();
  void infoAcknowledgment();
  void fileChunkReceived();
  void dragInfoReceived();
  void handleClipboardSendingEvent(const Event &, void *);
  void secureInputNotification();
  void setServerLanguages();
  void setActiveServerLanguage(const std::string &language);
  void checkMissedLanguages() const;

private:
  typedef EResult (ServerProxy::*MessageParser)(const uint8_t *);

  Client *m_client;
  deskflow::IStream *m_stream;

  uint32_t m_seqNum;

  bool m_compressMouse;
  bool m_compressMouseRelative;
  int32_t m_xMouse, m_yMouse;
  int32_t m_dxMouse, m_dyMouse;

  bool m_ignoreMouse;

  KeyModifierID m_modifierTranslationTable[kKeyModifierIDLast];

  double m_keepAliveAlarm;
  EventQueueTimer *m_keepAliveAlarmTimer;

  MessageParser m_parser;
  IEventQueue *m_events;
  std::string m_serverLanguage = "";
  bool m_isUserNotifiedAboutLanguageSyncError = false;
  deskflow::languages::LanguageManager m_languageManager;
};
