/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2025 Deskflow Developers.
 * SPDX-FileCopyrightText: (C) 2012 Symless Ltd.
 * SPDX-FileCopyrightText: (C) 2002 Chris Schoeneman
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#include "server/Server.h"

#include "arch/Arch.h"
#include "base/IEventQueue.h"
#include "base/Log.h"
#include "base/TMethodJob.h"
#include "deskflow/AppUtil.h"
#include "deskflow/DeskflowException.h"
#include "deskflow/IPlatformScreen.h"
#include "deskflow/OptionTypes.h"
#include "deskflow/PacketStreamFilter.h"
#include "deskflow/ProtocolTypes.h"
#include "deskflow/Screen.h"
#include "deskflow/StreamChunker.h"
#include "mt/Thread.h"
#include "net/TCPSocket.h"
#include "server/ClientListener.h"
#include "server/ClientProxy.h"
#include "server/ClientProxyUnknown.h"
#include "server/PrimaryClient.h"

#ifdef _WIN32
#include <array>
#endif
#include <cmath>
#include <cstdlib>
#include <cstring>
#include <ctime>
#include <stdexcept>

using namespace deskflow::server;

//
// Server
//

Server::Server(
    ServerConfig &config, PrimaryClient *primaryClient, deskflow::Screen *screen, IEventQueue *events,
    deskflow::ServerArgs const &args
)
    : m_primaryClient(primaryClient),
      m_active(primaryClient),
      m_config(&config),
      m_inputFilter(config.getInputFilter()),
      m_screen(screen),
      m_events(events),
      m_args(args)
{
  // must have a primary client and it must have a canonical name
  assert(m_primaryClient != nullptr);
  assert(config.isScreen(primaryClient->getName()));
  assert(m_screen != nullptr);

  std::string primaryName = getName(primaryClient);

  // clear clipboards
  for (auto &clipboard : m_clipboards) {
    clipboard.m_clipboardOwner = primaryName;
    clipboard.m_clipboardSeqNum = m_seqNum;
    if (clipboard.m_clipboard.open(0)) {
      clipboard.m_clipboard.empty();
      clipboard.m_clipboard.close();
    }
    clipboard.m_clipboardData = clipboard.m_clipboard.marshall();
  }

  // install event handlers
  m_events->addHandler(EventTypes::Timer, this, [this](const auto &) { handleSwitchWaitTimeout(); });
  m_events->addHandler(EventTypes::KeyStateKeyDown, m_inputFilter, [this](const auto &e) { handleKeyDownEvent(e); });
  m_events->addHandler(EventTypes::KeyStateKeyUp, m_inputFilter, [this](const auto &e) { handleKeyUpEvent(e); });
  m_events->addHandler(EventTypes::KeyStateKeyRepeat, m_inputFilter, [this](const auto &e) {
    handleKeyRepeatEvent(e);
  });
  m_events->addHandler(EventTypes::PrimaryScreenButtonDown, m_inputFilter, [this](const auto &e) {
    handleButtonDownEvent(e);
  });
  m_events->addHandler(EventTypes::PrimaryScreenButtonUp, m_inputFilter, [this](const auto &e) {
    handleButtonUpEvent(e);
  });
  m_events->addHandler(
      EventTypes::PrimaryScreenMotionOnPrimary, m_primaryClient->getEventTarget(),
      [this](const auto &e) { handleMotionPrimaryEvent(e); }
  );
  m_events->addHandler(
      EventTypes::PrimaryScreenMotionOnSecondary, m_primaryClient->getEventTarget(),
      [this](const auto &e) { handleMotionSecondaryEvent(e); }
  );
  m_events->addHandler(EventTypes::PrimaryScreenWheel, m_primaryClient->getEventTarget(), [this](const auto &e) {
    handleWheelEvent(e);
  });
  m_events->addHandler(
      EventTypes::PrimaryScreenSaverActivated, m_primaryClient->getEventTarget(),
      [this](const auto &) { onScreensaver(true); }
  );
  m_events->addHandler(
      EventTypes::PrimaryScreenSaverDeactivated, m_primaryClient->getEventTarget(),
      [this](const auto &) { onScreensaver(false); }
  );
  m_events->addHandler(EventTypes::ServerSwitchToScreen, m_inputFilter, [this](const auto &e) {
    handleSwitchToScreenEvent(e);
  });
  m_events->addHandler(EventTypes::ServerSwitchInDirection, m_inputFilter, [this](const auto &e) {
    handleSwitchInDirectionEvent(e);
  });
  m_events->addHandler(EventTypes::ServerToggleScreen, m_inputFilter, [this](const auto &e) {
    handleToggleScreenEvent(e);
  });
  m_events->addHandler(EventTypes::ServerKeyboardBroadcast, m_inputFilter, [this](const auto &e) {
    handleKeyboardBroadcastEvent(e);
  });
  m_events->addHandler(EventTypes::ServerLockCursorToScreen, m_inputFilter, [this](const auto &e) {
    handleLockCursorToScreenEvent(e);
  });
  m_events->addHandler(EventTypes::PrimaryScreenFakeInputBegin, m_inputFilter, [this](const auto &) {
    m_primaryClient->fakeInputBegin();
  });
  m_events->addHandler(EventTypes::PrimaryScreenFakeInputEnd, m_inputFilter, [this](const auto &) {
    m_primaryClient->fakeInputEnd();
  });

  // add connection
  addClient(m_primaryClient);

  // set initial configuration
  setConfig(config);

  // enable primary client
  m_primaryClient->enable();
  m_inputFilter->setPrimaryClient(m_primaryClient);

  // Determine if scroll lock is already set. If so, lock the cursor to the
  // primary screen
  if (m_primaryClient->getToggleMask() & KeyModifierScrollLock) {
    LOG_NOTE("scroll lock is on, locking cursor to screen");
    m_lockedToScreen = true;
  }
}

Server::~Server()
{
  // remove event handlers and timers
  using enum EventTypes;
  m_events->removeHandler(KeyStateKeyDown, m_inputFilter);
  m_events->removeHandler(KeyStateKeyUp, m_inputFilter);
  m_events->removeHandler(KeyStateKeyRepeat, m_inputFilter);
  m_events->removeHandler(PrimaryScreenButtonDown, m_inputFilter);
  m_events->removeHandler(PrimaryScreenButtonUp, m_inputFilter);
  m_events->removeHandler(PrimaryScreenMotionOnPrimary, m_primaryClient->getEventTarget());
  m_events->removeHandler(PrimaryScreenMotionOnSecondary, m_primaryClient->getEventTarget());
  m_events->removeHandler(PrimaryScreenWheel, m_primaryClient->getEventTarget());
  m_events->removeHandler(PrimaryScreenSaverActivated, m_primaryClient->getEventTarget());
  m_events->removeHandler(PrimaryScreenSaverDeactivated, m_primaryClient->getEventTarget());
  m_events->removeHandler(PrimaryScreenFakeInputBegin, m_inputFilter);
  m_events->removeHandler(PrimaryScreenFakeInputEnd, m_inputFilter);
  m_events->removeHandler(Timer, this);
  stopSwitch();

  try {
    // force immediate disconnection of secondary clients
    disconnect();
  } catch (std::exception &e) { // NOSONAR
    LOG_ERR("failed to disconnect: %s", e.what());
  }

  for (auto index = m_oldClients.begin(); index != m_oldClients.end(); ++index) {
    BaseClientProxy *client = index->first;
    m_events->deleteTimer(index->second);
    m_events->removeHandler(Timer, client);
    m_events->removeHandler(ClientProxyDisconnected, client);
    delete client;
  }

  // remove input filter
  m_inputFilter->setPrimaryClient(nullptr);

  // disable and disconnect primary client
  m_primaryClient->disable();
  removeClient(m_primaryClient);
}

bool Server::setConfig(const ServerConfig &config)
{
  // refuse configuration if it doesn't include the primary screen
  if (!config.isScreen(m_primaryClient->getName())) {
    return false;
  }

  // close clients that are connected but being dropped from the
  // configuration.
  closeClients(config);

  // cut over
  processOptions();

  // add ScrollLock as a hotkey to lock to the screen.  this was a
  // built-in feature in earlier releases and is now supported via
  // the user configurable hotkey mechanism.  if the user has already
  // registered ScrollLock for something else then that will win but
  // we will unfortunately generate a warning.  if the user has
  // configured a LockCursorToScreenAction then we don't add
  // ScrollLock as a hotkey.
  if (!m_disableLockToScreen && !m_config->hasLockToScreenAction()) {
    IPlatformScreen::KeyInfo *key = IPlatformScreen::KeyInfo::alloc(kKeyScrollLock, 0, 0, 0);
    InputFilter::Rule rule(new InputFilter::KeystrokeCondition(m_events, key));
    rule.adoptAction(new InputFilter::LockCursorToScreenAction(m_events), true);
    m_inputFilter->addFilterRule(rule);
  }

  // tell primary screen about reconfiguration
  m_primaryClient->reconfigure(getActivePrimarySides());

  // tell all (connected) clients about current options
  for (ClientList::const_iterator index = m_clients.begin(); index != m_clients.end(); ++index) {
    BaseClientProxy *client = index->second;
    sendOptions(client);
  }

  return true;
}

