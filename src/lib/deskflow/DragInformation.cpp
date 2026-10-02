/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2013 - 2016 Symless Ltd.
 * SPDX-FileCopyrightText: (C) 2026 Deskflow Devs
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 *
 * Ported from Barrier's DragInformation (barrier/src/lib/barrier/DragInformation.cpp)
 * to re-enable file drag and drop (DDRG).
 */

#include "deskflow/DragInformation.h"

#include "base/Log.h"

#include <cstdint>
#include <fstream>
#include <sstream>
#include <stdexcept>

void DragInformation::parseDragInfo(DragFileList &dragFileList, uint32_t fileNum, const std::string &data)
{
  dragFileList.clear();
  std::string slash("\\");
  if (data.find("/", 0) != std::string::npos) {
    slash = "/";
  }

  uint32_t index = 0;
  size_t startPos = 0;
  size_t findResult1 = 0;
  size_t findResult2 = 0;

  while (index < fileNum) {
    findResult1 = data.find(',', startPos);
    findResult2 = data.find_last_of(slash, findResult1);

    if (findResult1 == startPos) {
      // file number does not match, something goes wrong
      break;
    }

    // set filename
    if (findResult1 - findResult2 > 1) {
      std::string filename = data.substr(findResult2 + 1, findResult1 - findResult2 - 1);
      DragInformation di;
      di.setFilename(filename);
      dragFileList.push_back(di);
    }
    startPos = findResult1 + 1;

    // set filesize
    findResult2 = data.find(',', startPos);
    if (findResult2 - findResult1 > 1) {
      std::string filesize = data.substr(findResult1 + 1, findResult2 - findResult1 - 1);
      size_t size = stringToNum(filesize);
      dragFileList.at(index).setFilesize(size);
    }
    startPos = findResult1 + 1;

    ++index;
  }

  LOG_DEBUG("drag info received, total drag file number: %zu", dragFileList.size());

  for (size_t i = 0; i < dragFileList.size(); ++i) {
    LOG_DEBUG("dragging file %zu name: %s", i + 1, dragFileList.at(i).getFilename().c_str());
  }
}

std::string DragInformation::getDragFileExtension(const std::string &filename)
{
  size_t findResult = filename.find_last_of(".", filename.size());
  if (findResult != std::string::npos) {
    return filename.substr(findResult + 1, filename.size() - findResult - 1);
  }
  return "";
}

int DragInformation::setupDragInfo(DragFileList &fileList, std::string &output)
{
  int size = static_cast<int>(fileList.size());
  for (int i = 0; i < size; ++i) {
    output.append(fileList.at(i).getFilename());
    output.append(",");
    std::string filesize = getFileSize(fileList.at(i).getFilename());
    output.append(filesize);
    output.append(",");
  }
  return size;
}

bool DragInformation::isFileValid(const std::string &filename)
{
  std::fstream file(filename.c_str(), std::ios::in | std::ios::binary);
  bool result = file.is_open();
  file.close();
  return result;
}

size_t DragInformation::stringToNum(const std::string &str)
{
  std::istringstream iss(str.c_str());
  size_t size;
  iss >> size;
  return size;
}

std::string DragInformation::getFileSize(const std::string &filename)
{
  std::fstream file(filename.c_str(), std::ios::in | std::ios::binary);
  if (!file.is_open()) {
    throw std::runtime_error("failed to get file size");
  }

  file.seekg(0, std::ios::end);
  size_t size = static_cast<size_t>(file.tellg());

  std::stringstream ss;
  ss << size;

  file.close();
  return ss.str();
}