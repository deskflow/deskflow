// SPDX-FileCopyrightText: (C) 2026 Deskflow Developers
// SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception

#pragma once

#include <QDialog>

class QLabel;
class QPushButton;

namespace deskflow::gui {

// AX exposes granted/not granted, not a reliable denied/not-yet-requested split.
// Tests inject this interface and never link or call the native permission API.
class AccessibilityPermissionProvider
{
public:
  virtual ~AccessibilityPermissionProvider() = default;
  virtual bool isGranted() const = 0;
  virtual void request() = 0;
};

class CandidatePermissionDialog final : public QDialog
{
  Q_OBJECT

public:
  CandidatePermissionDialog(
      AccessibilityPermissionProvider &provider, int macOSMajorVersion, QWidget *parent = nullptr
  );
  void done(int result) override;

private:
  void refreshPermission(bool releasePendingRequest);
  void requestPermission();

  AccessibilityPermissionProvider &m_provider;
  QLabel *m_status = nullptr;
  QPushButton *m_requestButton = nullptr;
  QPushButton *m_continueButton = nullptr;
  bool m_requestPending = false;
  bool m_finished = false;
};

} // namespace deskflow::gui
