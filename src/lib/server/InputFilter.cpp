/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2025 Deskflow Developers
 * SPDX-FileCopyrightText: (C) 2012 - 2016 Symless Ltd.
 * SPDX-FileCopyrightText: (C) 2005 Chris Schoeneman
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#include "server/InputFilter.h"
#include "base/EventQueue.h"
#include "base/Log.h"
#include "deskflow/KeyMap.h"
#include "server/PrimaryClient.h"
#include "server/Server.h"

#include <cstdint>
#include <cstdlib>
#include <cstring>

// -----------------------------------------------------------------------------
// Input Filter Condition Classes
// -----------------------------------------------------------------------------

void InputFilter::Condition::enablePrimary(PrimaryClient *)
{
  // do nothing
}

void InputFilter::Condition::disablePrimary(PrimaryClient *)
{
  // do nothing
}

InputFilter::KeystrokeCondition::KeystrokeCondition(IEventQueue *events, IPlatformScreen::KeyInfo *info)
    : m_key(info->m_key),
      m_mask(info->m_mask),
      m_events(events)
{
  free(info);
}

InputFilter::KeystrokeCondition::KeystrokeCondition(IEventQueue *events, KeyID key, KeyModifierMask mask)
    : m_key(key),
      m_mask(mask),
      m_events(events)
{
  // do nothing
}

KeyID InputFilter::KeystrokeCondition::getKey() const
{
  return m_key;
}

KeyModifierMask InputFilter::KeystrokeCondition::getMask() const
{
  return m_mask;
}

InputFilter::Condition *InputFilter::KeystrokeCondition::clone() const
{
  return new KeystrokeCondition(m_events, m_key, m_mask);
}

std::string InputFilter::KeystrokeCondition::format() const
{
  return deskflow::string::sprintf("keystroke(%s)", deskflow::KeyMap::formatKey(m_key, m_mask).c_str());
}

InputFilter::FilterStatus InputFilter::KeystrokeCondition::match(const Event &event)
{
  using enum FilterStatus;
  FilterStatus status;

  // check for hotkey events
  if (EventTypes type = event.getType(); type == EventTypes::PrimaryScreenHotkeyDown) {
    status = Activate;
  } else if (type == EventTypes::PrimaryScreenHotkeyUp) {
    status = Deactivate;
  } else {
    return NoMatch;
  }

  // check if it's our hotkey
  if (const auto *kinfo = static_cast<IPlatformScreen::HotKeyInfo *>(event.getData()); kinfo->m_id != m_id) {
    return NoMatch;
  }

  return status;
}

void InputFilter::KeystrokeCondition::enablePrimary(PrimaryClient *primary)
{
  m_id = primary->registerHotKey(m_key, m_mask);
}

void InputFilter::KeystrokeCondition::disablePrimary(PrimaryClient *primary)
{
  primary->unregisterHotKey(m_id);
  m_id = 0;
}

InputFilter::MouseButtonCondition::MouseButtonCondition(IEventQueue *events, const IPlatformScreen::ButtonInfo &info)
    : m_button(info.m_button),
      m_mask(info.m_mask),
      m_events(events)
{
  // do nothing
}

InputFilter::MouseButtonCondition::MouseButtonCondition(IEventQueue *events, ButtonID button, KeyModifierMask mask)
    : m_button(button),
      m_mask(mask),
      m_events(events)
{
  // do nothing
}

ButtonID InputFilter::MouseButtonCondition::getButton() const
{
  return m_button;
}

KeyModifierMask InputFilter::MouseButtonCondition::getMask() const
{
  return m_mask;
}

InputFilter::Condition *InputFilter::MouseButtonCondition::clone() const
{
  return new MouseButtonCondition(m_events, m_button, m_mask);
}

std::string InputFilter::MouseButtonCondition::format() const
{
  std::string key = deskflow::KeyMap::formatKey(kKeyNone, m_mask);
  if (!key.empty()) {
    key += "+";
  }
  return deskflow::string::sprintf("mousebutton(%s%d)", key.c_str(), m_button);
}

