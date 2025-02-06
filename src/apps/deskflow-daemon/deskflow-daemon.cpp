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

  // Must be on the heap, as we're moving it to a thread.
  DaemonApp *pDaemon = new DaemonApp(&app);
  QObject::connect(pDaemon, &DaemonApp::serviceInstalled, &app, &QCoreApplication::quit);
  QObject::connect(pDaemon, &DaemonApp::serviceUninstalled, &app, &QCoreApplication::quit);
  pDaemon->init(argc, argv);

  QThread *thread = new QThread();
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
