/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2015 - 2016 Symless Ltd.
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#pragma once

#include "common/common.h"
#include <base/EventTypes.h>

class Chunk : public EventData
{
public:
  Chunk(size_t size);
  Chunk(Chunk const &) = delete;
  Chunk(Chunk &&) = delete;
  ~Chunk() override;

  Chunk &operator=(Chunk const &) = delete;
  Chunk &operator=(Chunk &&) = delete;

public:
  size_t m_dataSize;
  char *m_chunk;
};