void Server::adoptClient(BaseClientProxy *client)
{
  assert(client != nullptr);

  // watch for client disconnection
  m_events->addHandler(EventTypes::ClientProxyDisconnected, client, [this, client](const auto &) {
    handleClientDisconnected(client);
  });

  // name must be in our configuration
  if (!m_config->isScreen(client->getName())) {
    LOG_WARN("unrecognised client name \"%s\", check server config", client->getName().c_str());
    closeClient(client, kMsgEUnknown);
    return;
  }

  // add client to client list
  if (!addClient(client)) {
    // can only have one screen with a given name at any given time
    LOG_WARN("a client with name \"%s\" is already connected", getName(client).c_str());
    closeClient(client, kMsgEBusy);
    return;
  }
  LOG_IPC("client \"%s\" has connected", getName(client).c_str());

  // send configuration options to client
  sendOptions(client);

  // activate screen saver on new client if active on the primary screen
  if (m_activeSaver != nullptr) {
    client->screensaver(true);
  }

  // send notification
  auto *info = new Server::ScreenConnectedInfo(getName(client));
  m_events->addEvent(Event(EventTypes::ServerConnected, m_primaryClient->getEventTarget(), info));
}

void Server::disconnect()
{
  // close all secondary clients
  if (m_clients.size() > 1 || !m_oldClients.empty()) {
    Config emptyConfig(m_events);
    closeClients(emptyConfig);
  } else {
    m_events->addEvent(Event(EventTypes::ServerDisconnected, this));
  }
}

std::string Server::protocolString() const
{
  using enum NetworkProtocol;
  if (m_protocol == Synergy) {
    return kSynergyProtocolName;
  } else if (m_protocol == Barrier) {
    return kBarrierProtocolName;
  }
  throw InvalidProtocolException();
}

uint32_t Server::getNumClients() const
{
  return (int32_t)m_clients.size();
}

void Server::getClients(std::vector<std::string> &list) const
{
  list.clear();
  for (auto index = m_clients.begin(); index != m_clients.end(); ++index) {
    list.push_back(index->first);
  }
}

std::string Server::getName(const BaseClientProxy *client) const
{
  std::string name = m_config->getCanonicalName(client->getName());
  if (name.empty()) {
    name = client->getName();
  }
  return name;
}

uint32_t Server::getActivePrimarySides() const
{
  using enum DirectionMask;
  using enum Direction;
  uint32_t sides = 0;
  if (!isLockedToScreenServer()) {
    if (hasAnyNeighbor(m_primaryClient, Left)) {
      sides |= static_cast<int>(LeftMask);
    }
    if (hasAnyNeighbor(m_primaryClient, Right)) {
      sides |= static_cast<int>(RightMask);
    }
    if (hasAnyNeighbor(m_primaryClient, Top)) {
      sides |= static_cast<int>(TopMask);
    }
    if (hasAnyNeighbor(m_primaryClient, Bottom)) {
      sides |= static_cast<int>(BottomMask);
    }
  }
  return sides;
}

bool Server::isLockedToScreenServer() const
{
  // locked if scroll-lock is toggled on
  return m_lockedToScreen;
}

bool Server::isLockedToScreen() const
{
  if (m_disableLockToScreen) {
    return false;
  }

  // locked if we say we're locked
  if (isLockedToScreenServer()) {
    LOG_NOTE("cursor is locked to screen, check scroll lock key");
    return true;
  }

  // locked if primary says we're locked
  if (m_primaryClient->isLockedToScreen()) {
    return true;
  }

  // not locked
  return false;
}

int32_t Server::getJumpZoneSize(const BaseClientProxy *client) const
{
  if (client == m_primaryClient) {
    return m_primaryClient->getJumpZoneSize();
  } else {
    return 0;
  }
}

void Server::switchScreen(BaseClientProxy *dst, int32_t x, int32_t y, bool forScreensaver)
{
  assert(dst != nullptr);

  int32_t dx;
  int32_t dy;
  int32_t dw;
  int32_t dh;
  dst->getShape(dx, dy, dw, dh);

  // any of these conditions seem to trigger when the portal permission dialog
  // is visible on wayland. this was previously an assert, but that's pretty
  // annoying since it makes the mouse unusable on the server and you'll have to
  // ssh into your machine to kill it. better to just log a warning.
  if (x < dx) {
    LOG_WARN(
        "on switch, x (%d) is less than the left boundary dx (%d)", //
        x, dx
    );
  }
  if (y < dy) {
    LOG_WARN(
        "on switch, y (%d) is less than the top boundary dy (%d)", //
        y, dy
    );
  }
  if (x >= dx + dw) {
    LOG_WARN(
        "on switch, x (%d) exceeds the right boundary (dx + width = %d)", //
        x, dx + dw
    );
  }
  if (y >= dy + dh) {
    LOG_WARN(
        "on switch, y (%d) exceeds the bottom boundary (dy + height = %d)", //
        y, dy + dh
    );
  }

  assert(m_active != nullptr);

  LOG_INFO("switch from \"%s\" to \"%s\" at %d,%d", getName(m_active).c_str(), getName(dst).c_str(), x, y);

  // stop waiting to switch
  stopSwitch();

  // record new position
  m_x = x;
  m_y = y;
  m_xDelta = 0;
  m_yDelta = 0;
  m_xDelta2 = 0;
  m_yDelta2 = 0;

  // wrapping means leaving the active screen and entering it again.
  // since that's a waste of time we skip that and just warp the
  // mouse.
  if (m_active != dst) {
    // leave active screen
    if (!m_active->leave()) {
      // cannot leave screen
      LOG_WARN("can't leave screen");
      return;
    }

    // update the primary client's clipboards if we're leaving the
    // primary screen.
    if (m_active == m_primaryClient && m_enableClipboard) {
      for (ClipboardID id = 0; id < kClipboardEnd; ++id) {
        const ClipboardInfo &clipboard = m_clipboards[id];
        if (clipboard.m_clipboardOwner == getName(m_primaryClient)) {
          onClipboardChanged(m_primaryClient, id, clipboard.m_clipboardSeqNum);
        }
      }
    }

#if defined(__APPLE__)
    if (dst != m_primaryClient) {
      std::string secureInputApplication = m_primaryClient->getSecureInputApp();
      if (secureInputApplication != "") {
        // display notification on the server
        m_primaryClient->secureInputNotification(secureInputApplication);
        // display notification on the client
        dst->secureInputNotification(secureInputApplication);
      }
    }
#endif

    // cut over
    m_active = dst;

    // increment enter sequence number
    ++m_seqNum;

    // enter new screen
    m_active->enter(x, y, m_seqNum, m_primaryClient->getToggleMask(), forScreensaver);

    if (m_enableClipboard) {
      // send the clipboard data to new active screen
      for (ClipboardID id = 0; id < kClipboardEnd; ++id) {
        // Hackity hackity hack
        if (m_clipboards[id].m_clipboard.marshall().size() > (m_maximumClipboardSize * 1024)) {
          continue;
        }
        m_active->setClipboard(id, &m_clipboards[id].m_clipboard);
      }
    }

    Server::SwitchToScreenInfo *info = Server::SwitchToScreenInfo::alloc(m_active->getName());
    m_events->addEvent(Event(EventTypes::ServerScreenSwitched, this, info));
  } else {
    m_active->mouseMove(x, y);
  }
}

void Server::jumpToScreen(BaseClientProxy *newScreen)
{
  assert(newScreen != nullptr);

  // record the current cursor position on the active screen
  m_active->setJumpCursorPos(m_x, m_y);

  // get the last cursor position on the target screen
  int32_t x;
  int32_t y;
  newScreen->getJumpCursorPos(x, y);

  switchScreen(newScreen, x, y, false);
}

