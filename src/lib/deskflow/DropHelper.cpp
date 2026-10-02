/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2013 - 2016 Symless Ltd.
 * SPDX-FileCopyrightText: (C) 2026 Deskflow Devs
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 *
 * Ported from Barrier's DropHelper (barrier/src/lib/barrier/DropHelper.cpp)
 * to re-enable file drag and drop (DDRG).
 */

#include "deskflow/DropHelper.h"

#include "base/Log.h"

#include <fstream>

void DropHelper::writeToDir(const std::string &destination, DragFileList &fileList, std::string &data)
{
  LOG_DEBUG("dropping file, files=%zu target=%s", fileList.size(), destination.c_str());

  if (!destination.empty() && fileList.size() > 0) {
    std::ofstream file;
    std::string dropTarget = destination;
#ifdef SYSAPI_WIN32
    dropTarget.append("\\");
#else
    dropTarget.append("/");
#endif
    dropTarget.append(fileList.at(0).getFilename());
    file.open(dropTarget, std::ios::out | std::ios::binary);
    if (!file.is_open()) {
      LOG_ERR("drop file failed: can not open %s", dropTarget.c_str());
    }

    file.write(data.c_str(), static_cast<std::streamsize>(data.size()));
    file.close();

    LOG_INFO("dropped file \"%s\" in \"%s\"", fileList.at(0).getFilename().c_str(), destination.c_str());

    fileList.clear();
  } else {
    LOG_ERR("drop file failed: drop target is empty");
  }
}