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

#include "CInputFilter.h"
#include "CServer.h"
#include "CPrimaryClient.h"
#include "CKeyMap.h"
#include "CEventQueue.h"
#include "CLog.h"
#include "TMethodEventJob.h"
#include <cstdlib>
#include <cstring>

// -----------------------------------------------------------------------------
// Input Filter Condition Classes
// -----------------------------------------------------------------------------
CInputFilter::CCondition::CCondition()
{
	// do nothing
}

CInputFilter::CCondition::~CCondition()
{
	// do nothing
}

void
CInputFilter::CCondition::enablePrimary(CPrimaryClient*)
{
	// do nothing
}

void
CInputFilter::CCondition::disablePrimary(CPrimaryClient*)
{
	// do nothing
}

CInputFilter::CKeystrokeCondition::CKeystrokeCondition(
		IPlatformScreen::CKeyInfo* info) :
	m_id(0),
	m_key(info->m_key),
	m_mask(info->m_mask)
{
	free(info);
}

CInputFilter::CKeystrokeCondition::CKeystrokeCondition(
		KeyID key, KeyModifierMask mask) :
	m_id(0),
	m_key(key),
	m_mask(mask)
{
	// do nothing
}

CInputFilter::CKeystrokeCondition::~CKeystrokeCondition()
{
	// do nothing
}

KeyID
CInputFilter::CKeystrokeCondition::getKey() const
{
	return m_key;
}

KeyModifierMask
CInputFilter::CKeystrokeCondition::getMask() const
{
	return m_mask;
}

CInputFilter::CCondition*
CInputFilter::CKeystrokeCondition::clone() const
{
	return new CKeystrokeCondition(m_key, m_mask);
}

CString
CInputFilter::CKeystrokeCondition::format() const
{
	return CStringUtil::print("keystroke(%s)",
							CKeyMap::formatKey(m_key, m_mask).c_str());
}

CInputFilter::EFilterStatus
CInputFilter::CKeystrokeCondition::match(const CEvent& event)
{
	EFilterStatus status;

	// check for hotkey events
	CEvent::Type type = event.getType();
	if (type == IPrimaryScreen::getHotKeyDownEvent()) {
		status = kActivate;
	}
	else if (type == IPrimaryScreen::getHotKeyUpEvent()) {
		status = kDeactivate;
	}
	else {
		return kNoMatch;
	}

	// check if it's our hotkey
	IPrimaryScreen::CHotKeyInfo* kinfo =
		reinterpret_cast<IPlatformScreen::CHotKeyInfo*>(event.getData());
	if (kinfo->m_id != m_id) {
		return kNoMatch;
	}

	return status;
}

void
CInputFilter::CKeystrokeCondition::enablePrimary(CPrimaryClient* primary)
{
	m_id = primary->registerHotKey(m_key, m_mask);
}

void
CInputFilter::CKeystrokeCondition::disablePrimary(CPrimaryClient* primary)
{
	primary->unregisterHotKey(m_id);
	m_id = 0;
}

CInputFilter::CMouseButtonCondition::CMouseButtonCondition(
		IPlatformScreen::CButtonInfo* info) :
	m_button(info->m_button),
	m_mask(info->m_mask)
{
	free(info);
}

CInputFilter::CMouseButtonCondition::CMouseButtonCondition(
		ButtonID button, KeyModifierMask mask) :
	m_button(button),
	m_mask(mask)
{
	// do nothing
}

CInputFilter::CMouseButtonCondition::~CMouseButtonCondition()
{
	// do nothing
}

ButtonID
CInputFilter::CMouseButtonCondition::getButton() const
{
	return m_button;
}

KeyModifierMask
CInputFilter::CMouseButtonCondition::getMask() const
{
	return m_mask;
}

CInputFilter::CCondition*
CInputFilter::CMouseButtonCondition::clone() const
{
	return new CMouseButtonCondition(m_button, m_mask);
}

CString
CInputFilter::CMouseButtonCondition::format() const
{
	CString key = CKeyMap::formatKey(kKeyNone, m_mask);
	if (!key.empty()) {
		key += "+";
	}
	return CStringUtil::print("mousebutton(%s%d)", key.c_str(), m_button);
}

