/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2012 - 2016 Synergy App Ltd
 * SPDX-FileCopyrightText: (C) 2002 Chris Schoeneman
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#pragma once

#include "deskflow/ClipboardTypes.h"
#include "deskflow/IComputer.h"
#include "deskflow/KeyTypes.h"
#include "deskflow/MouseTypes.h"
#include "deskflow/OptionTypes.h"

#include <string>

class IClipboard;
class IPlatformComputer;
class IEventQueue;

namespace deskflow {

//! Platform independent computer
/*!
This is a platform independent computer.  It can work as either a
primary or secondary computer.
*/
class Computer : public IComputer
{
public:
  Computer(IPlatformComputer *platformScreen, IEventQueue *events);
  Computer(Computer const &) = delete;
  Computer(Computer &&) = delete;
  ~Computer() override;

  Computer &operator&(Computer const &) = delete;
  Computer &operator&(Computer &&) = delete;

  //! @name manipulators
  //@{

  //! Activate computer
  /*!
  Activate the computer, preparing it to report system and user events.
  For a secondary computer it also means disabling the computer saver if
  synchronizing it and preparing to synthesize events.
  */
  void enable();

  //! Deactivate computer
  /*!
  Undoes the operations in activate() and events are no longer
  reported.  It also releases keys that are logically pressed.
  */
  void disable();

  //! Enter computer
  /*!
  Called when the user navigates to this computer.  \p toggleMask has the
  toggle keys that should be turned on on the secondary computer.
  */
  void enter(KeyModifierMask toggleMask);

  //! Leave computer
  /*!
  Called when the user navigates off this computer.
  */
  bool leave();

  //! Update configuration
  /*!
  This is called when the configuration has changed.  \c activeSides
  is a bitmask of DirectionMask indicating which sides of the
  primary computer are linked to clients.
  */
  void reconfigure(uint32_t activeSides);

  //! Warp cursor
  /*!
  Warps the cursor to the absolute coordinates \c x,y.  Also
  discards input events up to and including the warp before
  returning.
  */
  void warpCursor(int32_t x, int32_t y);

  //! Set clipboard
  /*!
  Sets the system's clipboard contents.  This is usually called
  soon after an enter().
  */
  void setClipboard(ClipboardID, const IClipboard *);

  //! Grab clipboard
  /*!
  Grabs (i.e. take ownership of) the system clipboard.
  */
  void grabClipboard(ClipboardID);

  //! Activate/deactivate computer saver
  /*!
  Forcibly activates the computer saver if \c activate is true otherwise
  forcibly deactivates it.
  */
  void screensaver(bool activate) const;

  //! Notify of key press
  /*!
  Synthesize key events to generate a press of key \c id.  If possible
  match the given modifier mask.  The KeyButton identifies the physical
  key on the server that generated this key down.  The client must
  ensure that a key up or key repeat that uses the same KeyButton will
  synthesize an up or repeat for the same client key synthesized by
  keyDown().
  */
  void keyDown(KeyID id, KeyModifierMask, KeyButton, const std::string &);

  //! Notify of key repeat
  /*!
  Synthesize key events to generate a press and release of key \c id
  \c count times.  If possible match the given modifier mask.
  */
  void keyRepeat(KeyID id, KeyModifierMask, int32_t count, KeyButton, const std::string &lang);

  //! Notify of key release
  /*!
  Synthesize key events to generate a release of key \c id.  If possible
  match the given modifier mask.
  */
  void keyUp(KeyID id, KeyModifierMask, KeyButton);

  //! Notify of mouse press
  /*!
  Synthesize mouse events to generate a press of mouse button \c id.
  */
  void mouseDown(ButtonID id);

  //! Notify of mouse release
  /*!
  Synthesize mouse events to generate a release of mouse button \c id.
  */
  void mouseUp(ButtonID id);

  //! Notify of mouse motion
  /*!
  Synthesize mouse events to generate mouse motion to the absolute
  computer position \c xAbs,yAbs.
  */
  void mouseMove(int32_t xAbs, int32_t yAbs);

  //! Notify of mouse motion
  /*!
  Synthesize mouse events to generate mouse motion by the relative
  amount \c xRel,yRel.
  */
  void mouseRelativeMove(int32_t xRel, int32_t yRel) const;