float Server::mapToFraction(const BaseClientProxy *client, Direction dir, int32_t x, int32_t y) const
{
  int32_t sx;
  int32_t sy;
  int32_t sw;
  int32_t sh;
  client->getShape(sx, sy, sw, sh);
  switch (dir) {
    using enum Direction;
  case Left:
  case Right:
    return (y - sy + 0.5f) / static_cast<float>(sh);

  case Top:
  case Bottom:
    return (x - sx + 0.5f) / static_cast<float>(sw);

  case NoDirection:
    assert(0 && "bad direction");
    break;
  }
  return 0.0f;
}

void Server::mapToPixel(const BaseClientProxy *client, Direction dir, float f, int32_t &x, int32_t &y) const
{
  int32_t sx;
  int32_t sy;
  int32_t sw;
  int32_t sh;
  client->getShape(sx, sy, sw, sh);
  switch (dir) {
    using enum Direction;
  case Left:
  case Right:
    y = static_cast<int32_t>(f * sh) + sy;
    break;

  case Top:
  case Bottom:
    x = static_cast<int32_t>(f * sw) + sx;
    break;

  case NoDirection:
    assert(0 && "bad direction");
    break;
  }
}

bool Server::hasAnyNeighbor(const BaseClientProxy *client, Direction dir) const
{
  assert(client != nullptr);

  return m_config->hasNeighbor(getName(client), dir);
}

BaseClientProxy *Server::getNeighbor(const BaseClientProxy *src, Direction dir, int32_t &x, int32_t &y) const
{
  // note -- must be locked on entry

  assert(src != nullptr);

  // get source screen name
  std::string srcName = getName(src);
  assert(!srcName.empty());
  LOG_DEBUG2("find neighbor on %s of \"%s\"", Config::dirName(dir), srcName.c_str());

  // convert position to fraction
  float t = mapToFraction(src, dir, x, y);

  // search for the closest neighbor that exists in direction dir
  float tTmp;
  for (;;) {
    std::string dstName(m_config->getNeighbor(srcName, dir, t, &tTmp));

    // if nothing in that direction then return nullptr. if the
    // destination is the source then we can make no more
    // progress in this direction.  since we haven't found a
    // connected neighbor we return nullptr.
    if (dstName.empty()) {
      LOG_DEBUG2("no neighbor on %s of \"%s\"", Config::dirName(dir), srcName.c_str());
      return nullptr;
    }

    // look up neighbor cell.  if the screen is connected and
    // ready then we can stop.
    if (ClientList::const_iterator index = m_clients.find(dstName); index != m_clients.end()) {
      LOG_DEBUG2("\"%s\" is on %s of \"%s\" at %f", dstName.c_str(), Config::dirName(dir), srcName.c_str(), t);
      mapToPixel(index->second, dir, tTmp, x, y);
      return index->second;
    }

    // skip over unconnected screen
    LOG_DEBUG2("ignored \"%s\" on %s of \"%s\"", dstName.c_str(), Config::dirName(dir), srcName.c_str());
    srcName = dstName;

    // use position on skipped screen
    t = tTmp;
  }
}

BaseClientProxy *Server::mapToNeighbor(BaseClientProxy *src, Direction srcSide, int32_t &x, int32_t &y) const
{
  // note -- must be locked on entry

  assert(src != nullptr);

  // get the first neighbor
  BaseClientProxy *dst = getNeighbor(src, srcSide, x, y);
  if (dst == nullptr) {
    return nullptr;
  }

  // get the source screen's size
  int32_t dx;
  int32_t dy;
  int32_t dw;
  int32_t dh;
  BaseClientProxy *lastGoodScreen = src;
  lastGoodScreen->getShape(dx, dy, dw, dh);

  // find destination screen, adjusting x or y (but not both).  the
  // searches are done in a sort of canonical screen space where
  // the upper-left corner is 0,0 for each screen.  we adjust from
  // actual to canonical position on entry to and from canonical to
  // actual on exit from the search.
  switch (srcSide) {
    using enum Direction;
  case Left:
    x -= dx;
    while (dst != nullptr) {
      lastGoodScreen = dst;
      lastGoodScreen->getShape(dx, dy, dw, dh);
      x += dw;
      if (x >= 0) {
        break;
      }
      LOG_DEBUG2("skipping over screen %s", getName(dst).c_str());
      dst = getNeighbor(lastGoodScreen, srcSide, x, y);
    }
    assert(lastGoodScreen != nullptr);
    x += dx;
    break;

  case Right:
    x -= dx;
    while (dst != nullptr) {
      x -= dw;
      lastGoodScreen = dst;
      lastGoodScreen->getShape(dx, dy, dw, dh);
      if (x < dw) {
        break;
      }
      LOG_DEBUG2("skipping over screen %s", getName(dst).c_str());
      dst = getNeighbor(lastGoodScreen, srcSide, x, y);
    }
    assert(lastGoodScreen != nullptr);
    x += dx;
    break;

  case Top:
    y -= dy;
    while (dst != nullptr) {
      lastGoodScreen = dst;
      lastGoodScreen->getShape(dx, dy, dw, dh);
      y += dh;
      if (y >= 0) {
        break;
      }
      LOG_DEBUG2("skipping over screen %s", getName(dst).c_str());
      dst = getNeighbor(lastGoodScreen, srcSide, x, y);
    }
    assert(lastGoodScreen != nullptr);
    y += dy;
    break;

  case Bottom:
    y -= dy;
    while (dst != nullptr) {
      y -= dh;
      lastGoodScreen = dst;
      lastGoodScreen->getShape(dx, dy, dw, dh);
      if (y < dh) {
        break;
      }
      LOG_DEBUG2("skipping over screen %s", getName(dst).c_str());
      dst = getNeighbor(lastGoodScreen, srcSide, x, y);
    }
    assert(lastGoodScreen != nullptr);
    y += dy;
    break;

  case NoDirection:
    assert(0 && "bad direction");
    return nullptr;
  }

  // save destination screen
  assert(lastGoodScreen != nullptr);
  dst = lastGoodScreen;

  // if entering primary screen then be sure to move in far enough
  // to avoid the jump zone.  if entering a side that doesn't have
  // a neighbor (i.e. an asymmetrical side) then we don't need to
  // move inwards because that side can't provoke a jump.
  avoidJumpZone(dst, srcSide, x, y);

  return dst;
}

void Server::avoidJumpZone(const BaseClientProxy *dst, Direction dir, int32_t &x, int32_t &y) const
{
  // we only need to avoid jump zones on the primary screen
  if (dst != m_primaryClient) {
    return;
  }

  const std::string dstName(getName(dst));
  int32_t dx;
  int32_t dy;
  int32_t dw;
  int32_t dh;
  dst->getShape(dx, dy, dw, dh);
  float t = mapToFraction(dst, dir, x, y);
  int32_t z = getJumpZoneSize(dst);

  // move in far enough to avoid the jump zone.  if entering a side
  // that doesn't have a neighbor (i.e. an asymmetrical side) then we
  // don't need to move inwards because that side can't provoke a jump.
  switch (dir) {
    using enum Direction;
  case Left:
    if (!m_config->getNeighbor(dstName, Right, t, nullptr).empty() && x > dx + dw - 1 - z)
      x = dx + dw - 1 - z;
    break;

  case Right:
    if (!m_config->getNeighbor(dstName, Left, t, nullptr).empty() && x < dx + z)
      x = dx + z;
    break;

  case Top:
    if (!m_config->getNeighbor(dstName, Bottom, t, nullptr).empty() && y > dy + dh - 1 - z)
      y = dy + dh - 1 - z;
    break;

  case Bottom:
    if (!m_config->getNeighbor(dstName, Top, t, nullptr).empty() && y < dy + z)
      y = dy + z;
    break;

  case NoDirection:
    assert(0 && "bad direction");
  }
}