CInputFilter::EFilterStatus		
CInputFilter::CMouseButtonCondition::match(const CEvent& event)
{
	static const KeyModifierMask s_ignoreMask =
		KeyModifierAltGr | KeyModifierCapsLock |
		KeyModifierNumLock | KeyModifierScrollLock;

	EFilterStatus status;

	// check for hotkey events
	CEvent::Type type = event.getType();
	if (type == IPrimaryScreen::getButtonDownEvent()) {
		status = kActivate;
	}
	else if (type == IPrimaryScreen::getButtonUpEvent()) {
		status = kDeactivate;
	}
	else {
		return kNoMatch;
	}

	// check if it's the right button and modifiers.  ignore modifiers
	// that cannot be combined with a mouse button.
	IPlatformScreen::CButtonInfo* minfo =
		reinterpret_cast<IPlatformScreen::CButtonInfo*>(event.getData());
	if (minfo->m_button != m_button ||
		(minfo->m_mask & ~s_ignoreMask) != m_mask) {
		return kNoMatch;
	}

	return status;
}

CInputFilter::CScreenConnectedCondition::CScreenConnectedCondition(
				const CString& screen) :
	m_screen(screen)
{
	// do nothing
}

CInputFilter::CScreenConnectedCondition::~CScreenConnectedCondition()
{
	// do nothing
}

CInputFilter::CCondition*
CInputFilter::CScreenConnectedCondition::clone() const
{
	return new CScreenConnectedCondition(m_screen);
}

CString
CInputFilter::CScreenConnectedCondition::format() const
{
	return CStringUtil::print("connect(%s)", m_screen.c_str());
}

CInputFilter::EFilterStatus
CInputFilter::CScreenConnectedCondition::match(const CEvent& event)
{
	if (event.getType() == CServer::getConnectedEvent()) {
		CServer::CScreenConnectedInfo* info = 
			reinterpret_cast<CServer::CScreenConnectedInfo*>(event.getData());
		if (m_screen == info->m_screen || m_screen.empty()) {
			return kActivate;
		}
	}

	return kNoMatch;
}

// -----------------------------------------------------------------------------
// Input Filter Action Classes
// -----------------------------------------------------------------------------
CInputFilter::CAction::CAction()
{
	// do nothing
}

CInputFilter::CAction::~CAction()
{
	// do nothing
}

CInputFilter::CLockCursorToScreenAction::CLockCursorToScreenAction(Mode mode) :
	m_mode(mode)
{
	// do nothing
}

CInputFilter::CLockCursorToScreenAction::Mode
CInputFilter::CLockCursorToScreenAction::getMode() const
{
	return m_mode;
}

CInputFilter::CAction*
CInputFilter::CLockCursorToScreenAction::clone() const
{
	return new CLockCursorToScreenAction(*this);
}

CString
CInputFilter::CLockCursorToScreenAction::format() const
{
	static const char* s_mode[] = { "off", "on", "toggle" };

	return CStringUtil::print("lockCursorToScreen(%s)", s_mode[m_mode]);
}

void
CInputFilter::CLockCursorToScreenAction::perform(const CEvent& event)
{
	static const CServer::CLockCursorToScreenInfo::State s_state[] = {
		CServer::CLockCursorToScreenInfo::kOff,
		CServer::CLockCursorToScreenInfo::kOn,
		CServer::CLockCursorToScreenInfo::kToggle
	};

	// send event
	CServer::CLockCursorToScreenInfo* info = 
		CServer::CLockCursorToScreenInfo::alloc(s_state[m_mode]);
	EVENTQUEUE->addEvent(CEvent(CServer::getLockCursorToScreenEvent(),
								event.getTarget(), info,
								CEvent::kDeliverImmediately));
}

CInputFilter::CSwitchToScreenAction::CSwitchToScreenAction(
				const CString& screen) :
	m_screen(screen)
{
	// do nothing
}

CString
CInputFilter::CSwitchToScreenAction::getScreen() const
{
	return m_screen;
}

CInputFilter::CAction*
CInputFilter::CSwitchToScreenAction::clone() const
{
	return new CSwitchToScreenAction(*this);
}

CString
CInputFilter::CSwitchToScreenAction::format() const
{
	return CStringUtil::print("switchToScreen(%s)", m_screen.c_str());
}

