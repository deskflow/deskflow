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
		kNoMatch,
		kActivate,
		kDeactivate
	};

	class CCondition {
	public:
		CCondition();
		virtual ~CCondition();

		virtual CCondition*		clone() const = 0;
		virtual CString			format() const = 0;

		virtual EFilterStatus	match(const CEvent&) = 0;

		virtual void			enablePrimary(CPrimaryClient*);
		virtual void			disablePrimary(CPrimaryClient*);
	};
	
	// CKeystrokeCondition
	class CKeystrokeCondition : public CCondition {
	public:
		CKeystrokeCondition(IPlatformScreen::CKeyInfo*);
		virtual ~CKeystrokeCondition();

		virtual CCondition*		clone() const;
		virtual CString			format() const;
		virtual EFilterStatus	match(const CEvent&);
		virtual void			enablePrimary(CPrimaryClient*);
		virtual void			disablePrimary(CPrimaryClient*);

	private:
		CKeystrokeCondition(KeyID key, KeyModifierMask mask);

	private:
		UInt32					m_id;
		KeyID					m_key;
		KeyModifierMask			m_mask;
	};

	// CMouseButtonCondition
	class CMouseButtonCondition : public CCondition {
	public:
		CMouseButtonCondition(IPlatformScreen::CButtonInfo*);
		virtual ~CMouseButtonCondition();

		virtual CCondition*		clone() const;
		virtual CString			format() const;
		virtual EFilterStatus	match(const CEvent&);

	private:
		CMouseButtonCondition(ButtonID, KeyModifierMask mask);

	private:
		ButtonID				m_button;
		KeyModifierMask			m_mask;
	};

	// CScreenConnectedCondition
	class CScreenConnectedCondition : public CCondition {
	public:
		CScreenConnectedCondition(const CString& screen);
		virtual ~CScreenConnectedCondition();

		virtual CCondition*		clone() const;
		virtual CString			format() const;
		virtual EFilterStatus	match(const CEvent&);

	private:
		CString					m_screen;
	};

	// -------------------------------------------------------------------------
	// Input Filter Action Classes
	// -------------------------------------------------------------------------
	
	class CAction {
    public:
		CAction();
		virtual	~CAction();

		virtual CAction*		clone() const = 0;
		virtual CString			format() const = 0;

        virtual void			perform(const CEvent&) = 0;
    };
	
	// CLockCursorToScreenAction
	class CLockCursorToScreenAction : public CAction {
	public:
		enum Mode { kOff, kOn, kToggle };

		CLockCursorToScreenAction(Mode = kToggle);

		// CAction overrides
		virtual CAction*		clone() const;
		virtual CString			format() const;
		virtual void			perform(const CEvent&);

	private:
		Mode					m_mode;
	};
	
	// CSwitchToScreenAction
	class CSwitchToScreenAction : public CAction {
	public:
		CSwitchToScreenAction(const CString& screen);

		// CAction overrides
		virtual CAction*		clone() const;
		virtual CString			format() const;
		virtual void			perform(const CEvent&);

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
		virtual void			perform(const CEvent&);

	private:
		EDirection				m_direction;
	};
	
	// CKeystrokeAction
	class CKeystrokeAction : public CAction {
	public:
		CKeystrokeAction(IPlatformScreen::CKeyInfo* adoptedInfo, bool press);
		~CKeystrokeAction();

		// CAction overrides
		virtual CAction*		clone() const;
		virtual CString			format() const;
		virtual void			perform(const CEvent&);

	private:
		IPlatformScreen::CKeyInfo*	m_keyInfo;
		bool						m_press;
	};

	// CMouseButtonAction -- modifier combinations not implemented yet
	class CMouseButtonAction : public CAction {
	public:
		CMouseButtonAction(IPlatformScreen::CButtonInfo* adoptedInfo,
									bool press);
		~CMouseButtonAction();

		// CAction overrides
		virtual CAction*		clone() const;
		virtual CString			format() const;
		virtual void			perform(const CEvent&);

	private:
		IPlatformScreen::CButtonInfo*	m_buttonInfo;
		bool							m_press;
	};

	class CRule {
	public:
		CRule();
		CRule(CCondition* adopted);
		CRule(const CRule&);
		~CRule();

		CRule& operator=(const CRule&);

		// add an action to the rule
		void			adoptAction(CAction*, bool onActivation);

		// enable/disable
		void			enable(CPrimaryClient*);
		void			disable(CPrimaryClient*);

		// event handling
		bool			handleEvent(const CEvent&);

		// convert rule to a string
		CString			format() const;

	private:
		void			clear();
		void			copy(const CRule&);

	private:
		typedef std::vector<CAction*> CActionList;

		CCondition*		m_condition;
		CActionList		m_activateActions;
		CActionList		m_deactivateActions;
	};

	// -------------------------------------------------------------------------
	// Input Filter Class
	// -------------------------------------------------------------------------
	typedef std::vector<CRule> CRuleList;

	CInputFilter();
	CInputFilter(const CInputFilter&);
	virtual ~CInputFilter();

	CInputFilter&		operator=(const CInputFilter&);

	// add rule, adopting the condition and the actions
	void				addFilterRule(const CRule& rule);

	// enable event filtering using the given primary client.  disable
	// if client is NULL.
	void				setPrimaryClient(CPrimaryClient* client);

	// convert rules to a string
	CString				format(const CString& linePrefix) const;

private:
	// event handling
	void				handleEvent(const CEvent&, void*);

private:
	CRuleList			m_ruleList;
	CPrimaryClient*		m_primaryClient;
};

#endif
