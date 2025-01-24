/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2012 - 2016 Symless Ltd.
 * SPDX-FileCopyrightText: (C) 2002 Chris Schoeneman
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#pragma once

#include "deskflow/DragInformation.h"
#include "deskflow/IScreen.h"
#include "deskflow/clipboard_types.h"
#include "deskflow/key_types.h"
#include "deskflow/mouse_types.h"
#include "deskflow/option_types.h"

class IClipboard;
class IPlatformScreen;
class IEventQueue;

namespace deskflow {

//! Platform independent screen
/*!
This is a platform independent screen.  It can work as either a
primary or secondary screen.
*/
class Screen : public IScreen
{
public:
  Screen(IPlatformScreen *platformScreen, IEventQueue *events);
  Screen(Screen const &) = delete;
  Screen(Screen &&) = delete;
  virtual ~Screen();

  Screen &operator&(Screen const &) = delete;
  Screen &operator&(Screen &&) = delete;

#ifdef TEST_ENV
  Screen() : m_mock(true)
  {
  }
#endif

  //! @name manipulators
  //@{

  //! Activate screen
  /*!
  Activate the screen, preparing it to report system and user events.
  For a secondary screen it also means disabling the screen saver if
  synchronizing it and preparing to synthesize events.
  */
  virtual void enable();

  //! Deactivate screen
  /*!
  Undoes the operations in activate() and events are no longer
  reported.  It also releases keys that are logically pressed.
  */
  virtual void disable();

  //! Enter screen
  /*!
  Called when the user navigates to this screen.  \p toggleMask has the
  toggle keys that should be turned on on the secondary screen.
  */
  void enter(KeyModifierMask toggleMask);

  //! Leave screen
  /*!
  Called when the user navigates off this screen.
  */
  bool leave();

  //! Update configuration
  /*!
  This is called when the configuration has changed.  \c activeSides
  is a bitmask of EDirectionMask indicating which sides of the
  primary screen are linked to clients.
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

  //! Activate/deactivate screen saver
  /*!
  Forcibly activates the screen saver if \c activate is true otherwise
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
  screen position \c xAbs,yAbs.
  */
  void mouseMove(int32_t xAbs, int32_t yAbs);

  //! Notify of mouse motion
  /*!
  Synthesize mouse events to generate mouse motion by the relative
  amount \c xRel,yRel.
  */
  void mouseRelativeMove(int32_t xRel, int32_t yRel);

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
  virtual void resetOptions();

  //! Notify of options changes
  /*!
  Set options to given values.  Ignores unknown options and doesn't
  modify options that aren't given in \c options.
  */
  virtual void setOptions(const OptionsList &options);

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

  //! Prepare to synthesize input on primary screen
  /*!
  Prepares the primary screen to receive synthesized input.  We do not
  want to receive this synthesized input as user input so this method
  ensures that we ignore it.  Calls to \c fakeInputBegin() may not be
  nested.
  */
  void fakeInputBegin();

  //! Done synthesizing input on primary screen
  /*!
  Undoes whatever \c fakeInputBegin() did.
  */
  void fakeInputEnd();

  //! Change dragging status
  void setDraggingStarted(bool started);

  //! Fake a files dragging operation
  void startDraggingFiles(DragFileList &fileList);

  void setEnableDragDrop(bool enabled);

  //! Determine the name of the app causing a secure input state
  /*!
  On MacOS check which app causes a secure input state to be enabled. No
  alternative on other platforms
  */
  std::string getSecureInputApp() const;

  //@}
  //! @name accessors
  //@{

  //! Test if cursor on screen
  /*!
  Returns true iff the cursor is on the screen.
  */
  bool isOnScreen() const;

  //! Get screen lock state
  /*!
  Returns true if there's any reason that the user should not be
  allowed to leave the screen (usually because a button or key is
  pressed).  If this method returns true it logs a message as to
  why at the CLOG_DEBUG level.
  */
  bool isLockedToScreen() const;

  //! Get jump zone size
  /*!
  Return the jump zone size, the size of the regions on the edges of
  the screen that cause the cursor to jump to another screen.
  */
  int32_t getJumpZoneSize() const;

  //! Get cursor center position
  /*!
  Return the cursor center position which is where we park the
  cursor to compute cursor motion deltas and should be far from
  the edges of the screen, typically the center.
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

  //! Test if file is dragged on primary screen
  bool isDraggingStarted() const;

  //! Test if file is dragged on secondary screen
  bool isFakeDraggingStarted() const;

  //! Get the filename of the file being dragged
  std::string &getDraggingFilename() const;

  //! Clear the filename of the file that was dragged
  void clearDraggingFilename();

  //! Get the drop target directory
  const std::string &getDropTarget() const;

  //@}

  // IScreen overrides
  virtual void *getEventTarget() const;
  virtual bool getClipboard(ClipboardID id, IClipboard *) const;
  virtual void getShape(int32_t &x, int32_t &y, int32_t &width, int32_t &height) const;
  virtual void getCursorPos(int32_t &x, int32_t &y) const;

  IPlatformScreen *getPlatformScreen()
  {
    return m_screen;
  }

protected:
  void enablePrimary();
  void enableSecondary();
  void disablePrimary();
  void disableSecondary();

  void enterPrimary();
  void enterSecondary(KeyModifierMask toggleMask);
  void leavePrimary();
  void leaveSecondary();

private:
  // our platform dependent screen
  IPlatformScreen *m_screen;

  // true if screen is being used as a primary screen, false otherwise
  bool m_isPrimary;

  // true if screen is enabled
  bool m_enabled;

  // true if the cursor is on this screen
  bool m_entered;

  // note toggle keys that toggles on up/down (false) or on
  // transition (true)
  KeyModifierMask m_halfDuplex;

  // true if we're faking input on a primary screen
  bool m_fakeInput;

  IEventQueue *m_events;

  bool m_mock;
  bool m_enableDragDrop;
};

} // namespace deskflow
