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

#ifndef CDEVICEMANAGER_H
#define CDEVICEMANAGER_H

#include "IDeviceManager.h"
#include <map>

using namespace std;

class CDeviceManager : public IDeviceManager
{  
public:
      CDeviceManager(); 
      virtual ~CDeviceManager();   
    
      virtual void setJumpCursorPos(const CString& name, SInt32 x, SInt32 y, UInt8 id);
      virtual void getJumpCursorPos(const CString& name, SInt32& x, SInt32& y, UInt8 id) const;

      //FIXXME
      virtual void setCursorDelta(SInt32 x, SInt32 y, UInt8 id);
      virtual void getCursorDelta(SInt32& x, SInt32& y, UInt8 id) const;

      virtual void setCursorDelta2(SInt32 x, SInt32 y, UInt8 id);
      virtual void getCursorDelta2(SInt32& x, SInt32& y, UInt8 id) const;
        
      virtual void getLastCursorPos(SInt32& x, SInt32& y, UInt8 id) const;
      virtual void setLastCursorPos(SInt32 x, SInt32 y, UInt8 id);

      virtual void getLocation(CString& screenName, UInt8 id) const;
      virtual void setLocation(CString *screenName, UInt8 id);

      virtual UInt8 getServerId(UInt8 id) const;
      virtual void setServerId(UInt8 sId, UInt8 id);
    
      virtual UInt8 getAttachment(UInt8 id) const;    
      virtual UInt8 getIdFromSid(UInt8 sId) const;
    
      virtual CBaseClientProxy *getActiveClient(UInt8 id) const;
      virtual void setActiveClient(CBaseClientProxy *active, UInt8 id);   

      virtual CKeyState *getKeyState(UInt8 id) const;    
      virtual void setKeyState(CKeyState *keyState, UInt8 id);    
    
      virtual bool hasEntered(UInt8 id) const;    
      virtual void setEntered(bool value, UInt8 id);
   
      virtual UInt8 getXtestId(UInt8 id);    
      virtual void setXtestId(UInt8 xtestId, UInt8 id);

      virtual void getAllPointerIDs(std::list<UInt8>& list) const;
    
      virtual void getAllKeyboardIDs(std::list<UInt8>& list) const;
    
      virtual void getAllDeviceIDs(std::list<UInt8>& list) const;   

      virtual void setIsOnScreen(bool value, UInt8 id);
    
      virtual void setLockedToScreen(bool value, UInt8 id);

      virtual void setPrimary(bool value);    

      virtual void setPrimaryClient(CPrimaryClient *primaryClient);

      virtual void setRelativeMoves(bool value, UInt8 id);	

      virtual bool isLockedToScreen(UInt8 id) const;   

      virtual bool isOnScreen(UInt8 id) const;
    
      virtual bool isPointer(UInt8 id) const;

      virtual bool isRelativeMovesSet(UInt8 id) const;
      
      static IDeviceManager* getInstance();    
      virtual void addDevice(bool isPointer, UInt8 kId, UInt8 id);    
      virtual void removeDevice(UInt8 id);

private:
      class CDeviceInfo {
      public:
        CDeviceInfo(UInt8 attachment, UInt8 id);
      public:
        // pointer and keyboard device IDs
	// since they always come in twos
	UInt8 			m_id;
	UInt8 			m_attachment;
	UInt8			m_xtestDevice;
	
	// device ID on the server
	UInt8 			m_sId;
		
	CString		 	m_serverName;
	
	// cursor location
        SInt32			m_xCursor, m_yCursor;
	SInt32			m_xJumpPos, m_yJumpPos;
	SInt32			m_xDelta, m_yDelta;
	SInt32			m_xDelta2, m_yDelta2;

	// some boolean properties
	bool 			m_onScreen;
	bool 			m_lockedToScreen;
	bool 			m_isPointer;
	
	// relative mouse move option
	bool 			m_relativeMoves;
	
	// variable used by CScreen
	bool			m_entered;
	
	CBaseClientProxy*	m_active;
	
	// we store a pointer to the keystate here 
	// instead of the platform specific C*Screen
	CKeyState*		m_keyState;    
	
    };
    
private:    
	bool     		m_isPrimary;
	CBaseClientProxy* 	m_primaryClient;
	const char* 		m_screenName;
	typedef map<UInt8, CDeviceInfo*> CDeviceMap;
	CDeviceMap*		m_devices;
    
protected:
    // pointer to singleton device manager
    static CDeviceManager*	s_instance;

};

#endif // CDEVICEMANAGER_H
