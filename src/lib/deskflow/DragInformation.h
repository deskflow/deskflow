/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2013 - 2016 Symless Ltd.
 * SPDX-FileCopyrightText: (C) 2026 Deskflow Devs
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 *
 * Ported from Barrier's DragInformation (barrier/src/lib/barrier/DragInformation.cpp)
 * to re-enable file drag and drop (DDRG).
 */

#pragma once

#include "base/Event.h"

#include <cstdint>
#include <string>
#include <vector>

class DragInformation;

using DragFileList = std::vector<DragInformation>;

/// Event payload carried by EventTypes::DragInfoSending (DDRG).
class DragInfoEventData : public EventData
{
public:
  uint32_t fileCount = 0;
  std::string info;
};

class DragInformation
{
public:
  DragInformation() = default;

  std::string &getFilename() { return m_filename; }
  void setFilename(std::string &name) { m_filename = name; }
  size_t getFilesize() const { return m_filesize; }
  void setFilesize(size_t size) { m_filesize = size; }

  static void parseDragInfo(DragFileList &dragFileList, uint32_t fileNum, const std::string &data);
  static std::string getDragFileExtension(const std::string &filename);
  // helper function to setup drag info
  // example: filename1,filesize1,filename2,filesize2,
  // return file count
  static int setupDragInfo(DragFileList &fileList, std::string &output);

  static bool isFileValid(const std::string &filename);

private:
  static size_t stringToNum(const std::string &str);
  static std::string getFileSize(const std::string &filename);

private:
  std::string m_filename;
  size_t m_filesize = 0;
};