void
CInputFilter::CSwitchToScreenAction::perform(const CEvent& event)
{
	// pick screen name.  if m_screen is empty then use the screen from
	// event if it has one.
	CString screen = m_screen;
	if (screen.empty() && event.getType() == CServer::getConnectedEvent()) {
		CServer::CScreenConnectedInfo* info = 
			reinterpret_cast<CServer::CScreenConnectedInfo*>(event.getData());
		screen = info->m_screen;
	}

	// send event
	CServer::CSwitchToScreenInfo* info =
		CServer::CSwitchToScreenInfo::alloc(screen);
	EVENTQUEUE->addEvent(CEvent(CServer::getSwitchToScreenEvent(),
								event.getTarget(), info,
								CEvent::kDeliverImmediately));
}

CInputFilter::CSwitchInDirectionAction::CSwitchInDirectionAction(
				EDirection direction) :
	m_direction(direction)
{
	// do nothing
}

EDirection
CInputFilter::CSwitchInDirectionAction::getDirection() const
{
	return m_direction;
}

CInputFilter::CAction*
CInputFilter::CSwitchInDirectionAction::clone() const
{
	return new CSwitchInDirectionAction(*this);
}

CString
CInputFilter::CSwitchInDirectionAction::format() const
{
	static const char* s_names[] = {
		"",
		"left",
		"right",
		"up",
		"down"
	};

	return CStringUtil::print("switchInDirection(%s)", s_names[m_direction]);
}

void
CInputFilter::CSwitchInDirectionAction::perform(const CEvent& event)
{
	CServer::CSwitchInDirectionInfo* info =
		CServer::CSwitchInDirectionInfo::alloc(m_direction);
	EVENTQUEUE->addEvent(CEvent(CServer::getSwitchInDirectionEvent(),
								event.getTarget(), info,
								CEvent::kDeliverImmediately));
}

CInputFilter::CKeyboardBroadcastAction::CKeyboardBroadcastAction(Mode mode) :
	m_mode(mode)
{
	// do nothing
}

CInputFilter::CKeyboardBroadcastAction::CKeyboardBroadcastAction(
		Mode mode,
		const std::set<CString>& screens) :
	m_mode(mode),
	m_screens(IKeyState::CKeyInfo::join(screens))
{
	// do nothing
}

CInputFilter::CKeyboardBroadcastAction::Mode
CInputFilter::CKeyboardBroadcastAction::getMode() const
{
	return m_mode;
}

std::set<CString>
CInputFilter::CKeyboardBroadcastAction::getScreens() const
{
	std::set<CString> screens;
	IKeyState::CKeyInfo::split(m_screens.c_str(), screens);
	return screens;
}

CInputFilter::CAction*
CInputFilter::CKeyboardBroadcastAction::clone() const
{
	return new CKeyboardBroadcastAction(*this);
}

CString
CInputFilter::CKeyboardBroadcastAction::format() const
{
	static const char* s_mode[] = { "off", "on", "toggle" };
	static const char* s_name = "keyboardBroadcast";

	if (m_screens.empty() || m_screens[0] == '*') {
		return CStringUtil::print("%s(%s)", s_name, s_mode[m_mode]);
	}
	else {
		return CStringUtil::print("%s(%s,%.*s)", s_name, s_mode[m_mode],
							m_screens.size() - 2,
							m_screens.c_str() + 1);
	}
}

void
CInputFilter::CKeyboardBroadcastAction::perform(const CEvent& event)
{
	static const CServer::CKeyboardBroadcastInfo::State s_state[] = {
		CServer::CKeyboardBroadcastInfo::kOff,
		CServer::CKeyboardBroadcastInfo::kOn,
		CServer::CKeyboardBroadcastInfo::kToggle
	};

	// send event
	CServer::CKeyboardBroadcastInfo* info = 
		CServer::CKeyboardBroadcastInfo::alloc(s_state[m_mode], m_screens);
	EVENTQUEUE->addEvent(CEvent(CServer::getKeyboardBroadcastEvent(),
								event.getTarget(), info,
								CEvent::kDeliverImmediately));
}

CInputFilter::CKeystrokeAction::CKeystrokeAction(
		IPlatformScreen::CKeyInfo* info, bool press) :
	m_keyInfo(info),
	m_press(press)
{
	// do nothing
}

CInputFilter::CKeystrokeAction::~CKeystrokeAction()
{
	free(m_keyInfo);
}