InputFilter::FilterStatus InputFilter::MouseButtonCondition::match(const Event &event)
{
  static const KeyModifierMask s_ignoreMask =
      KeyModifierAltGr | KeyModifierCapsLock | KeyModifierNumLock | KeyModifierScrollLock;

  FilterStatus status;

  using enum FilterStatus;
  // check for hotkey events
  if (EventTypes type = event.getType(); type == EventTypes::PrimaryScreenButtonDown) {
    status = Activate;
  } else if (type == EventTypes::PrimaryScreenButtonUp) {
    status = Deactivate;
  } else {
    return NoMatch;
  }

  // check if it's the right button and modifiers.  ignore modifiers
  // that cannot be combined with a mouse button.
  if (const auto *minfo = static_cast<IPlatformScreen::ButtonInfo *>(event.getData());
      minfo->m_button != m_button || (minfo->m_mask & ~s_ignoreMask) != m_mask) {
    return NoMatch;
  }

  return status;
}

InputFilter::ScreenConnectedCondition::ScreenConnectedCondition(IEventQueue *events, const std::string &screen)
    : m_screen(screen),
      m_events(events)
{
  // do nothing
}

InputFilter::Condition *InputFilter::ScreenConnectedCondition::clone() const
{
  return new ScreenConnectedCondition(m_events, m_screen);
}

std::string InputFilter::ScreenConnectedCondition::format() const
{
  return deskflow::string::sprintf("connect(%s)", m_screen.c_str());
}

InputFilter::FilterStatus InputFilter::ScreenConnectedCondition::match(const Event &event)
{
  if (event.getType() == EventTypes::ServerConnected) {
    const auto *info = static_cast<Server::ScreenConnectedInfo *>(event.getData());
    if (m_screen == info->m_screen || m_screen.empty()) {
      return FilterStatus::Activate;
    }
  }

  return FilterStatus::NoMatch;
}

// -----------------------------------------------------------------------------
// Input Filter Action Classes
// -----------------------------------------------------------------------------

InputFilter::LockCursorToScreenAction::LockCursorToScreenAction(IEventQueue *events, Mode mode)
    : m_mode(mode),
      m_events(events)
{
  // do nothing
}

InputFilter::LockCursorToScreenAction::Mode InputFilter::LockCursorToScreenAction::getMode() const
{
  return m_mode;
}

InputFilter::Action *InputFilter::LockCursorToScreenAction::clone() const
{
  return new LockCursorToScreenAction(*this);
}

std::string InputFilter::LockCursorToScreenAction::format() const
{
  static const char *s_mode[] = {"off", "on", "toggle"};

  return deskflow::string::sprintf("lockCursorToScreen(%s)", s_mode[m_mode]);
}

void InputFilter::LockCursorToScreenAction::perform(const Event &event)
{
  static const Server::LockCursorToScreenInfo::State s_state[] = {
      Server::LockCursorToScreenInfo::kOff, Server::LockCursorToScreenInfo::kOn, Server::LockCursorToScreenInfo::kToggle
  };

  // send event
  Server::LockCursorToScreenInfo *info = Server::LockCursorToScreenInfo::alloc(s_state[m_mode]);
  m_events->addEvent(
      Event(EventTypes::ServerLockCursorToScreen, event.getTarget(), info, Event::EventFlags::DeliverImmediately)
  );
}

InputFilter::RestartServer::RestartServer(Mode mode) : m_mode(mode)
{
  // do nothing
}

InputFilter::RestartServer::Mode InputFilter::RestartServer::getMode() const
{
  return m_mode;
}

InputFilter::Action *InputFilter::RestartServer::clone() const
{
  return new RestartServer(*this);
}

std::string InputFilter::RestartServer::format() const
{
  static const char *s_mode[] = {"restart"};

  return deskflow::string::sprintf("restartServer(%s)", s_mode[m_mode]);
}

void InputFilter::RestartServer::perform(const Event &)
{
  // HACK Super hack we should gracefully exit
  exit(0);
}

