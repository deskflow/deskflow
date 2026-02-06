/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2025 Deskflow Developers
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#pragma once

#include "deskflow/ClipboardTypes.h"
#include "deskflow/IClipboard.h"

namespace deskflow {

//! EI/Portal clipboard implementation
/*!
This class implements a clipboard for EI (libei) and portal-based input capture.
It stores clipboard data in memory for the specified clipboard ID.
*/
class EiClipboard : public IClipboard
{
public:
  explicit EiClipboard(ClipboardID id);
  ~EiClipboard() override = default;

  //! Get clipboard ID
  ClipboardID getID() const
  {
    return m_id;
  }

  //! @name IClipboard overrides
  //@{
  bool empty() override;
  void add(Format, const std::string &data) override;
  bool open(Time) const override;
  void close() const override;
  Time getTime() const override;
  bool has(Format) const override;
  std::string get(Format) const override;
  //@}

private:
  ClipboardID m_id;
  mutable bool m_open = false;
  mutable Time m_time = 0;
  bool m_owner = false;
  Time m_timeOwned = 0;
  bool m_added[static_cast<int>(Format::TotalFormats)] = {false, false, false};
  std::string m_data[static_cast<int>(Format::TotalFormats)] = {"", "", ""};
};

} // namespace deskflow
