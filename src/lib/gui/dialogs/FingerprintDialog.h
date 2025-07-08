/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2025 Deskflow Developers
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#pragma once

#include "net/Fingerprint.h"

#include <QDialog>
#include <QDialogButtonBox>

enum class FingerprintDialogMode
{
  Local,
  Client,
  Server
};

class QLabel;
class FingerprintPreview;

class FingerprintDialog : public QDialog
{
  Q_OBJECT

public:
  explicit FingerprintDialog(
      QWidget *parent = nullptr, const Fingerprint &fingerprint = {},
      FingerprintDialogMode mode = FingerprintDialogMode::Local
  );
  ~FingerprintDialog() override = default;

Q_SIGNALS:
  void requestLocalPrintsDialog();

private:
  QLabel *m_lblHeader = nullptr;
  QLabel *m_lblFooter = nullptr;
  FingerprintPreview *m_fingerprintPreview = nullptr;
  QDialogButtonBox *m_buttonBox = nullptr;
};
