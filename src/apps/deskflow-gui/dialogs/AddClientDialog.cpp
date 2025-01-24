/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2014 - 2016 Symless Ltd.
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#include "AddClientDialog.h"
#include "ui_AddClientDialog.h"

#include <QLabel>
#include <QPushButton>

AddClientDialog::AddClientDialog(const QString &clientName, QWidget *parent)
    : QDialog(parent, Qt::WindowTitleHint | Qt::WindowSystemMenuHint),
      ui{std::make_unique<Ui::AddClientDialog>()},
      m_AddResult(kAddClientIgnore)
{
  ui->setupUi(this);

  ui->m_pLabelHead->setText(
      "A client wants to connect. "
      "Please choose a location for " +
      clientName + "."
  );

  QIcon icon(":res/icons/64x64/video-display.png");
  QSize IconSize(32, 32);

  m_pButtonLeft = new QPushButton(this);
  m_pButtonLeft->setIcon(icon);
  m_pButtonLeft->setIconSize(IconSize);
  ui->gridLayout->addWidget(m_pButtonLeft, 2, 0, 1, 1, Qt::AlignCenter);
  connect(m_pButtonLeft, &QPushButton::clicked, this, &AddClientDialog::handleButtonLeft);

  m_pButtonUp = new QPushButton(this);
  m_pButtonUp->setIcon(icon);
  m_pButtonUp->setIconSize(IconSize);
  ui->gridLayout->addWidget(m_pButtonUp, 1, 1, 1, 1, Qt::AlignCenter);
  connect(m_pButtonUp, &QPushButton::clicked, this, &AddClientDialog::handleButtonUp);

  m_pButtonRight = new QPushButton(this);
  m_pButtonRight->setIcon(icon);
  m_pButtonRight->setIconSize(IconSize);
  ui->gridLayout->addWidget(m_pButtonRight, 2, 2, 1, 1, Qt::AlignCenter);
  connect(m_pButtonRight, &QPushButton::clicked, this, &AddClientDialog::handleButtonRight);

  m_pButtonDown = new QPushButton(this);
  m_pButtonDown->setIcon(icon);
  m_pButtonDown->setIconSize(IconSize);
  ui->gridLayout->addWidget(m_pButtonDown, 3, 1, 1, 1, Qt::AlignCenter);
  connect(m_pButtonDown, &QPushButton::clicked, this, &AddClientDialog::handleButtonDown);

  m_pLabelCenter = new QLabel(this);
  m_pLabelCenter->setPixmap(QPixmap(":res/icons/64x64/video-display.png"));
  ui->gridLayout->addWidget(m_pLabelCenter, 2, 1, 1, 1, Qt::AlignCenter);

#if defined(Q_OS_MAC)
  ui->m_pDialogButtonBox->setLayoutDirection(Qt::RightToLeft);
#endif

  QPushButton *advanced = ui->m_pDialogButtonBox->addButton("Advanced", QDialogButtonBox::HelpRole);
  connect(advanced, &QPushButton::clicked, this, &AddClientDialog::handleButtonAdvanced);
}

AddClientDialog::~AddClientDialog() = default;

void AddClientDialog::handleButtonLeft()
{
  m_AddResult = kAddClientLeft;
  close();
}

void AddClientDialog::handleButtonUp()
{
  m_AddResult = kAddClientUp;
  close();
}

void AddClientDialog::handleButtonRight()
{
  m_AddResult = kAddClientRight;
  close();
}

void AddClientDialog::handleButtonDown()
{
  m_AddResult = kAddClientDown;
  close();
}

void AddClientDialog::handleButtonAdvanced()
{
  m_AddResult = kAddClientOther;
  close();
}
