/*
 * synergy -- mouse and keyboard sharing utility
 * Copyright (C) 2013-2016 Symless Ltd.
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

#include "common/stdvector.h"
#include "base/String.h"
#include "base/EventTypes.h"

class DragInformation;
typedef std::vector<DragInformation> DragFileList;

class DragInformation {
public:
    DragInformation ();
    ~DragInformation () {
    }

    String&
    getFilename () {
        return m_filename;
    }
    void
    setFilename (String& name) {
        m_filename = name;
    }
    size_t
    getFilesize () {
        return m_filesize;
    }
    void
    setFilesize (size_t size) {
        m_filesize = size;
    }

    static void
    parseDragInfo (DragFileList& dragFileList, UInt32 fileNum, String data);
    static String getDragFileExtension (String filename);
    // helper function to setup drag info
    // example: filename1,filesize1,filename2,filesize2,
    // return file count
    static int setupDragInfo (DragFileList& fileList, String& output);

    static bool isFileValid (String filename);

private:
    static size_t stringToNum (String& str);
    static String getFileSize (String& filename);

private:
    String m_filename;
    size_t m_filesize;
};
