/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2012 - 2016 Symless Ltd.
 * SPDX-FileCopyrightText: (C) 2002 Chris Schoeneman
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#pragma once

#include "arch/XArch.h"

//! Lazy error message string evaluation for unix
class XArchEvalUnix : public XArchEval
{
public:
  explicit XArchEvalUnix(int error) : m_error(error)
  {
  }
  ~XArchEvalUnix() noexcept override = default;

  std::string eval() const override;

private:
  int m_error;
};
