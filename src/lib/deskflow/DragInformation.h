/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2013 - 2016 Symless Ltd.
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
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
