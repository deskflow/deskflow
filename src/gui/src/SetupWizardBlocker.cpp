/*
 * synergy -- mouse and keyboard sharing utility
 * Copyright (C) 2021 Symless Ltd.
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

#include "SetupWizardBlocker.h"

#include "MainWindow.h"

#include <QDesktopServices>
#include <QUrl>

static const std::vector<const char *> blockerTitels = {
    "Wayland is not yet supported",
};

static const std::vector<const char *> blockerText = {
    "We have detected your system is using Wayland which is not currently \n"
    "supported, but we are working on it. It's top of our priority list. \n"
    "\n"
    "Please switch to Xorg if you wish to continue using Synergy today.",
};

SetupWizardBlocker::SetupWizardBlocker(BlockerType type) {
  setupUi(this);

  m_pLabelTitle->setText(blockerTitels[static_cast<int>(type)]);
  m_pLabelInfo->setText(blockerText[static_cast<int>(type)]);

  connect(
      m_pButtonSupport, &QPushButton::released, this,
      &SetupWizardBlocker::onlineSupport);
  connect(
      m_pButtonCancel, &QPushButton::released, this,
      &SetupWizardBlocker::cancel);
}

void SetupWizardBlocker::onlineSupport() {
  QDesktopServices::openUrl(
      QUrl("https://symless.com/synergy/help?source=gui"));
  cancel();
}

void SetupWizardBlocker::cancel() {
  QDialog::reject();
  QCoreApplication::quit();
}
