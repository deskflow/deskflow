/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2025 Deskflow Developers
 * SPDX-FileCopyrightText: (C) 2012 - 2016 Symless Ltd.
 * SPDX-FileCopyrightText: (C) 2005 Chris Schoeneman
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#pragma once

#include "base/DirectionTypes.h"
#include "deskflow/IPlatformScreen.h"
#include "deskflow/KeyTypes.h"
#include "deskflow/MouseTypes.h"

#include <set>

class PrimaryClient;
class Event;
class IEventQueue;

class InputFilter
{
public:
  // -------------------------------------------------------------------------
  // Input Filter Condition Classes
  // -------------------------------------------------------------------------
  enum class FilterStatus
  {
    NoMatch,
    Activate,
    Deactivate
  };

  class Condition
  {
  public:
    Condition() = default;
    virtual ~Condition() = default;

    virtual Condition *clone() const = 0;
    virtual std::string format() const = 0;

    virtual FilterStatus match(const Event &) = 0;

    virtual void enablePrimary(PrimaryClient *);
    virtual void disablePrimary(PrimaryClient *);
  };

  // KeystrokeCondition
  class KeystrokeCondition : public Condition
  {
  public:
    KeystrokeCondition(IEventQueue *events, IPlatformScreen::KeyInfo *);
    KeystrokeCondition(IEventQueue *events, KeyID key, KeyModifierMask mask);
    ~KeystrokeCondition() override = default;

    KeyID getKey() const;
    KeyModifierMask getMask() const;

    // Condition overrides
    Condition *clone() const override;
    std::string format() const override;
    FilterStatus match(const Event &) override;
    void enablePrimary(PrimaryClient *) override;
    void disablePrimary(PrimaryClient *) override;

  private:
    uint32_t m_id = 0;
    KeyID m_key;
    KeyModifierMask m_mask;
    IEventQueue *m_events;
  };

  // MouseButtonCondition
  class MouseButtonCondition : public Condition
  {
  public:
    MouseButtonCondition(IEventQueue *events, const IPlatformScreen::ButtonInfo &);
    MouseButtonCondition(IEventQueue *events, ButtonID, KeyModifierMask mask);
    ~MouseButtonCondition() override = default;

    ButtonID getButton() const;
    KeyModifierMask getMask() const;

    // Condition overrides
    Condition *clone() const override;
    std::string format() const override;
    FilterStatus match(const Event &) override;

  private:
    ButtonID m_button;
    KeyModifierMask m_mask;
    IEventQueue *m_events;
  };

  // ScreenConnectedCondition
  class ScreenConnectedCondition : public Condition
  {
  public:
    ScreenConnectedCondition(IEventQueue *events, const std::string &screen);
    ~ScreenConnectedCondition() override = default;

    // Condition overrides
    Condition *clone() const override;
    std::string format() const override;
    FilterStatus match(const Event &) override;

  private:
    std::string m_screen;
    IEventQueue *m_events;
  };

  // -------------------------------------------------------------------------
  // Input Filter Action Classes
  // -------------------------------------------------------------------------

  class Action
  {
  public:
    Action() = default;
    virtual ~Action() = default;

    virtual Action *clone() const = 0;
    virtual std::string format() const = 0;

    virtual void perform(const Event &) = 0;
  };

  // LockCursorToScreenAction
  class LockCursorToScreenAction : public Action
  {
  public:
    enum Mode
    {
      kOff,
      kOn,
      kToggle
    };

    explicit LockCursorToScreenAction(IEventQueue *events, Mode = kToggle);

    Mode getMode() const;

    // Action overrides
    Action *clone() const override;
    std::string format() const override;
    void perform(const Event &) override;

  private:
    Mode m_mode;
    IEventQueue *m_events;
  };

  class RestartServer : public Action
  {
  public:
    enum Mode
    {
      restart
    };

    explicit RestartServer(Mode = restart);

    Mode getMode() const;

    // Action overrides
    Action *clone() const override;
    std::string format() const override;
    void perform(const Event &) override;

  private:
    Mode m_mode;
  };

  // SwitchToScreenAction
  class SwitchToScreenAction : public Action
  {
  public:
    SwitchToScreenAction(IEventQueue *events, const std::string &screen);

    std::string getScreen() const;

    // Action overrides
    Action *clone() const override;
    std::string format() const override;
    void perform(const Event &) override;

  private:
    std::string m_screen;
    IEventQueue *m_events;
  };

  // SwitchInDirectionAction
  class SwitchInDirectionAction : public Action
  {
  public:
    SwitchInDirectionAction(IEventQueue *events, Direction);

    Direction getDirection() const;

    // Action overrides
    Action *clone() const override;
    std::string format() const override;
    void perform(const Event &) override;

  private:
    Direction m_direction;
    IEventQueue *m_events;
  };

  // SwitchToNextScreenAction
  class SwitchToNextScreenAction : public Action
  {
  public:
    explicit SwitchToNextScreenAction(IEventQueue *events);

