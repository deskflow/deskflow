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
#include "TMethodEventJob.h"
#include <stdlib.h>

// -----------------------------------------------------------------------------
// Input Filter Condition Classes
// -----------------------------------------------------------------------------
CInputFilter::CCondition::CCondition(EActionMode actionMode) :
	m_actionMode(actionMode),
	m_clearMask(0),
	m_inputFilter(NULL)
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

void
CInputFilter::CCondition::setInputFilter(CInputFilter* inputFilter)
{
	m_inputFilter = inputFilter;
}

KeyModifierMask
CInputFilter::CCondition::getClearMask() const
{
	return m_clearMask;
}

void
CInputFilter::CCondition::setClearMask(KeyModifierMask mask)
{
	m_clearMask = mask;
	m_inputFilter->setClearMaskDirty();
}

KeyModifierMask
CInputFilter::CCondition::getLastMask() const
{
	return m_inputFilter->getLastMask();
}

CInputFilter::EActionMode
CInputFilter::CCondition::getActionMode() const
{
	return m_actionMode;
}

CInputFilter::CKeystrokeCondition::CKeystrokeCondition(
		IPlatformScreen::CKeyInfo* info, EActionMode actionMode) :
	CCondition(actionMode),
	m_id(0),
	m_key(info->m_key),
	m_mask(info->m_mask)
{
	free(info);
}

CInputFilter::CKeystrokeCondition::CKeystrokeCondition(
		KeyID key, KeyModifierMask mask, EActionMode actionMode) :
	CCondition(actionMode),
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

CInputFilter::CCondition*
CInputFilter::CKeystrokeCondition::clone() const
{
	return new CKeystrokeCondition(*this);
}

CString
CInputFilter::CKeystrokeCondition::format() const
{
	return CStringUtil::print("keystroke(%s%s)",
							CKeyMap::formatKey(m_key, m_mask).c_str(),
							getActionMode() == kModeToggle ? ",toggle" : "");
}

