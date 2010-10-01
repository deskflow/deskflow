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

#include "CDeviceManager.h"
#include "CLog.h"
#include "BasicTypes.h"
#include <X11/Xlib.h>

CDeviceManager *CDeviceManager::s_instance = NULL; 

CDeviceManager::CDeviceManager() :
		m_isPrimary(false)
{
    //m_devices = new CDeviceList;
    m_devices = new CDeviceMap;
}


CDeviceManager::~CDeviceManager()
{
    delete m_devices;
}

IDeviceManager*
CDeviceManager::getInstance()
{
    if(s_instance)
      return s_instance;
    else
    {
      s_instance = new CDeviceManager();      
      LOG((CLOG_DEBUG "Returning DM instance: %p", s_instance));
      return s_instance;
    }
}

void 
CDeviceManager::setPrimary(bool value)
{
    m_isPrimary = value; 
}


void
CDeviceManager::addDevice(bool isPointer, UInt8 attachment, UInt8 id)
{
    LOG((CLOG_DEBUG "adding device (%d) with attachment (%d). isPointer: %d", id, attachment, isPointer));
    CDeviceInfo	*info = new CDeviceInfo(attachment, id);
    LOG((CLOG_DEBUG "DM:%p m_primaryClient address: %p", this, m_primaryClient));
    if(m_isPrimary)
    {
	info->m_active = m_primaryClient;	
	info->m_onScreen = true;
	info->m_entered = m_isPrimary;
	//LOG((CLOG_DEBUG2 "info->m_active name %s",info->m_active->getName().c_str()));
    }
    info->m_isPointer = isPointer;

    //m_devices->push_front(info); 
    m_devices->insert(pair<UInt8,CDeviceInfo*>(id,info))	;
    LOG((CLOG_DEBUG "done adding device"));
}

void 
CDeviceManager::removeDevice(UInt8 id)
{
      LOG((CLOG_DEBUG "erasing dev %d", id));
      m_devices->erase(id);      
//     CDeviceMap::iterator i;
//     for(i = m_devices->begin(); i != m_devices->end(); ++i)
//     {
// 	CDeviceInfo *tmp = *i;
// 	if(tmp->m_id == id)
// 	{
// 	    LOG((CLOG_DEBUG "erasing dev %d", id));
// 	    m_devices->erase(i);
// // 	    LOG((CLOG_DEBUG "deleting dev %d", id));
// // 	    delete tmp;
// 	}
//     }
}

UInt8 
CDeviceManager::getAttachment(UInt8 id) const
{
    CDeviceMap::iterator i = m_devices->find(id);
    return (*i).second->m_attachment;
//     CDeviceList::iterator i;
//     for(i = m_devices->begin(); i != m_devices->end(); ++i)
//     {
// 	CDeviceInfo *tmp = *i;
// 	if(tmp->m_id == id)
// 	{
// 	    return tmp->m_attachment;
// 	}
//     }
}

void 
CDeviceManager::setPrimaryClient(CPrimaryClient *primaryClient)
{
    if(primaryClient)
    {      
        m_primaryClient = primaryClient;
	LOG((CLOG_DEBUG "setPrimaryClient DM:%p m_primaryClient address: %p", this, m_primaryClient));
	m_isPrimary = true;
    }
}

void 
CDeviceManager::setActiveClient(CBaseClientProxy *active, UInt8 id)
{
    CDeviceMap::iterator i = m_devices->find(id);
    (*i).second->m_active = active;
    /*CDeviceList::iterator i;
    for(i = m_devices->begin(); i != m_devices->end(); ++i)
    {
	CDeviceInfo *tmp = *i;
	if(tmp->m_id == id)
	{
	    tmp->m_active = active;
	    
	}
    }   */ 
}
    