  //! Notify of mouse wheel motion
  /*!
  Synthesize mouse events to generate mouse wheel motion of \c xDelta
  and \c yDelta.  Deltas are positive for motion away from the user or
  to the right and negative for motion towards the user or to the left.
  Each wheel click should generate a delta of +/-120.
  */
  void mouseWheel(int32_t xDelta, int32_t yDelta) const;

  //! Notify of options changes
  /*!
  Resets all options to their default values.
  */
  void resetOptions();

  //! Notify of options changes
  /*!
  Set options to given values.  Ignores unknown options and doesn't
  modify options that aren't given in \c options.
  */
  void setOptions(const OptionsList &options);

  //! Set clipboard sequence number
  /*!
  Sets the sequence number to use in subsequent clipboard events.
  */
  void setSequenceNumber(uint32_t);

  //! Register a system hotkey
  /*!
  Registers a system-wide hotkey for key \p key with modifiers \p mask.
  Returns an id used to unregister the hotkey.
  */
  uint32_t registerHotKey(KeyID key, KeyModifierMask mask);

  //! Unregister a system hotkey
  /*!
  Unregisters a previously registered hot key.
  */
  void unregisterHotKey(uint32_t id);

  //! Prepare to synthesize input on primary computer
  /*!
  Prepares the primary computer to receive synthesized input.  We do not
  want to receive this synthesized input as user input so this method
  ensures that we ignore it.  Calls to \c fakeInputBegin() may not be
  nested.
  */
  void fakeInputBegin();

  //! Done synthesizing input on primary computer
  /*!
  Undoes whatever \c fakeInputBegin() did.
  */
  void fakeInputEnd();

  //! Determine the name of the app causing a secure input state
  /*!
  On MacOS check which app causes a secure input state to be enabled. No
  alternative on other platforms
  */
  std::string getSecureInputApp() const;

  //@}
  //! @name accessors
  //@{

  //! Test if cursor on computer
  /*!
  Returns true iff the cursor is on the computer.
  */
  bool isOnComputer() const;

  //! Get computer lock state
  /*!
  Returns true if there's any reason that the user should not be
  allowed to leave the computer (usually because a button or key is
  pressed).  If this method returns true it logs a message as to
  why at the CLOG_DEBUG level.
  */
  bool isLockedToComputer() const;

  //! Get jump zone size
  /*!
  Return the jump zone size, the size of the regions on the edges of
  the computer that cause the cursor to jump to another computer.
  */
  int32_t getJumpZoneSize() const;

  //! Get cursor center position
  /*!
  Return the cursor center position which is where we park the
  cursor to compute cursor motion deltas and should be far from
  the edges of the computer, typically the center.
  */
  void getCursorCenter(int32_t &x, int32_t &y) const;

  //! Get the active modifiers
  /*!
  Returns the modifiers that are currently active according to our
  shadowed state.
  */
  KeyModifierMask getActiveModifiers() const;

  //! Get the active modifiers from OS
  /*!
  Returns the modifiers that are currently active according to the
  operating system.
  */
  KeyModifierMask pollActiveModifiers() const;

  //@}

  // IComputer overrides
  void *getEventTarget() const override;
  bool getClipboard(ClipboardID id, IClipboard *) const override;
  void getShape(int32_t &x, int32_t &y, int32_t &width, int32_t &height) const override;
  void getCursorPos(int32_t &x, int32_t &y) const override;

  IPlatformComputer *getPlatformComputer()
  {
    return m_computer;
  }

protected:
  void enablePrimary();
  void enableSecondary();
  void disablePrimary();
  void disableSecondary();

  void enterPrimary() const;
  void enterSecondary(KeyModifierMask toggleMask) const;
  void leavePrimary();
  void leaveSecondary();

private:
  // our platform dependent computer
  IPlatformComputer *m_computer = nullptr;

  // true if computer is being used as a primary computer, false otherwise
  bool m_isPrimary = false;

  // true if computer is enabled
  bool m_enabled = false;

  // true if the cursor is on this computer
  bool m_entered = false;

  // note toggle keys that toggles on up/down (false) or on
  // transition (true)
  KeyModifierMask m_halfDuplex;

  // true if we're faking input on a primary computer
  bool m_fakeInput = false;

  IEventQueue *m_events = nullptr;
};

} // namespace deskflow
