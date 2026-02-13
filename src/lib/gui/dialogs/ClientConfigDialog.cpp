/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2026 Deskflow Developers
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#include "ClientConfigDialog.h"
#include "ui_ClientConfigDialog.h"

#include "common/Settings.h"

#include <QPushButton>

ClientConfigDialog::ClientConfigDialog(QWidget *parent) : QDialog(parent), ui(new Ui::ClientConfigDialog)
{
  ui->setupUi(this);
  updateText();
  initConnections();
  load();
  setButtonBoxEnabledButtons();
}

ClientConfigDialog::~ClientConfigDialog()
{
  delete ui;
}

void ClientConfigDialog::changeEvent(QEvent *e)
{
  QDialog::changeEvent(e);
  if (e->type() == QEvent::LanguageChange) {
    ui->retranslateUi(this);
    updateText();
  }
}

void ClientConfigDialog::updateText() const
{
  ui->buttonBox->button(QDialogButtonBox::Save)->setToolTip(tr("Close and save changes"));
  ui->buttonBox->button(QDialogButtonBox::Cancel)->setToolTip(tr("Close and forget changes"));
  ui->buttonBox->button(QDialogButtonBox::Reset)->setToolTip(tr("Reset to stored values"));
  ui->buttonBox->button(QDialogButtonBox::RestoreDefaults)->setToolTip(tr("Reset to default values"));
}

void ClientConfigDialog::initConnections() const
{
  connect(ui->buttonBox, &QDialogButtonBox::accepted, this, &ClientConfigDialog::save);
  connect(ui->buttonBox, &QDialogButtonBox::rejected, this, &QDialog::reject);
  connect(ui->buttonBox->button(QDialogButtonBox::Reset), &QPushButton::clicked, this, &ClientConfigDialog::load);
  connect(
      ui->buttonBox->button(QDialogButtonBox::RestoreDefaults), &QPushButton::clicked, this,
      &ClientConfigDialog::resetToDefault
  );

  connect(ui->cbLanguageSync, &QCheckBox::checkStateChanged, this, &ClientConfigDialog::setButtonBoxEnabledButtons);
  connect(ui->cbYScrollInvert, &QCheckBox::checkStateChanged, this, &ClientConfigDialog::setButtonBoxEnabledButtons);
  connect(ui->sbYScrollScale, &QDoubleSpinBox::valueChanged, this, &ClientConfigDialog::setButtonBoxEnabledButtons);
  connect(ui->cbXScrollInvert, &QCheckBox::checkStateChanged, this, &ClientConfigDialog::setButtonBoxEnabledButtons);
  connect(ui->sbXScrollScale, &QDoubleSpinBox::valueChanged, this, &ClientConfigDialog::setButtonBoxEnabledButtons);
}

bool ClientConfigDialog::isModified() const
{
  return (ui->cbLanguageSync->isChecked() != Settings::value(Settings::Client::LanguageSync).toBool()) ||
         (ui->cbYScrollInvert->isChecked() != Settings::value(Settings::Client::InvertYScroll).toBool()) ||
         (ui->sbYScrollScale->value() != Settings::value(Settings::Client::YScrollScale).toDouble()) ||
         (ui->cbXScrollInvert->isChecked() != Settings::value(Settings::Client::InvertXScroll).toBool()) ||
         (ui->sbXScrollScale->value() != Settings::value(Settings::Client::XScrollScale).toDouble());
}

bool ClientConfigDialog::isDefault() const
{
  return (ui->cbLanguageSync->isChecked() == Settings::defaultValue(Settings::Client::LanguageSync).toBool()) &&
         (ui->cbYScrollInvert->isChecked() == Settings::defaultValue(Settings::Client::InvertYScroll).toBool()) &&
         (ui->sbYScrollScale->value() == Settings::defaultValue(Settings::Client::YScrollScale).toDouble()) &&
         (ui->cbXScrollInvert->isChecked() == Settings::defaultValue(Settings::Client::InvertXScroll).toBool()) &&
         (ui->sbXScrollScale->value() == Settings::defaultValue(Settings::Client::XScrollScale).toDouble());
}

void ClientConfigDialog::setButtonBoxEnabledButtons() const
{
  const bool modified = isModified();
  ui->buttonBox->button(QDialogButtonBox::Save)->setEnabled(modified);
  ui->buttonBox->button(QDialogButtonBox::Reset)->setEnabled(modified);
  ui->buttonBox->button(QDialogButtonBox::RestoreDefaults)->setEnabled(!isDefault());
}

void ClientConfigDialog::load()
{
  ui->cbLanguageSync->setChecked(Settings::value(Settings::Client::LanguageSync).toBool());
  ui->cbYScrollInvert->setChecked(Settings::value(Settings::Client::InvertYScroll).toBool());
  ui->sbYScrollScale->setValue(Settings::value(Settings::Client::YScrollScale).toDouble());
  ui->cbXScrollInvert->setChecked(Settings::value(Settings::Client::InvertXScroll).toBool());
  ui->sbXScrollScale->setValue(Settings::value(Settings::Client::XScrollScale).toDouble());
}

void ClientConfigDialog::resetToDefault()
{
  ui->cbLanguageSync->setChecked(Settings::defaultValue(Settings::Client::LanguageSync).toBool());
  ui->cbYScrollInvert->setChecked(Settings::defaultValue(Settings::Client::InvertYScroll).toBool());
  ui->sbYScrollScale->setValue(Settings::defaultValue(Settings::Client::YScrollScale).toDouble());
  ui->cbXScrollInvert->setChecked(Settings::defaultValue(Settings::Client::InvertXScroll).toBool());
  ui->sbXScrollScale->setValue(Settings::defaultValue(Settings::Client::XScrollScale).toDouble());
}

void ClientConfigDialog::save()
{
  Settings::setValue(Settings::Client::LanguageSync, ui->cbLanguageSync->isChecked());
  Settings::setValue(Settings::Client::InvertYScroll, ui->cbYScrollInvert->isChecked());
  Settings::setValue(Settings::Client::YScrollScale, ui->sbYScrollScale->value());
  Settings::setValue(Settings::Client::InvertXScroll, ui->cbXScrollInvert->isChecked());
  Settings::setValue(Settings::Client::XScrollScale, ui->sbXScrollScale->value());
  QDialog::accept();
}
