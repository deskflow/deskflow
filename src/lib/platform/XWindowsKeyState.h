/*
 * Deskflow -- mouse and keyboard sharing utility
 * Copyright (C) 2012-2016 Symless Ltd.
 * Copyright (C) 2003 Chris Schoeneman
 *
 * This package is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * found in the file LICENSE that should have accompanied this file.
 *
 * This package is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#pragma once

#include "common/stdmap.h"
#include "common/stdvector.h"
#include "deskflow/KeyState.h"

#if X_DISPLAY_MISSING
#error X11 is required to build deskflow
#else
#include <X11/Xlib.h>
#if HAVE_X11_EXTENSIONS_XTEST_H
#include <X11/extensions/XTest.h>
#else
#error The XTest extension is required to build deskflow
#endif
#if HAVE_XKB_EXTENSION
#include <X11/extensions/XKBstr.h>
#endif
#endif

class IEventQueue;

//! X Windows key state
/*!
A key state for X Windows.
*/
class XWindowsKeyState : public KeyState
{
public:
  using KeycodeList = std::vector<int>;
  enum
  {
    kGroupPoll = -1,
    kGroupPollAndSet = -2
  };

  XWindowsKeyState(Display *, bool useXKB, IEventQueue *events);
  XWindowsKeyState(Display *, bool useXKB, IEventQueue *events, deskflow::KeyMap &keyMap);
  ~XWindowsKeyState();

  //! @name modifiers
  //@{

  //! Set active group
  /*!
  Sets the active group to \p group.  This is the group returned by
  \c pollActiveGroup().  If \p group is \c kGroupPoll then
  \c pollActiveGroup() will really poll, but that's a slow operation
  on X11.  If \p group is \c kGroupPollAndSet then this will poll the
  active group now and use it for future calls to \c pollActiveGroup().
  */
  void setActiveGroup(SInt32 group);

  //! Set the auto-repeat state
  /*!
  Sets the auto-repeat state.
  */
  void setAutoRepeat(const XKeyboardState &);

  //@}
  //! @name accessors
  //@{

  //! Convert X modifier mask to deskflow mask
  /*!
  Returns the deskflow modifier mask corresponding to the X modifier
  mask in \p state.
  */
  KeyModifierMask mapModifiersFromX(unsigned int state) const;

  //! Convert deskflow modifier mask to X mask
  /*!
  Converts the deskflow modifier mask to the corresponding X modifier
  mask.  Returns \c true if successful and \c false if any modifier
  could not be converted.
  */
  bool mapModifiersToX(KeyModifierMask, unsigned int &) const;

  //! Convert deskflow key to all corresponding X keycodes
  /*!
  Converts the deskflow key \p key to all of the keycodes that map to
  that key.
  */
  void mapKeyToKeycodes(KeyID key, KeycodeList &keycodes) const;

  //@}

  // IKeyState overrides
  virtual bool fakeCtrlAltDel();
  virtual KeyModifierMask pollActiveModifiers() const;
  virtual SInt32 pollActiveGroup() const;
  virtual void pollPressedKeys(KeyButtonSet &pressedKeys) const;

protected:
  // KeyState overrides
  virtual void getKeyMap(deskflow::KeyMap &keyMap);
  virtual void fakeKey(const Keystroke &keystroke);

private:
  void init(Display *display, bool useXKB);
  void updateKeysymMap(deskflow::KeyMap &);
  void updateKeysymMapXKB(deskflow::KeyMap &);
  bool hasModifiersXKB() const;
  int getEffectiveGroup(KeyCode, int group) const;
  UInt32 getGroupFromState(unsigned int state) const;

  //! Create and send language change request to \p group by DBus interface
  /*!
  Returns the existance of nedeed DBus interface.
  */
  bool setCurrentLanguageWithDBus(SInt32 group) const;

  static void remapKeyModifiers(KeyID, SInt32, deskflow::KeyMap::KeyItem &, void *);

private:
  struct XKBModifierInfo
  {
  public:
    unsigned char m_level;
    UInt32 m_mask;
    bool m_lock;
  };

#ifdef TEST_ENV
public: // yuck
#endif
  using KeyModifierMaskList = std::vector<KeyModifierMask>;

private:
  using KeyModifierToXMask = std::map<KeyModifierMask, unsigned int>;
  using KeyToKeyCodeMap = std::multimap<KeyID, KeyCode>;
  using NonXKBModifierMap = std::map<KeyCode, unsigned int>;
  using XKBModifierMap = std::map<UInt32, XKBModifierInfo>;

  Display *m_display;
#if HAVE_XKB_EXTENSION
  XkbDescPtr m_xkb;
#endif
  SInt32 m_group;
  XKBModifierMap m_lastGoodXKBModifiers;
  NonXKBModifierMap m_lastGoodNonXKBModifiers;

  // X modifier (bit number) to deskflow modifier (mask) mapping
  KeyModifierMaskList m_modifierFromX;

  // deskflow modifier (mask) to X modifier (mask)
  KeyModifierToXMask m_modifierToX;

  // map KeyID to all keycodes that can synthesize that KeyID
  KeyToKeyCodeMap m_keyCodeFromKey;

  // autorepeat state
  XKeyboardState m_keyboardState;

#ifdef TEST_ENV
public:
  SInt32 group() const
  {
    return m_group;
  }
  void group(const SInt32 &group)
  {
    m_group = group;
  }
  KeyModifierMaskList modifierFromX() const
  {
    return m_modifierFromX;
  }
#endif
};