InputFilter::SwitchToScreenAction::SwitchToScreenAction(IEventQueue *events, const std::string &screen)
    : m_screen(screen),
      m_events(events)
{
  // do nothing
}

std::string InputFilter::SwitchToScreenAction::getScreen() const
{
  return m_screen;
}

InputFilter::Action *InputFilter::SwitchToScreenAction::clone() const
{
  return new SwitchToScreenAction(*this);
}

std::string InputFilter::SwitchToScreenAction::format() const
{
  return deskflow::string::sprintf("switchToScreen(%s)", m_screen.c_str());
}

void InputFilter::SwitchToScreenAction::perform(const Event &event)
{
  // pick screen name.  if m_screen is empty then use the screen from
  // event if it has one.
  std::string screen = m_screen;
  if (screen.empty() && event.getType() == EventTypes::ServerConnected) {
    const auto *info = static_cast<Server::ScreenConnectedInfo *>(event.getData());
    screen = info->m_screen;
  }

  // send event
  Server::SwitchToScreenInfo *info = Server::SwitchToScreenInfo::alloc(screen);
  m_events->addEvent(
      Event(EventTypes::ServerSwitchToScreen, event.getTarget(), info, Event::EventFlags::DeliverImmediately)
  );
}

InputFilter::SwitchInDirectionAction::SwitchInDirectionAction(IEventQueue *events, Direction direction)
    : m_direction(direction),
      m_events(events)
{
  // do nothing
}

Direction InputFilter::SwitchInDirectionAction::getDirection() const
{
  return m_direction;
}

InputFilter::Action *InputFilter::SwitchInDirectionAction::clone() const
{
  return new SwitchInDirectionAction(*this);
}

std::string InputFilter::SwitchInDirectionAction::format() const
{
  static const char *s_names[] = {"", "left", "right", "up", "down"};

  return deskflow::string::sprintf("switchInDirection(%s)", s_names[static_cast<int>(m_direction)]);
}

void InputFilter::SwitchInDirectionAction::perform(const Event &event)
{
  Server::SwitchInDirectionInfo *info = Server::SwitchInDirectionInfo::alloc(m_direction);
  m_events->addEvent(
      Event(EventTypes::ServerSwitchInDirection, event.getTarget(), info, Event::EventFlags::DeliverImmediately)
  );
}

InputFilter::SwitchToNextScreenAction::SwitchToNextScreenAction(IEventQueue *events) : m_events(events)
{
  // do nothing
}

InputFilter::Action *InputFilter::SwitchToNextScreenAction::clone() const
{
  return new SwitchToNextScreenAction(*this);
}

std::string InputFilter::SwitchToNextScreenAction::format() const
{
  return "switchToNextScreen()";
}

void InputFilter::SwitchToNextScreenAction::perform(const Event &event)
{
  m_events->addEvent(
      Event(EventTypes::ServerToggleScreen, event.getTarget(), nullptr, Event::EventFlags::DeliverImmediately)
  );
}

InputFilter::KeyboardBroadcastAction::KeyboardBroadcastAction(IEventQueue *events, Mode mode)
    : m_mode(mode),
      m_events(events)
{
  // do nothing
}

InputFilter::KeyboardBroadcastAction::KeyboardBroadcastAction(
    IEventQueue *events, Mode mode, const std::set<std::string> &screens
)
    : m_mode(mode),
      m_screens(IKeyState::KeyInfo::join(screens)),
      m_events(events)
{
  // do nothing
}

InputFilter::KeyboardBroadcastAction::Mode InputFilter::KeyboardBroadcastAction::getMode() const
{
  return m_mode;
}

std::set<std::string> InputFilter::KeyboardBroadcastAction::getScreens() const
{
  std::set<std::string> screens;
  IKeyState::KeyInfo::split(m_screens.c_str(), screens);
  return screens;
}

InputFilter::Action *InputFilter::KeyboardBroadcastAction::clone() const
{
  return new KeyboardBroadcastAction(*this);
}

