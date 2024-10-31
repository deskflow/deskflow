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

#include "common/copyright.h"
#include "common/version.h"
#include "gui/style_utils.h"

#include <QClipboard>
#include <QDateTime>
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QStyle>

AboutDialog::AboutDialog(QWidget *parent) : QDialog(parent)
{
  setWindowTitle(tr("About Deskflow"));

  QString version = QString::fromStdString(deskflow::version());

  auto copyIcon = QIcon::fromTheme(
      QIcon::ThemeIcon::EditCopy, deskflow::gui::isDarkMode() ? QIcon(s_darkCopy) : QIcon(s_lightCopy)
  );

  auto btnCopyVersion = new QPushButton(copyIcon, QString(), this);
  btnCopyVersion->setFlat(true);
  connect(btnCopyVersion, &QPushButton::clicked, this, [version] { qApp->clipboard()->setText(version); });

  auto versionLayout = new QHBoxLayout();
  versionLayout->addWidget(new QLabel(tr("Version:")));
  versionLayout->addWidget(new QLabel(version));
  versionLayout->addWidget(btnCopyVersion);
  versionLayout->addSpacerItem(new QSpacerItem(0, 0, QSizePolicy::Expanding, QSizePolicy::Fixed));

  auto lblLogo = new QLabel(this);
  lblLogo->setPixmap(deskflow::gui::isDarkMode() ? s_darkLogo : s_lightLogo);

  auto lblCopyright = new QLabel(QString::fromStdString(deskflow::kCopyright));

  auto boldFont = font();
  boldFont.setBold(true);

  auto lblDevsTitle = new QLabel(tr("Important developers"));
  lblDevsTitle->setFont(boldFont);

  auto lblDevsBody = new QLabel(QStringLiteral("%1\n").arg(s_awesomeDevs.join(", ")));
  lblDevsBody->setWordWrap(true);

  auto btnOk = new QPushButton(tr("Ok"), this);
  btnOk->setDefault(true);
  connect(btnOk, &QPushButton::clicked, this, [this] { close(); });

  auto mainLayout = new QVBoxLayout();
  mainLayout->addWidget(lblLogo);
  mainLayout->addLayout(versionLayout);
  mainLayout->addWidget(new QLabel(tr("Keyboard and mouse sharing application"), this));
  mainLayout->addWidget(lblCopyright);
  mainLayout->addWidget(lblDevsTitle);
  mainLayout->addWidget(lblDevsBody);
  mainLayout->addWidget(btnOk);

  setLayout(mainLayout);
  adjustSize();
  setFixedSize(size());
}
