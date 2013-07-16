/*
 * synergy -- mouse and keyboard sharing utility
 * Copyright (C) 2012 Bolton Software Ltd.
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
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef CINPUTFILTER_H
#define CINPUTFILTER_H

#include "KeyTypes.h"
#include "MouseTypes.h"
#include "ProtocolTypes.h"
#include "IPlatformScreen.h"
#include "CString.h"
#include "stdmap.h"
#include "stdset.h"

class CPrimaryClient;
class CEvent;
class IEventQueue;

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
		CKeystrokeCondition(IEventQueue* events, IPlatformScreen::CKeyInfo*);
		CKeystrokeCondition(IEventQueue* events, KeyID key, KeyModifierMask mask);
		virtual ~CKeystrokeCondition();

		KeyID					getKey() const;
		KeyModifierMask			getMask() const;

		// CCondition overrides
		virtual CCondition*		clone() const;
		virtual CString			format() const;
		virtual EFilterStatus	match(const CEvent&);
		virtual void			enablePrimary(CPrimaryClient*);
		virtual void			disablePrimary(CPrimaryClient*);

	private:
		UInt32					m_id;
		KeyID					m_key;
		KeyModifierMask			m_mask;
		IEventQueue*			m_events;
	};

	// CMouseButtonCondition
	class CMouseButtonCondition : public CCondition {
	public:
		CMouseButtonCondition(IEventQueue* events, IPlatformScreen::CButtonInfo*);
		CMouseButtonCondition(IEventQueue* events, ButtonID, KeyModifierMask mask);
		virtual ~CMouseButtonCondition();

		ButtonID				getButton() const;
		KeyModifierMask			getMask() const;

		// CCondition overrides
		virtual CCondition*		clone() const;
		virtual CString			format() const;
		virtual EFilterStatus	match(const CEvent&);

	private:
		ButtonID				m_button;
		KeyModifierMask			m_mask;
		IEventQueue*			m_events;
	};

	// CScreenConnectedCondition
	class CScreenConnectedCondition : public CCondition {
	public:
		CScreenConnectedCondition(IEventQueue* events, const CString& screen);
		virtual ~CScreenConnectedCondition();

		// CCondition overrides
		virtual CCondition*		clone() const;
		virtual CString			format() const;
		virtual EFilterStatus	match(const CEvent&);

	private:
		CString					m_screen;
		IEventQueue*			m_events;
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

		CLockCursorToScreenAction(IEventQueue* events, Mode = kToggle);

		Mode					getMode() const;

		// CAction overrides
		virtual CAction*		clone() const;
		virtual CString			format() const;
		virtual void			perform(const CEvent&);

	private:
		Mode					m_mode;
		IEventQueue*			m_events;
	};
	
	// CSwitchToScreenAction
	class CSwitchToScreenAction : public CAction {
	public:
		CSwitchToScreenAction(IEventQueue* events, const CString& screen);

		CString					getScreen() const;

		// CAction overrides
		virtual CAction*		clone() const;
		virtual CString			format() const;
		virtual void			perform(const CEvent&);

	private:
		CString					m_screen;
		IEventQueue*			m_events;
	};
	
	// CSwitchInDirectionAction
	class CSwitchInDirectionAction : public CAction {
	public:
		CSwitchInDirectionAction(IEventQueue* events, EDirection);

		EDirection				getDirection() const;

		// CAction overrides
		virtual CAction*		clone() const;
		virtual CString			format() const;
		virtual void			perform(const CEvent&);

	private:
		EDirection				m_direction;
		IEventQueue*			m_events;
	};
	
	// CKeyboardBroadcastAction
	class CKeyboardBroadcastAction : public CAction {
	public:
		enum Mode { kOff, kOn, kToggle };

		CKeyboardBroadcastAction(IEventQueue* events, Mode = kToggle);
		CKeyboardBroadcastAction(IEventQueue* events, Mode, const std::set<CString>& screens);

		Mode					getMode() const;
		std::set<CString>		getScreens() const;

		// CAction overrides
		virtual CAction*		clone() const;
		virtual CString			format() const;
		virtual void			perform(const CEvent&);

	private:
		Mode					m_mode;
		CString					m_screens;
		IEventQueue*			m_events;
	};

	// CKeystrokeAction
	class CKeystrokeAction : public CAction {
	public:
		CKeystrokeAction(IEventQueue* events, IPlatformScreen::CKeyInfo* adoptedInfo, bool press);
		~CKeystrokeAction();

		void					adoptInfo(IPlatformScreen::CKeyInfo*);
		const IPlatformScreen::CKeyInfo*
								getInfo() const;
		bool					isOnPress() const;

		// CAction overrides
		virtual CAction*		clone() const;
		virtual CString			format() const;
		virtual void			perform(const CEvent&);

	protected:
		virtual const char*		formatName() const;

	private:
		IPlatformScreen::CKeyInfo*	m_keyInfo;
		bool					m_press;
		IEventQueue*			m_events;
	};

	// CMouseButtonAction -- modifier combinations not implemented yet
	class CMouseButtonAction : public CAction {
	public:
		CMouseButtonAction(IEventQueue* events,
									IPlatformScreen::CButtonInfo* adoptedInfo,
									bool press);
		~CMouseButtonAction();

		const IPlatformScreen::CButtonInfo*
								getInfo() const;
		bool					isOnPress() const;

		// CAction overrides
		virtual CAction*		clone() const;
		virtual CString			format() const;
		virtual void			perform(const CEvent&);

	protected:
		virtual const char*		formatName() const;

	private:
		IPlatformScreen::CButtonInfo*	m_buttonInfo;
		bool					m_press;
		IEventQueue*			m_events;
	};

	class CRule {
	public:
		CRule();
		CRule(CCondition* adopted);
		CRule(const CRule&);
		~CRule();

		CRule& operator=(const CRule&);

		// replace the condition
		void			setCondition(CCondition* adopted);

		// add an action to the rule
		void			adoptAction(CAction*, bool onActivation);

		// remove an action from the rule
		void			removeAction(bool onActivation, UInt32 index);

		// replace an action in the rule
		void			replaceAction(CAction* adopted,
							bool onActivation, UInt32 index);

		// enable/disable
		void			enable(CPrimaryClient*);
		void			disable(CPrimaryClient*);

		// event handling
		bool			handleEvent(const CEvent&);

		// convert rule to a string
		CString			format() const;

		// get the rule's condition
		const CCondition*
						getCondition() const;

		// get number of actions
		UInt32			getNumActions(bool onActivation) const;

		// get action by index
		const CAction&	getAction(bool onActivation, UInt32 index) const;

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

	CInputFilter(IEventQueue* events);
	CInputFilter(const CInputFilter&);
	virtual ~CInputFilter();

#ifdef TEST_ENV
	CInputFilter() : m_primaryClient(NULL) { }
#endif

	CInputFilter&		operator=(const CInputFilter&);

	// add rule, adopting the condition and the actions
	void				addFilterRule(const CRule& rule);

	// remove a rule
	void				removeFilterRule(UInt32 index);

	// get rule by index
	CRule&				getRule(UInt32 index);

	// enable event filtering using the given primary client.  disable
	// if client is NULL.
	virtual void		setPrimaryClient(CPrimaryClient* client);

	// convert rules to a string
	CString				format(const CString& linePrefix) const;

	// get number of rules
	UInt32				getNumRules() const;

	//! Compare filters
	bool				operator==(const CInputFilter&) const;
	//! Compare filters
	bool				operator!=(const CInputFilter&) const;

private:
	// event handling
	void				handleEvent(const CEvent&, void*);

private:
	CRuleList			m_ruleList;
	CPrimaryClient*		m_primaryClient;
	IEventQueue*		m_events;
};

#endif