void
CInputFilter::CKeystrokeAction::adoptInfo(IPlatformScreen::CKeyInfo* info)
{
	free(m_keyInfo);
	m_keyInfo = info;
}

const IPlatformScreen::CKeyInfo*
CInputFilter::CKeystrokeAction::getInfo() const
{
	return m_keyInfo;
}

bool
CInputFilter::CKeystrokeAction::isOnPress() const
{
	return m_press;
}

CInputFilter::CAction*
CInputFilter::CKeystrokeAction::clone() const
{
	IKeyState::CKeyInfo* info = IKeyState::CKeyInfo::alloc(*m_keyInfo);
	return new CKeystrokeAction(info, m_press);
}

CString
CInputFilter::CKeystrokeAction::format() const
{
	const char* type = formatName();

	if (m_keyInfo->m_screens[0] == '\0') {
		return CStringUtil::print("%s(%s)", type,
							CKeyMap::formatKey(m_keyInfo->m_key,
								m_keyInfo->m_mask).c_str());
	}
	else if (m_keyInfo->m_screens[0] == '*') {
		return CStringUtil::print("%s(%s,*)", type,
							CKeyMap::formatKey(m_keyInfo->m_key,
								m_keyInfo->m_mask).c_str());
	}
	else {
		return CStringUtil::print("%s(%s,%.*s)", type,
							CKeyMap::formatKey(m_keyInfo->m_key,
								m_keyInfo->m_mask).c_str(),
							strlen(m_keyInfo->m_screens + 1) - 1,
							m_keyInfo->m_screens + 1);
	}
}

void
CInputFilter::CKeystrokeAction::perform(const CEvent& event)
{
	CEvent::Type type = m_press ? IPlatformScreen::getKeyDownEvent() :
								IPlatformScreen::getKeyUpEvent();
	EVENTQUEUE->addEvent(CEvent(IPlatformScreen::getFakeInputBeginEvent(),
								event.getTarget(), NULL,
								CEvent::kDeliverImmediately));
	EVENTQUEUE->addEvent(CEvent(type, event.getTarget(), m_keyInfo,
								CEvent::kDeliverImmediately |
								CEvent::kDontFreeData));
	EVENTQUEUE->addEvent(CEvent(IPlatformScreen::getFakeInputEndEvent(),
								event.getTarget(), NULL,
								CEvent::kDeliverImmediately));
}

const char*
CInputFilter::CKeystrokeAction::formatName() const
{
	return (m_press ? "keyDown" : "keyUp");
}

CInputFilter::CMouseButtonAction::CMouseButtonAction(
		IPlatformScreen::CButtonInfo* info, bool press) : 
	m_buttonInfo(info),
	m_press(press)
{
	// do nothing
}

CInputFilter::CMouseButtonAction::~CMouseButtonAction()
{
	free(m_buttonInfo);
}

const IPlatformScreen::CButtonInfo*
CInputFilter::CMouseButtonAction::getInfo() const
{
	return m_buttonInfo;
}

bool
CInputFilter::CMouseButtonAction::isOnPress() const
{
	return m_press;
}

CInputFilter::CAction*
CInputFilter::CMouseButtonAction::clone() const
{
	IPlatformScreen::CButtonInfo* info =
		IPrimaryScreen::CButtonInfo::alloc(*m_buttonInfo);
	return new CMouseButtonAction(info, m_press);
}

CString
CInputFilter::CMouseButtonAction::format() const
{
	const char* type = formatName();

	CString key = CKeyMap::formatKey(kKeyNone, m_buttonInfo->m_mask);
	return CStringUtil::print("%s(%s%s%d)", type,
							key.c_str(), key.empty() ? "" : "+",
							m_buttonInfo->m_button);
}

void
CInputFilter::CMouseButtonAction::perform(const CEvent& event)

{
	// send modifiers
	IPlatformScreen::CKeyInfo* modifierInfo = NULL;
	if (m_buttonInfo->m_mask != 0) {
		KeyID key = m_press ? kKeySetModifiers : kKeyClearModifiers;
		modifierInfo =
			IKeyState::CKeyInfo::alloc(key, m_buttonInfo->m_mask, 0, 1);
		EVENTQUEUE->addEvent(CEvent(IPlatformScreen::getKeyDownEvent(),
								event.getTarget(), modifierInfo,
								CEvent::kDeliverImmediately));
	}

	// send button
	CEvent::Type type = m_press ? IPlatformScreen::getButtonDownEvent() :
								IPlatformScreen::getButtonUpEvent();
	EVENTQUEUE->addEvent(CEvent(type, event.getTarget(), m_buttonInfo,
								CEvent::kDeliverImmediately |
								CEvent::kDontFreeData));
}