bool Server::isSwitchOkay(
    BaseClientProxy *newScreen, Direction dir, int32_t x, int32_t y, int32_t xActive, int32_t yActive
)
{
  LOG_DEBUG1("try to leave \"%s\" on %s", getName(m_active).c_str(), Config::dirName(dir));

  // is there a neighbor?
  if (newScreen == nullptr) {
    // there's no neighbor.  we don't want to switch and we don't
    // want to try to switch later.
    LOG_DEBUG1("no neighbor %s", Config::dirName(dir));
    stopSwitch();
    return false;
  }

  // should we switch or not?
  bool preventSwitch = false;
  bool allowSwitch = false;

  // note if the switch direction has changed.  save the new
  // direction and screen if so.
  bool isNewDirection = (dir != m_switchDir);
  if (isNewDirection || m_switchScreen == nullptr) {
    m_switchDir = dir;
    m_switchScreen = newScreen;
  }

  // is this a double tap and do we care?
  if (!allowSwitch && m_switchTwoTapDelay > 0.0) {
    if (isNewDirection || !isSwitchTwoTapStarted() || !shouldSwitchTwoTap()) {
      // tapping a different or new edge or second tap not
      // fast enough.  prepare for second tap.
      preventSwitch = true;
      startSwitchTwoTap();
    } else {
      // got second tap
      allowSwitch = true;
    }
  }

  // if waiting before a switch then prepare to switch later
  if (!allowSwitch && m_switchWaitDelay > 0.0) {
    if (isNewDirection || !isSwitchWaitStarted()) {
      startSwitchWait(x, y);
    }
    preventSwitch = true;
  }

  // are we in a locked corner?  first check if screen has the option set
  // and, if not, check the global options.
  const Config::ScreenOptions *options = m_config->getOptions(getName(m_active));
  if (options == nullptr || !options->contains(kOptionScreenSwitchCorners)) {
    options = m_config->getOptions("");
  }
  if (options != nullptr && options->contains(kOptionScreenSwitchCorners)) {
    // get corner mask and size
    Config::ScreenOptions::const_iterator i = options->find(kOptionScreenSwitchCorners);
    auto corners = static_cast<uint32_t>(i->second);
    i = options->find(kOptionScreenSwitchCornerSize);
    int32_t size = 0;
    if (i != options->end()) {
      size = i->second;
    }

    // see if we're in a locked corner
    if ((getCorner(m_active, xActive, yActive, size) & corners) != 0) {
      // yep, no switching
      LOG_DEBUG1("locked in corner");
      preventSwitch = true;
      stopSwitch();
    }
  }

  // ignore if mouse is locked to screen and don't try to switch later
  if (!preventSwitch && isLockedToScreen()) {
    LOG_DEBUG1("locked to screen");
    preventSwitch = true;
    stopSwitch();
  }

  // check for optional needed modifiers
  if (KeyModifierMask mods = this->m_primaryClient->getToggleMask();
      !preventSwitch && ((this->m_switchNeedsShift && ((mods & KeyModifierShift) != KeyModifierShift)) ||
                         (this->m_switchNeedsControl && ((mods & KeyModifierControl) != KeyModifierControl)) ||
                         (this->m_switchNeedsAlt && ((mods & KeyModifierAlt) != KeyModifierAlt)))) {
    LOG_DEBUG1("need modifiers to switch");
    preventSwitch = true;
    stopSwitch();
  }

  return !preventSwitch;
}

void Server::noSwitch(int32_t x, int32_t y)
{
  armSwitchTwoTap(x, y);
  stopSwitchWait();
}

void Server::stopSwitch()
{
  if (m_switchScreen != nullptr) {
    m_switchScreen = nullptr;
    m_switchDir = Direction::NoDirection;
    stopSwitchTwoTap();
    stopSwitchWait();
  }
}

void Server::startSwitchTwoTap()
{
  m_switchTwoTapEngaged = true;
  m_switchTwoTapArmed = false;
  m_switchTwoTapTimer.reset();
  LOG_DEBUG1("waiting for second tap");
}

void Server::armSwitchTwoTap(int32_t x, int32_t y)
{
  if (m_switchTwoTapEngaged) {
    if (m_switchTwoTapTimer.getTime() > m_switchTwoTapDelay) {
      // second tap took too long.  disengage.
      stopSwitchTwoTap();
    } else if (!m_switchTwoTapArmed) {
      // still time for a double tap.  see if we left the tap
      // zone and, if so, arm the two tap.
      int32_t ax;
      int32_t ay;
      int32_t aw;
      int32_t ah;
      m_active->getShape(ax, ay, aw, ah);
      int32_t tapZone = m_primaryClient->getJumpZoneSize();
      if (tapZone < m_switchTwoTapZone) {
        tapZone = m_switchTwoTapZone;
      }
      if (x >= ax + tapZone && x < ax + aw - tapZone && y >= ay + tapZone && y < ay + ah - tapZone) {
        // win32 can generate bogus mouse events that appear to
        // move in the opposite direction that the mouse actually
        // moved.  try to ignore that crap here.
        switch (m_switchDir) {
          using enum Direction;
        case Left:
          m_switchTwoTapArmed = (m_xDelta > 0 && m_xDelta2 > 0);
          break;

        case Right:
          m_switchTwoTapArmed = (m_xDelta < 0 && m_xDelta2 < 0);
          break;

        case Top:
          m_switchTwoTapArmed = (m_yDelta > 0 && m_yDelta2 > 0);
          break;

        case Bottom:
          m_switchTwoTapArmed = (m_yDelta < 0 && m_yDelta2 < 0);
          break;

        default:
          break;
        }
      }
    }
  }
}

void Server::stopSwitchTwoTap()
{
  m_switchTwoTapEngaged = false;
  m_switchTwoTapArmed = false;
}

bool Server::isSwitchTwoTapStarted() const
{
  return m_switchTwoTapEngaged;
}

bool Server::shouldSwitchTwoTap() const
{
  // this is the second tap if two-tap is armed and this tap
  // came fast enough
  return (m_switchTwoTapArmed && m_switchTwoTapTimer.getTime() <= m_switchTwoTapDelay);
}

void Server::startSwitchWait(int32_t x, int32_t y)
{
  stopSwitchWait();
  m_switchWaitX = x;
  m_switchWaitY = y;
  m_switchWaitTimer = m_events->newOneShotTimer(m_switchWaitDelay, this);
  LOG_DEBUG1("waiting to switch");
}

void Server::stopSwitchWait()
{
  if (m_switchWaitTimer != nullptr) {
    m_events->deleteTimer(m_switchWaitTimer);
    m_switchWaitTimer = nullptr;
  }
}

bool Server::isSwitchWaitStarted() const
{
  return (m_switchWaitTimer != nullptr);
}

uint32_t Server::getCorner(const BaseClientProxy *client, int32_t x, int32_t y, int32_t size) const
{
  assert(client != nullptr);

  // get client screen shape
  int32_t ax;
  int32_t ay;
  int32_t aw;
  int32_t ah;
  client->getShape(ax, ay, aw, ah);

  // check for x,y on the left or right
  int32_t xSide;
  if (x <= ax) {
    xSide = -1;
  } else if (x >= ax + aw - 1) {
    xSide = 1;
  } else {
    xSide = 0;
  }

  // check for x,y on the top or bottom
  int32_t ySide;
  if (y <= ay) {
    ySide = -1;
  } else if (y >= ay + ah - 1) {
    ySide = 1;
  } else {
    ySide = 0;
  }

  // if against the left or right then check if y is within size
  if (xSide != 0) {
    if (y < ay + size) {
      return (xSide < 0) ? s_topLeftCornerMask : s_topRightCornerMask;
    } else if (y >= ay + ah - size) {
      return (xSide < 0) ? s_bottomLeftCornerMask : s_bottomRightCornerMask;
    }
  }

  // if against the left or right then check if y is within size
  if (ySide != 0) {
    if (x < ax + size) {
      return (ySide < 0) ? s_topLeftCornerMask : s_bottomLeftCornerMask;
    } else if (x >= ax + aw - size) {
      return (ySide < 0) ? s_topRightCornerMask : s_bottomRightCornerMask;
    }
  }

  return s_noCornerMask;
}

void Server::stopRelativeMoves()
{
  if (m_relativeMoves && m_active != m_primaryClient) {
    // warp to the center of the active client so we know where we are
    int32_t ax;
    int32_t ay;
    int32_t aw;
    int32_t ah;
    m_active->getShape(ax, ay, aw, ah);
    m_x = ax + (aw >> 1);
    m_y = ay + (ah >> 1);
    m_xDelta = 0;
    m_yDelta = 0;
    m_xDelta2 = 0;
    m_yDelta2 = 0;
    LOG_DEBUG2("synchronize move on %s by %d,%d", getName(m_active).c_str(), m_x, m_y);
    m_active->mouseMove(m_x, m_y);
  }
}

void Server::sendOptions(BaseClientProxy *client) const
{
  OptionsList optionsList;

  // look up options for client
  const Config::ScreenOptions *options = m_config->getOptions(getName(client));
  if (options != nullptr) {
    // convert options to a more convenient form for sending
    optionsList.reserve(2 * options->size());
    for (auto [optionId, optionValue] : *options) {
      optionsList.push_back(optionId);
      optionsList.push_back(static_cast<uint32_t>(optionValue));
    }
  }

  // look up global options
  options = m_config->getOptions("");
  if (options != nullptr) {
    // convert options to a more convenient form for sending
    optionsList.reserve(optionsList.size() + 2 * options->size());
    for (auto [optionId, optionValue] : *options) {
      optionsList.push_back(optionId);
      optionsList.push_back(static_cast<uint32_t>(optionValue));
    }
  }

  // send the options
  client->resetOptions();
  client->setOptions(optionsList);
}

