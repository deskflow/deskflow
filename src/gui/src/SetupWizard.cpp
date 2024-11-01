/*
 * Deskflow -- mouse and keyboard sharing utility
 *
 * SPDX-FileCopyrightText: Copyright (C) 2024 Chris Rizzitello <sithlord48@gmail.com>
 * SPDX-FileCopyrightText: Copyright (C) 2012 Symless Ltd.
 * SPDX-License-Identifier: GPL-2.0
 */

#include "SetupWizard.h"

#include <QApplication>
#include <QFormLayout>
#include <QPushButton>

#include "gui/config/AppConfig.h"
#include "gui/styles.h"
#include "gui/validators/ScreenNameValidator.h"
#include "gui/validators/ValidationError.h"

SetupWizard::SetupWizard(AppConfig &appConfig)
    : m_appConfig{appConfig},
      m_lblError{new QLabel(this)},
      m_lineName{new QLineEdit(this)},
      m_btnApply{new QPushButton(tr("Continue"), this)}
{
  setWindowTitle(tr("Setup Deskflow"));

  setFixedSize(740, 550);

  auto headerFont = QFont(QStringLiteral("Cantarell"), 18, 600);

  auto labelTitle = new QLabel(this);
  labelTitle->setFont(headerFont);
  labelTitle->setText(tr("Name your computer"));
  labelTitle->setAlignment(Qt::AlignHCenter | Qt::AlignHCenter);

  auto labelName = new QLabel(this);
  labelName->setText(tr("Computer Name"));

  auto formLayout = new QFormLayout();
  formLayout->addRow(labelName, m_lineName);
  formLayout->addWidget(m_lblError);
  formLayout->addWidget(new QLabel(tr("Call your computer something short and meaningful, but it must have:"), this));
  formLayout->addWidget(new QLabel(tr("\t⬤ No spaces"), this));
  formLayout->addWidget(new QLabel(tr("\t⬤ Only these special characters: _ - ."), this));
  formLayout->addWidget(new QLabel(tr("\t⬤ Only English characters and numbers"), this));
  formLayout->addWidget(new QLabel(tr("\t⬤ A different name from other computers"), this));

  auto labelImage = new QLabel(this);
  labelImage->setPixmap(QPixmap(QStringLiteral(":/image/welcome.png")));

  auto mainLayout = new QVBoxLayout();
  mainLayout->setContentsMargins(9, 30, 9, 9);
  mainLayout->addWidget(labelImage, 0, Qt::AlignHCenter);
  mainLayout->addWidget(labelTitle);
  mainLayout->addLayout(formLayout);
  mainLayout->addSpacerItem(new QSpacerItem(0, 25, QSizePolicy::Fixed, QSizePolicy::Fixed));
  mainLayout->addWidget(m_btnApply);

  setLayout(mainLayout);

  m_lblError->setStyleSheet(deskflow::gui::kStyleErrorActiveLabel);

  m_lineName->setText(appConfig.screenName());
  m_lineName->setValidator(
      new validators::ScreenNameValidator(m_lineName, new validators::ValidationError(this, m_lblError))
  );

  connect(m_btnApply, &QPushButton::clicked, this, &SetupWizard::accept);
  connect(m_lineName, &QLineEdit::textChanged, this, &SetupWizard::nameChanged);
}

void SetupWizard::accept()
{
  m_appConfig.setWizardHasRun();
  m_appConfig.setScreenName(m_lineName->text());
  QDialog::accept();
}

void SetupWizard::nameChanged(const QString &error)
{
  m_btnApply->setEnabled(m_lineName->hasAcceptableInput());
}

void SetupWizard::reject()
{
  QDialog::reject();
  QApplication::exit();
}