const char*
CInputFilter::CMouseButtonAction::formatName() const
{
	return (m_press ? "mouseDown" : "mouseUp");
}

//
// CInputFilter::CRule
//

CInputFilter::CRule::CRule() :
	m_condition(NULL)
{
	// do nothing
}

CInputFilter::CRule::CRule(CCondition* adoptedCondition) :
	m_condition(adoptedCondition)
{
	// do nothing
}

CInputFilter::CRule::CRule(const CRule& rule) :
	m_condition(NULL)
{
	copy(rule);
}

CInputFilter::CRule::~CRule()
{
	clear();
}

CInputFilter::CRule&
CInputFilter::CRule::operator=(const CRule& rule)
{
	if (&rule != this) {
		copy(rule);
	}
	return *this;
}

void
CInputFilter::CRule::clear()
{
	delete m_condition;
	for (CActionList::iterator i = m_activateActions.begin();
								i != m_activateActions.end(); ++i) {
		delete *i;
	}
	for (CActionList::iterator i = m_deactivateActions.begin();
								i != m_deactivateActions.end(); ++i) {
		delete *i;
	}

	m_condition = NULL;
	m_activateActions.clear();
	m_deactivateActions.clear();
}

void
CInputFilter::CRule::copy(const CRule& rule)
{
	clear();
	if (rule.m_condition != NULL) {
		m_condition = rule.m_condition->clone();
	}
	for (CActionList::const_iterator i = rule.m_activateActions.begin();
								i != rule.m_activateActions.end(); ++i) {
		m_activateActions.push_back((*i)->clone());
	}
	for (CActionList::const_iterator i = rule.m_deactivateActions.begin();
								i != rule.m_deactivateActions.end(); ++i) {
		m_deactivateActions.push_back((*i)->clone());
	}
}

void
CInputFilter::CRule::setCondition(CCondition* adopted)
{
	delete m_condition;
	m_condition = adopted;
}

void
CInputFilter::CRule::adoptAction(CAction* action, bool onActivation)
{
	if (action != NULL) {
		if (onActivation) {
			m_activateActions.push_back(action);
		}
		else {
			m_deactivateActions.push_back(action);
		}
	}
}

void
CInputFilter::CRule::removeAction(bool onActivation, UInt32 index)
{
	if (onActivation) {
		delete m_activateActions[index];
		m_activateActions.erase(m_activateActions.begin() + index);
	}
	else {
		delete m_deactivateActions[index];
		m_deactivateActions.erase(m_deactivateActions.begin() + index);
	}
}

void
CInputFilter::CRule::replaceAction(CAction* adopted,
				bool onActivation, UInt32 index)
{
	if (adopted == NULL) {
		removeAction(onActivation, index);
	}
	else if (onActivation) {
		delete m_activateActions[index];
		m_activateActions[index] = adopted;
	}
	else {
		delete m_deactivateActions[index];
		m_deactivateActions[index] = adopted;
	}
}

void
CInputFilter::CRule::enable(CPrimaryClient* primaryClient)
{
	if (m_condition != NULL) {
		m_condition->enablePrimary(primaryClient);
	}
}

void
CInputFilter::CRule::disable(CPrimaryClient* primaryClient)
{
	if (m_condition != NULL) {
		m_condition->disablePrimary(primaryClient);
	}
}

bool
CInputFilter::CRule::handleEvent(const CEvent& event)
{
	// NULL condition never matches
	if (m_condition == NULL) {
		return false;
	}

	// match
	const CActionList* actions;
	switch (m_condition->match(event)) {
	default:
		// not handled
		return false;

	case kActivate:
		actions = &m_activateActions;
		LOG((CLOG_DEBUG1 "activate actions"));
		break;

	case kDeactivate:
		actions = &m_deactivateActions;
		LOG((CLOG_DEBUG1 "deactivate actions"));
		break;
	}

	// perform actions
	for (CActionList::const_iterator i = actions->begin();
								i != actions->end(); ++i) {
		LOG((CLOG_DEBUG1 "hotkey: %s", (*i)->format().c_str()));
		(*i)->perform(event);
	}

	return true;
}