void Server::processOptions()
{
  const Config::ScreenOptions *options = m_config->getOptions("");
  if (options == nullptr) {
    return;
  }

  m_switchNeedsShift = false;   // it seems if i don't add these
  m_switchNeedsControl = false; // lines, the 'reload config' option
  m_switchNeedsAlt = false;     // doesnt' work correct.

  bool newRelativeMoves = m_relativeMoves;
  for (auto [optionId, optionValue] : *options) {
    const OptionID id = optionId;
    const OptionValue value = optionValue;
    if (id == kOptionProtocol) {
      using enum NetworkProtocol;
      const auto enumValue = static_cast<NetworkProtocol>(value);
      if (enumValue == Synergy) {
        m_protocol = Synergy;
      } else if (enumValue == Barrier) {
        m_protocol = Barrier;
      } else {
        throw InvalidProtocolException();
      }
    } else if (id == kOptionScreenSwitchDelay) {
      m_switchWaitDelay = 1.0e-3 * static_cast<double>(value);
      if (m_switchWaitDelay < 0.0) {
        m_switchWaitDelay = 0.0;
      }
      stopSwitchWait();
    } else if (id == kOptionScreenSwitchTwoTap) {
      m_switchTwoTapDelay = 1.0e-3 * static_cast<double>(value);
      if (m_switchTwoTapDelay < 0.0) {
        m_switchTwoTapDelay = 0.0;
      }
      stopSwitchTwoTap();
    } else if (id == kOptionScreenSwitchNeedsControl) {
      m_switchNeedsControl = (value != 0);
    } else if (id == kOptionScreenSwitchNeedsShift) {
      m_switchNeedsShift = (value != 0);
    } else if (id == kOptionScreenSwitchNeedsAlt) {
      m_switchNeedsAlt = (value != 0);
    } else if (id == kOptionRelativeMouseMoves) {
      newRelativeMoves = (value != 0);
    } else if (id == kOptionDisableLockToScreen) {
      m_disableLockToScreen = (value != 0);
    } else if (id == kOptionClipboardSharing) {
      m_enableClipboard = value;
      if (!m_enableClipboard) {
        LOG_NOTE("clipboard sharing is disabled");
      }
    } else if (id == kOptionClipboardSharingSize) {
      if (value <= 0) {
        m_maximumClipboardSize = 0;
        LOG_NOTE(
            "clipboard sharing is disabled because the "
            "maximum shared clipboard size is set to 0"
        );
      } else {
        m_maximumClipboardSize = static_cast<size_t>(value);
      }
    }
  }
  if (m_relativeMoves && !newRelativeMoves) {
    stopRelativeMoves();
  }
  m_relativeMoves = newRelativeMoves;
}

void Server::handleShapeChanged(BaseClientProxy *client)
{
  if (!m_clientSet.contains(client)) {
    return;
  }

  LOG_DEBUG("screen \"%s\" shape changed", getName(client).c_str());

  // update jump coordinate
  int32_t x;
  int32_t y;
  client->getCursorPos(x, y);
  client->setJumpCursorPos(x, y);

  // update the mouse coordinates
  if (client == m_active) {
    m_x = x;
    m_y = y;
  }

  // handle resolution change to primary screen
  if (client == m_primaryClient) {
    if (client == m_active) {
      onMouseMovePrimary(m_x, m_y);
    } else {
      onMouseMoveSecondary(0, 0);
    }
  }
}

void Server::handleClipboardGrabbed(const Event &event, BaseClientProxy *grabber)
{
  if (!m_enableClipboard || (m_maximumClipboardSize == 0)) {
    return;
  }

  // ignore events from unknown clients
  if (!m_clientSet.contains(grabber)) {
    return;
  }
  const auto *info = static_cast<const IScreen::ClipboardInfo *>(event.getData());

  // ignore grab if sequence number is old.  always allow primary
  // screen to grab.
  ClipboardInfo &clipboard = m_clipboards[info->m_id];
  if (grabber != m_primaryClient && info->m_sequenceNumber < clipboard.m_clipboardSeqNum) {
    LOG_INFO("ignored screen \"%s\" grab of clipboard %d", getName(grabber).c_str(), info->m_id);
    return;
  }

  // mark screen as owning clipboard
  LOG_INFO(
      "screen \"%s\" grabbed clipboard %d from \"%s\"", getName(grabber).c_str(), info->m_id,
      clipboard.m_clipboardOwner.c_str()
  );
  clipboard.m_clipboardOwner = getName(grabber);
  clipboard.m_clipboardSeqNum = info->m_sequenceNumber;

  // clear the clipboard data (since it's not known at this point)
  if (clipboard.m_clipboard.open(0)) {
    clipboard.m_clipboard.empty();
    clipboard.m_clipboard.close();
  }
  clipboard.m_clipboardData = clipboard.m_clipboard.marshall();

  // tell all other screens to take ownership of clipboard.  tell the
  // grabber that it's clipboard isn't dirty.
  for (auto index = m_clients.begin(); index != m_clients.end(); ++index) {
    BaseClientProxy *client = index->second;
    if (client == grabber) {
      client->setClipboardDirty(info->m_id, false);
    } else {
      client->grabClipboard(info->m_id);
    }
  }

  if (grabber == m_primaryClient && m_active != m_primaryClient) {
    LOG_INFO("clipboard grabbed while active screen was changed, resending clipboard data");
    for (ClipboardID id = 0; id < kClipboardEnd; ++id) {
      onClipboardChanged(m_primaryClient, id, m_clipboards[id].m_clipboardSeqNum);
    }
  }
}

void Server::handleClipboardChanged(const Event &event, BaseClientProxy *client)
{
  // ignore events from unknown clients
  if (!m_clientSet.contains(client)) {
    return;
  }
  const auto *info = static_cast<const IScreen::ClipboardInfo *>(event.getData());
  onClipboardChanged(client, info->m_id, info->m_sequenceNumber);
}

void Server::handleKeyDownEvent(const Event &event)
{
  const auto *info = static_cast<IPlatformScreen::KeyInfo *>(event.getData());
  auto lang = AppUtil::instance().getCurrentLanguageCode();
  onKeyDown(info->m_key, info->m_mask, info->m_button, lang, info->m_screens);
}

void Server::handleKeyUpEvent(const Event &event)
{
  auto *info = static_cast<IPlatformScreen::KeyInfo *>(event.getData());
  onKeyUp(info->m_key, info->m_mask, info->m_button, info->m_screens);
}

void Server::handleKeyRepeatEvent(const Event &event)
{
  const auto *info = static_cast<IPlatformScreen::KeyInfo *>(event.getData());
  auto lang = AppUtil::instance().getCurrentLanguageCode();
  onKeyRepeat(info->m_key, info->m_mask, info->m_count, info->m_button, lang);
}

void Server::handleButtonDownEvent(const Event &event)
{
  const auto *info = static_cast<IPlatformScreen::ButtonInfo *>(event.getData());
  onMouseDown(info->m_button);
}

void Server::handleButtonUpEvent(const Event &event)
{
  const auto *info = static_cast<IPlatformScreen::ButtonInfo *>(event.getData());
  onMouseUp(info->m_button);
}

void Server::handleMotionPrimaryEvent(const Event &event)
{
  const auto *info = static_cast<IPlatformScreen::MotionInfo *>(event.getData());
  onMouseMovePrimary(info->m_x, info->m_y);
}

void Server::handleMotionSecondaryEvent(const Event &event)
{
  const auto *info = static_cast<IPlatformScreen::MotionInfo *>(event.getData());
  onMouseMoveSecondary(info->m_x, info->m_y);
}

void Server::handleWheelEvent(const Event &event)
{
  const auto *info = static_cast<IPlatformScreen::WheelInfo *>(event.getData());
  onMouseWheel(info->m_xDelta, info->m_yDelta);
}

void Server::handleSwitchWaitTimeout()
{
  // ignore if mouse is locked to screen
  if (isLockedToScreen()) {
    LOG_DEBUG1("locked to screen");
    stopSwitch();
    return;
  }

  // switch screen
  switchScreen(m_switchScreen, m_switchWaitX, m_switchWaitY, false);
}

void Server::handleClientDisconnected(BaseClientProxy *client)
{
  // client has disconnected.  it might be an old client or an
  // active client.  we don't care so just handle it both ways.
  removeActiveClient(client);
  removeOldClient(client);

  delete client;
}

