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

#include "base/String.h"
#include "base/EventTypes.h"
#include "common/stdmap.h"
#include "common/stdvector.h"

#if X_DISPLAY_MISSING
#error X11 is required to build synergy
#else
#include <X11/Xlib.h>
#endif

//! X11 utility functions
class XWindowsUtil {
public:
    typedef std::vector<KeySym> KeySyms;

    //! Get property
    /*!
    Gets property \c property on \c window.  \b Appends the data to
    \c *data if \c data is not NULL, saves the property type in \c *type
    if \c type is not NULL, and saves the property format in \c *format
    if \c format is not NULL.  If \c deleteProperty is true then the
    property is deleted after being read.
    */
    static bool
    getWindowProperty (Display*, Window window, Atom property, String* data,
                       Atom* type, SInt32* format, bool deleteProperty);

    //! Set property
    /*!
    Sets property \c property on \c window to \c size bytes of data from
    \c data.
    */
    static bool
    setWindowProperty (Display*, Window window, Atom property, const void* data,
                       UInt32 size, Atom type, SInt32 format);

    //! Get X server time
    /*!
    Returns the current X server time.
    */
    static Time getCurrentTime (Display*, Window);

    //! Convert KeySym to KeyID
    /*!
    Converts a KeySym to the equivalent KeyID.  Returns kKeyNone if the
    KeySym cannot be mapped.
    */
    static UInt32 mapKeySymToKeyID (KeySym);

    //! Convert KeySym to corresponding KeyModifierMask
    /*!
    Converts a KeySym to the corresponding KeyModifierMask, or 0 if the
    KeySym is not a modifier.
    */
    static UInt32 getModifierBitForKeySym (KeySym keysym);

    //! Convert Atom to its string
    /*!
    Converts \p atom to its string representation.
    */
    static String atomToString (Display*, Atom atom);

    //! Convert several Atoms to a string
    /*!
    Converts each atom in \p atoms to its string representation and
    concatenates the results.
    */
    static String
    atomsToString (Display* display, const Atom* atom, UInt32 num);

    //! Prepare a property of atoms for use
    /*!
    64-bit systems may need to modify a property's data if it's a
    list of Atoms before using it.
    */
    static void convertAtomProperty (String& data);

    //! Append an Atom to property data
    /*!
    Converts \p atom to a 32-bit on-the-wire format and appends it to
    \p data.
    */
    static void appendAtomData (String& data, Atom atom);

    //! Replace an Atom in property data
    /*!
    Converts \p atom to a 32-bit on-the-wire format and replaces the atom
    at index \p index in \p data.
    */
    static void replaceAtomData (String& data, UInt32 index, Atom atom);

    //! Append an Time to property data
    /*!
    Converts \p time to a 32-bit on-the-wire format and appends it to
    \p data.
    */
    static void appendTimeData (String& data, Time time);

    //! X11 error handler
    /*!
    This class sets an X error handler in the c'tor and restores the
    previous error handler in the d'tor.  A lock should only be
    installed while the display is locked by the thread.

    ErrorLock() ignores errors
    ErrorLock(bool* flag) sets *flag to true if any error occurs
    */
    class ErrorLock {
    public:
        //! Error handler type
        typedef void (*ErrorHandler) (Display*, XErrorEvent*, void* userData);

        /*!
        Ignore X11 errors.
        */
        ErrorLock (Display*);

        /*!
        Set \c *errorFlag if any error occurs.
        */
        ErrorLock (Display*, bool* errorFlag);

        /*!
        Call \c handler on each error.
        */
        ErrorLock (Display*, ErrorHandler handler, void* userData);

        ~ErrorLock ();

    private:
        void install (ErrorHandler, void*);
        static int internalHandler (Display*, XErrorEvent*);
        static void ignoreHandler (Display*, XErrorEvent*, void*);
        static void saveHandler (Display*, XErrorEvent*, void*);

    private:
        typedef int (*XErrorHandler) (Display*, XErrorEvent*);

        Display* m_display;
        ErrorHandler m_handler;
        void* m_userData;
        XErrorHandler m_oldXHandler;
        ErrorLock* m_next;
        static ErrorLock* s_top;
    };

private:
    class PropertyNotifyPredicateInfo {
    public:
        Window m_window;
        Atom m_property;
    };

    static Bool
    propertyNotifyPredicate (Display*, XEvent* xevent, XPointer arg);

    static void initKeyMaps ();

private:
    typedef std::map<KeySym, UInt32> KeySymMap;

    static KeySymMap s_keySymToUCS4;
};
