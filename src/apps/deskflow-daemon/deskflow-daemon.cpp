/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2012 - 2016 Symless Ltd.
 * SPDX-FileCopyrightText: (C) 2012 Nick Bolton
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#include "deskflow/DaemonApp.h"

#include "base/Log.h"

#include <QCoreApplication>
#include <QThread>

int main(int argc, char **argv)
{
  LOG((CLOG_PRINT "%s daemon (v%s)", kAppName, kVersion));
  QCoreApplication app(argc, argv);

  // Daemon app must be on the heap, as we're moving it to a thread.
  // Deliberately do not set ownership of the daemon to the app, as this would prevent us
  // being able to move it to a thread and delete it when the thread finishes.
  auto *pDaemon = new DaemonApp(); // NOSONAR
  QObject::connect(pDaemon, &DaemonApp::mainLoopFinished, &app, &QCoreApplication::quit);
  QObject::connect(pDaemon, &DaemonApp::fatalErrorOccurred, &app, &QCoreApplication::quit);
  QObject::connect(pDaemon, &DaemonApp::serviceInstalled, &app, &QCoreApplication::quit);
  QObject::connect(pDaemon, &DaemonApp::serviceUninstalled, &app, &QCoreApplication::quit);
  pDaemon->init(argc, argv);

  // Thread must be on the heap, as the thread needs to be deleted only when the loop finishes,
  // and not when we go out of scope.
  // Deliberately do not set ownership of the thread to the daemon, as we want to delete the thread
  // when the loop finishes, and not when the daemon is deleted.
  QThread *thread = new QThread(); // NOSONAR
  pDaemon->moveToThread(thread);
  QObject::connect(thread, &QThread::started, pDaemon, &DaemonApp::run);
  QObject::connect(thread, &QThread::finished, pDaemon, &QObject::deleteLater);
  QObject::connect(thread, &QThread::finished, thread, &QThread::deleteLater);
  thread->start();

  return QCoreApplication::exec();
}

#if SYSAPI_WIN32

#define WIN32_LEAN_AND_MEAN
#include <Windows.h>

int WINAPI WinMain(HINSTANCE, HINSTANCE, LPSTR, int)
{
  return main(__argc, __argv);
}

#endif
