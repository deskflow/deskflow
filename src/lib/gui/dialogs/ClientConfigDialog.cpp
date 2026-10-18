/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2026 Deskflow Developers
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#include "ClientConfigDialog.h"
#include "ui_ClientConfigDialog.h"

#include "common/KeySequence.h"
#include "common/NavigationTypes.h"
#include "common/Settings.h"
#include "gui/widgets/SettingsDialogButtonBox.h"

#include <QPushButton>

ClientConfigDialog::ClientConfigDialog(QWidget *parent)
    : QDialog(parent),
      ui(new Ui::ClientConfigDialog),
      m_buttonBox{new SettingsDialogButtonBox(this)}
{
  ui->setupUi(this);
  layout()->addWidget(m_buttonBox);
  initConnections();
  load();
  updateControls();
  setButtonBoxEnabledButtons();
}

ClientConfigDialog::~ClientConfigDialog()
{
  delete ui;
}

void ClientConfigDialog::changeEvent(QEvent *e)
{
  QDialog::changeEvent(e);
  if (e->type() == QEvent::LanguageChange)
    ui->retranslateUi(this);
}

void ClientConfigDialog::updateControls() const
{
  const auto writable = Settings::isWritable();
  ui->cbDynamicConnectTime->setEnabled(writable);
  ui->cbLanguageSync->setEnabled(writable);
  ui->cbXScrollInvert->setEnabled(writable);
  ui->sbXScrollScale->setEnabled(writable);
  ui->cbYScrollInvert->setEnabled(writable);
  ui->sbYScrollScale->setEnabled(writable);
  ui->comboNavigationAction1->setEnabled(writable);
  ui->comboNavigationAction2->setEnabled(writable);
  ui->keyNavigationAction1->setEnabled(
      writable && ui->comboNavigationAction1->currentIndex() == static_cast<int>(NavigationOutputAction::Keystroke)
  );
  ui->keyNavigationAction2->setEnabled(
      writable && ui->comboNavigationAction2->currentIndex() == static_cast<int>(NavigationOutputAction::Keystroke)
  );
}

void ClientConfigDialog::initConnections() const
{
  connect(m_buttonBox, &SettingsDialogButtonBox::accepted, this, &ClientConfigDialog::save);
  connect(m_buttonBox, &SettingsDialogButtonBox::rejected, this, &QDialog::reject);
  connect(m_buttonBox, &SettingsDialogButtonBox::reset, this, &ClientConfigDialog::load);
  connect(m_buttonBox, &SettingsDialogButtonBox::restoreDefault, this, &ClientConfigDialog::resetToDefault);

  connect(
      ui->cbDynamicConnectTime, &QCheckBox::checkStateChanged, this, &ClientConfigDialog::setButtonBoxEnabledButtons
  );
  connect(ui->cbLanguageSync, &QCheckBox::checkStateChanged, this, &ClientConfigDialog::setButtonBoxEnabledButtons);
  connect(ui->cbYScrollInvert, &QCheckBox::checkStateChanged, this, &ClientConfigDialog::setButtonBoxEnabledButtons);
  connect(ui->sbYScrollScale, &QDoubleSpinBox::valueChanged, this, &ClientConfigDialog::setButtonBoxEnabledButtons);
  connect(ui->cbXScrollInvert, &QCheckBox::checkStateChanged, this, &ClientConfigDialog::setButtonBoxEnabledButtons);
  connect(ui->sbXScrollScale, &QDoubleSpinBox::valueChanged, this, &ClientConfigDialog::setButtonBoxEnabledButtons);
  connect(ui->comboNavigationAction1, &QComboBox::currentIndexChanged, this, [this] {
    updateControls();
    setButtonBoxEnabledButtons();
  });
  connect(ui->comboNavigationAction2, &QComboBox::currentIndexChanged, this, [this] {
    updateControls();
    setButtonBoxEnabledButtons();
  });
  connect(
      ui->keyNavigationAction1, &KeySequenceWidget::keySequenceChanged, this,
      &ClientConfigDialog::setButtonBoxEnabledButtons
  );
  connect(
      ui->keyNavigationAction2, &KeySequenceWidget::keySequenceChanged, this,
      &ClientConfigDialog::setButtonBoxEnabledButtons
  );
  connect(Settings::instance(), &Settings::settingsWritableChanged, this, &ClientConfigDialog::updateControls);
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
         (ui->comboNavigationAction1->currentIndex() !=
          Settings::value(Settings::Client::NavigationGestureAction1).toInt()) ||
         (ui->comboNavigationAction2->currentIndex() !=
          Settings::value(Settings::Client::NavigationGestureAction2).toInt()) ||
         (ui->keyNavigationAction1->keySequence().toString() !=
          Settings::value(Settings::Client::NavigationGestureShortcut1).toString()) ||
         (ui->keyNavigationAction2->keySequence().toString() !=
          Settings::value(Settings::Client::NavigationGestureShortcut2).toString());
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
         (ui->comboNavigationAction1->currentIndex() ==
          Settings::defaultValue(Settings::Client::NavigationGestureAction1).toInt()) &&
         (ui->comboNavigationAction2->currentIndex() ==
          Settings::defaultValue(Settings::Client::NavigationGestureAction2).toInt()) &&
         (ui->keyNavigationAction1->keySequence().toString() ==
          Settings::defaultValue(Settings::Client::NavigationGestureShortcut1).toString()) &&
         (ui->keyNavigationAction2->keySequence().toString() ==
          Settings::defaultValue(Settings::Client::NavigationGestureShortcut2).toString());
}

