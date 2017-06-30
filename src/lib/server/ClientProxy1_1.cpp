/*
 * synergy -- mouse and keyboard sharing utility
 * Copyright (C) 2012-2016 Symless Ltd.
 * Copyright (C) 2002 Chris Schoeneman
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

#include "server/ClientProxy1_1.h"

#include "synergy/ProtocolUtil.h"
#include "base/Log.h"

#include <cstring>

//
// ClientProxy1_1
//

ClientProxy1_1::ClientProxy1_1 (const String& name, synergy::IStream* stream,
                                IEventQueue* events)
    : ClientProxy1_0 (name, stream, events) {
    // do nothing
}

ClientProxy1_1::~ClientProxy1_1 () {
    // do nothing
}

void
ClientProxy1_1::keyDown (KeyID key, KeyModifierMask mask, KeyButton button) {
    LOG ((CLOG_DEBUG1
          "send key down to \"%s\" id=%d, mask=0x%04x, button=0x%04x",
          getName ().c_str (),
          key,
          mask,
          button));
    ProtocolUtil::writef (getStream (), kMsgDKeyDown, key, mask, button);
}

void
ClientProxy1_1::keyRepeat (KeyID key, KeyModifierMask mask, SInt32 count,
                           KeyButton button) {
    LOG ((
        CLOG_DEBUG1
        "send key repeat to \"%s\" id=%d, mask=0x%04x, count=%d, button=0x%04x",
        getName ().c_str (),
        key,
        mask,
        count,
        button));
    ProtocolUtil::writef (
        getStream (), kMsgDKeyRepeat, key, mask, count, button);
}

void
ClientProxy1_1::keyUp (KeyID key, KeyModifierMask mask, KeyButton button) {
    LOG ((CLOG_DEBUG1 "send key up to \"%s\" id=%d, mask=0x%04x, button=0x%04x",
          getName ().c_str (),
          key,
          mask,
          button));
    ProtocolUtil::writef (getStream (), kMsgDKeyUp, key, mask, button);
}
