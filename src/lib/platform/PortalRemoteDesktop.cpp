/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2025 Deskflow Developers
 * SPDX-FileCopyrightText: (C) 2024 Symless Ltd.
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#include "platform/PortalRemoteDesktop.h"
#include "base/Log.h"
#include "platform/EiScreen.h"
#include "platform/PortalClipboard.h"
#include "platform/PortalSessionProxy.h"

namespace deskflow {

PortalRemoteDesktop::PortalRemoteDesktop(EiScreen *screen, IEventQueue *events) : m_screen{screen}, m_events{events}
{
  m_sessionProxy = std::make_unique<PortalSessionProxy>(this);
  m_clipboard = std::make_unique<PortalClipboard>(this);

  connect(m_sessionProxy.get(), &PortalSessionProxy::sessionReady, [this](int fd) {
    LOG_DEBUG("Remote desktop session ready with fd %d", fd);
    m_events->addEvent(Event(EventTypes::EIConnected, m_screen->getEventTarget(), EiScreen::EiConnectInfo::alloc(fd)));
  });

  connect(m_sessionProxy.get(), &PortalSessionProxy::sessionClosed, [this]() {
    LOG_ERR("Remote desktop session closed");
    m_events->addEvent(Event(EventTypes::EISessionClosed, m_screen->getEventTarget()));
  });

  connect(m_clipboard.get(), &PortalClipboard::clipboardChanged, [this]() { emit clipboardChanged(); });

  m_sessionProxy->initSession();
}

PortalRemoteDesktop::~PortalRemoteDesktop() = default;

IClipboard *PortalRemoteDesktop::getClipboard() const
{
  return m_clipboard.get();
}

} // namespace deskflow
