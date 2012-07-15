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
#include <rfb/Exception.h>
#include <rfb/CMsgHandler.h>

using namespace rfb;

CMsgHandler::CMsgHandler()
{
}

CMsgHandler::~CMsgHandler()
{
}

void CMsgHandler::setDesktopSize(int width, int height)
{
  cp.width = width;
  cp.height = height;
}

void CMsgHandler::setCursor(int w, int h, const Point& hotspot, void* data, void* mask)
{
}

void CMsgHandler::setPixelFormat(const PixelFormat& pf)
{
  cp.setPF(pf);
}

void CMsgHandler::setName(const char* name)
{
  cp.setName(name);
}

void CMsgHandler::serverInit()
{
  throw Exception("CMsgHandler::serverInit called");
}

void CMsgHandler::framebufferUpdateStart()
{
}

void CMsgHandler::framebufferUpdateEnd()
{
}

void CMsgHandler::beginRect(const Rect& r, unsigned int encoding)
{
}

void CMsgHandler::endRect(const Rect& r, unsigned int encoding)
{
}


void CMsgHandler::setColourMapEntries(int firstColour, int nColours,
                                      rdr::U16* rgbs)
{
  throw Exception("CMsgHandler::setColourMapEntries called");
}

void CMsgHandler::bell()
{
}

void CMsgHandler::serverCutText(const char* str, int len)
{
}

void CMsgHandler::fillRect(const Rect& r, Pixel pix)
{
}

void CMsgHandler::imageRect(const Rect& r, void* pixels)
{
}

void CMsgHandler::copyRect(const Rect& r, int srcX, int srcY)
{
}