void ClientConfigDialog::setButtonBoxEnabledButtons() const
{
  const bool modified = isModified();
  m_buttonBox->enableSave(modified && navigationShortcutsValid());
  m_buttonBox->enableReset(modified);
  m_buttonBox->enableRestoreDefaults(!isDefault());
}

bool ClientConfigDialog::navigationShortcutsValid() const
{
  const auto custom = static_cast<int>(NavigationOutputAction::Keystroke);
  const auto action1 = ui->keyNavigationAction1->keySequence();
  const auto action2 = ui->keyNavigationAction2->keySequence();
  return (ui->comboNavigationAction1->currentIndex() != custom || (action1.valid() && !action1.isMouseButton())) &&
         (ui->comboNavigationAction2->currentIndex() != custom || (action2.valid() && !action2.isMouseButton()));
}

void ClientConfigDialog::load()
{
  ui->cbDynamicConnectTime->setChecked(Settings::value(Settings::Client::DynamicConnectionRetry).toBool());
  ui->cbLanguageSync->setChecked(Settings::value(Settings::Client::LanguageSync).toBool());
  ui->cbYScrollInvert->setChecked(Settings::value(Settings::Client::InvertYScroll).toBool());
  ui->sbYScrollScale->setValue(Settings::value(Settings::Client::YScrollScale).toDouble());
  ui->cbXScrollInvert->setChecked(Settings::value(Settings::Client::InvertXScroll).toBool());
  ui->sbXScrollScale->setValue(Settings::value(Settings::Client::XScrollScale).toDouble());
  ui->comboNavigationAction1->setCurrentIndex(Settings::value(Settings::Client::NavigationGestureAction1).toInt());
  ui->comboNavigationAction2->setCurrentIndex(Settings::value(Settings::Client::NavigationGestureAction2).toInt());
  ui->keyNavigationAction1->setKeySequence(
      KeySequence::fromString(Settings::value(Settings::Client::NavigationGestureShortcut1).toString())
  );
  ui->keyNavigationAction2->setKeySequence(
      KeySequence::fromString(Settings::value(Settings::Client::NavigationGestureShortcut2).toString())
  );
  updateControls();
}

void ClientConfigDialog::resetToDefault()
{
  ui->cbDynamicConnectTime->setChecked(Settings::defaultValue(Settings::Client::DynamicConnectionRetry).toBool());
  ui->cbLanguageSync->setChecked(Settings::defaultValue(Settings::Client::LanguageSync).toBool());
  ui->cbYScrollInvert->setChecked(Settings::defaultValue(Settings::Client::InvertYScroll).toBool());
  ui->sbYScrollScale->setValue(Settings::defaultValue(Settings::Client::YScrollScale).toDouble());
  ui->cbXScrollInvert->setChecked(Settings::defaultValue(Settings::Client::InvertXScroll).toBool());
  ui->sbXScrollScale->setValue(Settings::defaultValue(Settings::Client::XScrollScale).toDouble());
  ui->comboNavigationAction1->setCurrentIndex(
      Settings::defaultValue(Settings::Client::NavigationGestureAction1).toInt()
  );
  ui->comboNavigationAction2->setCurrentIndex(
      Settings::defaultValue(Settings::Client::NavigationGestureAction2).toInt()
  );
  ui->keyNavigationAction1->setKeySequence({});
  ui->keyNavigationAction2->setKeySequence({});
  updateControls();
}

void ClientConfigDialog::save()
{
  Settings::setValue(Settings::Client::DynamicConnectionRetry, ui->cbDynamicConnectTime->isChecked());
  Settings::setValue(Settings::Client::LanguageSync, ui->cbLanguageSync->isChecked());
  Settings::setValue(Settings::Client::InvertYScroll, ui->cbYScrollInvert->isChecked());
  Settings::setValue(Settings::Client::YScrollScale, ui->sbYScrollScale->value());
  Settings::setValue(Settings::Client::InvertXScroll, ui->cbXScrollInvert->isChecked());
  Settings::setValue(Settings::Client::XScrollScale, ui->sbXScrollScale->value());
  Settings::setValue(Settings::Client::NavigationGestureAction1, ui->comboNavigationAction1->currentIndex());
  Settings::setValue(Settings::Client::NavigationGestureAction2, ui->comboNavigationAction2->currentIndex());
  Settings::setValue(Settings::Client::NavigationGestureShortcut1, ui->keyNavigationAction1->keySequence().toString());
  Settings::setValue(Settings::Client::NavigationGestureShortcut2, ui->keyNavigationAction2->keySequence().toString());
  QDialog::accept();
}
