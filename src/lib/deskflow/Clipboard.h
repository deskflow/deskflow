/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2012 - 2016 Symless Ltd.
 * SPDX-FileCopyrightText: (C) 2002 Chris Schoeneman
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#pragma once

#include "deskflow/IClipboard.h"

//! Memory buffer clipboard
/*!
This class implements a clipboard that stores data in memory.
*/
class Clipboard : public IClipboard
{
public:
  Clipboard();
  ~Clipboard() override = default;

  //! @name manipulators
  //@{

  //! Unmarshall clipboard data
  /*!
  Extract marshalled clipboard data and store it in this clipboard.
  Sets the clipboard time to \c time.
  */
  void unmarshall(const std::string &data, Time time);

  //@}
  //! @name accessors
  //@{

  //! Marshall clipboard data
  /*!
  Merge this clipboard's data into a single buffer that can be later
  unmarshalled to restore the clipboard and return the buffer.
  */
  std::string marshall() const;

  //@}

  // IClipboard overrides
  bool empty() final;
  void add(Format, const std::string &data) override;
  bool open(Time) const final;
  void close() const override;
  Time getTime() const override;
  bool has(Format) const override;
  std::string get(Format) const override;

private:
  mutable bool m_open = false;
  mutable Time m_time;
  bool m_owner = false;
  Time m_timeOwned;
  bool m_added[static_cast<int>(Format::TotalFormats)] = {false, false, false};
  std::string m_data[static_cast<int>(Format::TotalFormats)] = {"", "", ""};
};
