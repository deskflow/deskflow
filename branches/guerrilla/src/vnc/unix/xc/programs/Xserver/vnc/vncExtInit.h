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
#ifndef __VNCEXTINIT_H__
#define __VNCEXTINIT_H__

#include <rfb/Configuration.h>
#include "XserverDesktop.h"

extern void vncClientCutText(const char* str, int len);
extern void vncQueryConnect(XserverDesktop* desktop, void* opaqueId);
extern void vncClientGone(int fd);
extern void vncBell();
extern void* vncFbptr[];
extern int vncInetdSock;
extern rfb::StringParameter httpDir;

#endif
