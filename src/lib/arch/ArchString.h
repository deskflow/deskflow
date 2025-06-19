/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2025 Deskflow Developers
 * SPDX-FileCopyrightText: (C) 2012 - 2016 Symless Ltd.
 * SPDX-FileCopyrightText: (C) 2002 Chris Schoeneman
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#pragma once

#include "common/Common.h"
#include "common/IInterface.h"

#include <stdarg.h>

//! Interface for architecture dependent string operations
/*!
This interface defines the string operations required by
deskflow.  Each architecture must implement this interface.
*/
class ArchString : public IInterface
{
public:
  ArchString() = default;
  ArchString(const ArchString &) = delete;
  ArchString(ArchString &&) = delete;
  ~ArchString() override = default;

  ArchString &operator=(const ArchString &) = delete;
  ArchString &operator=(ArchString &&) = delete;

  //! Wide character encodings
  /*!
  The known wide character encodings
  */
  enum class EWideCharEncoding : uint8_t
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
  int convStringMBToWC(wchar_t *, const char *, uint32_t n, bool *errors) const;

  //! Convert wide character string to multibyte string
  int convStringWCToMB(char *, const wchar_t *, uint32_t n, bool *errors) const;

  //! Return the architecture's native wide character encoding
  EWideCharEncoding getWideCharEncoding() const;

  //@}
};
