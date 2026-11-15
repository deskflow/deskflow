/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2026 Deskflow Developers
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#pragma once

#include "base/Event.h"
#include "deskflow/FileDragTypes.h"

#include <utility>

namespace deskflow::filedrag {

/**
 * @brief Data of an EventTypes::FileDrag event posted by a platform computer
 *
 * The target is the platform computer's event target.
 */
class FileDragEvent : public EventData
{
public:
  enum class Kind
  {
    OfferReady, ///< Primary: a file drag is waiting at a watched edge
    OfferLost,  ///< Primary: it left the edge again (or ended there)
    Send,       ///< Send \c message to the peer
    LoopEnded,  ///< Secondary: the local drag loop ended (\c dropped tells how)
  };

  explicit FileDragEvent(Kind kind, Message message = {}, bool dropped = false)
      : m_kind(kind),
        m_message(std::move(message)),
        m_dropped(dropped)
  {
  }
  ~FileDragEvent() override = default;

  Kind kind() const
  {
    return m_kind;
  }
  const Message &message() const
  {
    return m_message;
  }
  bool dropped() const
  {
    return m_dropped;
  }

private:
  Kind m_kind;
  Message m_message;
  bool m_dropped;
};

} // namespace deskflow::filedrag
