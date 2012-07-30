/* Copyright (C) 2002-2005 RealVNC Ltd.  All Rights Reserved.
 * 
 * This is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 * 
 * This software is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this software; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307,
 * USA.
 */

// -=- ServerCore.cxx

// This header will define the Server interface, from which ServerMT and
// ServerST will be derived.

#include <string.h>
#include <rfb/util.h>
#include <rfb/ServerCore.h>

rfb::IntParameter rfb::Server::idleTimeout
("IdleTimeout",
 "The number of seconds after which an idle VNC connection will be dropped "
 "(zero means no timeout)",
 3600, 0);
rfb::IntParameter rfb::Server::clientWaitTimeMillis
("ClientWaitTimeMillis",
 "The number of milliseconds to wait for a client which is no longer "
 "responding",
 20000, 0);
rfb::BoolParameter rfb::Server::compareFB
("CompareFB",
 "Perform pixel comparison on framebuffer to reduce unnecessary updates",
 true);
rfb::BoolParameter rfb::Server::protocol3_3
("Protocol3.3",
 "Always use protocol version 3.3 for backwards compatibility with "
 "badly-behaved clients",
 false);
rfb::BoolParameter rfb::Server::alwaysShared
("AlwaysShared",
 "Always treat incoming connections as shared, regardless of the client-"
 "specified setting",
 false);
rfb::BoolParameter rfb::Server::neverShared
("NeverShared",
 "Never treat incoming connections as shared, regardless of the client-"
 "specified setting",
 false);
rfb::BoolParameter rfb::Server::disconnectClients
("DisconnectClients",
 "Disconnect existing clients if an incoming connection is non-shared. "
 "If combined with NeverShared then new connections will be refused "
 "while there is a client active",
 true);
rfb::BoolParameter rfb::Server::acceptKeyEvents
("AcceptKeyEvents",
 "Accept key press and release events from clients.",
 true);
rfb::BoolParameter rfb::Server::acceptPointerEvents
("AcceptPointerEvents",
 "Accept pointer press and release events from clients.",
 true);
rfb::BoolParameter rfb::Server::acceptCutText
("AcceptCutText",
 "Accept clipboard updates from clients.",
 true);
rfb::BoolParameter rfb::Server::sendCutText
("SendCutText",
 "Send clipboard changes to clients.",
 true);
rfb::BoolParameter rfb::Server::queryConnect
("QueryConnect",
 "Prompt the local user to accept or reject incoming connections.",
 false);
