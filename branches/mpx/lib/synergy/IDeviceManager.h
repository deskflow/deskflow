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

#ifndef IDEVICEMANAGER_H
#define IDEVICEMANAGER_H

#include "IInterface.h"
#include "BasicTypes.h"
#include "CBaseClientProxy.h"
#include "CPrimaryClient.h"
#include "CKeyState.h"
#include <CString.h>
#include <list>

class IDeviceManager : public IInterface
{
public:

    virtual ~IDeviceManager() { }

    virtual CBaseClientProxy *getActiveClient(UInt8 id) const = 0;   
    virtual void setActiveClient(CBaseClientProxy *active, UInt8 id) = 0;    

    virtual void getCursorDelta(SInt32& x, SInt32& y, UInt8 id) const = 0;
    virtual void setCursorDelta(SInt32 x, SInt32 y, UInt8 id) = 0;

    virtual void getCursorDelta2(SInt32& x, SInt32& y, UInt8 id) const = 0;
    virtual void setCursorDelta2(SInt32 x, SInt32 y, UInt8 id) = 0;

    // moved this from baseclientproxy since it is device id dependent
    virtual void getJumpCursorPos(const CString& name, SInt32& x, SInt32& y, UInt8 id) const = 0;
    virtual void setJumpCursorPos(const CString& name, SInt32 x, SInt32 y, UInt8 id) = 0;

    virtual CKeyState *getKeyState(UInt8 id) const = 0;
    virtual void setKeyState(CKeyState *keyState, UInt8 id) = 0;        

    // position is also a per device information
    virtual void getLastCursorPos(SInt32& x, SInt32& y, UInt8 id) const = 0;    
    virtual void setLastCursorPos(SInt32 x, SInt32 y, UInt8 id) = 0;
            
    virtual UInt8 getServerId(UInt8 id) const = 0;
    virtual void setServerId(UInt8 sId, UInt8 id) = 0;        
        
    virtual bool hasEntered(UInt8 id) const = 0;
    virtual void setEntered(bool value, UInt8 id) = 0;   

    virtual UInt8 getXtestId(UInt8 id) = 0;    
    virtual void setXtestId(UInt8 xtestId, UInt8 id) = 0;

    virtual UInt8 getIdFromSid(UInt8 sId) const = 0;
    
    virtual void getAllPointerIDs(std::list<UInt8>& list) const = 0;
    
    virtual void getAllKeyboardIDs(std::list<UInt8>& list) const = 0;

    virtual void getAllDeviceIDs(std::list<UInt8>& list) const = 0;    
        
    virtual UInt8 getAttachment(UInt8 id) const = 0;
    
    virtual void getLocation(CString& screenName, UInt8 id) const = 0;    

    // set flag to false if a pointer leaves the screen
    virtual void setIsOnScreen(bool value, UInt8 id) = 0;
    
    // keep track on which machine the pointer is atm
    virtual void setLocation(CString *screenName, UInt8 id) = 0;
        
    virtual void setRelativeMoves(bool value, UInt8 id) = 0;
    
    virtual void setLockedToScreen(bool value, UInt8 id) = 0;    
    
    virtual void setPrimaryClient(CPrimaryClient *primaryClient) = 0;

    virtual void setPrimary(bool value) = 0;
    // we need to keep track of our multiple pointers
    virtual bool isOnScreen(UInt8 id) const = 0;
    
    virtual bool isLockedToScreen(UInt8 id) const = 0;
    
    virtual bool isRelativeMovesSet(UInt8 id) const = 0;
    
    virtual bool isPointer(UInt8 id) const = 0;
    
    static IDeviceManager* getInsance();
    virtual void addDevice(bool isPointer, UInt8 attachment, UInt8 id) = 0;    
    virtual void removeDevice(UInt8 id) = 0;

};

#endif // IDEVICEMANAGER_H
