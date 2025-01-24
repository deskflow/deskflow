/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2012 - 2016 Symless Ltd.
 * SPDX-FileCopyrightText: (C) 2002 Chris Schoeneman
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#pragma once

#include "arch/IArchNetwork.h"
#include "base/EventTypes.h"

//! Network address type
/*!
This class represents a network address.
*/
class NetworkAddress
{
public:
  /*!
  Constructs the invalid address
  */
  NetworkAddress() = default;

  /*!
  Construct the wildcard address with the given port.  \c port must
  not be zero.
  */
  NetworkAddress(int port);

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
  NetworkAddress(const std::string &hostname, int port = 0);

  NetworkAddress(const NetworkAddress &);

  ~NetworkAddress();

  NetworkAddress &operator=(const NetworkAddress &);

  //! @name manipulators
  //@{

  //! Resolve address
  /*!
  Resolves the hostname to an address.  This can be done any number of
  times and is done automatically by the c'tor taking a hostname.
  Throws XSocketAddress if resolution is unsuccessful, after which
  \c isValid returns false until the next call to this method.
  index - determine index of IP we would like to use from resolved addresses
  Returns count of successfully resolved addressed.
  */
  size_t resolve(size_t index = 0);

  //@}
  //! @name accessors
  //@{

  //! Check address equality
  /*!
  Returns true if this address is equal to \p address.
  */
  bool operator==(const NetworkAddress &address) const;

  //! Check address inequality
  /*!
  Returns true if this address is not equal to \p address.
  */
  bool operator!=(const NetworkAddress &address) const;

  //! Check address validity
  /*!
  Returns true if this is not the invalid address.
  */
  bool isValid() const;

  //! Get address
  /*!
  Returns the address in the platform's native network address
  structure.
  */
  const ArchNetAddress &getAddress() const;

  //! Get port
  /*!
  Returns the port passed to the c'tor as a suffix to the hostname,
  if that existed, otherwise as passed directly to the c'tor.
  */
  int getPort() const;

  //! Get hostname
  /*!
  Returns the hostname passed to the c'tor sans any port suffix.
  */
  std::string getHostname() const;

  //@}

private:
  void checkPort();

private:
  ArchNetAddress m_address = nullptr;
  std::string m_hostname;
  int m_port = 0;
};