CBaseClientProxy *
CDeviceManager::getActiveClient(UInt8 id) const
{
    CDeviceMap::iterator i = m_devices->find(id);
    return (*i).second->m_active;
// //     LOG((CLOG_DEBUG "searching for m_active of dev(%d)", id));
//     //LOG((CLOG_DEBUG2 "m_primaryClient name: %s", m_primaryClient->getName().c_str()));
//     CDeviceList::iterator i;
//     for(i = m_devices->begin(); i != m_devices->end(); ++i)
//     {
// 	CDeviceInfo *tmp = *i;
// 	if(tmp->m_id == id)
// 	{
// 	    if(tmp->m_active)
// 	    {
// // 		LOG((CLOG_DEBUG "found m_active"));
// // 		LOG((CLOG_DEBUG "m_active name: %p", tmp->m_active));
// 		return tmp->m_active;
// 	    }
// 	    else
// 	    {
// // 		LOG((CLOG_DEBUG "no m_active"));
// 		return NULL;
// 	    }
// 	}
//     }  
}
        
void 
CDeviceManager::setKeyState(CKeyState *keyState, UInt8 id)
{
    LOG((CLOG_DEBUG2 "setKeyState keyState for device %d", id));
    CDeviceMap::iterator i = m_devices->find(id);
    (*i).second->m_keyState = keyState;
/*    CDeviceList::iterator i;
    for(i = m_devices->begin(); i != m_devices->end(); ++i)
    {
	CDeviceInfo *tmp = *i;
	if(tmp->m_id == id)
	{
	    tmp->m_keyState = keyState;
	}
    }    */
}
	
CKeyState *
CDeviceManager::getKeyState(UInt8 id) const
{
    LOG((CLOG_DEBUG2 "getKeyState for device: %d",id));
    CDeviceMap::iterator i = m_devices->find(id);
    if((*i).second->m_isPointer)
      i = m_devices->find((*i).second->m_attachment);
    return (*i).second->m_keyState;
/*    CDeviceList::iterator i;
    CDeviceList::iterator j;
    
    if(!m_devices)
      LOG((CLOG_DEBUG "m_devices = NULL !"));

    for(i = m_devices->begin(); i != m_devices->end(); ++i)
    {
	CDeviceInfo *tmp = *i;
	if(!tmp)
	     LOG((CLOG_DEBUG "tmp is NULL"));
	if(tmp->m_id == id)
	{
	    //LOG((CLOG_DEBUG "keyState, found id %d",id));
	    if(!tmp->m_isPointer)
	    {
		//LOG((CLOG_DEBUG "device: %d m_isPointer: %d",id, tmp->m_isPointer));
		if(tmp->m_keyState)
		    return tmp->m_keyState;
		else
		    return NULL;
	    }
	    else
	    {
		  //  LOG((CLOG_DEBUG "get attachment for %d",id));
		    UInt8 kId = getAttachment(id);
		    //LOG((CLOG_DEBUG "getKeyState for device: %d attached to %d",kId, id));
		    for(j = m_devices->begin(); j != m_devices->end(); ++j)
		    {
			CDeviceInfo *tmp2 = *j;
			if(tmp2->m_id == kId)
			{
		//	  LOG((CLOG_DEBUG "found deviceinfo for %d",kId));
			  if(tmp2->m_keyState)
			  {
		//	      LOG((CLOG_DEBUG "got keystate for %d",kId));
			      return tmp2->m_keyState;
			  }
			  else
			      return NULL;
			}
		    }
	    }
	}
    } */
}
	
void 
CDeviceManager::setServerId(UInt8 sId, UInt8 id)
{    
    CDeviceMap::iterator i = m_devices->find(id);
    (*i).second->m_sId = sId;
//     CDeviceList::iterator i;
//     for(i = m_devices->begin(); i != m_devices->end(); ++i)
//     {
// 	CDeviceInfo *tmp = *i;
// 	if(tmp->m_id == id)
// 	{
// 	    tmp->m_sId = sId;
// 	}
//     }    
}
    
UInt8 
CDeviceManager::getServerId(UInt8 id) const
{     
    CDeviceMap::iterator i = m_devices->find(id);
    return (*i).second->m_sId;
//     CDeviceList::iterator i;
//     for(i = m_devices->begin(); i != m_devices->end(); ++i)
//     {
// 	CDeviceInfo *tmp = *i;
// 	if(tmp->m_id == id)
// 	{
// 	    return tmp->m_sId;
// 	}
//     }
}
    
