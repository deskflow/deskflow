/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2012 - 2016 Symless Ltd.
 * SPDX-FileCopyrightText: (C) 2002 Chris Schoeneman
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#pragma once

#include "base/EventTypes.h"
#include "common/stdlist.h"
#include "common/stdvector.h"

//! FIFO of bytes
/*!
This class maintains a FIFO (first-in, last-out) buffer of bytes.
*/
class StreamBuffer
{
public:
  StreamBuffer();
  ~StreamBuffer();

  //! @name manipulators
  //@{

  //! Read data without removing from buffer
  /*!
  Return a pointer to memory with the next \c n bytes in the buffer
  (which must be <= getSize()).  The caller must not modify the returned
  memory nor delete it.
  */
  const void *peek(uint32_t n);

  //! Discard data
  /*!
  Discards the next \c n bytes.  If \c n >= getSize() then the buffer
  is cleared.
  */
  void pop(uint32_t n);

  //! Write data to buffer
  /*!
  Appends \c n bytes from \c data to the buffer.
  */
  void write(const void *data, uint32_t n);

  //@}
  //! @name accessors
  //@{

  //! Get size of buffer
  /*!
  Returns the number of bytes in the buffer.
  */
  uint32_t getSize() const;

  //@}

private:
  static const uint32_t kChunkSize;

  using Chunk = std::vector<uint8_t>;
  using ChunkList = std::list<Chunk>;

  ChunkList m_chunks;
  uint32_t m_size;
  uint32_t m_headUsed;
};
