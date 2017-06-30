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

#define WIN32_LEAN_AND_MEAN
#include <Windows.h>

//! Lazy error message string evaluation for windows
class XArchEvalWindows : public XArchEval {
public:
    XArchEvalWindows () : m_error (GetLastError ()) {
    }
    XArchEvalWindows (DWORD error) : m_error (error) {
    }
    virtual ~XArchEvalWindows () {
    }

    virtual std::string eval () const;

private:
    DWORD m_error;
};

//! Lazy error message string evaluation for winsock
class XArchEvalWinsock : public XArchEval {
public:
    XArchEvalWinsock (int error) : m_error (error) {
    }
    virtual ~XArchEvalWinsock () {
    }

    virtual std::string eval () const;

private:
    int m_error;
};
