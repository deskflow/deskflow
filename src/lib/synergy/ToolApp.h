/*
 * synergy -- mouse and keyboard sharing utility
 * Copyright (C) 2014-2016 Symless Ltd.
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

#pragma once

#include "synergy/App.h"
#include "synergy/ToolArgs.h"
#include "common/basic_types.h"

class ToolApp : public MinimalApp {
public:
    UInt32 run (int argc, char** argv);
    void help ();

private:
    void loginAuth ();
    void notifyActivation ();
    void notifyUpdate ();

private:
    ToolArgs m_args;
};
