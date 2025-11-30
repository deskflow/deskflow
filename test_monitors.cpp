/*
 * Simple test program to verify monitor detection
 */

#include "src/lib/gui/core/MonitorDetection.h"
#include "src/lib/gui/config/Screen.h"
#include <QApplication>
#include <QDebug>

int main(int argc, char *argv[])
{
  QApplication app(argc, argv);
  
  qDebug() << "=== Monitor Detection Test ===";
  
  auto monitors = deskflow::gui::detectMonitors();
  
  qDebug() << "Detected" << monitors.size() << "monitor(s):";
  
  for (int i = 0; i < monitors.size(); ++i) {
    const auto &m = monitors[i];
    qDebug() << "  Monitor" << (i+1) << ":"
             << m.name
             << "at" << m.geometry
             << (m.isPrimary ? "(PRIMARY)" : "");
  }

  Screen testScreen("TestComputer");
  deskflow::gui::populateScreenMonitors(testScreen);
  qDebug() << "\nScreen" << testScreen.name() << "has" << testScreen.monitors().size() << "monitor(s)";
  qDebug() << "Has multiple monitors:" << testScreen.hasMultipleMonitors();
  return 0;
}
