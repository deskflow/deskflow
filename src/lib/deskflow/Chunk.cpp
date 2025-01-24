/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2015 - 2016 Symless Ltd.
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#include "deskflow/Chunk.h"
#include "base/String.h"

Chunk::Chunk(size_t size) : m_dataSize(0)
{
  m_chunk = new char[size];
  memset(m_chunk, 0, size);
}

Chunk::~Chunk()
{
  delete[] m_chunk;
}
