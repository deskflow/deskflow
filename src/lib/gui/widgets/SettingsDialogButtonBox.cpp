/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2026 Deskflow Developers
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#include "SettingsDialogButtonBox.h"
#include "common/Settings.h"

#include <QDialogButtonBox>
#include <QEvent>
#include <QHBoxLayout>
#include <QIcon>
#include <QLabel>
#include <QPushButton>

SettingsDialogButtonBox::SettingsDialogButtonBox(QWidget *parent)
    : QWidget{parent},
      m_buttonBox{new QDialogButtonBox(this)},
      m_lblText{new QLabel(this)},
      m_lblIcon{new QLabel(this)}
{
  m_lblIcon->setPixmap(QIcon::fromTheme(QIcon::ThemeIcon::DialogWarning).pixmap(24, 24));
  m_buttonBox->addButton(QDialogButtonBox::Save);
  m_buttonBox->addButton(QDialogButtonBox::Cancel);
  m_buttonBox->addButton(QDialogButtonBox::Reset);
  m_buttonBox->addButton(QDialogButtonBox::RestoreDefaults);
  settingsWritableChanged(Settings::isWritable());

  auto layout = new QHBoxLayout(this);
  layout->addWidget(m_lblIcon);
  layout->addWidget(m_lblText);
  layout->addWidget(m_buttonBox);
  setLayout(layout);

  updateText();

  connect(m_buttonBox, &QDialogButtonBox::accepted, this, &SettingsDialogButtonBox::accepted);
  connect(m_buttonBox, &QDialogButtonBox::rejected, this, &SettingsDialogButtonBox::rejected);
  connect(m_buttonBox->button(QDialogButtonBox::Reset), &QPushButton::clicked, this, &SettingsDialogButtonBox::reset);
  connect(
      m_buttonBox->button(QDialogButtonBox::RestoreDefaults), &QPushButton::clicked, this,
      &SettingsDialogButtonBox::restoreDefault
  );
  connect(
      Settings::instance(), &Settings::settingsWritableChanged, this, &SettingsDialogButtonBox::settingsWritableChanged
  );
}

void SettingsDialogButtonBox::enableSave(bool enable) const
{
  m_buttonBox->button(QDialogButtonBox::Save)->setEnabled(enable);
}

void SettingsDialogButtonBox::enableReset(bool enable) const
{
  m_buttonBox->button(QDialogButtonBox::Reset)->setEnabled(enable);
}

void SettingsDialogButtonBox::enableRestoreDefaults(bool enable) const
{
  m_buttonBox->button(QDialogButtonBox::RestoreDefaults)->setEnabled(enable);
}

void SettingsDialogButtonBox::changeEvent(QEvent *e)
{
  QWidget::changeEvent(e);
  if (e->type() == QEvent::LanguageChange)
    updateText();
}

void SettingsDialogButtonBox::updateText()
{
  m_lblText->setText(tr("Settings are read only"));
  m_lblText->setToolTip(tr("%1 is not writable").arg(Settings::settingsFile()));

  m_buttonBox->button(QDialogButtonBox::Save)->setToolTip(tr("Close and save changes"));
  m_buttonBox->button(QDialogButtonBox::Cancel)->setToolTip(tr("Close and forget changes"));
  m_buttonBox->button(QDialogButtonBox::Reset)->setToolTip(tr("Reset to stored values"));
  m_buttonBox->button(QDialogButtonBox::RestoreDefaults)->setToolTip(tr("Reset to default values"));
}

void SettingsDialogButtonBox::settingsWritableChanged(bool writable)
{
  // Hide all first to prevent any change in overall width
  m_lblIcon->setVisible(false);
  m_lblText->setVisible(false);
  m_buttonBox->button(QDialogButtonBox::Save)->setVisible(false);
  m_buttonBox->button(QDialogButtonBox::Reset)->setVisible(false);
  m_buttonBox->button(QDialogButtonBox::RestoreDefaults)->setVisible(false);

  m_lblIcon->setVisible(!writable);
  m_lblText->setVisible(!writable);
  m_buttonBox->button(QDialogButtonBox::Save)->setVisible(writable);
  m_buttonBox->button(QDialogButtonBox::Reset)->setVisible(writable);
  m_buttonBox->button(QDialogButtonBox::RestoreDefaults)->setVisible(writable);
}
