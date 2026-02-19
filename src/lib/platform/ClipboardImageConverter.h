/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2026 Deskflow Developers
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#pragma once

#include <string>

namespace deskflow::platform::clipboard {

//! Convert Deskflow bitmap data (DIB) to an encoded image format (for example PNG, TIFF, BMP).
std::string encodeBitmapToImage(const std::string &bitmapData, const char *format);

//! Convert encoded image bytes (for example PNG, TIFF, BMP) to Deskflow bitmap data (DIB).
std::string decodeImageToBitmap(const std::string &imageData, const char *formatHint = nullptr);

} // namespace deskflow::platform::clipboard
