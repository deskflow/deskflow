/*
* barrier -- mouse and keyboard sharing utility
* Copyright (C) 2018 Debauchee Open Source Group
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

#include "../DataDirectories.h"
#include "KnownFolderPaths.h"

const std::string& DataDirectories::profile()
{
    if (_profile.empty())
        _profile = localAppDataPath() + "\\Barrier";
    return _profile;
}
const std::string& DataDirectories::profile(const std::string& path)
{
    _profile = path;
    return _profile;
}

const std::string& DataDirectories::global()
{
    if (_global.empty())
        _global = programDataPath() + "\\Barrier";
    return _global;
}
const std::string& DataDirectories::global(const std::string& path)
{
    _global = path;
    return _global;
}

const std::string& DataDirectories::systemconfig()
{
    // systemconfig() is a special case in that it will track the current value
    // of global() unless and until it is explictly set otherwise
    // previously it would default to the windows folder which was horrible!
    if (_systemconfig.empty())
        return global();
    return _systemconfig;
}

const std::string& DataDirectories::systemconfig(const std::string& path)
{
    _systemconfig = path;
    return _systemconfig;
}
