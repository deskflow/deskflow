/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2012 - 2016 Symless Ltd.
 * SPDX-FileCopyrightText: (C) 2002 Chris Schoeneman
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#pragma once

#include "Common.h"
class QString;
//! Base class of interfaces
/*!
This is the base class of all interface classes.  An interface class has
only pure virtual methods.
*/
class IInterface
{
public:
  //! Interface destructor does nothing
  virtual ~IInterface() = default;
};
