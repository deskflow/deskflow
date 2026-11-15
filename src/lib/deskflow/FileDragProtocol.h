/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2026 Deskflow Developers
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#pragma once

#include "deskflow/FileDragTypes.h"

#include <cstdint>
#include <string>

namespace deskflow {
class IStream;
}

namespace deskflow::filedrag {

//! True if the 4 byte \p code is one of the kMsgDFileDrag* codes
bool isFileDragCode(const uint8_t *code);

//! Writes \p message to \p stream in its kMsgDFileDrag* wire format
void writeMessage(deskflow::IStream *stream, const Message &message);

/**
 * @brief Reads the arguments of a file drag message
 *
 * The 4 byte \p code must already have been read from \p stream and must satisfy
 * isFileDragCode(). Returns false (with \p error) if the arguments are malformed or out
 * of range; the caller should then treat the peer as broken.
 */
bool readMessage(deskflow::IStream *stream, const uint8_t *code, Message &message, std::string *error = nullptr);

} // namespace deskflow::filedrag