    // Action overrides
    Action *clone() const override;
    std::string format() const override;
    void perform(const Event &) override;

  private:
    IEventQueue *m_events;
  };

  // KeyboardBroadcastAction
  class KeyboardBroadcastAction : public Action
  {
  public:
    enum Mode
    {
      kOff,
      kOn,
      kToggle
    };

    explicit KeyboardBroadcastAction(IEventQueue *events, Mode = kToggle);
    explicit KeyboardBroadcastAction(IEventQueue *events, Mode, const std::set<std::string> &screens);

    Mode getMode() const;
    std::set<std::string> getScreens() const;

    // Action overrides
    Action *clone() const override;
    std::string format() const override;
    void perform(const Event &) override;

  private:
    Mode m_mode;
    std::string m_screens;
    IEventQueue *m_events;
  };

  // KeystrokeAction
  class KeystrokeAction : public Action
  {
  public:
    KeystrokeAction(IEventQueue *events, IPlatformScreen::KeyInfo *adoptedInfo, bool press);
    KeystrokeAction(KeystrokeAction const &) = delete;
    KeystrokeAction(KeystrokeAction &&) = delete;
    ~KeystrokeAction() override;

    KeystrokeAction &operator=(KeystrokeAction const &) = delete;
    KeystrokeAction &operator=(KeystrokeAction &&) = delete;

    void adoptInfo(IPlatformScreen::KeyInfo *);
    const IPlatformScreen::KeyInfo *getInfo() const;
    bool isOnPress() const;

    // Action overrides
    Action *clone() const override;
    std::string format() const override;
    void perform(const Event &) override;

  protected:
    virtual const char *formatName() const;

  private:
    IPlatformScreen::KeyInfo *m_keyInfo;
    bool m_press;
    IEventQueue *m_events;
  };

  // MouseButtonAction -- modifier combinations not implemented yet
  class MouseButtonAction : public Action
  {
  public:
    MouseButtonAction(IEventQueue *events, const IPlatformScreen::ButtonInfo &adoptedInfo, bool press);
    MouseButtonAction(MouseButtonAction const &) = delete;
    MouseButtonAction(MouseButtonAction &&) = delete;
    ~MouseButtonAction() override = default;

    MouseButtonAction &operator=(MouseButtonAction const &) = delete;
    MouseButtonAction &operator=(MouseButtonAction &&) = delete;

    const IPlatformScreen::ButtonInfo &getInfo() const;
    bool isOnPress() const;

    // Action overrides
    Action *clone() const override;
    std::string format() const override;
    void perform(const Event &) override;

  protected:
    virtual const char *formatName() const;

  private:
    IPlatformScreen::ButtonInfo m_buttonInfo;
    bool m_press;
    IEventQueue *m_events;
  };

  class Rule
  {
  public:
    Rule() = default;
    explicit Rule(Condition *adopted);
    Rule(const Rule &);
    ~Rule();

    Rule &operator=(const Rule &);

    // replace the condition
    void setCondition(Condition *adopted);

    // add an action to the rule
    void adoptAction(Action *, bool onActivation);

    // remove an action from the rule
    void removeAction(bool onActivation, uint32_t index);

    // replace an action in the rule
    void replaceAction(Action *adopted, bool onActivation, uint32_t index);

    // enable/disable
    void enable(PrimaryClient *);
    void disable(PrimaryClient *);

    // event handling
    bool handleEvent(const Event &e);

    // convert rule to a string
    std::string format() const;

    // get the rule's condition
    const Condition *getCondition() const;

    // get number of actions
    uint32_t getNumActions(bool onActivation) const;

    // get action by index
    const Action &getAction(bool onActivation, uint32_t index) const;

  private:
    void clear();
    void copy(const Rule &);

  private:
    using ActionList = std::vector<Action *>;

    Condition *m_condition = nullptr;
    ActionList m_activateActions;
    ActionList m_deactivateActions;
  };

  // -------------------------------------------------------------------------
  // Input Filter Class
  // -------------------------------------------------------------------------
  using RuleList = std::vector<Rule>;

  explicit InputFilter(IEventQueue *events);
  InputFilter(const InputFilter &);
  ~InputFilter();

  InputFilter &operator=(const InputFilter &);

  // add rule, adopting the condition and the actions
  void addFilterRule(const Rule &rule);

  // remove a rule
  void removeFilterRule(uint32_t index);

  // get rule by index
  Rule &getRule(uint32_t index);

  // enable event filtering using the given primary client.  disable
  // if client is nullptr.
  void setPrimaryClient(PrimaryClient *client);

  // convert rules to a string
  std::string format(const std::string_view &linePrefix) const;

  // get number of rules
  uint32_t getNumRules() const;

  //! Compare filters
  bool operator==(const InputFilter &) const;

private:
  // event handling
  void handleEvent(const Event &);

private:
  RuleList m_ruleList;
  PrimaryClient *m_primaryClient = nullptr;
  IEventQueue *m_events;
};
