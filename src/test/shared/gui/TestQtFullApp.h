/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2024 Symless Ltd.
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#pragma once

#include <QApplication>

static int argc = 0;          // NOLINT NOSONAR
static char **argv = nullptr; // NOLINT NOSONAR

/**
 * Prefer using `TestQtCoreApp` instead.
 */
class TestQtFullApp : public QApplication
{
public:
  explicit TestQtFullApp() : QApplication(argc, argv)
  {
#if defined(Q_OS_WIN)
#error "this object causes windows ci to freeze"
#endif
  }
};