void Server::handleClientCloseTimeout(BaseClientProxy *client)
{
  // client took too long to disconnect.  just dump it.
  LOG_NOTE("forced disconnection of client \"%s\"", getName(client).c_str());
  removeOldClient(client);

  delete client;
}

void Server::handleSwitchToScreenEvent(const Event &event)
{
  auto *info = static_cast<SwitchToScreenInfo *>(event.getData());

  ClientList::const_iterator index = m_clients.find(info->m_screen);
  if (index == m_clients.end()) {
    LOG_DEBUG1("screen \"%s\" not active", info->m_screen);
  } else {
    jumpToScreen(index->second);
  }
}

void Server::handleSwitchInDirectionEvent(const Event &event)
{
  const auto *info = static_cast<SwitchInDirectionInfo *>(event.getData());

  // jump to screen in chosen direction from center of this screen
  int32_t x = m_x;
  int32_t y = m_y;
  BaseClientProxy *newScreen = getNeighbor(m_active, info->m_direction, x, y);
  if (newScreen == nullptr) {
    LOG_DEBUG1("no neighbor %s", Config::dirName(info->m_direction));
  } else {
    jumpToScreen(newScreen);
  }
}

void Server::handleToggleScreenEvent(const Event &event)
{
  // Get the list of connected screens in config order
  std::vector<std::string> screens;
  getClients(screens);

  if (screens.size() < 2) {
    LOG_DEBUG1("not enough screens to toggle");
    return;
  }

  // Find the current active screen
  std::string currentScreen = getName(m_active);
  auto it = std::find(screens.begin(), screens.end(), currentScreen);
  if (it == screens.end()) {
    LOG_DEBUG1("current screen not found in list");
    return;
  }

  // Find the next screen
  auto nextIt = it + 1;
  if (nextIt == screens.end()) {
    nextIt = screens.begin();
  }

  // Find the client for the next screen
  ClientList::const_iterator clientIt = m_clients.find(*nextIt);
  if (clientIt == m_clients.end()) {
    LOG_DEBUG1("next screen not active");
    return;
  }

  jumpToScreen(clientIt->second);
}

void Server::handleKeyboardBroadcastEvent(const Event &event)
{
  const auto *info = (KeyboardBroadcastInfo *)event.getData();

  // choose new state
  bool newState;
  switch (info->m_state) {
  default:
  case KeyboardBroadcastInfo::kOn:
    newState = true;
    break;

  case KeyboardBroadcastInfo::kOff:
    newState = false;
    break;

  case KeyboardBroadcastInfo::kToggle:
    newState = !m_keyboardBroadcasting;
    break;
  }

  // enter new state
  if (newState != m_keyboardBroadcasting || info->m_screens != m_keyboardBroadcastingScreens) {
    m_keyboardBroadcasting = newState;
    m_keyboardBroadcastingScreens = info->m_screens;
    LOG(
        (CLOG_DEBUG "keyboard broadcasting %s: %s", m_keyboardBroadcasting ? "on" : "off",
         m_keyboardBroadcastingScreens.c_str())
    );
  }
}

void Server::handleLockCursorToScreenEvent(const Event &event)
{
  const auto *info = (LockCursorToScreenInfo *)event.getData();

  // choose new state
  bool newState;
  switch (info->m_state) {
  default:
  case LockCursorToScreenInfo::kOn:
    newState = true;
    break;

  case LockCursorToScreenInfo::kOff:
    newState = false;
    break;

  case LockCursorToScreenInfo::kToggle:
    newState = !m_lockedToScreen;
    break;
  }

  // enter new state
  if (newState != m_lockedToScreen) {
    m_lockedToScreen = newState;
    LOG_NOTE("cursor %s current screen", m_lockedToScreen ? "locked to" : "unlocked from");

    m_primaryClient->reconfigure(getActivePrimarySides());
    if (!isLockedToScreenServer()) {
      stopRelativeMoves();
    }
  }
}

void Server::onClipboardChanged(const BaseClientProxy *sender, ClipboardID id, uint32_t seqNum)
{
  ClipboardInfo &clipboard = m_clipboards[id];

  // ignore update if sequence number is old
  if (seqNum < clipboard.m_clipboardSeqNum) {
    LOG_INFO("ignored screen \"%s\" update of clipboard %d (mis-sequenced)", getName(sender).c_str(), id);
    return;
  }

  // should be the expected client
  assert(sender == m_clients.find(clipboard.m_clipboardOwner)->second);

  // get data
  sender->getClipboard(id, &clipboard.m_clipboard);

  std::string data = clipboard.m_clipboard.marshall();
  if (data.size() > m_maximumClipboardSize * 1024) {
    LOG_NOTE(
        "not updating clipboard because it's over the size limit (%i KB) configured by the server",
        m_maximumClipboardSize
    );
    return;
  }

  // ignore if data hasn't changed
  if (data == clipboard.m_clipboardData) {
    LOG_DEBUG("ignored screen \"%s\" update of clipboard %d (unchanged)", clipboard.m_clipboardOwner.c_str(), id);
    return;
  }

  // got new data
  LOG_INFO("screen \"%s\" updated clipboard %d", clipboard.m_clipboardOwner.c_str(), id);
  clipboard.m_clipboardData = data;

  // tell all clients except the sender that the clipboard is dirty
  for (ClientList::const_iterator index = m_clients.begin(); index != m_clients.end(); ++index) {
    BaseClientProxy *client = index->second;
    client->setClipboardDirty(id, client != sender);
  }

  // send the new clipboard to the active screen
  m_active->setClipboard(id, &clipboard.m_clipboard);
}

void Server::onScreensaver(bool activated)
{
  LOG_DEBUG("onScreenSaver %s", activated ? "activated" : "deactivated");

  if (activated) {
    // save current screen and position
    m_activeSaver = m_active;
    m_xSaver = m_x;
    m_ySaver = m_y;

    // jump to primary screen
    if (m_active != m_primaryClient) {
      switchScreen(m_primaryClient, 0, 0, true);
    }
  } else {
    // jump back to previous screen and position.  we must check
    // that the position is still valid since the screen may have
    // changed resolutions while the screen saver was running.
    if (m_activeSaver != nullptr && m_activeSaver != m_primaryClient) {
      // check position
      BaseClientProxy *screen = m_activeSaver;
      int32_t x;
      int32_t y;
      int32_t w;
      int32_t h;
      screen->getShape(x, y, w, h);
      int32_t zoneSize = getJumpZoneSize(screen);
      if (m_xSaver < x + zoneSize) {
        m_xSaver = x + zoneSize;
      } else if (m_xSaver >= x + w - zoneSize) {
        m_xSaver = x + w - zoneSize - 1;
      }
      if (m_ySaver < y + zoneSize) {
        m_ySaver = y + zoneSize;
      } else if (m_ySaver >= y + h - zoneSize) {
        m_ySaver = y + h - zoneSize - 1;
      }

      // jump
      switchScreen(screen, m_xSaver, m_ySaver, false);
    }

    // reset state
    m_activeSaver = nullptr;
  }

  // send message to all clients
  for (ClientList::const_iterator index = m_clients.begin(); index != m_clients.end(); ++index) {
    BaseClientProxy *client = index->second;
    client->screensaver(activated);
  }
}

void Server::onKeyDown(KeyID id, KeyModifierMask mask, KeyButton button, const std::string &lang, const char *screens)
{
  LOG_DEBUG1("onKeyDown id=%d mask=0x%04x button=0x%04x lang=%s", id, mask, button, lang.c_str());
  assert(m_active != nullptr);

  // relay
  if (!m_keyboardBroadcasting && IKeyState::KeyInfo::isDefault(screens)) {
    m_active->keyDown(id, mask, button, lang);
  } else {
    if (!screens && m_keyboardBroadcasting) {
      screens = m_keyboardBroadcastingScreens.c_str();
      if (IKeyState::KeyInfo::isDefault(screens)) {
        screens = "*";
      }
    }
    for (ClientList::const_iterator index = m_clients.begin(); index != m_clients.end(); ++index) {
      if (IKeyState::KeyInfo::contains(screens, index->first)) {
        index->second->keyDown(id, mask, button, lang);
      }
    }
  }
}

