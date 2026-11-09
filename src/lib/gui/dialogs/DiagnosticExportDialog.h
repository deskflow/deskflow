// SPDX-FileCopyrightText: (C) 2026 Deskflow Developers
// SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
#pragma once
#include <QDialog>
class QLabel;
class DiagnosticExportDialog : public QDialog
{
  Q_OBJECT
public:
  explicit DiagnosticExportDialog(const QByteArray &snapshot, QWidget *parent = nullptr);
  static QByteArray sanitized(const QByteArray &snapshot);
  static bool saveNewFile(const QString &path, const QByteArray &sanitizedSnapshot);
  QByteArray payload() const
  {
    return m_payload;
  }

private:
  QByteArray m_payload;
  QLabel *m_result;
};
