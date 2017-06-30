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

#include "synergy/DropHelper.h"

#include "base/Log.h"

#include <fstream>

void
DropHelper::writeToDir (const String& destination, DragFileList& fileList,
                        String& data) {
    LOG ((CLOG_DEBUG "dropping file, files=%i target=%s",
          fileList.size (),
          destination.c_str ()));

    if (!destination.empty () && fileList.size () > 0) {
        std::fstream file;
        String dropTarget = destination;
#ifdef SYSAPI_WIN32
        dropTarget.append ("\\");
#else
        dropTarget.append ("/");
#endif
        dropTarget.append (fileList.at (0).getFilename ());
        file.open (dropTarget.c_str (), std::ios::out | std::ios::binary);
        if (!file.is_open ()) {
            LOG ((CLOG_ERR "drop file failed: can not open %s",
                  dropTarget.c_str ()));
        }

        file.write (data.c_str (), data.size ());
        file.close ();

        LOG ((CLOG_DEBUG "%s is saved to %s",
              fileList.at (0).getFilename ().c_str (),
              destination.c_str ()));

        fileList.clear ();
    } else {
        LOG ((CLOG_ERR "drop file failed: drop target is empty"));
    }
}