std::string InputFilter::KeyboardBroadcastAction::format() const
{
  static const char *s_mode[] = {"off", "on", "toggle"};
  static const char *s_name = "keyboardBroadcast";

  if (m_screens.empty() || m_screens[0] == '*') {
    return deskflow::string::sprintf("%s(%s)", s_name, s_mode[m_mode]);
  } else {
    return deskflow::string::sprintf(
        "%s(%s,%.*s)", s_name, s_mode[m_mode], static_cast<int>(m_screens.size() >= 2 ? m_screens.size() - 2 : 0),
        m_screens.c_str() + 1
    );
  }
}

void InputFilter::KeyboardBroadcastAction::perform(const Event &event)
{
  static const Server::KeyboardBroadcastInfo::State s_state[] = {
      Server::KeyboardBroadcastInfo::kOff, Server::KeyboardBroadcastInfo::kOn, Server::KeyboardBroadcastInfo::kToggle
  };

  // send event
  Server::KeyboardBroadcastInfo *info = Server::KeyboardBroadcastInfo::alloc(s_state[m_mode], m_screens);
  m_events->addEvent(
      Event(EventTypes::ServerKeyboardBroadcast, event.getTarget(), info, Event::EventFlags::DeliverImmediately)
  );
}

InputFilter::KeystrokeAction::KeystrokeAction(IEventQueue *events, IPlatformScreen::KeyInfo *info, bool press)
    : m_keyInfo(info),
      m_press(press),
      m_events(events)
{
  // do nothing
}

InputFilter::KeystrokeAction::~KeystrokeAction()
{
  free(m_keyInfo);
}

void InputFilter::KeystrokeAction::adoptInfo(IPlatformScreen::KeyInfo *info)
{
  free(m_keyInfo);
  m_keyInfo = info;
}

const IPlatformScreen::KeyInfo *InputFilter::KeystrokeAction::getInfo() const
{
  return m_keyInfo;
}

bool InputFilter::KeystrokeAction::isOnPress() const
{
  return m_press;
}

InputFilter::Action *InputFilter::KeystrokeAction::clone() const
{
  IKeyState::KeyInfo *info = IKeyState::KeyInfo::alloc(*m_keyInfo);
  return new KeystrokeAction(m_events, info, m_press);
}

std::string InputFilter::KeystrokeAction::format() const
{
  const char *type = formatName();

  if (m_keyInfo->m_screens[0] == '\0') {
    return deskflow::string::sprintf(
        "%s(%s)", type, deskflow::KeyMap::formatKey(m_keyInfo->m_key, m_keyInfo->m_mask).c_str()
    );
  } else if (m_keyInfo->m_screens[0] == '*') {
    return deskflow::string::sprintf(
        "%s(%s,*)", type, deskflow::KeyMap::formatKey(m_keyInfo->m_key, m_keyInfo->m_mask).c_str()
    );
  } else {
    return deskflow::string::sprintf(
        "%s(%s,%.*s)", type, deskflow::KeyMap::formatKey(m_keyInfo->m_key, m_keyInfo->m_mask).c_str(),
        strnlen(m_keyInfo->m_screens + 1, SIZE_MAX) - 1, m_keyInfo->m_screens + 1
    );
  }
}

void InputFilter::KeystrokeAction::perform(const Event &event)
{
  using enum EventTypes;
  using Flags = Event::EventFlags;

  EventTypes type = m_press ? KeyStateKeyDown : KeyStateKeyUp;

  m_events->addEvent(Event(PrimaryScreenFakeInputBegin, event.getTarget(), nullptr, Flags::DeliverImmediately));
  m_events->addEvent(Event(type, event.getTarget(), m_keyInfo, Flags::DeliverImmediately | Flags::DontFreeData));
  m_events->addEvent(Event(PrimaryScreenFakeInputEnd, event.getTarget(), nullptr, Flags::DeliverImmediately));
}

const char *InputFilter::KeystrokeAction::formatName() const
{
  return (m_press ? "keyDown" : "keyUp");
}

