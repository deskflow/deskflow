/*
 * synergy -- mouse and keyboard sharing utility
 * Copyright (C) 2015 Synergy Ltd.
 *
 * This package is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * found in the file LICENSE that should have accompanied this file.
 *
 * This package is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#pragma once

#include "license/License.h"
#include "license/ProductEdition.h"

#include <QObject>

/**
 * @brief A convenience wrapper for `License` that provides Qt signals, etc.
 */
class LicenseHandler : public QObject {
  Q_OBJECT
  using License = synergy::license::License;
  using SerialKey = synergy::license::SerialKey;

public:
  class NoticeError : public std::runtime_error {
  public:
    NoticeError() : std::runtime_error("could not create notice") {}
  };
  class EmptySerialKeyError : public std::runtime_error {
  public:
    EmptySerialKeyError() : std::runtime_error("serial key is empty") {}
  };
  enum class ChangeSerialKeyResult {
    kSuccess,
    kFatal,
    kUnchanged,
    kInvalid,
    kExpired,
  };

  Edition productEdition() const;
  const License &license() const;
  void validate() const;
  QString validLicenseNotice() const;
  QString productName() const;
  ChangeSerialKeyResult changeSerialKey(const QString &hexString);

signals:
  void serialKeyChanged(const QString &serialKey) const;
  void invalidLicense() const;

private:
  QString validTrialNotice() const;
  QString validSubscriptionNotice() const;

  License m_license = License::invalid();
};
