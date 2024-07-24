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

class LicenseDisplay : public QObject {
  Q_OBJECT
  using License = synergy::license::License;

public:
  class NoticeError : public std::runtime_error {
  public:
    NoticeError() : std::runtime_error("Could not create notice") {}
  };

  Edition productEdition() const;
  const License &license() const;
  void validateLicense() const;
  QString noticeMessage() const;
  QString productName() const;

  /// @return true if the serial key was set successfully
  bool setLicense(const License &serialKey, bool acceptExpired = false);

signals:
  void serialKeyChanged(const QString &serialKey) const;
  void editionChanged(Edition) const;
  void invalidLicense() const;

private:
  QString getTrialNotice() const;
  QString getTimeLimitedNotice() const;

  // TODO: why accept expired?
  bool isValid(const License &license, bool acceptExpired) const;

  License m_license;
};
