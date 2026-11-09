/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2024 Chris Rizzitello <sithord48@gmail.com>
 * SPDX-FileCopyrightText: (C) 2012 - 2024 Synergy App Ltd
 * SPDX-FileCopyrightText: (C) 2008 Volker Lanz <vl@fidra.de>
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#include "common/CandidatePolicy.h"
#include "common/Constants.h"
#include "common/ExitCodes.h"
#include "common/I18N.h"
#include "common/PlatformInfo.h"
#include "common/Settings.h"
#include "common/UrlConstants.h"
#include "common/VersionInfo.h"
#include "gui/Diagnostic.h"
#include "gui/LabRun.h"
#include "gui/MainWindow.h"
#include "gui/Messages.h"
#include "gui/StyleUtils.h"
#include "gui/core/StartupHandoff.h"
#include "gui/core/WindowsStartupProvider.h"
#include <QJsonDocument>
#include <QJsonObject>
#include <QScrollArea>
#include <QScrollBar>
#if defined(Q_OS_MACOS) && defined(DESKFLOW_CANDIDATE_BUILD)
#include "gui/dialogs/CandidatePermissionDialog.h"
#include <QOperatingSystemVersion>
#endif

#include <QApplication>
#include <QCommandLineParser>
#include <QLocalSocket>
#include <QMessageBox>
#include <QSharedMemory>
#include <QTimer>

#if defined(Q_OS_MACOS)
#include <Carbon/Carbon.h>
#include <cstdlib>
#endif

#if defined(Q_OS_UNIX) && defined(QT_DEBUG)
#include <QLoggingCategory>
#endif

#if !defined(Q_OS_MACOS) && !defined(Q_OS_WIN)
#include "platform/XDGPortalRegistry.h"
#endif

using namespace deskflow::gui;

#if defined(Q_OS_MACOS)
bool checkMacAssistiveDevices();
#endif

#if defined(Q_OS_MACOS) && defined(DESKFLOW_CANDIDATE_BUILD)
namespace {
class MacAccessibilityPermissionProvider final : public AccessibilityPermissionProvider
{
public:
  bool isGranted() const override
  {
    return AXIsProcessTrusted();
  }
  void request() override
  {
    const void *keys[] = {kAXTrustedCheckOptionPrompt};
    const void *values[] = {kCFBooleanTrue};
    auto options = CFDictionaryCreate(nullptr, keys, values, 1, nullptr, nullptr);
    // Asynchronous system guidance. The return value is not an approval result.
    AXIsProcessTrustedWithOptions(options);
    CFRelease(options);
  }
};
} // namespace
#endif

const static auto kHeader = QStringLiteral("%1: %2\n").arg(kAppName, kDisplayVersion);