UInt8 
CDeviceManager::getIdFromSid(UInt8 sId) const
{
    LOG((CLOG_DEBUG2 "getting id for sId %d", sId));
    CDeviceMap::iterator i = m_devices->begin();
    for(i; i != m_devices->end(); i++)    	
	if((*i).second->m_sId == sId)
	  return (*i).second->m_id;	
}
void 
CDeviceManager::setJumpCursorPos(const CString& name, SInt32 x, SInt32 y, UInt8 id) 
{      
    CDeviceMap::iterator i = m_devices->find(id);
    (*i).second->m_xJumpPos = x;
    (*i).second->m_yJumpPos = y;
//     CDeviceList::iterator i;
//     for(i = m_devices->begin(); i != m_devices->end(); ++i)
//     {
// 	CDeviceInfo *tmp = *i;
// 	if(tmp->m_id == id)
// 	{
// 	    tmp->m_xJumpPos = x;
// 	    tmp->m_yJumpPos = y;	    
// 	}
//     }  
}
  
void 
CDeviceManager::getJumpCursorPos(const CString& name, SInt32& x, SInt32& y, UInt8 id) const
{
    CDeviceMap::iterator i = m_devices->find(id);
    x = (*i).second->m_xJumpPos;
    y = (*i).second->m_yJumpPos;
//     CDeviceList::iterator i;
//     for(i = m_devices->begin(); i != m_devices->end(); ++i)
//     {
// 	CDeviceInfo *tmp = *i;
// 	if(tmp->m_id == id)
// 	{
// 	    x = tmp->m_xJumpPos;
// 	    y = tmp->m_yJumpPos;	    
// 	}
//     }
}   
 
void 
CDeviceManager::setCursorDelta(SInt32 x, SInt32 y, UInt8 id)
{
    CDeviceMap::iterator i = m_devices->find(id);
    (*i).second->m_xDelta = x;
    (*i).second->m_yDelta = y;
//     CDeviceList::iterator i;
//     for(i = m_devices->begin(); i != m_devices->end(); ++i)
//     {
// 	CDeviceInfo *tmp = *i;
// 	if(tmp->m_id == id)
// 	{
// 	    tmp->m_xDelta = x;
// 	    tmp->m_yDelta = y;	    
// 	}
//     }

}

void 
CDeviceManager::getCursorDelta(SInt32& x, SInt32& y, UInt8 id) const
{
    CDeviceMap::iterator i = m_devices->find(id);
    x = (*i).second->m_xDelta;
    y = (*i).second->m_yDelta;
/*    CDeviceList::iterator i;
    for(i = m_devices->begin(); i != m_devices->end(); ++i)
    {
	CDeviceInfo *tmp = *i;
	if(tmp->m_id == id)
	{
	    x = tmp->m_xDelta;
	    y = tmp->m_yDelta;	    
	}
    }*/
  
}
 
void 
CDeviceManager::setCursorDelta2(SInt32 x, SInt32 y, UInt8 id)
{
    CDeviceMap::iterator i = m_devices->find(id);
    (*i).second->m_xDelta2 = x;
    (*i).second->m_yDelta2 = y;
/*    CDeviceList::iterator i;
    for(i = m_devices->begin(); i != m_devices->end(); ++i)
    {
	CDeviceInfo *tmp = *i;
	if(tmp->m_id == id)
	{
	    tmp->m_xDelta2 = x;
	    tmp->m_yDelta2 = y;	    
	}
    }*/
  
}

void
CDeviceManager::getCursorDelta2(SInt32& x, SInt32& y, UInt8 id) const
{
    CDeviceMap::iterator i = m_devices->find(id);
    x = (*i).second->m_xDelta2;
    y = (*i).second->m_yDelta2;
/*    CDeviceList::iterator i;
    for(i = m_devices->begin(); i != m_devices->end(); ++i)
    {
	CDeviceInfo *tmp = *i;
	if(tmp->m_id == id)
	{
	    x = tmp->m_xDelta2;
	    y = tmp->m_yDelta2;	    
	}
    }*/
  
}


