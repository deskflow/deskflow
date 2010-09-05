/*
 * synergy -- mouse and keyboard sharing utility
 * Copyright (C) 2005 Chris Schoeneman
 * 
 * This package is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * found in the file COPYING that should have accompanied this file.
 * 
 * This package is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */

#ifndef CINPUTFILTER_H
#define CINPUTFILTER_H

#include "KeyTypes.h"
#include "MouseTypes.h"
#include "ProtocolTypes.h"
#include "IPlatformScreen.h"
#include "CString.h"
#include "stdmap.h"

class CPrimaryClient;
class CEvent;

class CInputFilter {
public:
	// -------------------------------------------------------------------------
	// Input Filter Condition Classes
	// -------------------------------------------------------------------------
	enum EFilterStatus {
		kDiscard         = -1,
		kNoMatch         = 0,
		kNotHandled      = 0,
		kMatch           = 1,
		kHandled         = 1,
		kUpdateModifiers = 2
	};

	enum EActionMode {
		kModePass,
		kModeTrigger,
		kModeToggle,
		kModeTurnOff,
		kModeTurnOn
	};

	class CCondition {
	public:
		CCondition(EActionMode = kModePass);
		virtual ~CCondition();

		virtual CCondition*		clone() const = 0;
		virtual CString			format() const = 0;

		virtual EFilterStatus	match(CEvent&, void*, EActionMode&) = 0;
		virtual void			enablePrimary(CPrimaryClient*);
		virtual void			disablePrimary(CPrimaryClient*);

		void					setInputFilter(CInputFilter*);

		void					setClearMask(KeyModifierMask);
		KeyModifierMask			getClearMask() const;

		KeyModifierMask			getLastMask() const;
		EActionMode				getActionMode() const;

	private:
		EActionMode				m_actionMode;
		KeyModifierMask			m_clearMask;
		CInputFilter*			m_inputFilter;
	};
	
	// CKeystrokeCondition
	class CKeystrokeCondition : public CCondition {
	public:
		CKeystrokeCondition(IPlatformScreen::CKeyInfo*, EActionMode = kModePass);
		virtual ~CKeystrokeCondition();

		virtual CCondition*		clone() const;
		virtual CString			format() const;
		virtual EFilterStatus	match(CEvent &,void *,EActionMode&);
		virtual void			enablePrimary(CPrimaryClient*);
		virtual void			disablePrimary(CPrimaryClient*);

	private:
		CKeystrokeCondition(KeyID key, KeyModifierMask mask, EActionMode);

	private:
		UInt32					m_id;
		KeyID					m_key;
		KeyModifierMask			m_mask;
	};

	// CMouseButtonCondition
	class  CMouseButtonCondition : public CCondition {
	public:
		CMouseButtonCondition(IPlatformScreen::CButtonInfo*,EActionMode);
		virtual ~CMouseButtonCondition();

		virtual CCondition*		clone() const;
		virtual CString			format() const;
		virtual EFilterStatus	match(CEvent &,void *,EActionMode&);

	private:
		CMouseButtonCondition(ButtonID, KeyModifierMask mask, EActionMode);

	private:
		ButtonID				m_button;
		KeyModifierMask			m_mask;
	};

	// -------------------------------------------------------------------------
	// Input Filter Action Classes
	// -------------------------------------------------------------------------
	enum EActionState {
		kStateInvalid = -1,
		kStateOff     = 0,
		kStateOn      = 1
	};
	
	class CAction {
    public:
		CAction(EActionState = kStateOff);
		virtual	~CAction();

		virtual CAction*		clone() const = 0;
		virtual CString			format() const = 0;

        virtual EFilterStatus	perform(CEvent&, void*, EActionMode) = 0;
		virtual void			enablePrimary(CPrimaryClient*);
		virtual void			disablePrimary(CPrimaryClient*);

		void					setInputFilter(CInputFilter*);

		void					setModifierMask(KeyModifierMask);
		KeyModifierMask			getModifierMask() const;

		EActionState			switchMode(EActionMode);
		void					setState(EActionState);

	private:
		EActionState			m_state;
		KeyModifierMask			m_modifierMask;
		CInputFilter*			m_inputFilter;
    };
	
