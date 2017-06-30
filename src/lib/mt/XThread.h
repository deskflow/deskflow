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

#include "arch/XArch.h"

//! Thread exception to exit
/*!
Thrown by Thread::exit() to exit a thread.  Clients of Thread
must not throw this type but must rethrow it if caught (by
XThreadExit, XThread, or ...).
*/
class XThreadExit : public XThread {
public:
    //! \c result is the result of the thread
    XThreadExit (void* result) : m_result (result) {
    }
    ~XThreadExit () {
    }

public:
    void* m_result;
};
