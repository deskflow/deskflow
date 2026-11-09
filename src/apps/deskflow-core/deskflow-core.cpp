/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2025 Chris Rizzitello <sithlord48@gmail.com>
 * SPDX-FileCopyrightText: (C) 2012 - 2016, 2025 - 2026 Synergy App Ltd
 * SPDX-FileCopyrightText: (C) 2002 Chris Schoeneman
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#include "CoreArgParser.h"

#include "arch/Arch.h"
#include "base/EventQueue.h"
#include "base/Log.h"
#include "common/Constants.h"
#include "common/ExitCodes.h"
#include "common/IsolatedSession.h"
#include "common/Settings.h"
#include "deskflow/ClientApp.h"
#include "deskflow/ServerApp.h"
#include "deskflow/ipc/CoreIpcServer.h"
#include <QDir>
#include <QJsonDocument>

#if defined(Q_OS_WIN)
#include "arch/win32/ArchMiscWindows.h"
#endif

#include <QApplication>
#include <QFileInfo>
#include <QSharedMemory>
#include <QTextStream>
#include <QThread>

void qtMessageHandler(QtMsgType type, const QMessageLogContext &context, const QString &message)
{
  const auto utf8 = message.toUtf8();
  switch (type) {
  case QtDebugMsg:
    CLOG->print(context.file, context.line, CLOG_TAG_DEBUG "%s", utf8.constData());
    break;
  case QtInfoMsg:
    CLOG->print(context.file, context.line, CLOG_TAG_INFO "%s", utf8.constData());
    break;
  case QtWarningMsg:
    CLOG->print(context.file, context.line, CLOG_TAG_WARN "%s", utf8.constData());
    break;
  case QtCriticalMsg:
    CLOG->print(context.file, context.line, CLOG_TAG_ERR "%s", utf8.constData());
    break;
  case QtFatalMsg:
    CLOG->print(context.file, context.line, CLOG_TAG_CRIT "%s", utf8.constData());
    break;
  }
  if (type == QtFatalMsg) {
    abort();
  }
}

void showHelp(const CoreArgParser &parser)
{
  QTextStream(stdout) << parser.helpText();
}

App *createApp(const CoreArgParser &parser, EventQueue &events, const QString &processName)
{
  if (parser.serverMode()) {
    return new ServerApp(&events, processName);
  } else if (parser.clientMode()) {
    return new ClientApp(&events, processName);
  }
  return nullptr;
}

int main(int argc, char **argv)
{
#ifdef DESKFLOW_LAB_BUILD
  if (argc == 3 && (QString::fromLocal8Bit(argv[1]) == "--lab-network-plan" ||
                    QString::fromLocal8Bit(argv[1]) == "--lab-settings-check")) {
    const auto parent = QDir::cleanPath(QStringLiteral(DESKFLOW_LAB_RUNS_ROOT) + "/../../dual-end-test");
    const auto session = deskflow::lab::IsolatedSession::inspect(QString::fromLocal8Bit(argv[2]), parent);
    if (session.valid() && QString::fromLocal8Bit(argv[1]) == "--lab-settings-check") {
      // Offline integration check, before QApplication, Arch, input and IPC.
      QCoreApplication app(argc, argv);
      if (!deskflow::lab::IsolatedSession::activate(session.root, parent))
        return s_exitArgs;
      auto evidence = session.plan();
      evidence["actualSettings"] = Settings::settingsFile();
      evidence["actualCertificatePath"] = Settings::value(Settings::Security::Certificate).toString();
      evidence["actualTrustedServers"] = Settings::tlsTrustedServersDb();
      evidence["actualTrustedClients"] = Settings::tlsTrustedClientsDb();
      evidence["actualPort"] = Settings::value(Settings::Core::Port).toInt();
      evidence["actualProcessMode"] = Settings::value(Settings::Core::ProcessMode).toInt();
      evidence["actualTls"] = Settings::value(Settings::Security::TlsEnabled).toBool();
      evidence["actualPeerChecks"] = Settings::value(Settings::Security::CheckPeers).toBool();
      Settings::save(false);
      QTextStream(stdout) << QJsonDocument(evidence).toJson(QJsonDocument::Compact) << "\n";
      return 0;
    }
    QTextStream(stdout) << QJsonDocument(session.plan()).toJson(QJsonDocument::Compact) << "\n";
    return session.valid() ? 0 : s_exitArgs;
  }
  // This artifact can be inspected, but cannot capture or inject input, bind a
  // server port, or create IPC. Guard before QApplication and platform init.
  if (argc != 2 || (QString::fromLocal8Bit(argv[1]) != "--help" && QString::fromLocal8Bit(argv[1]) != "--version")) {
    QTextStream(stderr) << "DeskflowLab: input core disabled in review builds.\n";
    return s_exitArgs;
  }
#endif
#if defined(Q_OS_WIN)
  ArchMiscWindows::setInstanceWin32(GetModuleHandle(nullptr));
#endif

  QApplication::setApplicationName(QStringLiteral("%1 Core").arg(kAppName));
  QApplication app(argc, argv);

  Arch arch;
  arch.init();

  Log log;
  qInstallMessageHandler(qtMessageHandler);

  CoreArgParser parser(QCoreApplication::arguments());

  // Print any parser errors
  if (!parser.errorText().isEmpty()) {
    QTextStream(stdout) << parser.errorText() << "\n";
  }

  if (parser.help()) {
    showHelp(parser);
    return s_exitSuccess;
  }

  if (parser.version()) {
    QTextStream(stdout) << parser.versionText();
    return s_exitSuccess;
  }

  // Before we check any more args we need to check for a duplicate process.
  // Create a shared memory segment with a unique key
  // This is to prevent a new instance from running if one is already running
  QSharedMemory sharedMemory(
      deskflow::lab::IsolatedSession::active() ? deskflow::lab::IsolatedSession::active()->ipcName + "-lock"
                                               : QString(kCoreBinName)
  );

  // Attempt to attach first and detach in order to clean up stale shm chunks
  // This can happen if the previous instance was killed or crashed
  if (sharedMemory.attach())
    sharedMemory.detach();

  if (!sharedMemory.create(1) && parser.singleInstanceOnly()) {
    LOG_WARN("an instance of deskflow core is already running");
    return s_exitDuplicate;
  }

  parser.parse();

  EventQueue events;
  const auto processName = QFileInfo(argv[0]).fileName();

  App *coreApp = createApp(parser, events, processName);

  const auto ipcServer = new deskflow::core::ipc::CoreIpcServer(&app); // NOSONAR - Qt managed
  QObject::connect(
      ipcServer, &deskflow::core::ipc::IpcServer::stopProcessRequested, coreApp, &App::quit, Qt::DirectConnection
  );
  ipcServer->listen();

  QThread coreThread;
  QObject::connect(&coreThread, &QThread::finished, &app, &QApplication::quit);
  coreApp->run(coreThread);

  int exitCode = QApplication::exec();
  coreThread.wait();

  if (exitCode == s_exitSuccess) {
    exitCode = coreApp->getExitCode();
  }
  delete coreApp;

  LOG_DEBUG("core exited, code: %d", exitCode);
  return exitCode;
}
