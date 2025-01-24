/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2012 - 2016 Symless Ltd.
 * SPDX-FileCopyrightText: (C) 2002 Chris Schoeneman
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#pragma once

#include "base/EventTypes.h"
#include "common/stdmap.h"
#include "common/stdvector.h"

#include <string>

#if X_DISPLAY_MISSING
#error X11 is required to build deskflow
#else
#include <X11/Xlib.h>
#endif

//! X11 utility functions
class XWindowsUtil
{
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
  static bool getWindowProperty(
      Display *, Window window, Atom property, std::string *data, Atom *type, int32_t *format, bool deleteProperty
  );

  //! Set property
  /*!
  Sets property \c property on \c window to \c size bytes of data from
  \c data.
  */
  static bool setWindowProperty(
      Display *, Window window, Atom property, const void *data, uint32_t size, Atom type, int32_t format
  );

  //! Get X server time
  /*!
  Returns the current X server time.
  */
  static Time getCurrentTime(Display *, Window);

  //! Convert KeySym to KeyID
  /*!
  Converts a KeySym to the equivalent KeyID.  Returns kKeyNone if the
  KeySym cannot be mapped.
  */
  static uint32_t mapKeySymToKeyID(KeySym);

  //! Convert KeySym to corresponding KeyModifierMask
  /*!
  Converts a KeySym to the corresponding KeyModifierMask, or 0 if the
  KeySym is not a modifier.
  */
  static uint32_t getModifierBitForKeySym(KeySym keysym);

  //! Convert Atom to its string
  /*!
  Converts \p atom to its string representation.
  */
  static std::string atomToString(Display *, Atom atom);

  //! Convert several Atoms to a string
  /*!
  Converts each atom in \p atoms to its string representation and
  concatenates the results.
  */
  static std::string atomsToString(Display *display, const Atom *atom, uint32_t num);

  //! Prepare a property of atoms for use
  /*!
  64-bit systems may need to modify a property's data if it's a
  list of Atoms before using it.
  */
  static void convertAtomProperty(std::string &data);

  //! Append an Atom to property data
  /*!
  Converts \p atom to a 32-bit on-the-wire format and appends it to
  \p data.
  */
  static void appendAtomData(std::string &data, Atom atom);

  //! Replace an Atom in property data
  /*!
  Converts \p atom to a 32-bit on-the-wire format and replaces the atom
  at index \p index in \p data.
  */
  static void replaceAtomData(std::string &data, uint32_t index, Atom atom);

  //! Append an Time to property data
  /*!
  Converts \p time to a 32-bit on-the-wire format and appends it to
  \p data.
  */
  static void appendTimeData(std::string &data, Time time);

  //! X11 error handler
  /*!
  This class sets an X error handler in the c'tor and restores the
  previous error handler in the d'tor.  A lock should only be
  installed while the display is locked by the thread.

  ErrorLock() ignores errors
  ErrorLock(bool* flag) sets *flag to true if any error occurs
  */
  class ErrorLock
  {
  public:
    //! Error handler type
    typedef void (*ErrorHandler)(Display *, XErrorEvent *, void *userData);

    /*!
    Ignore X11 errors.
    */
    ErrorLock(Display *);
    ErrorLock(ErrorLock const &) = delete;
    ErrorLock(ErrorLock &&) = delete;

    ErrorLock &operator=(ErrorLock const &) = delete;
    ErrorLock &operator=(ErrorLock &&) = delete;

    /*!
    Set \c *errorFlag if any error occurs.
    */
    ErrorLock(Display *, bool *errorFlag);

    /*!
    Call \c handler on each error.
    */
    ErrorLock(Display *, ErrorHandler handler, void *userData);

    ~ErrorLock();

  private:
    void install(ErrorHandler, void *);
    static int internalHandler(Display *, XErrorEvent *);
    static void ignoreHandler(Display *, XErrorEvent *, void *);
    static void saveHandler(Display *, XErrorEvent *, void *);

  private:
    typedef int (*XErrorHandler)(Display *, XErrorEvent *);

    Display *m_display;
    ErrorHandler m_handler;
    void *m_userData;
    XErrorHandler m_oldXHandler;
    ErrorLock *m_next;
    static ErrorLock *s_top;
  };

private:
  class PropertyNotifyPredicateInfo
  {
  public:
    Window m_window;
    Atom m_property;
  };

  static Bool propertyNotifyPredicate(Display *, XEvent *xevent, XPointer arg);

  static void initKeyMaps();

private:
  typedef std::map<KeySym, uint32_t> KeySymMap;

  static KeySymMap s_keySymToUCS4;
};
