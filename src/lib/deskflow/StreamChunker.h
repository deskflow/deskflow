/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2013 - 2016 Symless Ltd.
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#pragma once

#include "deskflow/ClipboardTypes.h"

#include <string>

class IEventQueue;

class StreamChunker
{
public:
  static void sendClipboard(
      std::string &data, size_t size, ClipboardID id, uint32_t sequence, IEventQueue *events, void *eventTarget
  );
};
