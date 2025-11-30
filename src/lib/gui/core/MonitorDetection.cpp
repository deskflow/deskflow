/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2025 Deskflow Developers
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#include "MonitorDetection.h"
#include <QScreen>
#include <QDebug>

namespace deskflow::gui {

QVector<MonitorInfo> detectMonitors()
{
  QVector<MonitorInfo> monitors;
  
  qDebug() << "=== Starting monitor detection ===";
  const auto screens = QGuiApplication::screens();
  qDebug() << "Qt detected" << screens.size() << "screen(s)";
  
  if (screens.isEmpty()) {
    qWarning() << "No screens detected by Qt";
    return monitors;
  }

  QScreen *primaryScreen = QGuiApplication::primaryScreen();
  qDebug() << "Primary screen:" << (primaryScreen ? primaryScreen->name() : "NULL");
  
  for (const QScreen *screen : screens) {
    if (!screen) {
      continue;
    }
    
    const QRect geometry = screen->geometry();
    const bool isPrimary = (screen == primaryScreen);
    const QString name = screen->name();
    
    MonitorInfo monitor(
      name,
      geometry.x(),
      geometry.y(),
      geometry.width(),
      geometry.height(),
      isPrimary
    );
    
    monitors.append(monitor);
    
    qDebug() << "Detected monitor:" << name 
             << "at" << geometry.x() << "," << geometry.y()
             << "size" << geometry.width() << "x" << geometry.height()
             << (isPrimary ? "(primary)" : "");
  }
  
  return monitors;
}

void populateScreenMonitors(Screen &screen)
{
  const auto monitors = detectMonitors();
  screen.setMonitors(monitors);
  
  qDebug() << "Populated screen" << screen.name() 
           << "with" << monitors.size() << "monitor(s)";

  for (int i = 0; i < monitors.size(); ++i) {
    const auto &m = monitors[i];
    qDebug() << "  Monitor" << (i + 1) << ":" << m.name
             << "at" << m.geometry.x() << "," << m.geometry.y()
             << "size" << m.geometry.width() << "x" << m.geometry.height()
             << (m.isPrimary ? "(primary)" : "");
  }
}

}

