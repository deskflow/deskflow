/*
    <one line to give the program's name and a brief idea of what it does.>
    Copyright (C) <year>  <name of author>

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License along
    with this program; if not, write to the Free Software Foundation, Inc.,
    51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.

*/

#ifndef CCLIENTPROXY1_4_H
#define CCLIENTPROXY1_4_H

#include "CClientProxy1_3.h"
#include "CDeviceManager.h"

class CClientProxy1_4 : public CClientProxy1_3
{
  public:
    CClientProxy1_4(const CString& name, IStream* adoptedStream);
    virtual ~CClientProxy1_4(){ }
	
    virtual void keyDown(KeyID key, KeyModifierMask mask, KeyButton button, UInt8 id );
    virtual void keyRepeat(KeyID key, KeyModifierMask mask, SInt32 count, KeyButton button, UInt8 id );
    virtual void keyUp(KeyID key, KeyModifierMask mask, KeyButton button, UInt8 id );
    virtual void getCursorPos(SInt32& x, SInt32& y, UInt8 id) const;
    virtual void enter(SInt32 xAbs, SInt32 yAbs, UInt32 seqNum, KeyModifierMask mask, bool forScreensaver,  UInt8 kId, UInt8 pId);
    virtual bool leave(UInt8 id);
    virtual void mouseDown(ButtonID button, UInt8 id);
    virtual void mouseUp(ButtonID button, UInt8 id);
    virtual void mouseMove(SInt32 xAbs, SInt32 yAbs, UInt8 id);
    virtual void mouseWheel(SInt32 xDelta, SInt32 yDelta, UInt8 id);
    virtual void mouseRelativeMove(SInt32 xRel, SInt32 yRel, UInt8 id);
    
  private:
    CClientInfo			m_info;
    IDeviceManager*		m_dev;

};

#endif // CCLIENTPROXY1_4_H
