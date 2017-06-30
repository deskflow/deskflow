/*
 * synergy -- mouse and keyboard sharing utility
 * Copyright (C) 2013-2016 Symless Ltd.
 *
 * This package is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * found in the file LICENSE that should have accompanied this file.
 *
 * This package is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#pragma once

#include "synergy/clipboard_types.h"
#include "base/String.h"

class IEventQueue;
class Mutex;

class StreamChunker {
public:
    static void
    sendFile (char* filename, IEventQueue* events, void* eventTarget);
    static void
    sendClipboard (String& data, size_t size, ClipboardID id, UInt32 sequence,
                   IEventQueue* events, void* eventTarget);
    static void interruptFile ();

private:
    static bool s_isChunkingFile;
    static bool s_interruptFile;
    static Mutex* s_interruptMutex;
};