void CDeviceManager::getLastCursorPos(SInt32& x, SInt32& y, UInt8 id) const
{
    CDeviceMap::iterator i = m_devices->find(id);
    x = (*i).second->m_xCursor;
    y = (*i).second->m_yCursor;
//     CDeviceList::iterator i;
//     for(i = m_devices->begin(); i != m_devices->end(); ++i)
//     {
// 	CDeviceInfo *tmp = *i;
// 	if(tmp->m_id == id)
// 	{
// 	    x = tmp->m_xCursor;
// 	    y = tmp->m_yCursor ;	    
// 	}
//     }
}

void CDeviceManager::getLocation(CString& screenName, UInt8 id) const
{
    CDeviceMap::iterator i = m_devices->find(id);
    screenName = (*i).second->m_serverName;
}

bool CDeviceManager::isOnScreen(UInt8 id) const
{
    CDeviceMap::iterator i = m_devices->find(id);
    return (*i).second->m_onScreen;
//     CDeviceList::iterator i;
//     for(i = m_devices->begin(); i != m_devices->end(); ++i)
//     {
//       CDeviceInfo *tmp = *i;
//       if(tmp->m_id == id)
// 	    return(tmp->m_onScreen);
//     }
}

void CDeviceManager::setLastCursorPos(SInt32 x, SInt32 y, UInt8 id)
{
  CDeviceMap::iterator i = m_devices->find(id);
  (*i).second->m_xCursor = x;
  (*i).second->m_yCursor = y;

//     CDeviceList::iterator i;
//     for(i = m_devices->begin(); i != m_devices->end(); ++i)
//     {
// 	CDeviceInfo *tmp = *i;
// 	if(tmp->m_id == id)
// 	{
// 	    tmp->m_xCursor = x;
// 	    tmp->m_yCursor = y;	    
// 	}
//     }
}

void CDeviceManager::setLocation(CString* screenName, UInt8 id)
{
    CDeviceMap::iterator i = m_devices->find(id);
    (*i).second->m_serverName = screenName->c_str();
}

void CDeviceManager::setIsOnScreen(bool value, UInt8 id)
{
  CDeviceMap::iterator i = m_devices->find(id);
  (*i).second->m_onScreen = value;
//     CDeviceList::iterator i;
//     for(i = m_devices->begin(); i != m_devices->end(); ++i)
//     {
// 	CDeviceInfo *tmp = *i;
// 	if(tmp->m_id == id)
// 	    tmp->m_onScreen = value;
//     }
}

void 
CDeviceManager::getAllKeyboardIDs(std::list<UInt8>& list) const
{
//     CDeviceList::iterator i;
//     for(i = m_devices->begin(); i != m_devices->end(); ++i)
//     {
    CDeviceMap::iterator i = m_devices->begin();
    for(i; i != m_devices->end(); i++)
    {	
	if(!(*i).second->m_isPointer)
	    list.push_front((*i).second->m_id);
//	LOG((CLOG_DEBUG "getAllPointerIDs is processing dev(%d)", tmp->m_id));
	
    }
}
void 
CDeviceManager::getAllPointerIDs(std::list<UInt8>& list) const
{
//    LOG((CLOG_DEBUG "getAllPointerIDs!!!!!!!!!!!"));
/*    CDeviceList::iterator i;
    for(i = m_devices->begin(); i != m_devices->end(); ++i)
    {*/
    CDeviceMap::iterator i = m_devices->begin();    
    for(i; i != m_devices->end(); i++)
    {
	if((*i).second->m_isPointer)
	    list.push_front((*i).second->m_id);
//	LOG((CLOG_DEBUG "getAllPointerIDs is processing dev(%d)", tmp->m_id));
	
    }
}

void 
CDeviceManager::getAllDeviceIDs(std::list<UInt8>& list) const
{
//    LOG((CLOG_DEBUG "getAllPointerIDs!!!!!!!!!!!"));
    CDeviceMap::iterator i = m_devices->begin();
    for(i; i != m_devices->end(); i++)
    {
	list.push_front((*i).second->m_id);
//	LOG((CLOG_DEBUG "getAllPointerIDs is processing dev(%d)", tmp->m_id));
	
    }
}

bool 
CDeviceManager::isLockedToScreen(UInt8 id) const
{
  CDeviceMap::iterator i = m_devices->find(id);
  return (*i).second->m_lockedToScreen;
//     CDeviceList::iterator i;
//     for(i = m_devices->begin(); i != m_devices->end(); ++i)
//     {
//       CDeviceInfo *tmp = *i;
//       if(tmp->m_id == id)
// 	    return(tmp->m_lockedToScreen);
//     }
}
  