CString
CInputFilter::CRule::format() const
{
	CString s;
	if (m_condition != NULL) {
		// condition
		s += m_condition->format();
		s += " = ";

		// activate actions
		CActionList::const_iterator i = m_activateActions.begin();
		if (i != m_activateActions.end()) {
			s += (*i)->format();
			while (++i != m_activateActions.end()) {
				s += ", ";
				s += (*i)->format();
			}
		}

		// deactivate actions
		if (!m_deactivateActions.empty()) {
			s += "; ";
			i = m_deactivateActions.begin();
			if (i != m_deactivateActions.end()) {
				s += (*i)->format();
				while (++i != m_deactivateActions.end()) {
					s += ", ";
					s += (*i)->format();
				}
			}
		}
	}
	return s;
}

const CInputFilter::CCondition*
CInputFilter::CRule::getCondition() const
{
	return m_condition;
}

UInt32
CInputFilter::CRule::getNumActions(bool onActivation) const
{
	if (onActivation) {
		return static_cast<UInt32>(m_activateActions.size());
	}
	else {
		return static_cast<UInt32>(m_deactivateActions.size());
	}
}

const CInputFilter::CAction&
CInputFilter::CRule::getAction(bool onActivation, UInt32 index) const
{
	if (onActivation) {
		return *m_activateActions[index];
	}
	else {
		return *m_deactivateActions[index];
	}
}


// -----------------------------------------------------------------------------
// Input Filter Class
// -----------------------------------------------------------------------------
CInputFilter::CInputFilter() :
	m_primaryClient(NULL)
{
	// do nothing
}

CInputFilter::CInputFilter(const CInputFilter& x) :
	m_ruleList(x.m_ruleList),
	m_primaryClient(NULL)
{
	setPrimaryClient(x.m_primaryClient);
}

CInputFilter::~CInputFilter()
{
	setPrimaryClient(NULL);
}

CInputFilter&
CInputFilter::operator=(const CInputFilter& x)
{
	if (&x != this) {
		CPrimaryClient* oldClient = m_primaryClient;
		setPrimaryClient(NULL);

		m_ruleList = x.m_ruleList;

		setPrimaryClient(oldClient);
	}
	return *this;
}

void
CInputFilter::addFilterRule(const CRule& rule)
{
	m_ruleList.push_back(rule);
	if (m_primaryClient != NULL) {
		m_ruleList.back().enable(m_primaryClient);
	}
}

void
CInputFilter::removeFilterRule(UInt32 index)
{
	if (m_primaryClient != NULL) {
		m_ruleList[index].disable(m_primaryClient);
	}
	m_ruleList.erase(m_ruleList.begin() + index);
}

CInputFilter::CRule&
CInputFilter::getRule(UInt32 index)
{
	return m_ruleList[index];
}

