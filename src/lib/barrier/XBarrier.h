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

#pragma once

#include "base/XBase.h"

//! Generic barrier exception
XBASE_SUBCLASS(XBarrier, XBase);

//! Subscription error
/*!
Thrown when there is a problem with the subscription.
*/
XBASE_SUBCLASS(XSubscription, XBarrier);

//! Client error exception
/*!
Thrown when the client fails to follow the protocol.
*/
XBASE_SUBCLASS_WHAT(XBadClient, XBarrier);

//! Incompatible client exception
/*!
Thrown when a client attempting to connect has an incompatible version.
*/
class XIncompatibleClient : public XBarrier {
public:
    XIncompatibleClient(int major, int minor);

    //! @name accessors
    //@{

    //! Get client's major version number
    int getMajor() const noexcept;
    //! Get client's minor version number
    int getMinor() const noexcept;

    //@}

protected:
    virtual std::string getWhat() const noexcept;

private:
    int                    m_major;
    int                    m_minor;
};

//! Client already connected exception
/*!
Thrown when a client attempting to connect is using the same name as
a client that is already connected.
*/
class XDuplicateClient : public XBarrier {
public:
    XDuplicateClient(const std::string& name);
    virtual ~XDuplicateClient() noexcept { }

    //! @name accessors
    //@{

    //! Get client's name
    virtual const std::string& getName() const noexcept;

    //@}

protected:
    virtual std::string getWhat() const noexcept;

private:
    std::string m_name;
};

//! Client not in map exception
/*!
Thrown when a client attempting to connect is using a name that is
unknown to the server.
*/
class XUnknownClient : public XBarrier {
public:
    XUnknownClient(const std::string& name);
    virtual ~XUnknownClient() noexcept { }

    //! @name accessors
    //@{

    //! Get the client's name
    virtual const std::string& getName() const noexcept;

    //@}

protected:
    virtual std::string getWhat() const noexcept;

private:
    std::string                m_name;
};

//! Generic exit eception
/*!
Thrown when we want to abort, with the opportunity to clean up. This is a
little bit of a hack, but it's a better way of exiting, than just calling
exit(int).
*/
class XExitApp : public XBarrier {
public:
    XExitApp(int code);
    virtual ~XExitApp() noexcept { }

    //! Get the exit code
    int getCode() const noexcept;

protected:
    virtual std::string getWhat() const noexcept;

private:
    int    m_code;
};
