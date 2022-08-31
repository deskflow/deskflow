/*
 * synergy -- mouse and keyboard sharing utility
 * Copyright (C) 2012-2016 Symless Ltd.
 * Copyright (C) 2002 Chris Schoeneman
 *
 * This package is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * found in the file LICENSE that should have accompanied this file.
 *
 * This package is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <QCoreApplication>
#include <QTimer>
#include <QThread>
#include "synergy/ServerApp.h"
#include "arch/Arch.h"
#include "base/Log.h"
#include "base/EventQueue.h"

#if WINAPI_MSWINDOWS
#include "MSWindowsServerTaskBarReceiver.h"
#elif WINAPI_XWINDOWS
#include "XWindowsServerTaskBarReceiver.h"
#elif WINAPI_CARBON
#include "OSXServerTaskBarReceiver.h"
#else
#error Platform not supported.
#endif

struct {
  int argc{};
  char** argv{nullptr};
} gProcessArgs;

ArchThread gServerThread;

static void * serverThreadExecute(void * data) {
  Log log;
  EventQueue events;
  ServerApp app(&events, createTaskBarReceiver);
  app.run(gProcessArgs.argc, gProcessArgs.argv);
}

static void start() {
  gServerThread = Arch::getInstance()->newThread(&serverThreadExecute, nullptr);
  // gServerThread = new std::thread(&serverThreadExecute);
}

int
main(int argc, char** argv)
{
#if SYSAPI_WIN32
    // record window instance for tray icon, etc
    ArchMiscWindows::setInstanceWin32(GetModuleHandle(NULL));
#endif
    gProcessArgs = {.argc = argc, .argv = argv };
    Arch arch;
    arch.init();

    auto app = new QCoreApplication(argc, argv);
    auto timer = new QTimer(app);
    timer->setSingleShot(true);
    // QTimer::singleShot(1,start);
    QCoreApplication::connect(timer, &QTimer::timeout, [] () { start(); });
    timer->start();
    int res = QCoreApplication::exec();
    delete timer;
    QCoreApplication::quit();
    return res;
}
