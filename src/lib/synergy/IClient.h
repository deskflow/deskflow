/*
 * synergy -- mouse and keyboard sharing utility
 * Copyright (C) 2012-2016 Symless Ltd.
 * Copyright (C) 2002 Chris Schoeneman
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

#include "synergy/clipboard_types.h"
#include "synergy/IScreen.h"
#include "synergy/key_types.h"
#include "synergy/mouse_types.h"
#include "synergy/option_types.h"
#include "base/String.h"

//! Client interface
/*!
This interface defines the methods necessary for the server to
communicate with a client.
*/
class IClient : public IScreen {
public:
    //! @name manipulators
    //@{

    //! Enter screen
    /*!
    Enter the screen.  The cursor should be warped to \p xAbs,yAbs.
    \p mask is the expected toggle button state and the client should
    update its state to match.  \p forScreensaver is true iff the
    screen is being entered because the screen saver is starting.
    Subsequent clipboard events should report \p seqNum.
    */
    virtual void enter (SInt32 xAbs, SInt32 yAbs, UInt32 seqNum,
                        KeyModifierMask mask, bool forScreensaver) = 0;

    //! Leave screen
    /*!
    Leave the screen.  Return false iff the user may not leave the
    client's screen (because, for example, a button is down).
    */
    virtual bool leave () = 0;

    //! Set clipboard
    /*!
    Update the client's clipboard.  This implies that the client's
    clipboard is now up to date.  If the client's clipboard was
    already known to be up to date then this may do nothing.  \c data
    has marshalled clipboard data.
    */
    virtual void setClipboard (ClipboardID, const IClipboard*) = 0;

    //! Grab clipboard
    /*!
    Grab (i.e. take ownership of) the client's clipboard.  Since this
    is called when another client takes ownership of the clipboard it
    implies that the client's clipboard is out of date.
    */
    virtual void grabClipboard (ClipboardID) = 0;

    //! Mark clipboard dirty
    /*!
    Mark the client's clipboard as dirty (out of date) or clean (up to
    date).
    */
    virtual void setClipboardDirty (ClipboardID, bool dirty) = 0;

    //! Notify of key press
    /*!
    Synthesize key events to generate a press of key \c id.  If possible
    match the given modifier mask.  The KeyButton identifies the physical
    key on the server that generated this key down.  The client must
    ensure that a key up or key repeat that uses the same KeyButton will
    synthesize an up or repeat for the same client key synthesized by
    keyDown().
    */
    virtual void keyDown (KeyID id, KeyModifierMask, KeyButton) = 0;

    //! Notify of key repeat
    /*!
    Synthesize key events to generate a press and release of key \c id
    \c count times.  If possible match the given modifier mask.
    */
    virtual void
    keyRepeat (KeyID id, KeyModifierMask, SInt32 count, KeyButton) = 0;

    //! Notify of key release
    /*!
    Synthesize key events to generate a release of key \c id.  If possible
    match the given modifier mask.
    */
    virtual void keyUp (KeyID id, KeyModifierMask, KeyButton) = 0;

    //! Notify of mouse press
    /*!
    Synthesize mouse events to generate a press of mouse button \c id.
    */
    virtual void mouseDown (ButtonID id) = 0;

    //! Notify of mouse release
    /*!
    Synthesize mouse events to generate a release of mouse button \c id.
    */
    virtual void mouseUp (ButtonID id) = 0;

    //! Notify of mouse motion
    /*!
    Synthesize mouse events to generate mouse motion to the absolute
    screen position \c xAbs,yAbs.
    */
    virtual void mouseMove (SInt32 xAbs, SInt32 yAbs) = 0;

    //! Notify of mouse motion
    /*!
    Synthesize mouse events to generate mouse motion by the relative
    amount \c xRel,yRel.
    */
    virtual void mouseRelativeMove (SInt32 xRel, SInt32 yRel) = 0;

    //! Notify of mouse wheel motion
    /*!
    Synthesize mouse events to generate mouse wheel motion of \c xDelta
    and \c yDelta.  Deltas are positive for motion away from the user or
    to the right and negative for motion towards the user or to the left.
    Each wheel click should generate a delta of +/-120.
    */
    virtual void mouseWheel (SInt32 xDelta, SInt32 yDelta) = 0;

    //! Notify of screen saver change
    virtual void screensaver (bool activate) = 0;

    //! Notify of options changes
    /*!
    Reset all options to their default values.
    */
    virtual void resetOptions () = 0;

    //! Notify of options changes
    /*!
    Set options to given values.  Ignore unknown options and don't
    modify our options that aren't given in \c options.
    */
    virtual void setOptions (const OptionsList& options) = 0;

    //@}
    //! @name accessors
    //@{

    //! Get client name
    /*!
    Return the client's name.
    */
    virtual String getName () const = 0;

    //@}

    // IScreen overrides
    virtual void* getEventTarget () const = 0;
    virtual bool getClipboard (ClipboardID id, IClipboard*) const = 0;
    virtual void
    getShape (SInt32& x, SInt32& y, SInt32& width, SInt32& height) const = 0;
    virtual void getCursorPos (SInt32& x, SInt32& y) const = 0;
};