	// CLockCursorToScreenAction
	class CLockCursorToScreenAction : public CAction {
	public:
		// CAction overrides
		virtual CAction*		clone() const;
		virtual CString			format() const;
		virtual EFilterStatus	perform(CEvent&, void*, EActionMode);
	};
	
	// CSwitchToScreenAction
	class CSwitchToScreenAction : public CAction {
	public:
		CSwitchToScreenAction(const CString& screen);

		// CAction overrides
		virtual CAction*		clone() const;
		virtual CString			format() const;
		virtual EFilterStatus	perform(CEvent&, void*, EActionMode);

	private:
		CString					m_screen;
	};
	
	// CSwitchInDirectionAction
	class CSwitchInDirectionAction : public CAction {
	public:
		CSwitchInDirectionAction(EDirection);

		// CAction overrides
		virtual CAction*		clone() const;
		virtual CString			format() const;
		virtual EFilterStatus	perform(CEvent&, void*, EActionMode);

	private:
		EDirection				m_direction;
	};
	
	// CKeystrokeAction
	class CKeystrokeAction : public CAction {
	public:
		CKeystrokeAction(IPlatformScreen::CKeyInfo* adoptedInfo);
		~CKeystrokeAction();

		// CAction overrides
		virtual CAction*		clone() const;
		virtual CString			format() const;
		virtual EFilterStatus	perform(CEvent&, void*, EActionMode);

	private:
		IPlatformScreen::CKeyInfo*	m_keyInfo;
	};
	
	// CModifierAction -- not implemented yet
	class CModifierAction : public CAction {
	public:
		CModifierAction(KeyModifierMask, KeyModifierMask);		

		// CAction overrides
		virtual CAction*		clone() const;
		virtual CString			format() const;
		virtual EFilterStatus	perform(CEvent&, void*, EActionMode);

	private:
		KeyModifierMask	m_modifiers;
		KeyModifierMask	m_bitmask;
	};

	// CMouseButtonAction -- modifier combinations not implemented yet
	class CMouseButtonAction : public CAction {
	public:
		CMouseButtonAction(IPlatformScreen::CButtonInfo* adoptedInfo);
		~CMouseButtonAction();

		// CAction overrides
		virtual CAction*		clone() const;
		virtual CString			format() const;
		virtual EFilterStatus	perform(CEvent&, void*, EActionMode);

	private:
		IPlatformScreen::CButtonInfo	*m_buttonInfo;
	};

	// -------------------------------------------------------------------------
	// Input Filter Class
	// -------------------------------------------------------------------------
	typedef std::pair<CCondition*, CAction*> CRule;
	typedef std::vector<CRule> CRuleList;

	CInputFilter();
	CInputFilter(const CInputFilter&);
	virtual ~CInputFilter();

	CInputFilter&		operator=(const CInputFilter&);

	// parse config string and add rule
	void				addFilterRule(CCondition* cond, CAction* action);

	// enable event filtering using the given primary client.  disable
	// if client is NULL.
	void				setPrimaryClient(CPrimaryClient* client);

	// get masks
	KeyModifierMask		getLastMask() const;

	// mark masks as dirty
	void				setClearMaskDirty();
	void				setModifierMaskDirty();

	// get the rules
	const CRuleList&	getRules() const;

private:
	void				copyRules(const CRuleList&);
	void				deleteRules(CRuleList&) const;

	// event handling
	void				handleEvent(const CEvent&, void*);
	void				sendEvent(CEvent&);
	void				updateModifiers();
	
private:
	enum EDirtyFlags {
		kNotDirty       = 0x0000,
		kClearDirty     = 0x0001,
		kModifiersDirty = 0x0002
	};

	CRuleList			m_ruleList;
	CPrimaryClient*		m_primaryClient;

	// last state of modifier keys
	KeyModifierMask		m_lastMask;

	// designates if modifier or clear mask have changed
	UInt32				m_dirtyFlag;

	// modifiers set in this mask will be cleared from outgoing event
	KeyModifierMask		m_clearMask;

	// modifiers set in this mask will be added to the outgoing event
	KeyModifierMask		m_modifierMask;
};

#endif