InputFilter::MouseButtonAction::MouseButtonAction(
    IEventQueue *events, const IPlatformScreen::ButtonInfo &info, bool press
)
    : m_buttonInfo(info),
      m_press(press),
      m_events(events)
{
  // do nothing
}

const IPlatformScreen::ButtonInfo &InputFilter::MouseButtonAction::getInfo() const
{
  return m_buttonInfo;
}

bool InputFilter::MouseButtonAction::isOnPress() const
{
  return m_press;
}

InputFilter::Action *InputFilter::MouseButtonAction::clone() const
{
  return new MouseButtonAction(m_events, m_buttonInfo, m_press);
}

std::string InputFilter::MouseButtonAction::format() const
{
  const char *type = formatName();

  std::string key = deskflow::KeyMap::formatKey(kKeyNone, m_buttonInfo.m_mask);
  return deskflow::string::sprintf("%s(%s%s%d)", type, key.c_str(), key.empty() ? "" : "+", m_buttonInfo.m_button);
}

void InputFilter::MouseButtonAction::perform(const Event &event)

{
  // send modifiers
  using enum EventTypes;
  IPlatformScreen::KeyInfo *modifierInfo = nullptr;
  if (m_buttonInfo.m_mask != 0) {
    KeyID key = m_press ? kKeySetModifiers : kKeyClearModifiers;
    modifierInfo = IKeyState::KeyInfo::alloc(key, m_buttonInfo.m_mask, 0, 1);
    m_events->addEvent(Event(KeyStateKeyDown, event.getTarget(), modifierInfo, Event::EventFlags::DeliverImmediately));
  }

  // send button
  EventTypes type = m_press ? PrimaryScreenButtonDown : PrimaryScreenButtonUp;
  m_events->addEvent(Event(
      type, event.getTarget(), &m_buttonInfo, Event::EventFlags::DeliverImmediately | Event::EventFlags::DontFreeData
  ));
}

const char *InputFilter::MouseButtonAction::formatName() const
{
  return (m_press ? "mouseDown" : "mouseUp");
}

//
// InputFilter::Rule
//

InputFilter::Rule::Rule(Condition *adoptedCondition) : m_condition(adoptedCondition)
{
  // do nothing
}

InputFilter::Rule::Rule(const Rule &rule)
{
  copy(rule);
}

InputFilter::Rule::~Rule()
{
  clear();
}

InputFilter::Rule &InputFilter::Rule::operator=(const Rule &rule)
{
  if (&rule != this) {
    copy(rule);
  }
  return *this;
}

void InputFilter::Rule::clear()
{
  delete m_condition;
  for (auto i = m_activateActions.begin(); i != m_activateActions.end(); ++i) {
    delete *i;
  }
  for (auto i = m_deactivateActions.begin(); i != m_deactivateActions.end(); ++i) {
    delete *i;
  }

  m_condition = nullptr;
  m_activateActions.clear();
  m_deactivateActions.clear();
}

void InputFilter::Rule::copy(const Rule &rule)
{
  clear();
  if (rule.m_condition != nullptr) {
    m_condition = rule.m_condition->clone();
  }
  for (auto i = rule.m_activateActions.begin(); i != rule.m_activateActions.end(); ++i) {
    m_activateActions.push_back((*i)->clone());
  }
  for (auto i = rule.m_deactivateActions.begin(); i != rule.m_deactivateActions.end(); ++i) {
    m_deactivateActions.push_back((*i)->clone());
  }
}

void InputFilter::Rule::setCondition(Condition *adopted)
{
  delete m_condition;
  m_condition = adopted;
}

void InputFilter::Rule::adoptAction(Action *action, bool onActivation)
{
  if (action != nullptr) {
    if (onActivation) {
      m_activateActions.push_back(action);
    } else {
      m_deactivateActions.push_back(action);
    }
  }
}

void InputFilter::Rule::removeAction(bool onActivation, uint32_t index)
{
  if (onActivation) {
    delete m_activateActions[index];
    m_activateActions.erase(m_activateActions.begin() + index);
  } else {
    delete m_deactivateActions[index];
    m_deactivateActions.erase(m_deactivateActions.begin() + index);
  }
}

