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
#include "arch/IArchNetwork.h"

//! Network address type
/*!
This class represents a network address.
*/
class NetworkAddress {
public:
    /*!
    Constructs the invalid address
    */
    NetworkAddress ();

    /*!
    Construct the wildcard address with the given port.  \c port must
    not be zero.
    */
    NetworkAddress (int port);

    /*!
    Construct the network address for the given \c hostname and \c port.
    If \c hostname can be parsed as a numerical address then that's how
    it's used, otherwise it's used as a host name.  If \c hostname ends
    in ":[0-9]+" then that suffix is extracted and used as the port,
    overridding the port parameter.  The resulting port must be a valid
    port number (zero is not a valid port number) otherwise \c XSocketAddress
    is thrown with an error of \c XSocketAddress::kBadPort.  The hostname
    is not resolved by the c'tor;  use \c resolve to do that.
    */
    NetworkAddress (const String& hostname, int port);

    NetworkAddress (const NetworkAddress&);

    ~NetworkAddress ();

    NetworkAddress& operator= (const NetworkAddress&);

    //! @name manipulators
    //@{

    //! Resolve address
    /*!
    Resolves the hostname to an address.  This can be done any number of
    times and is done automatically by the c'tor taking a hostname.
    Throws XSocketAddress if resolution is unsuccessful, after which
    \c isValid returns false until the next call to this method.
    */
    void resolve ();

    //@}
    //! @name accessors
    //@{

    //! Check address equality
    /*!
    Returns true if this address is equal to \p address.
    */
    bool operator== (const NetworkAddress& address) const;

    //! Check address inequality
    /*!
    Returns true if this address is not equal to \p address.
    */
    bool operator!= (const NetworkAddress& address) const;

    //! Check address validity
    /*!
    Returns true if this is not the invalid address.
    */
    bool isValid () const;

    //! Get address
    /*!
    Returns the address in the platform's native network address
    structure.
    */
    const ArchNetAddress& getAddress () const;

    //! Get port
    /*!
    Returns the port passed to the c'tor as a suffix to the hostname,
    if that existed, otherwise as passed directly to the c'tor.
    */
    int getPort () const;

    //! Get hostname
    /*!
    Returns the hostname passed to the c'tor sans any port suffix.
    */
    String getHostname () const;

    //@}

private:
    void checkPort ();

private:
    ArchNetAddress m_address;
    String m_hostname;
    int m_port;
};
