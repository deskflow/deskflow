/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2013 - 2016 Symless Ltd.
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#pragma once

#include "deskflow/clipboard_types.h"

#include <string>

class IEventQueue;
class Mutex;

class StreamChunker
{
public:
  static void sendFile(char *filename, IEventQueue *events, void *eventTarget);
  static void sendClipboard(
      std::string &data, size_t size, ClipboardID id, uint32_t sequence, IEventQueue *events, void *eventTarget
  );
  static void interruptFile();

private:
  static bool s_isChunkingFile;
  static bool s_interruptFile;
  static Mutex *s_interruptMutex;
};
