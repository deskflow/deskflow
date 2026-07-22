/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2026 Deskflow Developers
 * SPDX-License-Identifier: MIT
 */

#include "base/Log.h"
#include "deskflow/ipc/CoreIpcServer.h"

#include <QCoreApplication>
#include <QDebug>
#include <QFile>

#include <memory>

#ifdef Q_OS_UNIX
#include <csignal>
#endif

namespace {

constexpr auto kIpcNameEnvironment = "DESKFLOW_CORE_PROCESS_TEST_IPC_NAME";
constexpr auto kGracefulStopResultEnvironment = "DESKFLOW_CORE_PROCESS_TEST_GRACEFUL_STOP_RESULT";

} // namespace

int main(int argc, char *argv[])
{
  QCoreApplication app(argc, argv);
  Log log;

#ifdef Q_OS_UNIX
  if (qEnvironmentVariableIsSet("DESKFLOW_CORE_PROCESS_TEST_IGNORE_TERMINATE")) {
    std::signal(SIGTERM, SIG_IGN);
  }
#endif

  const auto ipcName = qEnvironmentVariable(kIpcNameEnvironment);
  if (!ipcName.isEmpty()) {
    auto ipcServer = std::make_unique<deskflow::core::ipc::CoreIpcServer>(nullptr, ipcName);
    QObject::connect(ipcServer.get(), &deskflow::core::ipc::IpcServer::stopProcessRequested, &app, [&app] {
      QFile resultFile(qEnvironmentVariable(kGracefulStopResultEnvironment));
      if (resultFile.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
        resultFile.write("graceful-stop");
      } else {
        qWarning() << "failed to write graceful stop result:" << resultFile.fileName();
      }
      app.quit();
    });
    ipcServer->listen();
    ipcServer->broadcastCommand(QStringLiteral("connectionState"), QStringLiteral("Connected"));

    qInfo("ready");
    return app.exec();
  }

  qInfo("ready");
  return app.exec();
}
