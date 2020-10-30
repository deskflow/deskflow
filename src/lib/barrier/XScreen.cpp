/*
 * barrier -- mouse and keyboard sharing utility
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

#include "barrier/XScreen.h"

//
// XScreenOpenFailure
//

std::string XScreenOpenFailure::getWhat() const noexcept
{
    return format("XScreenOpenFailure", "unable to open screen");
}


//
// XScreenXInputFailure
//

std::string XScreenXInputFailure::getWhat() const noexcept
{
    return "";
}


//
// XScreenUnavailable
//

XScreenUnavailable::XScreenUnavailable(double timeUntilRetry) :
    m_timeUntilRetry(timeUntilRetry)
{
    // do nothing
}

XScreenUnavailable::~XScreenUnavailable() noexcept
{
    // do nothing
}

double
XScreenUnavailable::getRetryTime() const
{
    return m_timeUntilRetry;
}

std::string XScreenUnavailable::getWhat() const noexcept
{
    return format("XScreenUnavailable", "unable to open screen");
}