void Server::onKeyUp(KeyID id, KeyModifierMask mask, KeyButton button, const char *screens)
{
  LOG_DEBUG1("onKeyUp id=%d mask=0x%04x button=0x%04x", id, mask, button);
  assert(m_active != nullptr);

  // relay
  if (!m_keyboardBroadcasting && IKeyState::KeyInfo::isDefault(screens)) {
    m_active->keyUp(id, mask, button);
  } else {
    if (!screens && m_keyboardBroadcasting) {
      screens = m_keyboardBroadcastingScreens.c_str();
      if (IKeyState::KeyInfo::isDefault(screens)) {
        screens = "*";
      }
    }
    for (ClientList::const_iterator index = m_clients.begin(); index != m_clients.end(); ++index) {
      if (IKeyState::KeyInfo::contains(screens, index->first)) {
        index->second->keyUp(id, mask, button);
      }
    }
  }
}

void Server::onKeyRepeat(KeyID id, KeyModifierMask mask, int32_t count, KeyButton button, const std::string &lang)
{
  LOG(
      (CLOG_DEBUG1 "onKeyRepeat id=%d mask=0x%04x count=%d button=0x%04x lang=\"%s\"", id, mask, count, button,
       lang.c_str())
  );
  assert(m_active != nullptr);

  // relay
  m_active->keyRepeat(id, mask, count, button, lang);
}

void Server::onMouseDown(ButtonID id)
{
  LOG_DEBUG1("onMouseDown id=%d", id);
  assert(m_active != nullptr);

  // relay
  m_active->mouseDown(id);
}

void Server::onMouseUp(ButtonID id)
{
  LOG_DEBUG1("onMouseUp id=%d", id);
  assert(m_active != nullptr);

  // relay
  m_active->mouseUp(id);
}

bool Server::onMouseMovePrimary(int32_t x, int32_t y)
{
  LOG_DEBUG4("onMouseMovePrimary %d,%d", x, y);

  // mouse move on primary (server's) screen
  if (m_active != m_primaryClient) {
    // stale event -- we're actually on a secondary screen
    return false;
  }

  // save last delta
  m_xDelta2 = m_xDelta;
  m_yDelta2 = m_yDelta;

  // save current delta
  m_xDelta = x - m_x;
  m_yDelta = y - m_y;

  // save position
  m_x = x;
  m_y = y;

  // get screen shape
  int32_t ax;
  int32_t ay;
  int32_t aw;
  int32_t ah;
  m_active->getShape(ax, ay, aw, ah);
  int32_t zoneSize = getJumpZoneSize(m_active);

  // clamp position to screen
  int32_t xc = x;
  int32_t yc = y;
  if (xc < ax + zoneSize) {
    xc = ax;
  } else if (xc >= ax + aw - zoneSize) {
    xc = ax + aw - 1;
  }
  if (yc < ay + zoneSize) {
    yc = ay;
  } else if (yc >= ay + ah - zoneSize) {
    yc = ay + ah - 1;
  }

  // see if we should change screens
  // when the cursor is in a corner, there may be a screen either
  // horizontally or vertically.  check both directions.
  using enum Direction;
  auto dirh = NoDirection;
  auto dirv = NoDirection;
  int32_t xh = x;
  int32_t yv = y;
  if (x < ax + zoneSize) {
    xh -= zoneSize;
    dirh = Left;
  } else if (x >= ax + aw - zoneSize) {
    xh += zoneSize;
    dirh = Right;
  }
  if (y < ay + zoneSize) {
    yv -= zoneSize;
    dirv = Top;
  } else if (y >= ay + ah - zoneSize) {
    yv += zoneSize;
    dirv = Bottom;
  }
  if (dirh == NoDirection && dirv == NoDirection) {
    // still on local screen
    noSwitch(x, y);
    return false;
  }

  // check both horizontally and vertically
  std::array<Direction, 2> dirs = {dirh, dirv};
  std::array<int32_t, 2> xs = {xh, x};
  std::array<int32_t, 2> ys = {y, yv};
  for (int i = 0; i < 2; ++i) {
    Direction dir = dirs.at(i);
    if (dir == NoDirection) {
      continue;
    }
    x = xs.at(i);
    y = ys.at(i);
    // get jump destination
    BaseClientProxy *newScreen = mapToNeighbor(m_active, dir, x, y);

    // should we switch or not?
    if (isSwitchOkay(newScreen, dir, x, y, xc, yc)) {
      // switch screen
      switchScreen(newScreen, x, y, false);
      return true;
    }
  }

  return false;
}

void Server::onMouseMoveSecondary(int32_t dx, int32_t dy)
{
  LOG_DEBUG2("mouse move on secondary: %+d,%+d", dx, dy);

  // TODO: move this to client side and use a qt setting or cli arg instead of env var.
  const static auto adjustEnv = "DESKFLOW_MOUSE_ADJUSTMENT";
  if (const char *envVal = std::getenv(adjustEnv); envVal) {
    try {
      double multiplier = std::stod(envVal);
      dx = static_cast<int32_t>(std::round(dx * multiplier));
      dy = static_cast<int32_t>(std::round(dy * multiplier));
      LOG_DEBUG2("adjusted mouse x %.2f: %+d,%+d", multiplier, dx, dy);
    } catch (const std::exception &e) {
      LOG_ERR("invalid %s value: %s", adjustEnv, e.what());
    }
  }

  // mouse move on secondary (client's) screen
  assert(m_active != nullptr);
  if (m_active == m_primaryClient) {
    // stale event -- we're actually on the primary screen
    return;
  }

  // if doing relative motion on secondary screens and we're locked
  // to the screen (which activates relative moves) then send a
  // relative mouse motion.  when we're doing this we pretend as if
  // the mouse isn't actually moving because we're expecting some
  // program on the secondary screen to warp the mouse on us, so we
  // have no idea where it really is.
  if (m_relativeMoves && isLockedToScreenServer()) {
    LOG_DEBUG2("relative move on %s by %d,%d", getName(m_active).c_str(), dx, dy);
    m_active->mouseRelativeMove(dx, dy);
    return;
  }

  // save old position
  const int32_t xOld = m_x;
  const int32_t yOld = m_y;

  // save last delta
  m_xDelta2 = m_xDelta;
  m_yDelta2 = m_yDelta;

  // save current delta
  m_xDelta = dx;
  m_yDelta = dy;

  // accumulate motion
  m_x += dx;
  m_y += dy;

  // get screen shape
  int32_t ax;
  int32_t ay;
  int32_t aw;
  int32_t ah;
  m_active->getShape(ax, ay, aw, ah);

  // find direction of neighbor and get the neighbor
  bool jump = true;
  BaseClientProxy *newScreen;
  do {
    // clamp position to screen
    int32_t xc = m_x;
    int32_t yc = m_y;
    if (xc < ax) {
      xc = ax;
    } else if (xc >= ax + aw) {
      xc = ax + aw - 1;
    }
    if (yc < ay) {
      yc = ay;
    } else if (yc >= ay + ah) {
      yc = ay + ah - 1;
    }

    Direction dir;
    using enum Direction;
    if (m_x < ax) {
      dir = Left;
    } else if (m_x > ax + aw - 1) {
      dir = Right;
    } else if (m_y < ay) {
      dir = Top;
    } else if (m_y > ay + ah - 1) {
      dir = Bottom;
    } else {
      // we haven't left the screen
      newScreen = m_active;
      jump = false;

      // if waiting and mouse is not on the border we're waiting
      // on then stop waiting.  also if it's not on the border
      // then arm the double tap.
      if (m_switchScreen != nullptr) {
        bool clearWait;
        int32_t zoneSize = m_primaryClient->getJumpZoneSize();
        switch (m_switchDir) {
        case Left:
          clearWait = (m_x >= ax + zoneSize);
          break;

        case Right:
          clearWait = (m_x <= ax + aw - 1 - zoneSize);
          break;

        case Top:
          clearWait = (m_y >= ay + zoneSize);
          break;

        case Bottom:
          clearWait = (m_y <= ay + ah - 1 + zoneSize);
          break;

        default:
          clearWait = false;
          break;
        }
        if (clearWait) {
          // still on local screen
          noSwitch(m_x, m_y);
        }
      }

      // skip rest of block
      break;
    }

    // try to switch screen.  get the neighbor.
    newScreen = mapToNeighbor(m_active, dir, m_x, m_y);

    // see if we should switch
    if (!isSwitchOkay(newScreen, dir, m_x, m_y, xc, yc)) {
      newScreen = m_active;
      jump = false;
    }
  } while (false);

  if (jump) {
    int32_t newX = m_x;
    int32_t newY = m_y;

    // switch screens
    switchScreen(newScreen, newX, newY, false);
  } else {
    // same screen.  clamp mouse to edge.
    m_x = xOld + dx;
    m_y = yOld + dy;
    if (m_x < ax) {
      m_x = ax;
      LOG_DEBUG2("clamp to left of \"%s\"", getName(m_active).c_str());
    } else if (m_x > ax + aw - 1) {
      m_x = ax + aw - 1;
      LOG_DEBUG2("clamp to right of \"%s\"", getName(m_active).c_str());
    }
    if (m_y < ay) {
      m_y = ay;
      LOG_DEBUG2("clamp to top of \"%s\"", getName(m_active).c_str());
    } else if (m_y > ay + ah - 1) {
      m_y = ay + ah - 1;
      LOG_DEBUG2("clamp to bottom of \"%s\"", getName(m_active).c_str());
    }

    // warp cursor if it moved.
    if (m_x != xOld || m_y != yOld) {
      LOG_DEBUG2("move on %s to %d,%d", getName(m_active).c_str(), m_x, m_y);
      m_active->mouseMove(m_x, m_y);
    }
  }
}

