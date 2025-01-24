/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2013 - 2016 Symless Ltd.
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#include "deskflow/DragInformation.h"
#include "base/Log.h"

#include <fstream>
#include <sstream>
#include <stdexcept>

using namespace std;

DragInformation::DragInformation() : m_filename(), m_filesize(0)
{
}

void DragInformation::parseDragInfo(DragFileList &dragFileList, uint32_t fileNum, std::string data)
{
  size_t startPos = 0;
  size_t findResult1 = 0;
  size_t findResult2 = 0;
  dragFileList.clear();
  std::string slash("\\");
  if (data.find("/", startPos) != std::string::npos) {
    slash = "/";
  }

  uint32_t index = 0;
  while (index < fileNum) {
    findResult1 = data.find(',', startPos);
    findResult2 = data.find_last_of(slash, findResult1);

    if (findResult1 == startPos) {
      // TODO: file number does not match, something goes wrong
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

  LOG((CLOG_DEBUG "drag info received, total drag file number: %i", dragFileList.size()));

  for (size_t i = 0; i < dragFileList.size(); ++i) {
    LOG((CLOG_DEBUG "dragging file %i name: %s", i + 1, dragFileList.at(i).getFilename().c_str()));
  }
}

std::string DragInformation::getDragFileExtension(std::string filename)
{
  size_t findResult = std::string::npos;
  findResult = filename.find_last_of(".", filename.size());
  if (findResult != std::string::npos) {
    return filename.substr(findResult + 1, filename.size() - findResult - 1);
  } else {
    return "";
  }
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

bool DragInformation::isFileValid(std::string filename)
{
  bool result = false;
  std::fstream file(filename.c_str(), ios::in | ios::binary);

  if (file.is_open()) {
    result = true;
  }

  file.close();

  return result;
}

size_t DragInformation::stringToNum(std::string &str)
{
  istringstream iss(str.c_str());
  size_t size;
  iss >> size;
  return size;
}

std::string DragInformation::getFileSize(std::string &filename)
{
  std::fstream file(filename.c_str(), ios::in | ios::binary);

  if (!file.is_open()) {
    throw std::runtime_error("failed to get file size");
  }

  // check file size
  file.seekg(0, std::ios::end);
  size_t size = (size_t)file.tellg();

  stringstream ss;
  ss << size;

  file.close();

  return ss.str();
}
