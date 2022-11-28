/*
 * synergy -- mouse and keyboard sharing utility
 * Copyright (C) 2012-2022 Symless Ltd.
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
#include <iostream>
#include "synergy/ClientApp.h"
#include "synergy/ServerApp.h"
#include "arch/Arch.h"
#include "base/Log.h"
#include "base/EventQueue.h"
#include <vector>

void showHelp()
{
    std::cout<<"Usage: synergy-core <server | client> [...options]"<<std::endl;
    std::cout<<"sever - start as a server (synergys)"<<std::endl;
    std::cout<<"client - start as a client (synergyc)"<<std::endl;
    std::cout<<"use synergy-core <sever|client> --help for more information."<<std::endl;
}

bool isServer(int argc, char** argv)
{
    return (argc > 1 && argv[1] == std::string("server"));
}

bool isClient(int argc, char** argv)
{
    return (argc > 1 && argv[1] == std::string("client"));
}

std::vector<char*> getCommandLine(int argc, char** argv)
{
    std::vector<char*> commandLine;

    commandLine.reserve(argc - 1);
    commandLine.push_back(argv[0]);
    commandLine.insert(commandLine.end(), &argv[2], &argv[argc]);

    return commandLine;
}

int main(int argc, char** argv)
{
    Arch arch;
    arch.init();

    Log log;
    EventQueue events;

    if (isServer(argc, argv)) {
        auto commandLine = getCommandLine(argc, argv);
        ServerApp app(&events, nullptr);
        return app.run(commandLine.size(), &commandLine[0]);
    }
    else if (isClient(argc, argv)) {
        auto commandLine = getCommandLine(argc, argv);
        ClientApp app(&events, nullptr);
        return app.run(commandLine.size(), &commandLine[0]);
    }
    else {
        showHelp();
    }

    return 0;
}
