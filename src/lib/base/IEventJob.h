/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2012 - 2016 Symless Ltd.
 * SPDX-FileCopyrightText: (C) 2004 Chris Schoeneman
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#pragma once

#include "common/IInterface.h"

class Event;

//! Event handler interface
/*!
An event job is an interface for executing a event handler.
*/
class IEventJob : public IInterface
{
public:
  //! Run the job
  virtual void run(const Event &) = 0;
};
