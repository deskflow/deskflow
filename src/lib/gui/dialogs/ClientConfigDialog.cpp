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
  ui->tabWidget->setCurrentIndex(0);
  updateText();
  initConnections();
  load();
  updateSharingControls();
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

  connect(
      ui->cbDynamicConnectTime, &QCheckBox::checkStateChanged, this, &ClientConfigDialog::setButtonBoxEnabledButtons
  );
  connect(ui->cbLanguageSync, &QCheckBox::checkStateChanged, this, &ClientConfigDialog::setButtonBoxEnabledButtons);
  connect(ui->cbYScrollInvert, &QCheckBox::checkStateChanged, this, &ClientConfigDialog::setButtonBoxEnabledButtons);
  connect(ui->sbYScrollScale, &QDoubleSpinBox::valueChanged, this, &ClientConfigDialog::setButtonBoxEnabledButtons);
  connect(ui->cbXScrollInvert, &QCheckBox::checkStateChanged, this, &ClientConfigDialog::setButtonBoxEnabledButtons);
  connect(ui->sbXScrollScale, &QDoubleSpinBox::valueChanged, this, &ClientConfigDialog::setButtonBoxEnabledButtons);
  connect(ui->groupMouserClient, &QGroupBox::toggled, this, &ClientConfigDialog::onMouserClientToggled);
  connect(ui->lineMouserToken, &QLineEdit::textChanged, this, &ClientConfigDialog::setButtonBoxEnabledButtons);
}

bool ClientConfigDialog::isSharingModified() const
{
  return ui->groupMouserClient->isChecked() != m_mouserEnabled ||
         ui->lineMouserToken->text() != m_mouserToken;
}

bool ClientConfigDialog::isModified() const
{
  return (ui->cbDynamicConnectTime->isChecked() != Settings::value(Settings::Client::DynamicConnectionRetry).toBool()
         ) ||
         (ui->cbLanguageSync->isChecked() != Settings::value(Settings::Client::LanguageSync).toBool()) ||
         (ui->cbYScrollInvert->isChecked() != Settings::value(Settings::Client::InvertYScroll).toBool()) ||
         (ui->sbYScrollScale->value() != Settings::value(Settings::Client::YScrollScale).toDouble()) ||
         (ui->cbXScrollInvert->isChecked() != Settings::value(Settings::Client::InvertXScroll).toBool()) ||
         (ui->sbXScrollScale->value() != Settings::value(Settings::Client::XScrollScale).toDouble()) ||
         isSharingModified();
}

bool ClientConfigDialog::isDefault() const
{
  return (ui->cbDynamicConnectTime->isChecked() ==
          Settings::defaultValue(Settings::Client::DynamicConnectionRetry).toBool()) &&
         (ui->cbLanguageSync->isChecked() == Settings::defaultValue(Settings::Client::LanguageSync).toBool()) &&
         (ui->cbYScrollInvert->isChecked() == Settings::defaultValue(Settings::Client::InvertYScroll).toBool()) &&
         (ui->sbYScrollScale->value() == Settings::defaultValue(Settings::Client::YScrollScale).toDouble()) &&
         (ui->cbXScrollInvert->isChecked() == Settings::defaultValue(Settings::Client::InvertXScroll).toBool()) &&
         (ui->sbXScrollScale->value() == Settings::defaultValue(Settings::Client::XScrollScale).toDouble()) &&
         !ui->groupMouserClient->isChecked() && ui->lineMouserToken->text().isEmpty();
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
  ui->cbDynamicConnectTime->setChecked(Settings::value(Settings::Client::DynamicConnectionRetry).toBool());
  ui->cbLanguageSync->setChecked(Settings::value(Settings::Client::LanguageSync).toBool());
  ui->cbYScrollInvert->setChecked(Settings::value(Settings::Client::InvertYScroll).toBool());
  ui->sbYScrollScale->setValue(Settings::value(Settings::Client::YScrollScale).toDouble());
  ui->cbXScrollInvert->setChecked(Settings::value(Settings::Client::InvertXScroll).toBool());
  ui->sbXScrollScale->setValue(Settings::value(Settings::Client::XScrollScale).toDouble());

  m_mouserEnabled = Settings::value(Settings::Client::MouserEnabled).toBool();
  m_mouserToken = Settings::value(Settings::Client::MouserToken).toString();
  ui->groupMouserClient->setChecked(m_mouserEnabled);
  ui->lineMouserToken->setText(m_mouserToken);
  updateSharingControls();
}

void ClientConfigDialog::resetToDefault()
{
  ui->cbDynamicConnectTime->setChecked(Settings::defaultValue(Settings::Client::DynamicConnectionRetry).toBool());
  ui->cbLanguageSync->setChecked(Settings::defaultValue(Settings::Client::LanguageSync).toBool());
  ui->cbYScrollInvert->setChecked(Settings::defaultValue(Settings::Client::InvertYScroll).toBool());
  ui->sbYScrollScale->setValue(Settings::defaultValue(Settings::Client::YScrollScale).toDouble());
  ui->cbXScrollInvert->setChecked(Settings::defaultValue(Settings::Client::InvertXScroll).toBool());
  ui->sbXScrollScale->setValue(Settings::defaultValue(Settings::Client::XScrollScale).toDouble());

  m_mouserEnabled = false;
  m_mouserToken.clear();
  ui->groupMouserClient->setChecked(false);
  ui->lineMouserToken->clear();
  updateSharingControls();
}

void ClientConfigDialog::save()
{
  Settings::setValue(Settings::Client::DynamicConnectionRetry, ui->cbDynamicConnectTime->isChecked());
  Settings::setValue(Settings::Client::LanguageSync, ui->cbLanguageSync->isChecked());
  Settings::setValue(Settings::Client::InvertYScroll, ui->cbYScrollInvert->isChecked());
  Settings::setValue(Settings::Client::YScrollScale, ui->sbYScrollScale->value());
  Settings::setValue(Settings::Client::InvertXScroll, ui->cbXScrollInvert->isChecked());
  Settings::setValue(Settings::Client::XScrollScale, ui->sbXScrollScale->value());
  Settings::setValue(Settings::Client::MouserEnabled, ui->groupMouserClient->isChecked());
  Settings::setValue(Settings::Client::MouserToken, ui->lineMouserToken->text());
  QDialog::accept();
}

void ClientConfigDialog::updateSharingControls()
{
  const bool writable = Settings::isWritable();
  const bool sharingEnabled = ui->groupMouserClient->isChecked();
  ui->groupMouserClient->setEnabled(writable);
  ui->lineMouserToken->setEnabled(writable && sharingEnabled);
}

void ClientConfigDialog::onMouserClientToggled(bool /*enabled*/)
{
  updateSharingControls();
  setButtonBoxEnabledButtons();
}
