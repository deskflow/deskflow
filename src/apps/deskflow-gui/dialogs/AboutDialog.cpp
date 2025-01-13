/*
 * Deskflow -- mouse and keyboard sharing utility
 * Copyright (C) 2024 Chris Rizzitello <sithlord48@gmail.com>
 * Copyright (C) 2012 Symless Ltd.
 * Copyright (C) 2008 Volker Lanz (vl@fidra.de)
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

#include "AboutDialog.h"
#include "ui_AboutDialog.h"

#include "common/constants.h"
#include "gui/style_utils.h"

#include <QClipboard>
#include <QDateTime>
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QStyle>

AboutDialog::AboutDialog(QWidget *parent) : QDialog(parent), ui{std::make_unique<Ui::AboutDialog>()}
{
  ui->setupUi(this);

  const int px = (fontMetrics().height() * 6);
  const QSize pixmapSize(px, px);
  ui->lblIcon->setFixedSize(pixmapSize);

  ui->lblIcon->setPixmap(QPixmap(QIcon::fromTheme("deskflow").pixmap(QSize().scaled(pixmapSize, Qt::KeepAspectRatio))));

  ui->btnCopyVersion->setIcon(QIcon::fromTheme(QIcon::ThemeIcon::EditCopy));
  connect(ui->btnCopyVersion, &QPushButton::clicked, this, &AboutDialog::copyVersionText);

  // Set up the displayed version number
  auto versionString = QString(kVersion);
  if (versionString.endsWith(QStringLiteral(".0"))) {
    versionString.chop(2);
  } else {
    versionString.append(QStringLiteral(" (%1)").arg(kVersionGitSha));
  }

  ui->lblVersion->setText(versionString);

  ui->lblDescription->setText(kAppDescription);
  ui->lblCopyright->setText(kCopyright);
  ui->lblImportantDevs->setText(QStringLiteral("%1\n").arg(s_awesomeDevs.join(", ")));

  ui->btnOk->setDefault(true);
  connect(ui->btnOk, &QPushButton::clicked, this, [this] { close(); });

  adjustSize();
  resize(QSize(parent->width() * 0.65, height()));
  setMinimumSize(size());
}

void AboutDialog::copyVersionText()
{
  QString infoString = QStringLiteral("Deskflow: %1 (%2)\nQt: %3\nSystem: %4")
                           .arg(kVersion, kVersionGitSha, qVersion(), QSysInfo::prettyProductName());
#ifdef Q_OS_LINUX
  infoString.append(QStringLiteral("\nSession: %1 (%2)")
                        .arg(qEnvironmentVariable("XDG_CURRENT_DESKTOP"), qEnvironmentVariable("XDG_SESSION_TYPE")));
#endif
  QGuiApplication::clipboard()->setText(infoString);
}

AboutDialog::~AboutDialog() = default;
