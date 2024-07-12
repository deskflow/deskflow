/*
 * synergy -- mouse and keyboard sharing utility
 * Copyright (C) 2024 Symless Ltd.
 *
 * This package is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * found in the file LICENSE that should have accompanied this file.
 *
 * This package is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <gtest/gtest.h>
#include <qcoreapplication.h>

int main(int argc, char **argv) {
  // required to solve the issue where some qt objects need access to a qt app
  // on some platforms (e.g. QNetworkAccessManager).
  QCoreApplication app(argc, argv);

  testing::InitGoogleTest(&argc, argv);

  // gtest seems to randomly finish with error codes (e.g. -1, -1073741819)
  // even when no tests have failed. not sure what causes this, but it
  // happens on all platforms and  keeps leading to false positives.
  // according to the documentation, 1 is a failure, so we should be
  // able to trust that code.
  return (RUN_ALL_TESTS() == 1) ? 1 : 0;
}
