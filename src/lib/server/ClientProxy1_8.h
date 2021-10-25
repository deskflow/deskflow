/*
 * synergy -- mouse and keyboard sharing utility
 * Copyright (C) 2015-2021 Symless Ltd.
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
#ifndef SERVER_CLIENTPROXY1_8_H
#define SERVER_CLIENTPROXY1_8_H

#include "server/ClientProxy1_7.h"

class ClientProxy1_8 : public ClientProxy1_7
{
public:
    ClientProxy1_8(const String& name, synergy::IStream* adoptedStream, Server* server, IEventQueue* events);
    ~ClientProxy1_8() override = default;

    void        keyDown(KeyID, KeyModifierMask, KeyButton, const String&) override;

private:
    void synchronizeLanguages() const;

};

#endif // SERVER_CLIENTPROXY1_8_H
