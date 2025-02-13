/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2025 Deskflow Developers
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#pragma once

#include "net/FingerprintData.h"

#include <QDialog>
#include <QDialogButtonBox>

enum FingerprintDialogMode
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
      QWidget *parent = nullptr, const QList<deskflow::FingerprintData> &fingerprints = {},
      FingerprintDialogMode mode = FingerprintDialogMode::Local
  );
  ~FingerprintDialog() = default;

signals:
  void requestLocalPrintsDialog();

private:
  QLabel *m_lblHeader = nullptr;
  QLabel *m_lblFooter = nullptr;
  FingerprintPreview *m_fingerprintPreview = nullptr;
  QDialogButtonBox *m_buttonBox = nullptr;
};
