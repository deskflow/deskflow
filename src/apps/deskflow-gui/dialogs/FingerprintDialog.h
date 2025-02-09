/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2025 Deskflow Developers
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#pragma once

#include "net/FingerprintData.h"

#include <QDialog>
#include <QDialogButtonBox>

namespace Ui {
class FingerprintDialog;
}

enum FingerprintDialogMode
{
  Local,
  Remote
};

class FingerprintDialog : public QDialog
{
  Q_OBJECT

public:
  explicit FingerprintDialog(
      QWidget *parent = nullptr, const QList<deskflow::FingerprintData> &fingerprints = {},
      FingerprintDialogMode mode = FingerprintDialogMode::Local
  );
  ~FingerprintDialog();

private:
  Ui::FingerprintDialog *ui = nullptr;
};
