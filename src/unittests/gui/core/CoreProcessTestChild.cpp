/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2026 Deskflow Developers
 * SPDX-License-Identifier: MIT
 */

#include <QCoreApplication>
#include <QDebug>

#ifdef Q_OS_UNIX
#include <csignal>
#endif

int main(int argc, char *argv[])
{
  QCoreApplication app(argc, argv);

#ifdef Q_OS_UNIX
  if (qEnvironmentVariableIsSet("DESKFLOW_CORE_PROCESS_TEST_IGNORE_TERMINATE")) {
    std::signal(SIGTERM, SIG_IGN);
  }
#endif

  qInfo("ready");
  return app.exec();
}
