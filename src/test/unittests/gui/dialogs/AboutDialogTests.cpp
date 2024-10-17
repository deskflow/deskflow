/*
 * Deskflow -- mouse and keyboard sharing utility
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

// `TestQtFullApp` freezes on Windows CI, so exclude this test for now.
#ifndef WIN32

#include "common/copyright.h"
#include "gui/dialogs/AboutDialog.h"
#include "shared/gui/TestQtFullApp.h"

#include <QPixmap>
#include <QTimer>
#include <gmock/gmock.h>
#include <gtest/gtest.h>

using namespace testing;

namespace {

class DepsMock : public AboutDialog::Deps
{
public:
  MOCK_METHOD(bool, isDarkMode, (), (const, override));
};

} // namespace

TEST(AboutDialogTests, exec_setsDevelopersLabel)
{
  TestQtFullApp app;
  const auto deps = std::make_shared<NiceMock<DepsMock>>();
  AboutDialog aboutDialog(nullptr, deps);
  QTimer::singleShot(0, &aboutDialog, &QDialog::accept);

  aboutDialog.exec();

  const auto label = aboutDialog.findChild<QLabel *>("m_pDevelopersLabel");
  EXPECT_TRUE(label->text().contains("Chris&nbsp;Schoeneman"));
}

TEST(AboutDialogTests, exec_setsCopyrightLabel)
{
  TestQtFullApp app;
  const auto deps = std::make_shared<NiceMock<DepsMock>>();
  AboutDialog aboutDialog(nullptr, deps);
  QTimer::singleShot(0, &aboutDialog, &QDialog::accept);

  aboutDialog.exec();

  EXPECT_EQ(
      aboutDialog.findChild<QLabel *>("m_pCopyrightLabel")->text(), QString::fromStdString(deskflow::copyright())
  );
}

TEST(AboutDialogTests, exec_inDarkMode_usesDarkLogo)
{
  TestQtFullApp app;
  const auto deps = std::make_shared<NiceMock<DepsMock>>();
  AboutDialog aboutDialog(nullptr, deps);
  QTimer::singleShot(0, &aboutDialog, &QDialog::accept);
  EXPECT_CALL(*deps, isDarkMode()).WillOnce(Return(true));

  aboutDialog.exec();

  const QPixmap expectedLogo(":/image/logo-dark.png");
  const auto actualLogo = aboutDialog.findChild<QLabel *>("m_pLabel_Logo")->pixmap();
  EXPECT_FALSE(actualLogo.isNull());
  EXPECT_EQ(actualLogo.toImage(), expectedLogo.toImage());
}

TEST(AboutDialogTests, exec_notInDarkMode_usesLightLogo)
{
  TestQtFullApp app;
  const auto deps = std::make_shared<NiceMock<DepsMock>>();
  AboutDialog aboutDialog(nullptr, deps);
  QTimer::singleShot(0, &aboutDialog, &QDialog::accept);
  EXPECT_CALL(*deps, isDarkMode()).WillOnce(Return(false));

  aboutDialog.exec();

  const QPixmap expectedLogo(":/image/logo-light.png");
  const auto actualLogo = aboutDialog.findChild<QLabel *>("m_pLabel_Logo")->pixmap();
  EXPECT_FALSE(actualLogo.isNull());
  EXPECT_EQ(actualLogo.toImage(), expectedLogo.toImage());
}

#endif
