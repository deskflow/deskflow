/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2024 Symless Ltd.
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#pragma once

#include <QCoreApplication>

static int argc = 0;          // NOLINT NOSONAR
static char **argv = nullptr; // NOLINT NOSONAR

class TestQtCoreApp : public QCoreApplication
{
public:
  explicit TestQtCoreApp() : QCoreApplication(argc, argv)
  {
  }
};
