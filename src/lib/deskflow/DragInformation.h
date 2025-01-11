/*
 * Deskflow -- mouse and keyboard sharing utility
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

#include "base/EventTypes.h"
#include "common/stdvector.h"

#include <string>

class DragInformation;
using DragFileList = std::vector<DragInformation>;

class DragInformation
{
public:
  DragInformation();
  ~DragInformation()
  {
  }

  std::string &getFilename()
  {
    return m_filename;
  }
  void setFilename(std::string &name)
  {
    m_filename = name;
  }
  size_t getFilesize()
  {
    return m_filesize;
  }
  void setFilesize(size_t size)
  {
    m_filesize = size;
  }

  static void parseDragInfo(DragFileList &dragFileList, uint32_t fileNum, std::string data);
  static std::string getDragFileExtension(std::string filename);
  // helper function to setup drag info
  // example: filename1,filesize1,filename2,filesize2,
  // return file count
  static int setupDragInfo(DragFileList &fileList, std::string &output);

  static bool isFileValid(std::string filename);

private:
  static size_t stringToNum(std::string &str);
  static std::string getFileSize(std::string &filename);

private:
  std::string m_filename;
  size_t m_filesize;
};
