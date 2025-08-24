/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2025 Deskflow Developers
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#pragma once

#include "net/Fingerprint.h"

#include <QDialog>
#include <QDialogButtonBox>

class QLabel;
class FingerprintPreview;

enum class FingerprintDialogMode
{
  Local,
  Client,
  Server
};

class FingerprintDialog : public QDialog
{
  Q_OBJECT

public:
  explicit FingerprintDialog(
      QWidget *parent = nullptr, const Fingerprint &localFingerprint = {},
      FingerprintDialogMode mode = FingerprintDialogMode::Local, const Fingerprint &remoteFingerprint = {}
  );
  ~FingerprintDialog() override = default;

private:
  QLayout *makeLocalLayout(const Fingerprint &localFingerprint = {});
  QLayout *makeCompareLayout(
      const Fingerprint &localFingerprint = {}, bool isServer = true, const Fingerprint &remoteFingerprint = {}
  );
  void togglePreviewMode(bool hashMode);
  void updateModeButton(bool hashMode) const;
  QLabel *m_lblHeader = nullptr;
  QLabel *m_lblFooter = nullptr;
  FingerprintPreview *m_localPreview = nullptr;
  FingerprintPreview *m_remotePreview = nullptr;
  QDialogButtonBox *m_buttonBox = nullptr;
};
