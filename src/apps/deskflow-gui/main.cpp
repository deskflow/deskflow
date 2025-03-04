/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2024 Chris Rizzitello <sithord48@gmail.com>
 * SPDX-FileCopyrightText: (C) 2012 - 2024 Symless Ltd.
 * SPDX-FileCopyrightText: (C) 2008 Volker Lanz <vl@fidra.de>
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#include "MainWindow.h"
#include "common/constants.h"
#include "gui/Logger.h"
#include "gui/config/AppConfig.h"
#include "gui/config/ConfigScopes.h"
#include "gui/constants.h"
#include "gui/diagnostic.h"
#include "gui/dotenv.h"
#include "gui/messages.h"
#include "gui/string_utils.h"
#include "gui/style_utils.h"

#include <QApplication>
#include <QDebug>
#include <QGuiApplication>
#include <QLocalSocket>
#include <QMessageBox>
#include <QObject>
#include <QSharedMemory>
#include <QtGlobal>

#if defined(Q_OS_MAC)
#include <Carbon/Carbon.h>
#include <cstdlib>
#endif

#if defined(Q_OS_UNIX) && defined(QT_DEBUG)
#include <QLoggingCategory>
#endif

using namespace deskflow::gui;

class QThreadImpl : public QThread
{
public:
  static void msleep(unsigned long msecs)
  {
    QThread::msleep(msecs);
  }
};

#if defined(Q_OS_MAC)
bool checkMacAssistiveDevices();
#endif

bool hasArg(const QString &arg, const QStringList &args)
{
  return std::ranges::any_of(args, [&arg](const QString &a) { return a == arg; });
}

int main(int argc, char *argv[])
{
#if defined(Q_OS_UNIX) && defined(QT_DEBUG)
  // Fixes Fedora bug where qDebug() messages aren't printed.
  QLoggingCategory::setFilterRules(QStringLiteral("*.debug=true\nqt.*=false"));
#endif

#if defined(Q_OS_MAC)
  /* Workaround for QTBUG-40332 - "High ping when QNetworkAccessManager is
   * instantiated" */
  ::setenv("QT_BEARER_POLL_TIMEOUT", "-1", 1);
#endif

  QCoreApplication::setApplicationName(kAppName);
  QCoreApplication::setOrganizationName(kAppName);
  QGuiApplication::setDesktopFileName(QStringLiteral("org.deskflow.deskflow"));

  // used as a prefix for settings paths, and must not be a url.
  QCoreApplication::setOrganizationDomain(kOrgDomain);

  QApplication app(argc, argv);

  // Create a shared memory segment with a unique key
  // This is to prevent a new instance from running if one is already running
  QSharedMemory sharedMemory("deskflow-gui");

  // Attempt to attach first and detach in order to clean up stale shm chunks
  // This can happen if the previous instance was killed or crashed
  if (sharedMemory.attach())
    sharedMemory.detach();

  // If we can create 1 byte of SHM we are the only instance
  if (!sharedMemory.create(1)) {
    // Ping the running instance to have it show itself
    QLocalSocket socket;
    socket.connectToServer("deskflow-gui", QLocalSocket::ReadOnly);
    if (!socket.waitForConnected()) {
      // If we can't connect to the other instance tell the user its running.
      // This should never happen but just incase we should show something
      QMessageBox::information(nullptr, QObject::tr("Deskflow"), QObject::tr("Deskflow is already running"));
    }
    socket.disconnectFromServer();
    return 0;
  }

#if !defined(Q_OS_MAC)
  // causes dark mode to be used on some DE's
  if (qEnvironmentVariable("XDG_CURRENT_DESKTOP") != QLatin1String("KDE")) {
    QApplication::setStyle("fusion");
  }
#endif

  // Sets the fallback icon path
  setIconFallbackPaths();

  qInstallMessageHandler(deskflow::gui::messages::messageHandler);
  qInfo("%s v%s", kAppName, qPrintable(kVersion));

  dotenv();
  Logger::instance().loadEnvVars();

#if defined(Q_OS_MAC)

  if (app.applicationDirPath().startsWith("/Volumes/")) {
    QString msgBody = QStringLiteral("Please drag %1 to the Applications folder, "
                                     "and open it from there.");
    QMessageBox::information(NULL, kAppName, msgBody.arg(kAppName));
    return 1;
  }

  if (!checkMacAssistiveDevices()) {
    return 1;
  }
#endif

  ConfigScopes configScopes;

  // --no-reset
  QStringList arguments = QCoreApplication::arguments();
  const auto noReset = hasArg("--no-reset", arguments);
  const auto resetEnvVar = strToTrue(qEnvironmentVariable("DESKFLOW_RESET_ALL"));
  if (resetEnvVar && !noReset) {
    diagnostic::clearSettings(configScopes, false);
  }

  AppConfig appConfig(configScopes);

  QObject::connect(
      &configScopes, &ConfigScopes::saving, &appConfig, [&appConfig]() { appConfig.commit(); }, Qt::DirectConnection
  );

  MainWindow mainWindow(configScopes, appConfig);
  mainWindow.open();

  return QApplication::exec();
}

#if defined(Q_OS_MAC)
bool checkMacAssistiveDevices()
{
#if __MAC_OS_X_VERSION_MIN_REQUIRED >= 1090 // mavericks

  // new in mavericks, applications are trusted individually
  // with use of the accessibility api. this call will show a
  // prompt which can show the security/privacy/accessibility
  // tab, with a list of allowed applications. deskflow should
  // show up there automatically, but will be unchecked.

  if (AXIsProcessTrusted()) {
    return true;
  }

  const void *keys[] = {kAXTrustedCheckOptionPrompt};
  const void *trueValue[] = {kCFBooleanTrue};
  CFDictionaryRef options = CFDictionaryCreate(NULL, keys, trueValue, 1, NULL, NULL);

  bool result = AXIsProcessTrustedWithOptions(options);
  CFRelease(options);
  return result;

#else

  // now deprecated in mavericks.
  bool result = AXAPIEnabled();
  if (!result) {
    QString msgBody = QString("Please enable access to assistive devices "
                              "System Preferences -> Security & Privacy -> "
                              "Privacy -> Accessibility, then re-open %1.");
    QMessageBox::information(NULL, kAppName, msgBody.arg(kAppName));
  }
  return result;

#endif
}
#endif
