/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2015 - 2021 Symless Ltd.
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#pragma once

#include "server/ClientProxy1_7.h"

class ClientProxy1_8 : public ClientProxy1_7
{
public:
  ClientProxy1_8(const std::string &name, deskflow::IStream *adoptedStream, Server *server, IEventQueue *events);
  ~ClientProxy1_8() override = default;

  void keyDown(KeyID, KeyModifierMask, KeyButton, const std::string &) override;

private:
  void synchronizeLanguages() const;
};