void
CInputFilter::setPrimaryClient(CPrimaryClient* client)
{
	if (m_primaryClient == client) {
		return;
	}

	if (m_primaryClient != NULL) {
		for (CRuleList::iterator rule  = m_ruleList.begin();
								 rule != m_ruleList.end(); ++rule) {
			rule->disable(m_primaryClient);
		}

		EVENTQUEUE->removeHandler(IPlatformScreen::getKeyDownEvent(),
							m_primaryClient->getEventTarget());
		EVENTQUEUE->removeHandler(IPlatformScreen::getKeyUpEvent(),
							m_primaryClient->getEventTarget());
		EVENTQUEUE->removeHandler(IPlatformScreen::getKeyRepeatEvent(),
							m_primaryClient->getEventTarget());
		EVENTQUEUE->removeHandler(IPlatformScreen::getButtonDownEvent(),
							m_primaryClient->getEventTarget());
		EVENTQUEUE->removeHandler(IPlatformScreen::getButtonUpEvent(),
							m_primaryClient->getEventTarget());
		EVENTQUEUE->removeHandler(IPlatformScreen::getHotKeyDownEvent(),
							m_primaryClient->getEventTarget());
		EVENTQUEUE->removeHandler(IPlatformScreen::getHotKeyUpEvent(),
							m_primaryClient->getEventTarget());
		EVENTQUEUE->removeHandler(CServer::getConnectedEvent(),
							m_primaryClient->getEventTarget());
	}

	m_primaryClient = client;

	if (m_primaryClient != NULL) {
		EVENTQUEUE->adoptHandler(IPlatformScreen::getKeyDownEvent(),
							m_primaryClient->getEventTarget(),
							new TMethodEventJob<CInputFilter>(this,
								&CInputFilter::handleEvent));
		EVENTQUEUE->adoptHandler(IPlatformScreen::getKeyUpEvent(),
							m_primaryClient->getEventTarget(),
							new TMethodEventJob<CInputFilter>(this,
								&CInputFilter::handleEvent));
		EVENTQUEUE->adoptHandler(IPlatformScreen::getKeyRepeatEvent(),
							m_primaryClient->getEventTarget(),
							new TMethodEventJob<CInputFilter>(this,
								&CInputFilter::handleEvent));
		EVENTQUEUE->adoptHandler(IPlatformScreen::getButtonDownEvent(),
							m_primaryClient->getEventTarget(),
							new TMethodEventJob<CInputFilter>(this,
								&CInputFilter::handleEvent));
		EVENTQUEUE->adoptHandler(IPlatformScreen::getButtonUpEvent(),
							m_primaryClient->getEventTarget(),
							new TMethodEventJob<CInputFilter>(this,
								&CInputFilter::handleEvent));
		EVENTQUEUE->adoptHandler(IPlatformScreen::getHotKeyDownEvent(),
							m_primaryClient->getEventTarget(),
							new TMethodEventJob<CInputFilter>(this,
								&CInputFilter::handleEvent));
		EVENTQUEUE->adoptHandler(IPlatformScreen::getHotKeyUpEvent(),
							m_primaryClient->getEventTarget(),
							new TMethodEventJob<CInputFilter>(this,
								&CInputFilter::handleEvent));
		EVENTQUEUE->adoptHandler(CServer::getConnectedEvent(),
							m_primaryClient->getEventTarget(),
							new TMethodEventJob<CInputFilter>(this,
								&CInputFilter::handleEvent));

		for (CRuleList::iterator rule  = m_ruleList.begin();
								 rule != m_ruleList.end(); ++rule) {
			rule->enable(m_primaryClient);
		}
	}
}

CString
CInputFilter::format(const CString& linePrefix) const
{
	CString s;
	for (CRuleList::const_iterator i = m_ruleList.begin();
								i != m_ruleList.end(); ++i) {
		s += linePrefix;
		s += i->format();
		s += "\n";
	}
	return s;
}

UInt32
CInputFilter::getNumRules() const
{
	return static_cast<UInt32>(m_ruleList.size());
}

bool
CInputFilter::operator==(const CInputFilter& x) const
{
	// if there are different numbers of rules then we can't be equal
	if (m_ruleList.size() != x.m_ruleList.size()) {
		return false;
	}

	// compare rule lists.  the easiest way to do that is to format each
	// rule into a string, sort the strings, then compare the results.
	std::vector<CString> aList, bList;
	for (CRuleList::const_iterator i = m_ruleList.begin();
								i != m_ruleList.end(); ++i) {
		aList.push_back(i->format());
	}
	for (CRuleList::const_iterator i = x.m_ruleList.begin();
								i != x.m_ruleList.end(); ++i) {
		bList.push_back(i->format());
	}
	std::partial_sort(aList.begin(), aList.end(), aList.end());
	std::partial_sort(bList.begin(), bList.end(), bList.end());
	return (aList == bList);
}

bool
CInputFilter::operator!=(const CInputFilter& x) const
{
	return !operator==(x);
}

void
CInputFilter::handleEvent(const CEvent& event, void*)
{
	// copy event and adjust target
	CEvent myEvent(event.getType(), this, event.getData(),
								event.getFlags() | CEvent::kDontFreeData |
								CEvent::kDeliverImmediately);

	// let each rule try to match the event until one does
	for (CRuleList::iterator rule  = m_ruleList.begin();
							 rule != m_ruleList.end(); ++rule) {
		if (rule->handleEvent(myEvent)) {
			// handled
			return;
		}
	}

	// not handled so pass through
	EVENTQUEUE->addEvent(myEvent);
}
