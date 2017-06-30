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

#include "common/IInterface.h"
#include "common/basic_types.h"

#include <stdarg.h>

//! Interface for architecture dependent string operations
/*!
This interface defines the string operations required by
synergy.  Each architecture must implement this interface.
*/
class IArchString : public IInterface {
public:
    virtual ~IArchString ();

    //! Wide character encodings
    /*!
    The known wide character encodings
    */
    enum EWideCharEncoding {
        kUCS2,  //!< The UCS-2 encoding
        kUCS4,  //!< The UCS-4 encoding
        kUTF16, //!< The UTF-16 encoding
        kUTF32  //!< The UTF-32 encoding
    };

    //! @name manipulators
    //@{

    //! printf() to limited size buffer with va_list
    /*!
    This method is equivalent to vsprintf() except it will not write
    more than \c n bytes to the buffer, returning -1 if the output
    was truncated and the number of bytes written not including the
    trailing NUL otherwise.
    */
    virtual int vsnprintf (char* str, int size, const char* fmt, va_list ap);

    //! Convert multibyte string to wide character string
    virtual int
    convStringMBToWC (wchar_t*, const char*, UInt32 n, bool* errors);

    //! Convert wide character string to multibyte string
    virtual int
    convStringWCToMB (char*, const wchar_t*, UInt32 n, bool* errors);

    //! Return the architecture's native wide character encoding
    virtual EWideCharEncoding getWideCharEncoding () = 0;

    //@}
};
