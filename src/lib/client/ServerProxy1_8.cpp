/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2026 Synergy App Ltd
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#include "client/ServerProxy1_8.h"

#include "base/Log.h"
#include "deskflow/ProtocolTypes.h"
#include "deskflow/ProtocolUtil.h"
#include "deskflow/ipc/CoreIpc.h"

#include <cstring>
#include <string>

#include <QString>

ServerProxy1_8::ServerProxy1_8(Client *client, deskflow::IStream *stream, IEventQueue *events)
    : ServerProxy1_7(client, stream, events)
{
}

ServerProxy::ConnectionResult ServerProxy1_8::parseHandshakeMessage(const uint8_t *code)
{
  auto result = ConnectionResult::Okay;
  if (memcmp(code, kMsgDLanguageSynchronisation, 4) == 0) {
    setServerLayouts();
  } else {
    result = ServerProxy1_7::parseHandshakeMessage(code);
  }
  return result;
}

ServerProxy::ConnectionResult ServerProxy1_8::parseMessage(const uint8_t *code)
{
  auto result = ConnectionResult::Okay;
  if (memcmp(code, kMsgDKeyDown, 4) == 0) {
    uint16_t id = 0;
    uint16_t mask = 0;
    uint16_t button = 0;
    std::string lang;
    ProtocolUtil::readf(getStream(), kMsgDKeyDown + 4, &id, &mask, &button, &lang);
    LOG_VERBOSE("recv key down id=0x%08x, mask=0x%04x, button=0x%04x, lang=\"%s\"", id, mask, button, lang.c_str());

    setActiveServerLayout(lang);
    keyDown(id, mask, button, lang);
  } else if (memcmp(code, kMsgDKeyRepeat, 4) == 0) {
    uint16_t id = 0;
    uint16_t mask = 0;
    uint16_t count = 0;
    uint16_t button = 0;
    std::string lang;
    ProtocolUtil::readf(getStream(), kMsgDKeyRepeat + 4, &id, &mask, &count, &button, &lang);
    LOG_VERBOSE(
        "recv key repeat id=0x%08x, mask=0x%04x, count=%d, button=0x%04x, lang=\"%s\"", id, mask, count, button,
        lang.c_str()
    );

    keyRepeat(id, mask, count, button, lang);
  } else if (memcmp(code, kMsgCEnter, 4) == 0) {
    m_serverLayout.clear();
    m_isUserNotifiedAboutLayoutSyncError = false;
    result = ServerProxy1_7::parseMessage(code);
  } else {
    result = ServerProxy1_7::parseMessage(code);
  }
  return result;
}

void ServerProxy1_8::setServerLayouts()
{
  std::string serverLayouts;
  ProtocolUtil::readf(getStream(), kMsgDLanguageSynchronisation + 4, &serverLayouts);
  m_layoutManager.setRemoteLayouts(serverLayouts);

  if (const auto missingLayouts = m_layoutManager.getMissedLayouts(); !missingLayouts.empty()) {
    LOG_WARN("server layouts missing on this computer: %s", missingLayouts.c_str());
    ipcSendToClient("missingKeyboardLayouts", QString::fromStdString(missingLayouts));
  }
}

void ServerProxy1_8::setActiveServerLayout(const std::string_view &layout)
{
  if (layout.empty()) {
    LOG_VERBOSE("active server layout is empty");
    return;
  }

  if (m_serverLayout != layout) {
    m_isUserNotifiedAboutLayoutSyncError = false;
    m_serverLayout = layout;
  }

  if (!m_layoutManager.isLayoutInstalled(m_serverLayout)) {
    if (!m_isUserNotifiedAboutLayoutSyncError) {
      LOG_WARN("current server layout is not installed on client");
      m_isUserNotifiedAboutLayoutSyncError = true;
    }
  } else {
    m_isUserNotifiedAboutLayoutSyncError = false;
  }
}
