/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2015 - 2021 Synergy App Ltd
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#pragma once

#include "deskflow/FileChunk.h"
#include "deskflow/ProtocolTypes.h"
#include "server/ClientProxy1_7.h"

#include <string>

class IEventQueue;

class ClientProxy1_8 : public ClientProxy1_7
{
public:
  ClientProxy1_8(const std::string &name, deskflow::IStream *adoptedStream, Server *server, IEventQueue *events);
  ~ClientProxy1_8() override;

  void keyDown(KeyID, KeyModifierMask, KeyButton, const std::string &) override;
  void keyRepeat(KeyID, KeyModifierMask, int32_t count, KeyButton, const std::string &) override;

private:
  IEventQueue *m_events = nullptr;
  void synchronizeLanguages() const;
};
