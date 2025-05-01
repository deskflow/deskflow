/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2012 - 2016 Symless Ltd.
 * SPDX-FileCopyrightText: (C) 2004 Chris Schoeneman
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#pragma once

#include "platform/XWindowsClipboard.h"

//! Convert to/from some text encoding
class XWindowsClipboardAnyBitmapConverter : public IXWindowsClipboardConverter
{
public:
  XWindowsClipboardAnyBitmapConverter() = default;
  ~XWindowsClipboardAnyBitmapConverter() override = default;

  // IXWindowsClipboardConverter overrides
  IClipboard::EFormat getFormat() const override;
  Atom getAtom() const override = 0;
  int getDataSize() const override;
  std::string fromIClipboard(const std::string &) const override;
  std::string toIClipboard(const std::string &) const override;

protected:
  //! Convert from IClipboard format
  /*!
  Convert raw BGR pixel data to another image format.
  */
  virtual std::string doBGRFromIClipboard(const uint8_t *bgrData, uint32_t w, uint32_t h) const = 0;

  //! Convert from IClipboard format
  /*!
  Convert raw BGRA pixel data to another image format.
  */
  virtual std::string doBGRAFromIClipboard(const uint8_t *bgrData, uint32_t w, uint32_t h) const = 0;

  //! Convert to IClipboard format
  /*!
  Convert an image into raw BGR or BGRA image data and store the
  width, height, and image depth (24 or 32).
  */
  virtual std::string doToIClipboard(const std::string &, uint32_t &w, uint32_t &h, uint32_t &depth) const = 0;
};
