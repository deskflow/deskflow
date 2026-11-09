// SPDX-FileCopyrightText: (C) 2026 Deskflow Developers
// SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
#pragma once
#include <QAbstractNativeEventFilter>
#include <QObject>
namespace deskflow::gui {
class PowerMonitor : public QObject, public QAbstractNativeEventFilter
{
  Q_OBJECT
public:
  explicit PowerMonitor(QObject *parent = nullptr);
  ~PowerMonitor() override;
  bool nativeEventFilter(const QByteArray &, void *, qintptr *) override;
  void observe(bool sleeping);
Q_SIGNALS:
  void sleepingChanged(bool sleeping);

private:
  bool m_sleeping = false;
};
} // namespace deskflow::gui
