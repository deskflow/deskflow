// SPDX-FileCopyrightText: (C) 2026 Deskflow Developers
// SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
#pragma once
#include "gui/IdentityBindings.h"
#include <QDeadlineTimer>
#include <QDialog>
class QCheckBox;
class QLineEdit;
class QPushButton;
class QLabel;
// A review result only. It neither authenticates TLS nor writes a trust database.
class PairingReviewDialog : public QDialog
{
  Q_OBJECT
public:
  PairingReviewDialog(
      deskflow::gui::IdentityBindings &bindings, const QString &localFingerprint, QWidget *parent = nullptr
  );
  void accept() override;
  void reject() override;
  void expireReview();
  QString displayName() const;

private:
  deskflow::gui::IdentityBindings &m_bindings;
  quint64 m_generation;
  QLineEdit *m_name;
  QCheckBox *m_compared;
  QPushButton *m_confirm;
  QLabel *m_error;
  QDeadlineTimer m_deadline{120000, Qt::PreciseTimer};
};
