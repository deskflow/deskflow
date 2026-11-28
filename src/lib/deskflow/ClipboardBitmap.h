/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2026 Deskflow Developers
 * SPDX-FileCopyrightText: (C) 2014 - 2016, 2023 - 2026 Symless Ltd
 * SPDX-FileCopyrightText: (C) 2014 Ryan Chapman
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#pragma once

#include <QtTypes>

#include <string>

//! Converts between BMP files (what platform clipboards hold) and DIBs (what IClipboard::Format::Bitmap holds)
namespace deskflow::bmp {

//! Offset from the start of a DIB to its pixels (header, bitfield masks and palette), or 0 if not a valid DIB
quint32 dibPixelOffset(const quint8 *dib, qsizetype dibSize);

//! Wraps a DIB in a BMP file header; returns an empty string if \p dib is too small
std::string dibToBmp(const std::string &dib);

//! Strips the BMP file header, keeping the whole DIB header and dropping any gap before the pixels;
//! returns an empty string if \p bmp is not a BMP file
std::string bmpToDib(const std::string &bmp);

} // namespace deskflow::bmp
