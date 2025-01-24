/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2012 - 2016 Symless Ltd.
 * SPDX-FileCopyrightText: (C) 2002 Chris Schoeneman
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#pragma once

#include "arch/IArchString.h"
#include "common/common.h"
#include <string>

//! Unicode utility functions
/*!
This class provides functions for converting between various Unicode
encodings and the current locale encoding.
*/
class Unicode
{
public:
  //! @name accessors
  //@{

  //! Test UTF-8 string for validity
  /*!
  Returns true iff the string contains a valid sequence of UTF-8
  encoded characters.
  */
  static bool isUTF8(const std::string &);

  //! Convert from UTF-8 to UCS-2 encoding
  /*!
  Convert from UTF-8 to UCS-2.  If errors is not NULL then *errors
  is set to true iff any character could not be encoded in UCS-2.
  Decoding errors do not set *errors.
  */
  static std::string UTF8ToUCS2(const std::string &, bool *errors = NULL);

  //! Convert from UTF-8 to UCS-4 encoding
  /*!
  Convert from UTF-8 to UCS-4.  If errors is not NULL then *errors
  is set to true iff any character could not be encoded in UCS-4.
  Decoding errors do not set *errors.
  */
  static std::string UTF8ToUCS4(const std::string &, bool *errors = NULL);

  //! Convert from UTF-8 to UTF-16 encoding
  /*!
  Convert from UTF-8 to UTF-16.  If errors is not NULL then *errors
  is set to true iff any character could not be encoded in UTF-16.
  Decoding errors do not set *errors.
  */
  static std::string UTF8ToUTF16(const std::string &, bool *errors = NULL);

  //! Convert from UTF-8 to UTF-32 encoding
  /*!
  Convert from UTF-8 to UTF-32.  If errors is not NULL then *errors
  is set to true iff any character could not be encoded in UTF-32.
  Decoding errors do not set *errors.
  */
  static std::string UTF8ToUTF32(const std::string &, bool *errors = NULL);

  //! Convert from UTF-8 to the current locale encoding
  /*!
  Convert from UTF-8 to the current locale encoding.  If errors is not
  NULL then *errors is set to true iff any character could not be encoded.
  Decoding errors do not set *errors.
  */
  static std::string UTF8ToText(const std::string &, bool *errors = NULL);

  //! Convert from UCS-2 to UTF-8
  /*!
  Convert from UCS-2 to UTF-8.  If errors is not NULL then *errors is
  set to true iff any character could not be decoded.
  */
  static std::string UCS2ToUTF8(const std::string &, bool *errors = NULL);

  //! Convert from UCS-4 to UTF-8
  /*!
  Convert from UCS-4 to UTF-8.  If errors is not NULL then *errors is
  set to true iff any character could not be decoded.
  */
  static std::string UCS4ToUTF8(const std::string &, bool *errors = NULL);

  //! Convert from UTF-16 to UTF-8
  /*!
  Convert from UTF-16 to UTF-8.  If errors is not NULL then *errors is
  set to true iff any character could not be decoded.
  */
  static std::string UTF16ToUTF8(const std::string &, bool *errors = NULL);

  //! Convert from UTF-32 to UTF-8
  /*!
  Convert from UTF-32 to UTF-8.  If errors is not NULL then *errors is
  set to true iff any character could not be decoded.
  */
  static std::string UTF32ToUTF8(const std::string &, bool *errors = NULL);

  //! Convert from the current locale encoding to UTF-8
  /*!
  Convert from the current locale encoding to UTF-8.  If errors is not
  NULL then *errors is set to true iff any character could not be decoded.
  */
  static std::string textToUTF8(
      const std::string &, bool *errors = nullptr,
      IArchString::EWideCharEncoding encoding = IArchString::kPlatformDetermined
  );

  //@}

private:
  // convert UTF8 to wchar_t string (using whatever encoding is native
  // to the platform).  caller must delete[] the returned string.  the
  // string is *not* nul terminated;  the length (in characters) is
  // returned in size.
  static wchar_t *UTF8ToWideChar(const std::string &, uint32_t &size, bool *errors);

  // convert nul terminated wchar_t string (in platform's native
  // encoding) to UTF8.
  static std::string wideCharToUTF8(
      const wchar_t *, uint32_t size, bool *errors,
      IArchString::EWideCharEncoding encoding = IArchString::kPlatformDetermined
  );

  // internal conversion to UTF8
  static std::string doUCS2ToUTF8(const uint8_t *src, uint32_t n, bool *errors);
  static std::string doUCS4ToUTF8(const uint8_t *src, uint32_t n, bool *errors);
  static std::string doUTF16ToUTF8(const uint8_t *src, uint32_t n, bool *errors);
  static std::string doUTF32ToUTF8(const uint8_t *src, uint32_t n, bool *errors);

  // convert characters to/from UTF8
  static uint32_t fromUTF8(const uint8_t *&src, uint32_t &size);
  static void toUTF8(std::string &dst, uint32_t c, bool *errors);

private:
  static uint32_t s_invalid;
  static uint32_t s_replacement;
};