bool 
CDeviceManager::isPointer(UInt8 id) const
{
  CDeviceMap::iterator i = m_devices->find(id);
  return (*i).second->m_isPointer;
/*    CDeviceList::iterator i;
    for(i = m_devices->begin(); i != m_devices->end(); ++i)
    {
      CDeviceInfo *tmp = *i;
      if(tmp->m_id == id)
	    return(tmp->m_isPointer);
    }  */
}
bool
CDeviceManager::isRelativeMovesSet(UInt8 id) const
{
  CDeviceMap::iterator i = m_devices->find(id);
  return (*i).second->m_relativeMoves;
//     CDeviceList::iterator i;
//     for(i = m_devices->begin(); i != m_devices->end(); ++i)
//     {
//       CDeviceInfo *tmp = *i;
//       if(tmp->m_id == id)
// 	    return(tmp->m_relativeMoves);
//     }
}


void
CDeviceManager::setRelativeMoves(bool value, UInt8 id)
{
  CDeviceMap::iterator i = m_devices->find(id);
  (*i).second->m_relativeMoves = value;
//     CDeviceList::iterator i;
//     for(i = m_devices->begin(); i != m_devices->end(); ++i)
//     {
// 	CDeviceInfo *tmp = *i;
// 	if(tmp->m_id == id)
// 	    tmp->m_relativeMoves = value;
//     }
}

bool
CDeviceManager::hasEntered(UInt8 id) const
{
  CDeviceMap::iterator i = m_devices->find(id);
  return (*i).second->m_entered;
//     CDeviceList::iterator i;
//     for(i = m_devices->begin(); i != m_devices->end(); ++i)
//     {
//       CDeviceInfo *tmp = *i;
//       if(tmp->m_id == id)
// 	    return(tmp->m_entered);
//     }
}


void
CDeviceManager::setEntered(bool value, UInt8 id)
{
  CDeviceMap::iterator i = m_devices->find(id);
  (*i).second->m_entered = value;
//     CDeviceList::iterator i;
//     for(i = m_devices->begin(); i != m_devices->end(); ++i)
//     {
// 	CDeviceInfo *tmp = *i;
// 	if(tmp->m_id == id)
// 	    tmp->m_entered = value;
//     }
}

void 
CDeviceManager::setLockedToScreen(bool value, UInt8 id)
{
  CDeviceMap::iterator i = m_devices->find(id);
  (*i).second->m_lockedToScreen = value;
/*    CDeviceList::iterator i;
    for(i = m_devices->begin(); i != m_devices->end(); ++i)
    {
	CDeviceInfo *tmp = *i;
	if(tmp->m_id == id)
	    tmp->m_lockedToScreen = value;
    } */
} 

void 
CDeviceManager::setXtestId(UInt8 xtestId, UInt8 id)
{
    LOG((CLOG_DEBUG "adding xtest device (%d) for master device (%d).", xtestId,id));
    CDeviceMap::iterator i = m_devices->find(id);
    (*i).second->m_xtestDevice = xtestId;
}

UInt8 
CDeviceManager::getXtestId(UInt8 id)
{
    LOG((CLOG_DEBUG "getting xtest device id for master device (%d).", id));
    CDeviceMap::iterator i = m_devices->find(id);
    LOG((CLOG_DEBUG "returning xtest device id (%d).", (*i).second->m_xtestDevice));
    return (*i).second->m_xtestDevice;    
}

CDeviceManager::CDeviceInfo::CDeviceInfo(UInt8 attachment, UInt8 id) :
        m_sId(2), 
        m_xCursor(0), 
	m_yCursor(0),
	m_xJumpPos(0),
	m_yJumpPos(0),
	m_xDelta(0), 
	m_yDelta(0),
	m_xDelta2(0),
	m_yDelta2(0),
	m_onScreen(true), 
	m_lockedToScreen(false),
	m_isPointer(false), 
	m_relativeMoves(false),
	m_active(NULL)
{
    m_id = id;
    m_attachment = attachment;    
}


