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
#include "common/stdexcept.h"

//! Exception base class
/*!
This is the base class of most exception types.
*/
class XBase : public std::runtime_error {
public:
    //! Use getWhat() as the result of what()
    XBase ();
    //! Use \c msg as the result of what()
    XBase (const String& msg);
    virtual ~XBase () _NOEXCEPT;

    //! Reason for exception
    virtual const char* what () const _NOEXCEPT;

protected:
    //! Get a human readable string describing the exception
    virtual String
    getWhat () const throw () {
        return "";
    }

    //! Format a string
    /*!
    Looks up a message format using \c id, using \c defaultFormat if
    no format can be found, then replaces positional parameters in
    the format string and returns the result.
    */
    virtual String
    format (const char* id, const char* defaultFormat, ...) const throw ();

private:
    mutable String m_what;
};

/*!
\def XBASE_SUBCLASS
Convenience macro to subclass from XBase (or a subclass of it),
providing the c'tor taking a const String&.  getWhat() is not
declared.
*/
#define XBASE_SUBCLASS(name_, super_)                                          \
    class name_ : public super_ {                                              \
    public:                                                                    \
        name_ () : super_ () {                                                 \
        }                                                                      \
        name_ (const String& msg) : super_ (msg) {                             \
        }                                                                      \
        virtual ~name_ () _NOEXCEPT {                                          \
        }                                                                      \
    }

/*!
\def XBASE_SUBCLASS
Convenience macro to subclass from XBase (or a subclass of it),
providing the c'tor taking a const String&.  getWhat() must be
implemented.
*/
#define XBASE_SUBCLASS_WHAT(name_, super_)                                     \
    class name_ : public super_ {                                              \
    public:                                                                    \
        name_ () : super_ () {                                                 \
        }                                                                      \
        name_ (const String& msg) : super_ (msg) {                             \
        }                                                                      \
        virtual ~name_ () _NOEXCEPT {                                          \
        }                                                                      \
                                                                               \
    protected:                                                                 \
        virtual String getWhat () const throw ();                              \
    }

/*!
\def XBASE_SUBCLASS_FORMAT
Convenience macro to subclass from XBase (or a subclass of it),
providing the c'tor taking a const String&.  what() is overridden
to call getWhat() when first called;  getWhat() can format the
error message and can call what() to get the message passed to the
c'tor.
*/
#define XBASE_SUBCLASS_FORMAT(name_, super_)                                   \
    class name_ : public super_ {                                              \
    private:                                                                   \
        enum EState { kFirst, kFormat, kDone };                                \
                                                                               \
    public:                                                                    \
        name_ () : super_ (), m_state (kDone) {                                \
        }                                                                      \
        name_ (const String& msg) : super_ (msg), m_state (kFirst) {           \
        }                                                                      \
        virtual ~name_ () _NOEXCEPT {                                          \
        }                                                                      \
                                                                               \
        virtual const char*                                                    \
        what () const _NOEXCEPT {                                              \
            if (m_state == kFirst) {                                           \
                m_state     = kFormat;                                         \
                m_formatted = getWhat ();                                      \
                m_state     = kDone;                                           \
            }                                                                  \
            if (m_state == kDone) {                                            \
                return m_formatted.c_str ();                                   \
            } else {                                                           \
                return super_::what ();                                        \
            }                                                                  \
        }                                                                      \
                                                                               \
    protected:                                                                 \
        virtual String getWhat () const throw ();                              \
                                                                               \
    private:                                                                   \
        mutable EState m_state;                                                \
        mutable std::string m_formatted;                                       \
    }
