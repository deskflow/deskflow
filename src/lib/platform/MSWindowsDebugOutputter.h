/*
 * synergy -- mouse and keyboard sharing utility
 * Copyright (C) 2012-2016 Symless Ltd.
 * Copyright (C) 2012 Nick Bolton
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

#include "base/ILogOutputter.h"

//! Write log to debugger
/*!
This outputter writes output to the debugger. In Visual Studio, this
can be seen in the Output window.
*/
class MSWindowsDebugOutputter : public ILogOutputter {
public:
    MSWindowsDebugOutputter ();
    virtual ~MSWindowsDebugOutputter ();

    // ILogOutputter overrides
    virtual void open (const char* title);
    virtual void close ();
    virtual void show (bool showIfEmpty);
    virtual bool write (ELevel level, const char* message);
    virtual void flush ();
};
