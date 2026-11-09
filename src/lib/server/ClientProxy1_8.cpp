#include "deskflow/DisplayTopologyMessage.h"
/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2015 - 2021 Synergy App Ltd
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#include "base/Log.h"
#include "deskflow/KeyboardLayoutManager.h"
#include "deskflow/ProtocolUtil.h"

#include "ClientProxy1_8.h"
#include "common/Settings.h"
#include "deskflow/DeskflowException.h"
#include "server/Server.h"
#include <cstring>

void ClientProxy1_8::updateServerTopology()
{
  if (!m_monitorSupport)
    return;
  deskflow::display::writeTopologyMessage(
      getStream(), Settings::value(Settings::Core::ComputerName).toString(), deskflow::display::localTopology(),
      m_server->workspaceSnapshot()
  );
}
bool ClientProxy1_8::recvTopology()
{
  m_topology = deskflow::display::readTopologyMessage(getStream(), QString::fromStdString(getName())).topology;
  m_server->monitorTopologyChanged(this);
  return true;
}
bool ClientProxy1_8::parseHandshakeMessage(const uint8_t *code)
{
  if (m_monitorSupport && memcmp(code, "DTOP", 4) == 0)
    return recvTopology();
  return ClientProxy1_7::parseHandshakeMessage(code);
}
bool ClientProxy1_8::parseMessage(const uint8_t *code)
{
  if (m_monitorSupport && memcmp(code, "DTOP", 4) == 0)
    return recvTopology();
  if (m_monitorSupport && memcmp(code, kMsgDInfo, 4) == 0) {
    m_topology.reset();
    m_server->monitorTopologyChanged(this); // DTOP must validate the new shape first.
  }
  return ClientProxy1_7::parseMessage(code);
}

ClientProxy1_8::ClientProxy1_8(
    const std::string &name, deskflow::IStream *adoptedStream, Server *server, IEventQueue *events, bool monitors
)
    : ClientProxy1_7(name, adoptedStream, server, events),
      m_monitorSupport(monitors)
{
  synchronizeLanguages();
  updateServerTopology();
}

void ClientProxy1_8::synchronizeLanguages() const
{
  deskflow::KeyboardLayoutManager layoutManager;
  auto localLayouts = layoutManager.getSerializedLocalLayouts();
  if (!localLayouts.empty()) {
    LOG_VERBOSE("send server languages to the client: %s", localLayouts.c_str());
    ProtocolUtil::writef(getStream(), kMsgDLanguageSynchronisation, &localLayouts);
  } else {
    LOG_ERR("failed to read server languages");
  }
}

void ClientProxy1_8::keyDown(KeyID key, KeyModifierMask mask, KeyButton button, const std::string &language)
{
  LOG_VERBOSE(
      "send key down to \"%s\" id=%d, mask=0x%04x, button=0x%04x, layout=%s", getName().c_str(), key, mask, button,
      language.c_str()
  );
  ProtocolUtil::writef(getStream(), kMsgDKeyDown, key, mask, button, &language);
}

void ClientProxy1_8::keyRepeat(
    KeyID key, KeyModifierMask mask, int32_t count, KeyButton button, const std::string &language
)
{
  LOG_VERBOSE(
      "send key repeat to \"%s\" id=%d, mask=0x%04x, count=%d, button=0x%04x, layout=%s", getName().c_str(), key, mask,
      count, button, language.c_str()
  );
  ProtocolUtil::writef(getStream(), kMsgDKeyRepeat, key, mask, count, button, &language);
}
