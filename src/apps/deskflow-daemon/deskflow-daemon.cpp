/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2012 - 2016 Symless Ltd.
 * SPDX-FileCopyrightText: (C) 2012 Nick Bolton
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#include "arch/Arch.h"
#include "base/Log.h"
#include "common/constants.h"
#include "deskflow/DaemonApp.h"

#include "arch/Arch.h"
#include "base/EventQueue.h"
#include "base/Log.h"

#if SYSAPI_WIN32

#include "arch/win32/ArchMiscWindows.h"

#define WIN32_LEAN_AND_MEAN
#include <Windows.h>

#endif

#include <QCoreApplication>
#include <QThread>

int main(int argc, char **argv)
{
#if SYSAPI_WIN32
  // Save window instance for later use, e.g. `GetModuleFileName` which is used when installing the daemon.
  ArchMiscWindows::setInstanceWin32(GetModuleHandle(nullptr));
#endif

  Arch arch;
  arch.init();

  Log log;
  EventQueue events;

  LOG((CLOG_PRINT "%s daemon (v%s)", kAppName, kVersion));

  QCoreApplication app(argc, argv);

  const auto pDaemon = &DaemonApp::instance();
  QObject::connect(pDaemon, &DaemonApp::mainLoopFinished, &app, &QCoreApplication::quit);
  QObject::connect(pDaemon, &DaemonApp::fatalErrorOccurred, &app, &QCoreApplication::quit);
  QObject::connect(pDaemon, &DaemonApp::serviceInstalled, &app, &QCoreApplication::quit);
  QObject::connect(pDaemon, &DaemonApp::serviceUninstalled, &app, &QCoreApplication::quit);
  pDaemon->init(&events, argc, argv);

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

int WINAPI WinMain(HINSTANCE, HINSTANCE, LPSTR, int)
{
  return main(__argc, __argv);
}

#endif
