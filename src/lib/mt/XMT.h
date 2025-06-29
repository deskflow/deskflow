/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2012 - 2016 Symless Ltd.
 * SPDX-FileCopyrightText: (C) 2002 Chris Schoeneman
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#pragma once

#include "base/XBase.h"

/**
 * @brief XMT generic multithreading exception
 */
class XMT : public XBase
{
  using XBase::XBase;
};

/**
 * @brief XMTThreadUnavailable - Thrown when a thread cannot be created.
 */
class XMTThreadUnavailable : public XMT
{
  using XMT::XMT;

protected:
  std::string getWhat() const throw() override;
};
