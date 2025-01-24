/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2024 Symless Ltd.
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#include "gui/dotenv.h"

#include <QFile>
#include <QTextStream>

#include <gtest/gtest.h>

TEST(dotenv_tests, dotenv_fileDoesNotExist_doesNotLoadEnvVar)
{
  const QString envFile = "tmp/test/.env";

  deskflow::gui::dotenv(envFile);

  const QString actualValue = qEnvironmentVariable("TEST_ENV_VAR");
  EXPECT_TRUE(actualValue.isEmpty());
}

TEST(dotenv_tests, dotenv_envFileWithEntry_loadsEnvVar)
{
  const QString envFile = "tmp/test/.env";
  QFile file(envFile);
  if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
    FAIL() << "Failed to create: " << envFile.toStdString();
  }

  const QString key = "TEST_ENV_VAR";
  const QString value = R"("test value")";
  const QString entry = key + " = " + value;

  QTextStream out(&file);
  out << " # Comment" << Qt::endl;
  out << "FOOBAR" << Qt::endl;
  out << entry << Qt::endl;
  file.close();

  deskflow::gui::dotenv(envFile);

  const QString actualValue = qEnvironmentVariable(qPrintable(key));
  EXPECT_EQ("test value", actualValue.toStdString());

  QFile::remove(envFile);
}
