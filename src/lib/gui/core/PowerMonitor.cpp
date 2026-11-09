// SPDX-FileCopyrightText: (C) 2026 Deskflow Developers
// SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
#include "PowerMonitor.h"
#include <QCoreApplication>
#ifdef Q_OS_MACOS
#include "gui/OSXHelpers.h"
#elif defined(Q_OS_WIN)
#include <windows.h>
#endif
namespace deskflow::gui {
PowerMonitor::PowerMonitor(QObject *parent) : QObject(parent)
{
#ifndef DESKFLOW_LAB_BUILD
#ifdef Q_OS_MACOS
  observeOSXPower(this, [this](bool sleeping) { observe(sleeping); });
#elif defined(Q_OS_WIN)
  QCoreApplication::instance()->installNativeEventFilter(this);
#endif
#endif
}
PowerMonitor::~PowerMonitor()
{
  if (QCoreApplication::instance())
    QCoreApplication::instance()->removeNativeEventFilter(this);
}
void PowerMonitor::observe(bool sleeping)
{
  if (m_sleeping == sleeping)
    return;
  m_sleeping = sleeping;
  Q_EMIT sleepingChanged(sleeping);
}
bool PowerMonitor::nativeEventFilter(const QByteArray &, void *message, qintptr *)
{
#ifdef Q_OS_WIN
  const auto *m = static_cast<MSG *>(message);
  if (m && m->message == WM_POWERBROADCAST) {
    if (m->wParam == PBT_APMSUSPEND)
      observe(true);
    else if (m->wParam == PBT_APMRESUMEAUTOMATIC || m->wParam == PBT_APMRESUMESUSPEND)
      observe(false);
  }
#else
  Q_UNUSED(message);
#endif
  return false;
}
} // namespace deskflow::gui
