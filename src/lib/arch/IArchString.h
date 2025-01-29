/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2025 Deskflow Developers
 * SPDX-FileCopyrightText: (C) 2012 - 2016 Symless Ltd.
 * SPDX-FileCopyrightText: (C) 2002 Chris Schoeneman
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#pragma once

#include "common/IInterface.h"
#include "common/common.h"

#include <stdarg.h>

//! Interface for architecture dependent string operations
/*!
This interface defines the string operations required by
deskflow.  Each architecture must implement this interface.
*/
class IArchString : public IInterface
{
public:
  IArchString() = default;
  IArchString(const IArchString &) = delete;
  IArchString(IArchString &&) = delete;
  virtual ~IArchString();

  IArchString &operator=(const IArchString &) = delete;
  IArchString &operator=(IArchString &&) = delete;

  //! Wide character encodings
  /*!
  The known wide character encodings
  */
  enum EWideCharEncoding
  {
    kUCS2,  //!< The UCS-2 encoding
    kUCS4,  //!< The UCS-4 encoding
    kUTF16, //!< The UTF-16 encoding
    kUTF32, //!< The UTF-32 encoding
    kPlatformDetermined
  };

  //! @name manipulators
  //@{

  //! Convert multibyte string to wide character string
  virtual int convStringMBToWC(wchar_t *, const char *, uint32_t n, bool *errors);

  //! Convert wide character string to multibyte string
  virtual int convStringWCToMB(char *, const wchar_t *, uint32_t n, bool *errors);

  //! Return the architecture's native wide character encoding
  virtual EWideCharEncoding getWideCharEncoding() = 0;

  //@}
};
