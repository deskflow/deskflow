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
#include "base/ELevel.h"

//! Interface for architecture dependent logging
/*!
This interface defines the logging operations required by
synergy.  Each architecture must implement this interface.
*/
class IArchLog : public IInterface {
public:
    //! @name manipulators
    //@{

    //! Open the log
    /*!
    Opens the log for writing.  The log must be opened before being
    written to.
    */
    virtual void openLog (const char* name) = 0;

    //! Close the log
    /*!
    Close the log.
    */
    virtual void closeLog () = 0;

    //! Show the log
    /*!
    Causes the log to become visible.  This generally only makes sense
    for a log in a graphical user interface.  Other implementations
    will do nothing.  Iff \p showIfEmpty is \c false then the implementation
    may optionally only show the log if it's not empty.
    */
    virtual void showLog (bool showIfEmpty) = 0;

    //! Write to the log
    /*!
    Writes the given string to the log with the given level.
    */
    virtual void writeLog (ELevel, const char*) = 0;

    //@}
};
