#ifndef CEVENT_H
#define CEVENT_H

#include "BasicTypes.h"
#include "KeyTypes.h"
#include "MouseTypes.h"

class ISocket;

class CEventBase {
  public:
	enum EType {
		kNull,
		kKeyDown,
		kKeyRepeat,
		kKeyUp,
		kMouseDown,
		kMouseUp,
		kMouseMove,
		kMouseWheel,
		kScreenSize
	};

	EType				m_type;
};

class CEventKey : public CEventBase {
  public:
	KeyID				m_key;
	KeyModifierMask			m_mask;
	SInt32				m_count;
};

class CEventMouse : public CEventBase {
  public:
	ButtonID			m_button;
	SInt32				m_x;		// or wheel delta
	SInt32				m_y;
};

class CEventSize : public CEventBase {
  public:
	SInt32				m_w;
	SInt32				m_h;
};

class CEvent {
  public:
	union {
	  public:
		CEventBase		m_any;
		CEventKey		m_key;
		CEventMouse		m_mouse;
		CEventSize		m_size;
	};
};

#endif

