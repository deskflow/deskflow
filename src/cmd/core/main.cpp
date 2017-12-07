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

#include "core/ClientApp.h"
#include "arch/Arch.h"
#include "base/EventQueue.h"
#include "base/Log.h"
#include "core/ServerApp.h"

#include <iostream>

int
main(int argc, char** argv) 
{
    // TODO(andrew): use existing arg parse code
    bool server = false, client = false;
    if (argc > 1) {
        server = std::string(argv[1]) == "--server";
        client = std::string(argv[1]) == "--client";
    }

#if SYSAPI_WIN32
    // record window instance for tray icon, etc
    ArchMiscWindows::setInstanceWin32(GetModuleHandle(NULL));
#endif

    Arch arch;
    arch.init();

    Log log;
    EventQueue events;

    if (server) {
        ServerApp app(&events);
        return app.run(argc, argv);
    }
    if (client) {
        ClientApp app(&events);
        return app.run(argc, argv);
    }
    else {
        // TODO(andrew): use common error code
        std::cerr << "error: use --client or --server args" << std::endl;
        return 1;
    }
}