void Server::onMouseWheel(int32_t xDelta, int32_t yDelta)
{
  LOG_DEBUG1("onMouseWheel %+d,%+d", xDelta, yDelta);
  assert(m_active != nullptr);

  // relay
  m_active->mouseWheel(xDelta, yDelta);
}

bool Server::addClient(BaseClientProxy *client)
{
  std::string name = getName(client);
  if (m_clients.contains(name)) {
    return false;
  }

  // add event handlers
  m_events->addHandler(EventTypes::ScreenShapeChanged, client->getEventTarget(), [this, client](const auto &) {
    handleShapeChanged(client);
  });
  m_events->addHandler(EventTypes::ClipboardGrabbed, client->getEventTarget(), [this, client](const auto &e) {
    handleClipboardGrabbed(e, client);
  });
  m_events->addHandler(EventTypes::ClipboardChanged, client->getEventTarget(), [this, client](const auto &e) {
    handleClipboardChanged(e, client);
  });

  // add to list
  m_clientSet.insert(client);
  m_clients.insert(std::make_pair(name, client));

  // initialize client data
  int32_t x;
  int32_t y;
  client->getCursorPos(x, y);
  client->setJumpCursorPos(x, y);

  // tell primary client about the active sides
  m_primaryClient->reconfigure(getActivePrimarySides());

  return true;
}

bool Server::removeClient(BaseClientProxy *client)
{
  using enum EventTypes;
  // return false if not in list
  ClientSet::iterator i = m_clientSet.find(client);
  if (i == m_clientSet.end()) {
    return false;
  }

  // remove event handlers
  m_events->removeHandler(ScreenShapeChanged, client->getEventTarget());
  m_events->removeHandler(ClipboardGrabbed, client->getEventTarget());
  m_events->removeHandler(ClipboardChanged, client->getEventTarget());

  // remove from list
  m_clients.erase(getName(client));
  m_clientSet.erase(i);

  return true;
}

void Server::closeClient(BaseClientProxy *client, const char *msg)
{
  assert(client != m_primaryClient);
  assert(msg != nullptr);

  // send message to client.  this message should cause the client
  // to disconnect.  we add this client to the closed client list
  // and install a timer to remove the client if it doesn't respond
  // quickly enough.  we also remove the client from the active
  // client list since we're not going to listen to it anymore.
  // note that this method also works on clients that are not in
  // the m_clients list.  adoptClient() may call us with such a
  // client.
  LOG_NOTE("disconnecting client \"%s\"", getName(client).c_str());

  // send message
  // FIXME -- avoid type cast (kinda hard, though)
  auto clientProxy = static_cast<ClientProxy *>(client);
  clientProxy->close(msg);

  // install timer.  wait timeout seconds for client to close.
  double timeout = 5.0;
  EventQueueTimer *timer = m_events->newOneShotTimer(timeout, nullptr);
  m_events->addHandler(EventTypes::Timer, timer, [this, client](const auto &) { handleClientCloseTimeout(client); });

  // move client to closing list
  removeClient(client);

  m_oldClients.insert(std::make_pair(client, timer));

  // if this client is the active screen then we have to
  // jump off of it
  forceLeaveClient(client);
}

void Server::closeClients(const ServerConfig &config)
{
  // collect the clients that are connected but are being dropped
  // from the configuration (or who's canonical name is changing).
  using RemovedClients = std::set<BaseClientProxy *>;
  RemovedClients removed;
  for (auto index = m_clients.begin(); index != m_clients.end(); ++index) {
    if (!config.isCanonicalName(index->first)) {
      removed.insert(index->second);
    }
  }

  // don't close the primary client
  removed.erase(m_primaryClient);

  // now close them.  we collect the list then close in two steps
  // because closeClient() modifies the collection we iterate over.
  for (auto &client : removed) {
    closeClient(client, kMsgCClose);
  }
}

void Server::removeActiveClient(BaseClientProxy *client)
{
  if (removeClient(client)) {
    forceLeaveClient(client);
    m_events->removeHandler(EventTypes::ClientProxyDisconnected, client);
    if (m_clients.size() == 1 && m_oldClients.empty()) {
      m_events->addEvent(Event(EventTypes::ServerDisconnected, this));
    }
  }
}

void Server::removeOldClient(BaseClientProxy *client)
{
  using enum EventTypes;
  OldClients::iterator i = m_oldClients.find(client);
  if (i != m_oldClients.end()) {
    m_events->removeHandler(ClientProxyDisconnected, client);
    m_events->removeHandler(Timer, i->second);
    m_events->deleteTimer(i->second);
    m_oldClients.erase(i);
    if (m_clients.size() == 1 && m_oldClients.empty()) {
      m_events->addEvent(Event(ServerDisconnected, this));
    }
  }
}

void Server::forceLeaveClient(const BaseClientProxy *client)
{
  if (const auto *active = (m_activeSaver != nullptr) ? m_activeSaver : m_active; active == client) {
    // record new position (center of primary screen)
    m_primaryClient->getCursorCenter(m_x, m_y);

    // stop waiting to switch to this client
    if (active == m_switchScreen) {
      stopSwitch();
    }

    // don't notify active screen since it has probably already
    // disconnected.
    LOG(
        (CLOG_INFO "jump from \"%s\" to \"%s\" at %d,%d", getName(active).c_str(), getName(m_primaryClient).c_str(),
         m_x, m_y)
    );

    // cut over
    m_active = m_primaryClient;

    // enter new screen (unless we already have because of the
    // screen saver)
    if (m_activeSaver == nullptr) {
      m_primaryClient->enter(m_x, m_y, m_seqNum, m_primaryClient->getToggleMask(), false);
    }
  }

  // if this screen had the cursor when the screen saver activated
  // then we can't switch back to it when the screen saver
  // deactivates.
  if (m_activeSaver == client) {
    m_activeSaver = nullptr;
  }

  // tell primary client about the active sides
  m_primaryClient->reconfigure(getActivePrimarySides());
}

//
// Server::LockCursorToScreenInfo
//

Server::LockCursorToScreenInfo *Server::LockCursorToScreenInfo::alloc(State state)
{
  auto *info = (LockCursorToScreenInfo *)malloc(sizeof(LockCursorToScreenInfo));
  info->m_state = state;
  return info;
}

//
// Server::SwitchToScreenInfo
//

Server::SwitchToScreenInfo *Server::SwitchToScreenInfo::alloc(const std::string &screen)
{
  auto *info = (SwitchToScreenInfo *)malloc(sizeof(SwitchToScreenInfo) + screen.size());
  std::copy(screen.c_str(), screen.c_str() + screen.size() + 1, info->m_screen);
  return info;
}

//
// Server::SwitchInDirectionInfo
//

Server::SwitchInDirectionInfo *Server::SwitchInDirectionInfo::alloc(Direction direction)
{
  auto *info = (SwitchInDirectionInfo *)malloc(sizeof(SwitchInDirectionInfo));
  info->m_direction = direction;
  return info;
}

//
// Server::KeyboardBroadcastInfo
//

Server::KeyboardBroadcastInfo *Server::KeyboardBroadcastInfo::alloc(State state)
{
  auto *info = (KeyboardBroadcastInfo *)malloc(sizeof(KeyboardBroadcastInfo));
  info->m_state = state;
  info->m_screens[0] = '\0';
  return info;
}

Server::KeyboardBroadcastInfo *Server::KeyboardBroadcastInfo::alloc(State state, const std::string &screens)
{
  auto *info = (KeyboardBroadcastInfo *)malloc(sizeof(KeyboardBroadcastInfo) + screens.size());
  info->m_state = state;
  std::copy(screens.c_str(), screens.c_str() + screens.size() + 1, info->m_screens);
  return info;
}