void InputFilter::Rule::replaceAction(Action *adopted, bool onActivation, uint32_t index)
{
  if (adopted == nullptr) {
    removeAction(onActivation, index);
  } else if (onActivation) {
    delete m_activateActions[index];
    m_activateActions[index] = adopted;
  } else {
    delete m_deactivateActions[index];
    m_deactivateActions[index] = adopted;
  }
}

void InputFilter::Rule::enable(PrimaryClient *primaryClient)
{
  if (m_condition != nullptr) {
    m_condition->enablePrimary(primaryClient);
  }
}

void InputFilter::Rule::disable(PrimaryClient *primaryClient)
{
  if (m_condition != nullptr) {
    m_condition->disablePrimary(primaryClient);
  }
}

bool InputFilter::Rule::handleEvent(const Event &event)
{
  // nullptr condition never matches
  if (m_condition == nullptr) {
    return false;
  }

  // match
  const ActionList *actions;
  switch (m_condition->match(event)) {
    using enum FilterStatus;
  default:
    // not handled
    return false;

  case Activate:
    actions = &m_activateActions;
    LOG_DEBUG1("activate actions");
    break;

  case Deactivate:
    actions = &m_deactivateActions;
    LOG_DEBUG1("deactivate actions");
    break;
  }

  // perform actions
  for (auto action : *actions) {
    LOG_DEBUG1("hotkey: %s", action->format().c_str());
    action->perform(event);
  }

  return true;
}