int main(int argc, char *argv[])
{
#if defined(Q_OS_UNIX) && defined(QT_DEBUG)
  // Fixes Fedora bug where qDebug() messages aren't printed.
  QLoggingCategory::setFilterRules(QStringLiteral("*.debug=true\nqt.*=false"));
#endif

#if !defined(Q_OS_MACOS) && !defined(Q_OS_WIN)
  if (QT_VERSION < QT_VERSION_CHECK(6, 10, 0))
    deskflow::platform::setAppId();
#endif

  QCoreApplication::setApplicationName(kAppName);
  QCoreApplication::setOrganizationName(kAppName);
  QCoreApplication::setApplicationVersion(kVersion);
  QCoreApplication::setOrganizationDomain(kOrgDomain); // used in prefix, can't be a url
  if (deskflow::gui::startup::startupWorkerRequested(argc, argv)) {
    QCoreApplication worker(argc, argv);
    return deskflow::gui::startup::runStartupWorker(worker.arguments(), QStringLiteral("%1-gui").arg(kAppId));
  }
  QGuiApplication::setDesktopFileName(kRevFqdnName);

#ifdef DESKFLOW_LAB_BUILD
  const auto labRoot = deskflow::gui::lab::prepare(argc, argv);
  if (labRoot.isEmpty()) {
    QTextStream(stderr) << "DeskflowLab requires --lab-root with a NEW child of " << DESKFLOW_LAB_RUNS_ROOT << '\n';
    return s_exitArgs;
  }
#endif

  QApplication app(argc, argv);
  app.setProperty("candidateInstanceKey", QStringLiteral("%1-gui").arg(kAppId));
  auto startupMode = MainWindow::StartupMode::Normal;

  // Ensure the I18N object is made before strings
  QTextStream(stdout) << "initial language: " << I18N::currentLanguage() << '\n';

  // Add Command Line Options
  auto helpOption = QCommandLineOption({"h", "help"}, "Display Help on the command line");
  auto versionOption = QCommandLineOption({"v", "version"}, "Display version information");
  auto resetOption = QCommandLineOption("reset", "Reset all settings");

  QCommandLineParser parser;
  parser.setSingleDashWordOptionMode(QCommandLineParser::ParseAsLongOptions);
  parser.addOption(helpOption);
  parser.addOption(versionOption);
  parser.addOption(resetOption);
#ifdef Q_OS_WIN
  parser.addOption(QCommandLineOption("managed-start", "Start from the current user login/recovery task"));
#endif
#ifdef DESKFLOW_LAB_BUILD
  parser.addOption(QCommandLineOption("lab-root", "New isolated run directory", "path"));
  parser.addOption(QCommandLineOption("lab-language", "Review language", "language"));
  parser.addOption(QCommandLineOption("lab-scenario", "Synthetic UI review scenario", "scenario", "empty"));
  parser.addOption(QCommandLineOption("lab-window-size", "Isolated viewport width x height", "size"));
  parser.addOption(QCommandLineOption("lab-smoke", "Save a real GUI screenshot and exit"));
  parser.addOption(QCommandLineOption("lab-theme", "Review palette", "theme"));
#endif
  parser.parse(QCoreApplication::arguments());
#ifdef DESKFLOW_LAB_BUILD
  app.setStyle("Fusion");
  if (parser.value("lab-theme") == "dark") {
    QPalette palette;
    palette.setColor(QPalette::Window, QColor("#202630"));
    palette.setColor(QPalette::WindowText, QColor("#e7edf5"));
    palette.setColor(QPalette::Base, QColor("#18202c"));
    palette.setColor(QPalette::Text, QColor("#e7edf5"));
    palette.setColor(QPalette::Button, QColor("#303b4b"));
    palette.setColor(QPalette::ButtonText, QColor("#e7edf5"));
    palette.setColor(QPalette::Highlight, QColor("#72a8f6"));
    palette.setColor(QPalette::HighlightedText, QColor("#111c2c"));
    palette.setColor(QPalette::Disabled, QPalette::ButtonText, QColor("#8791a2"));
    palette.setColor(QPalette::Disabled, QPalette::Text, QColor("#8791a2"));
    app.setPalette(palette);
  }
#endif

  if (!parser.errorText().isEmpty()) {
    qCritical().noquote() << parser.errorText() << "\nUse --help for more information.";
    return s_exitArgs;
  }

  if (parser.isSet(helpOption)) {
    QTextStream(stdout) << kHeader << QStringLiteral("  %1\n\n").arg(kAppDescription)
                        << parser.helpText().replace(QApplication::applicationFilePath(), kAppId);
    return s_exitSuccess;
  }

  if (parser.isSet(versionOption)) {
    QTextStream(stdout) << kHeader << kCopyright << Qt::endl;
    return s_exitSuccess;
  }

#ifdef Q_OS_WIN
  // A task launch is never an instruction to override the user's pause or
  // restore preference. Exit successfully so Task Scheduler does not retry it.
  if (parser.isSet("managed-start") && !deskflow::gui::winstartup::mayLaunch(
                                           Settings::value(deskflow::candidate::pausedKey).toBool(),
                                           Settings::value(Settings::Gui::AutoStartCore).toBool()
                                       ))
    return s_exitSuccess;
#endif
#ifndef DESKFLOW_LAB_BUILD
  const auto shmId = QStringLiteral("%1-gui").arg(kAppId);
  // Create a shared memory segment with a unique key
  // This is to prevent a new instance from running if one is already running
  QSharedMemory sharedMemory(shmId);

  // Attempt to attach first and detach in order to clean up stale shm chunks
  // This can happen if the previous instance was killed or crashed
  if (sharedMemory.attach())
    sharedMemory.detach();

  // If we can create 1 byte of SHM we are the only instance
  if (!sharedMemory.create(1)) {
#ifdef Q_OS_WIN
    // Managed duplicate launches neither raise the window nor trigger retries.
    if (parser.isSet("managed-start"))
      return s_exitSuccess;
#endif
    // Ping the running instance to have it show itself
    QLocalSocket socket;
    socket.connectToServer(shmId, QLocalSocket::ReadOnly);
    if (!socket.waitForConnected()) {
      // If we can't connect to the other instance tell the user its running.
      // This should never happen but just incase we should show something
      QMessageBox::information(nullptr, kAppName, QObject::tr("%1 is already running").arg(kAppName));
    }
    socket.disconnectFromServer();
    return s_exitDuplicate;
  }
#endif

  if (!deskflow::platform::isMac() && qEnvironmentVariable("XDG_CURRENT_DESKTOP") != QLatin1String("KDE")) {
    QApplication::setStyle("fusion");
  }

  // Sets the fallback icon path and fallback theme
  updateIconTheme();

  qInstallMessageHandler(deskflow::gui::messages::messageHandler);
  qInfo("%s v%s", kAppName, kDisplayVersion);

#if defined(Q_OS_MACOS)

  if (app.applicationDirPath().startsWith("/Volumes/")) {
    QString msgBody = QStringLiteral(
        "Please drag %1 to the Applications folder, "
        "and open it from there."
    );
    QMessageBox::information(nullptr, kAppName, msgBody.arg(kAppName));
    return 1;
  }

#ifndef DESKFLOW_LAB_BUILD
#ifdef DESKFLOW_CANDIDATE_BUILD
  MacAccessibilityPermissionProvider permissionProvider;
  if (!permissionProvider.isGranted()) {
    // No MainWindow/core/TLS initialization until the user explicitly continues.
    CandidatePermissionDialog guide(permissionProvider, QOperatingSystemVersion::current().majorVersion());
    // This is temporarily the only window; accepting it must not enqueue app quit.
    const bool quitOnLastWindowClosed = app.quitOnLastWindowClosed();
    app.setQuitOnLastWindowClosed(false);
    const int result = guide.exec();
    app.setQuitOnLastWindowClosed(quitOnLastWindowClosed);
    if (result != QDialog::Accepted)
      return s_exitSuccess;
    startupMode = MainWindow::StartupMode::PermissionReview;
  }
#else
  if (!checkMacAssistiveDevices()) {
    return 1;
  }
#endif
#endif
#endif

  // --no-reset
  if (parser.isSet(resetOption)) {
#ifdef DESKFLOW_LAB_BUILD
    return s_exitArgs;
#endif
    diagnostic::clearSettings(false);
  }

  MainWindow mainWindow(startupMode);
  mainWindow.open();
#ifdef DESKFLOW_LAB_BUILD
  mainWindow.setWindowTitle(QObject::tr("Deskflow Lab · Preview only"));
  if (!mainWindow.labScenario(parser.value("lab-scenario")))
    return s_exitArgs;
  if (parser.isSet("lab-window-size")) {
    const auto parts = parser.value("lab-window-size").split('x');
    if (parts.size() != 2 || parts[0].toInt() < 640 || parts[0].toInt() > 1600 || parts[1].toInt() < 320 ||
        parts[1].toInt() > 1200)
      return s_exitArgs;
    const QSize viewport(parts[0].toInt(), parts[1].toInt());
    QTimer::singleShot(200, &mainWindow, [&mainWindow, viewport] { mainWindow.resize(viewport); });
  }
  if (parser.isSet("lab-smoke")) {
    QTimer::singleShot(400, &app, [&] {
      const bool saved = mainWindow.grab().save(labRoot + "/mainwindow.png");
      if (auto *scroll = mainWindow.findChild<QScrollArea *>("mainContentScroll")) {
        const auto *content = scroll->widget();
        QJsonObject geometry{
            {"width", mainWindow.width()},
            {"height", mainWindow.height()},
            {"dpr", mainWindow.devicePixelRatioF()},
            {"verticalScrollMaximum", scroll->verticalScrollBar()->maximum()},
            {"horizontalScrollMaximum", scroll->horizontalScrollBar()->maximum()},
            {"viewportHeight", scroll->viewport()->height()},
            {"contentHeight", content->height()},
            {"contentMinimumHeight", content->minimumHeight()},
            {"contentHintHeight", content->sizeHint().height()}
        };
        QFile out(labRoot + "/geometry.json");
        if (out.open(QIODevice::WriteOnly))
          out.write(QJsonDocument(geometry).toJson());
      }
      if (auto *dialog = QApplication::activeModalWidget())
        dialog->grab().save(labRoot + "/dialog.png");
      else
        for (auto *window : QApplication::topLevelWidgets())
          if (window != &mainWindow && window->isVisible())
            window->grab().save(labRoot + "/dialog.png");
      app.exit(saved && deskflow::gui::lab::writeEvidence(labRoot) ? 0 : 2);
    });
  }
#endif

  return QApplication::exec();
}

#if defined(Q_OS_MACOS)
bool checkMacAssistiveDevices()
{
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
  CFDictionaryRef options = CFDictionaryCreate(nullptr, keys, trueValue, 1, nullptr, nullptr);

  bool result = AXIsProcessTrustedWithOptions(options);
  CFRelease(options);
  return result;
}
#endif
