/*
 * Deskflow -- mouse and keyboard sharing utility
 * Copyright (C) 2012-2016 Symless Ltd.
 * Copyright (C) 2008 Volker Lanz (vl@fidra.de)
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

#include "deskflow/AppRole.h"
#include "net/FingerprintData.h"
#include <QDialog>
#include <memory>

namespace Ui {
class FingerprintAcceptDialog;
}

class FingerprintAcceptDialog : public QDialog
{
  Q_OBJECT

public:
  explicit FingerprintAcceptDialog(
      QWidget *parent, AppRole type, const deskflow::FingerprintData &fingerprint_sha1,
      const deskflow::FingerprintData &fingerprint_sha256
  );
  ~FingerprintAcceptDialog() override;

private:
  std::unique_ptr<Ui::FingerprintAcceptDialog> ui_;
};