std::string InputFilter::Rule::format() const
{
  std::string s;
  if (m_condition != nullptr) {
    // condition
    s += m_condition->format();
    s += " = ";

    // activate actions
    auto i = m_activateActions.begin();
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

const InputFilter::Condition *InputFilter::Rule::getCondition() const
{
  return m_condition;
}

uint32_t InputFilter::Rule::getNumActions(bool onActivation) const
{
  if (onActivation) {
    return static_cast<uint32_t>(m_activateActions.size());
  } else {
    return static_cast<uint32_t>(m_deactivateActions.size());
  }
}

const InputFilter::Action &InputFilter::Rule::getAction(bool onActivation, uint32_t index) const
{
  if (onActivation) {
    return *m_activateActions[index];
  } else {
    return *m_deactivateActions[index];
  }
}

// -----------------------------------------------------------------------------
// Input Filter Class
// -----------------------------------------------------------------------------
InputFilter::InputFilter(IEventQueue *events) : m_events(events)
{
  // do nothing
}

InputFilter::InputFilter(const InputFilter &x) : m_ruleList(x.m_ruleList), m_events(x.m_events)
{
  setPrimaryClient(x.m_primaryClient);
}

InputFilter::~InputFilter()
{
  setPrimaryClient(nullptr);
}

InputFilter &InputFilter::operator=(const InputFilter &x)
{
  if (&x != this) {
    PrimaryClient *oldClient = m_primaryClient;
    setPrimaryClient(nullptr);

    m_ruleList = x.m_ruleList;

    setPrimaryClient(oldClient);
  }
  return *this;
}

void InputFilter::addFilterRule(const Rule &rule)
{
  m_ruleList.push_back(rule);
  if (m_primaryClient != nullptr) {
    m_ruleList.back().enable(m_primaryClient);
  }
}

void InputFilter::removeFilterRule(uint32_t index)
{
  if (m_primaryClient != nullptr) {
    m_ruleList[index].disable(m_primaryClient);
  }
  m_ruleList.erase(m_ruleList.begin() + index);
}

InputFilter::Rule &InputFilter::getRule(uint32_t index)
{
  return m_ruleList[index];
}

void InputFilter::setPrimaryClient(PrimaryClient *client)
{
  if (m_primaryClient == client) {
    return;
  }

  using enum EventTypes;
  if (m_primaryClient != nullptr) {
    for (auto rule = m_ruleList.begin(); rule != m_ruleList.end(); ++rule) {
      rule->disable(m_primaryClient);
    }

    m_events->removeHandler(KeyStateKeyDown, m_primaryClient->getEventTarget());
    m_events->removeHandler(KeyStateKeyUp, m_primaryClient->getEventTarget());
    m_events->removeHandler(KeyStateKeyRepeat, m_primaryClient->getEventTarget());
    m_events->removeHandler(PrimaryScreenButtonDown, m_primaryClient->getEventTarget());
    m_events->removeHandler(PrimaryScreenButtonUp, m_primaryClient->getEventTarget());
    m_events->removeHandler(PrimaryScreenHotkeyDown, m_primaryClient->getEventTarget());
    m_events->removeHandler(PrimaryScreenHotkeyUp, m_primaryClient->getEventTarget());
    m_events->removeHandler(ServerConnected, m_primaryClient->getEventTarget());
  }

  m_primaryClient = client;

  if (m_primaryClient != nullptr) {
    m_events->addHandler(KeyStateKeyDown, m_primaryClient->getEventTarget(), [this](const auto &e) { handleEvent(e); });
    m_events->addHandler(KeyStateKeyUp, m_primaryClient->getEventTarget(), [this](const auto &e) { handleEvent(e); });
    m_events->addHandler(KeyStateKeyRepeat, m_primaryClient->getEventTarget(), [this](const auto &e) {
      handleEvent(e);
    });
    m_events->addHandler(PrimaryScreenButtonDown, m_primaryClient->getEventTarget(), [this](const auto &e) {
      handleEvent(e);
    });
    m_events->addHandler(PrimaryScreenButtonUp, m_primaryClient->getEventTarget(), [this](const auto &e) {
      handleEvent(e);
    });
    m_events->addHandler(PrimaryScreenHotkeyDown, m_primaryClient->getEventTarget(), [this](const auto &e) {
      handleEvent(e);
    });
    m_events->addHandler(PrimaryScreenHotkeyUp, m_primaryClient->getEventTarget(), [this](const auto &e) {
      handleEvent(e);
    });
    m_events->addHandler(ServerConnected, m_primaryClient->getEventTarget(), [this](const auto &e) { handleEvent(e); });

    for (auto rule = m_ruleList.begin(); rule != m_ruleList.end(); ++rule) {
      rule->enable(m_primaryClient);
    }
  }
}

std::string InputFilter::format(const std::string_view &linePrefix) const
{
  std::string s;
  for (auto i = m_ruleList.begin(); i != m_ruleList.end(); ++i) {
    s += linePrefix;
    s += i->format();
    s += "\n";
  }
  return s;
}

uint32_t InputFilter::getNumRules() const
{
  return static_cast<uint32_t>(m_ruleList.size());
}

bool InputFilter::operator==(const InputFilter &x) const
{
  // if there are different numbers of rules then we can't be equal
  if (m_ruleList.size() != x.m_ruleList.size()) {
    return false;
  }

  // compare rule lists.  the easiest way to do that is to format each
  // rule into a string, sort the strings, then compare the results.
  std::vector<std::string> aList;
  std::vector<std::string> bList;
  for (auto i = m_ruleList.begin(); i != m_ruleList.end(); ++i) {
    aList.push_back(i->format());
  }
  for (auto i = x.m_ruleList.begin(); i != x.m_ruleList.end(); ++i) {
    bList.push_back(i->format());
  }
  std::ranges::partial_sort(aList, aList.end());
  std::ranges::partial_sort(bList, bList.end());
  return (aList == bList);
}

void InputFilter::handleEvent(const Event &event)
{
  // copy event and adjust target
  Event myEvent(
      event.getType(), this, event.getData(),
      event.getFlags() | Event::EventFlags::DontFreeData | Event::EventFlags::DeliverImmediately
  );

  // let each rule try to match the event until one does
  for (auto rule = m_ruleList.begin(); rule != m_ruleList.end(); ++rule) {
    if (rule->handleEvent(myEvent)) {
      // handled
      return;
    }
  }

  // not handled so pass through
  m_events->addEvent(myEvent);
}