CInputFilter::EFilterStatus
CInputFilter::CKeystrokeCondition::match(CEvent& event, void*,
				EActionMode& outMode)
{
	// check for hotkey events
	CEvent::Type type = event.getType();
	if (type == IPrimaryScreen::getHotKeyDownEvent()) {
		outMode = kModeTurnOn;
	}
	else if (type == IPrimaryScreen::getHotKeyUpEvent()) {
		outMode = kModeTurnOff;
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

	// convert event type for toggled conditions
	if (getActionMode() != kModePass) {
		if (type != IPlatformScreen::getHotKeyDownEvent()) {
			return kDiscard;
		}
		outMode = getActionMode();
	}
	return kMatch;
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
		IPlatformScreen::CButtonInfo* info, EActionMode actionMode) :
	CCondition(actionMode),
	m_button(info->m_button),
	m_mask(info->m_mask)
{
	free(info);
}

CInputFilter::CMouseButtonCondition::CMouseButtonCondition(
		ButtonID button, KeyModifierMask mask, EActionMode actionMode) :
	CCondition(actionMode),
	m_button(button),
	m_mask(mask)
{
	// do nothing
}

CInputFilter::CMouseButtonCondition::~CMouseButtonCondition()
{
	// do nothing
}

CInputFilter::CCondition*
CInputFilter::CMouseButtonCondition::clone() const
{
	return new CMouseButtonCondition(*this);
}

CString
CInputFilter::CMouseButtonCondition::format() const
{
	return CStringUtil::print("mousebutton(%s+%d%s)",
							CKeyMap::formatKey(kKeyNone, m_mask).c_str(),
							m_button,
							getActionMode() == kModeToggle ? ",toggle" : "");
}

CInputFilter::EFilterStatus		
CInputFilter::CMouseButtonCondition::match(CEvent& event, void*,
				EActionMode& outMode)
{
	// check for hotkey events
	bool down;
	CEvent::Type type = event.getType();
	if (type == IPrimaryScreen::getButtonDownEvent()) {
		outMode = kModeTurnOn;
		down    = true;
	}
	else if (type == IPrimaryScreen::getButtonUpEvent()) {
		outMode = kModeTurnOff;
		down    = false;
	}
	else {
		return kNoMatch;
	}

	// check if it's the right button and modifiers
	IPlatformScreen::CButtonInfo* minfo =
		reinterpret_cast<IPlatformScreen::CButtonInfo*>(event.getData());
	if (minfo->m_button != m_button || getLastMask() != m_mask) {
		return kNoMatch;
	}

	// convert event type for toggled conditions
	if (getActionMode() != kModePass) {
		if (type != IPlatformScreen::getButtonDownEvent()) {
			return kDiscard;
		}
		outMode = getActionMode();
	}

	setClearMask(down ? m_mask : 0);
	return kMatch;
}

// -----------------------------------------------------------------------------
// Input Filter Action Classes
// -----------------------------------------------------------------------------
CInputFilter::CAction::CAction(EActionState actionState) :
	m_state(actionState),
	m_modifierMask(0),
	m_inputFilter(NULL)
{
	// do nothing
}

CInputFilter::CAction::~CAction()
{
	// do nothing
}

void
CInputFilter::CAction::enablePrimary(CPrimaryClient*)
{
	// do nothing
}

void
CInputFilter::CAction::disablePrimary(CPrimaryClient*)
{
	// do nothing
}

void
CInputFilter::CAction::setInputFilter(CInputFilter* inputFilter)
{
	m_inputFilter = inputFilter;
}

KeyModifierMask
CInputFilter::CAction::getModifierMask() const
{
	return m_modifierMask;
}

void
CInputFilter::CAction::setModifierMask(KeyModifierMask mask)
{
	m_modifierMask = mask;
	m_inputFilter->setModifierMaskDirty();
}

CInputFilter::EActionState
CInputFilter::CAction::switchMode(EActionMode actionMode)
{
	EActionState newState;
	switch (actionMode) {
	case kModeTrigger:
		newState = m_state;
		break;

	case kModeToggle:
		if (m_state == kStateOff) {
			newState = kStateOn;
		}
		else {
			newState = kStateOff;
		}
		break;

	case kModeTurnOn:
		newState = kStateOn;
		break;

	case kModeTurnOff:
		newState = kStateOff;
		break;

	default:
		newState = kStateInvalid;
		break;
	}

	return newState;
}

void
CInputFilter::CAction::setState(EActionState state)
{
	m_state = state;
}

CInputFilter::CAction*
CInputFilter::CLockCursorToScreenAction::clone() const
{
	return new CLockCursorToScreenAction(*this);
}

CString
CInputFilter::CLockCursorToScreenAction::format() const
{
	return "lockCursorToScreen";
}

CInputFilter::EFilterStatus
CInputFilter::CLockCursorToScreenAction::perform(
				CEvent& event, void*, EActionMode actionMode)
{
	EActionState newState = switchMode(actionMode);

	if (newState != kStateOn && newState != kStateOff) {
		return kNotHandled;
	}

	setState(newState);

	// prepare event
	CServer::CLockCursorToScreenInfo* info = 
		CServer::CLockCursorToScreenInfo::alloc(newState != kStateOff);
	event = CEvent(CServer::getLockCursorToScreenEvent(),
								event.getTarget(), info,
								CEvent::kDeliverImmediately);

	return kHandled;
}

CInputFilter::CSwitchToScreenAction::CSwitchToScreenAction(
				const CString& screen) :
	m_screen(screen)
{
	// do nothing
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

CInputFilter::EFilterStatus
CInputFilter::CSwitchToScreenAction::perform(
				CEvent& event, void*, EActionMode actionMode)
{
	// only process "on" or "trigger" action modes
	if (actionMode != kModeTrigger && actionMode != kModeTurnOn) {
		return kNotHandled;
	}

	CServer::CSwitchToScreenInfo* info =
		CServer::CSwitchToScreenInfo::alloc(m_screen);
	event = CEvent(CServer::getSwitchToScreenEvent(),
								event.getTarget(), info,
								CEvent::kDeliverImmediately);

	return kHandled;
}

CInputFilter::CSwitchInDirectionAction::CSwitchInDirectionAction(
		EDirection direction) :
	m_direction(direction)
{
	// do nothing
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

CInputFilter::EFilterStatus
CInputFilter::CSwitchInDirectionAction::perform(
				CEvent& event, void*, EActionMode actionMode)
{
	// only process "on" or "trigger" action modes
	if (actionMode != kModeTrigger && actionMode != kModeTurnOn) {
		return kNotHandled;
	}

	CServer::CSwitchInDirectionInfo* info =
		CServer::CSwitchInDirectionInfo::alloc(m_direction);
	event = CEvent(CServer::getSwitchInDirectionEvent(),
								event.getTarget(), info,
								CEvent::kDeliverImmediately);

	return kHandled;
}

CInputFilter::CKeystrokeAction::CKeystrokeAction(
		IPlatformScreen::CKeyInfo* info) :
	m_keyInfo(info)
{
	// do nothing
}

CInputFilter::CKeystrokeAction::~CKeystrokeAction()
{
	free(m_keyInfo);
}

CInputFilter::CAction*
CInputFilter::CKeystrokeAction::clone() const
{
	IKeyState::CKeyInfo* info = IKeyState::CKeyInfo::alloc(*m_keyInfo);
	return new CKeystrokeAction(info);
}

CString
CInputFilter::CKeystrokeAction::format() const
{
	return CStringUtil::print("keystroke(%s)",
							CKeyMap::formatKey(m_keyInfo->m_key,
								m_keyInfo->m_mask).c_str());
}

CInputFilter::EFilterStatus
CInputFilter::CKeystrokeAction::perform(
				CEvent& event, void*, EActionMode actionMode)
{
	EActionState newState = switchMode(actionMode);
	CEvent::Type type;

	if (newState == kStateOn) {
		type = IPlatformScreen::getKeyDownEvent();
	}
	else if (newState == kStateOff) {
		type = IPlatformScreen::getKeyUpEvent();
	}
	else {
		return kNotHandled;
	}

	setState(newState);

	event = CEvent(type, event.getTarget(), m_keyInfo,
								CEvent::kDeliverImmediately |
								CEvent::kDontFreeData);

	return kHandled;
}

CInputFilter::CModifierAction::CModifierAction(
				KeyModifierMask modifiers, KeyModifierMask bitmask) :
	m_modifiers(modifiers),
	m_bitmask(bitmask)
{
	// do nothing
}

CInputFilter::CAction*
CInputFilter::CModifierAction::clone() const
{
	return new CModifierAction(m_modifiers, m_bitmask);
}

CString
CInputFilter::CModifierAction::format() const
{
	return CStringUtil::print("modifier(%s)",
							CKeyMap::formatKey(kKeyNone, m_modifiers).c_str());
}

CInputFilter::EFilterStatus		
CInputFilter::CModifierAction::perform(
				CEvent&, void*, EActionMode actionMode)
{
	EActionState newState = switchMode(actionMode);

	if (newState == kStateOn) {
		setModifierMask(m_modifiers);
	}
	else if (newState == kStateOff) {
		setModifierMask(0);
	}
	else {
		return kNotHandled;
	}

	setState(newState);

	// FIXME. set proper key event, so updateModifiers will get called from
	// within sendEvent automatically

	return kUpdateModifiers;
}

CInputFilter::CMouseButtonAction::CMouseButtonAction(
		IPlatformScreen::CButtonInfo* info) : 
	m_buttonInfo(info)
{
	// do nothing
}

CInputFilter::CMouseButtonAction::~CMouseButtonAction()
{
	free(m_buttonInfo);
}

CInputFilter::CAction*
CInputFilter::CMouseButtonAction::clone() const
{
	IPlatformScreen::CButtonInfo* info =
		IPrimaryScreen::CButtonInfo::alloc(*m_buttonInfo);
	return new CMouseButtonAction(info);
}

CString
CInputFilter::CMouseButtonAction::format() const
{
	return CStringUtil::print("mousebutton(%s+%d)",
							CKeyMap::formatKey(kKeyNone,
								m_buttonInfo->m_mask).c_str(),
							m_buttonInfo->m_button);
}

CInputFilter::EFilterStatus
CInputFilter::CMouseButtonAction::perform(
				CEvent& event, void*, EActionMode actionMode)

{
	EActionState newState = switchMode(actionMode);
	CEvent::Type type;

	if (newState == kStateOn) {
		type = IPlatformScreen::getButtonDownEvent();
	}
	else if (newState == kStateOff) {
		type = IPlatformScreen::getButtonUpEvent();
	}
	else {
		return kNotHandled;
	}

	setState(newState);

	event = CEvent(type, event.getTarget(), m_buttonInfo,
								CEvent::kDeliverImmediately |
								CEvent::kDontFreeData);

	return kHandled;
}

// -----------------------------------------------------------------------------
// Input Filter Class
// -----------------------------------------------------------------------------
CInputFilter::CInputFilter() :
	m_primaryClient(NULL),
	m_lastMask(0),
	m_dirtyFlag(kNotDirty),
	m_clearMask(0),
	m_modifierMask(0)
{
	// do nothing
}

CInputFilter::CInputFilter(const CInputFilter& x) :
	m_primaryClient(NULL)
{
	copyRules(x.m_ruleList);
	m_lastMask     = x.m_lastMask;
	m_dirtyFlag    = x.m_dirtyFlag;
	m_clearMask    = x.m_clearMask;
	m_modifierMask = x.m_modifierMask;
	setPrimaryClient(x.m_primaryClient);
}

CInputFilter::~CInputFilter()
{
	setPrimaryClient(NULL);
	deleteRules(m_ruleList);
}

CInputFilter&
CInputFilter::operator=(const CInputFilter& x)
{
	if (&x != this) {
		setPrimaryClient(NULL);

		copyRules(x.m_ruleList);
		m_lastMask     = x.m_lastMask;
		m_dirtyFlag    = x.m_dirtyFlag;
		m_clearMask    = x.m_clearMask;
		m_modifierMask = x.m_modifierMask;

		setPrimaryClient(x.m_primaryClient);
	}
	return *this;
}

void
CInputFilter::copyRules(const CRuleList& rules)
{
	CRuleList newRules;
	for (CRuleList::const_iterator i = rules.begin(); i != rules.end(); ++i) {
		CCondition* cond = i->first->clone();
		CAction* action  = i->second->clone();
		newRules.push_back(std::make_pair(cond, action));
	}
	m_ruleList.swap(newRules);
	deleteRules(newRules);
}

void
CInputFilter::deleteRules(CRuleList& rules) const
{
	for (CRuleList::iterator i = rules.begin(); i != rules.end(); ++i) {
		delete i->first;
		delete i->second;
	}
	rules.clear();
}

void
CInputFilter::addFilterRule(CCondition* cond, CAction* action)
{
	m_ruleList.push_back(std::make_pair(cond, action));
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
			rule->first->disablePrimary(m_primaryClient);
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

		for (CRuleList::iterator rule  = m_ruleList.begin();
								 rule != m_ruleList.end(); ++rule) {
			rule->first->enablePrimary(m_primaryClient);

			// workaround for "this" problem of addFilterRule
			rule->first->setInputFilter(this);
			rule->second->setInputFilter(this);
		}
	}
}

KeyModifierMask
CInputFilter::getLastMask() const
{
	return m_lastMask;
}

void
CInputFilter::setClearMaskDirty()
{
	m_dirtyFlag |= kClearDirty;
}

void
CInputFilter::setModifierMaskDirty()
{
	m_dirtyFlag |= kModifiersDirty;
}

const CInputFilter::CRuleList&
CInputFilter::getRules() const
{
	return m_ruleList;
}

void
CInputFilter::handleEvent(const CEvent& event, void* arg)
{
	// get a modifiable copy of this event.
	// set target to us, set kDontFreeData and kDeliverImmediately because the
	// original event will be destroyed after this method exits.
	CEvent evt(event.getType(), this, event.getData(),
								event.getFlags() | CEvent::kDontFreeData |
								CEvent::kDeliverImmediately);

	// clear dirty flag
	m_dirtyFlag = kNotDirty;

	EActionMode	actionMode = kModePass;
	// match event against filter rules and perform actions
	for (CRuleList::iterator rule  = m_ruleList.begin();
							 rule != m_ruleList.end(); ++rule) {
		EFilterStatus conditionStatus;
		EFilterStatus actionStatus;
		conditionStatus = rule->first->match(evt, arg, actionMode);
		if (conditionStatus == kDiscard) {
			return;
		}
		else if (conditionStatus == kNoMatch) {
			continue;
		}

		actionStatus = rule->second->perform(evt, arg, actionMode);
		if (actionStatus == kDiscard) {
			// discard event
			return;
		}
		else if (actionStatus == kNotHandled) {
			continue;
		}
		else if (actionStatus == kUpdateModifiers) {
			updateModifiers();
			return;
		}

		// if we got here then the rule has matched and action returned
		// kHandled, so send the event.
		break;
	}

	sendEvent(evt);
}

void
CInputFilter::sendEvent(CEvent& event)
{
	CEvent::Type type = event.getType();
	// process keyboard modifiers here
	if (type == IPlatformScreen::getKeyDownEvent() ||
		type == IPlatformScreen::getKeyUpEvent() || 
		type == IPlatformScreen::getKeyRepeatEvent()) {
		// get CKeyInfo from event
		IPlatformScreen::CKeyInfo* kinfo =
			reinterpret_cast<IPlatformScreen::CKeyInfo*>(event.getData());

		// save mask
		m_lastMask = kinfo->m_mask;

		// prepare new mask
		KeyModifierMask newMask = kinfo->m_mask;
		updateModifiers();
		newMask &= ~m_clearMask;
		newMask |= m_modifierMask;

		// set new mask
		kinfo->m_mask = newMask;
	}

	// add event to eventqueue
	EVENTQUEUE->addEvent(event);
}

void
CInputFilter::updateModifiers()
{
	// FIXME: clearMasks of all conditions and modifier masks of all actions
	// are aggregated here.  a proper implementation would generate required
	// key up and down events here.
	if ((m_dirtyFlag & kClearDirty) != 0) {
		m_clearMask = 0;
		for (CRuleList::iterator rule  = m_ruleList.begin();
								 rule != m_ruleList.end(); ++rule) {
			m_clearMask |= rule->first->getClearMask();
		}
	}
	if ((m_dirtyFlag & kModifiersDirty) != 0) {
		m_modifierMask = 0;
		for (CRuleList::iterator rule  = m_ruleList.begin();
								 rule != m_ruleList.end(); ++rule) {
			m_modifierMask |= rule->second->getModifierMask();
		}
	}
}
