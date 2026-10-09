/*
 * SPDX-FileCopyrightText: (C) 2026 Deskflow Developers
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */
#include "gui/handoff/Native.h"

#include <QCoreApplication>
#include <QFile>
#include <QStandardPaths>
#include <QTextStream>
#include <csignal>
#include <stdexcept>

#import <Foundation/Foundation.h>

int main(int argc, char **argv)
{
  QCoreApplication app(argc, argv);
  // SSH disconnects close stdin; let receive() clean its staging folder on EOF.
  std::signal(SIGHUP, SIG_IGN);
  @autoreleasepool {
    try {
      if (argc != 1)
        throw std::runtime_error("This receiver reads a Deskflow handoff from standard input; it takes no arguments.");
      QFile input;
      if (!input.open(stdin, QIODevice::ReadOnly))
        throw std::runtime_error("Could not read transfer");
      const auto request =
          deskflow::handoff::receive(input, QStandardPaths::writableLocation(QStandardPaths::DownloadLocation));
      deskflow::handoff::openReceived(request);
      QTextStream(stdout) << "Received successfully" << Qt::endl;
      return 0;
    } catch (const std::exception &error) {
      QTextStream(stderr) << error.what() << Qt::endl;
      return 1;
    }
  }
}
