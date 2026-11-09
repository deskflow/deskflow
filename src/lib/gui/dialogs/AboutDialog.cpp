/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2024 Chris Rizzitello <sithlord48@gmail.com>
 * SPDX-FileCopyrightText: (C) 2012 - 2016 Synergy App Ltd
 * SPDX-FileCopyrightText: (C) 2008 Volker Lanz <vl@fidra.de>
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#include "AboutDialog.h"
#include "ui_AboutDialog.h"

#include "common/Constants.h"
#include "common/Settings.h"
#include "common/VersionInfo.h"

#include "gui/DialogPresentation.h"
#include "gui/ExperienceStyle.h"
#include <QClipboard>

AboutDialog::AboutDialog(QWidget *parent) : QDialog(parent), ui{std::make_unique<Ui::AboutDialog>()}
{
  ui->setupUi(this);
  deskflow::gui::applyExperienceStyle(this);
  layout()->setContentsMargins(20, 20, 20, 16);
  layout()->setSpacing(16);

  const int px = 72;
  const QSize pixmapSize(px, px);
  ui->lblIcon->setFixedSize(pixmapSize);

  ui->lblIcon->setPixmap(QIcon(":/icons/deskflow-light/apps/64/org.deskflow.deskflow.svg").pixmap(pixmapSize));

  ui->btnCopyVersion->setText(tr("Copy version"));
  ui->btnCopyVersion->setIcon(QIcon());
  ui->btnCopyVersion->setMaximumWidth(QWIDGETSIZE_MAX);
  new deskflow::gui::FormButtonMetrics(ui->btnCopyVersion);
  ui->scrollArea->setFrameShape(QFrame::NoFrame);
  connect(ui->btnCopyVersion, &QPushButton::clicked, this, &AboutDialog::copyVersionText);

  ui->lblVersion->setText(QString(kVersionGitSha).isEmpty() ? kVersion : kDisplayVersion);
  ui->lblDescription->setText(kAppDescription);
  ui->lblCopyright->setText(kCopyright);

  // Use non-breaking space in each awesome dev name so names are not split across lines.
  QStringList devsNbsp;
  for (const auto &dev : s_awesomeDevs) {
    QString withNbsp = dev;
    devsNbsp.append(withNbsp.replace(" ", QStringLiteral("&nbsp;")));
  }

  ui->lblImportantDevs->setTextFormat(Qt::RichText);
  ui->lblImportantDevs->setText(QStringLiteral("%1\n").arg(devsNbsp.join(", ")));

  ui->btnOk->setDefault(true);
  connect(ui->btnOk, &QPushButton::clicked, this, &AboutDialog::close);
}

void AboutDialog::copyVersionText() const
{
  const auto version = QString(kVersionGitSha).isEmpty() ? QString(kVersion) : QString(kDisplayVersion);
  QString infoString =
      QStringLiteral("%1: %2\nQt: %3\nSystem: %4").arg(kAppName, version, qVersion(), QSysInfo::prettyProductName());
  if (Settings::isPortableMode()) {
    infoString.append(QStringLiteral("\nPortable Mode"));
  }
#ifdef Q_OS_LINUX
  infoString.append(QStringLiteral("\nSession: %1 (%2)")
                        .arg(qEnvironmentVariable("XDG_CURRENT_DESKTOP"), qEnvironmentVariable("XDG_SESSION_TYPE")));
#endif
  QGuiApplication::clipboard()->setText(infoString);
}

AboutDialog::~AboutDialog() = default;
