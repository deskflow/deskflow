/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2014 - 2016 Symless Ltd.
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#include "deskflow/DropHelper.h"

#include "base/Log.h"

#include <fstream>

void DropHelper::writeToDir(const std::string &destination, DragFileList &fileList, std::string &data)
{
  LOG((CLOG_DEBUG "dropping file, files=%i target=%s", fileList.size(), destination.c_str()));

  if (!destination.empty() && fileList.size() > 0) {
    std::fstream file;
    std::string dropTarget = destination;
#ifdef SYSAPI_WIN32
    dropTarget.append("\\");
#else
    dropTarget.append("/");
#endif
    dropTarget.append(fileList.at(0).getFilename());
    file.open(dropTarget.c_str(), std::ios::out | std::ios::binary);
    if (!file.is_open()) {
      LOG((CLOG_ERR "drop file failed: can not open %s", dropTarget.c_str()));
    }

    file.write(data.c_str(), data.size());
    file.close();

    LOG((CLOG_DEBUG "%s is saved to %s", fileList.at(0).getFilename().c_str(), destination.c_str()));

    fileList.clear();
  } else {
    LOG((CLOG_ERR "drop file failed: drop target is empty"));
  }
}
