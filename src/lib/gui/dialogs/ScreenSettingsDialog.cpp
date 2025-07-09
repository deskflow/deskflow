/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2012 Symless Ltd.
 * SPDX-FileCopyrightText: (C) 2008 Volker Lanz <vl@fidra.de>
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#include "ScreenSettingsDialog.h"
#include "ui_ScreenSettingsDialog.h"

#include "gui/Styles.h"
#include "gui/config/Screen.h"
#include "validators/AliasValidator.h"
#include "validators/ScreenNameValidator.h"
#include "validators/ValidationError.h"

#include <QMessageBox>

using namespace deskflow::gui;
using enum ScreenConfig::Modifier;
using enum ScreenConfig::SwitchCorner;
using enum ScreenConfig::Fix;

ScreenSettingsDialog::~ScreenSettingsDialog() = default;

ScreenSettingsDialog::ScreenSettingsDialog(QWidget *parent, Screen *pScreen, const ScreenList *pScreens)
    : QDialog(parent, Qt::WindowTitleHint | Qt::WindowSystemMenuHint),
      ui{std::make_unique<Ui::ScreenSettingsDialog>()},
      m_pScreen(pScreen)
{

  ui->setupUi(this);

  ui->m_pLabelAliasError->setStyleSheet(kStyleErrorActiveLabel);
  ui->m_pLabelNameError->setStyleSheet(kStyleErrorActiveLabel);

  ui->m_pLineEditName->setText(m_pScreen->name());

  const auto valNameError = new validators::ValidationError(this, ui->m_pLabelNameError);
  const auto valName = new validators::ScreenNameValidator(ui->m_pLineEditName, valNameError, pScreens);
  ui->m_pLineEditName->setValidator(valName);
  ui->m_pLineEditName->selectAll();

  const auto valAliasError = new validators::ValidationError(this, ui->m_pLabelAliasError);
  const auto valAlias = new validators::AliasValidator(ui->m_pLineEditAlias, valAliasError);
  ui->m_pLineEditAlias->setValidator(valAlias);

  for (int i = 0; i < m_pScreen->aliases().count(); i++)
    new QListWidgetItem(m_pScreen->aliases()[i], ui->m_pListAliases);

  ui->m_pComboBoxShift->setCurrentIndex(m_pScreen->modifier(static_cast<int>(Shift)));
  ui->m_pComboBoxCtrl->setCurrentIndex(m_pScreen->modifier(static_cast<int>(Ctrl)));
  ui->m_pComboBoxAlt->setCurrentIndex(m_pScreen->modifier(static_cast<int>(Alt)));
  ui->m_pComboBoxMeta->setCurrentIndex(m_pScreen->modifier(static_cast<int>(Meta)));
  ui->m_pComboBoxSuper->setCurrentIndex(m_pScreen->modifier(static_cast<int>(Super)));

  ui->m_pCheckBoxCornerTopLeft->setChecked(m_pScreen->switchCorner(static_cast<int>(TopLeft)));
  ui->m_pCheckBoxCornerTopRight->setChecked(m_pScreen->switchCorner(static_cast<int>(TopRight)));
  ui->m_pCheckBoxCornerBottomLeft->setChecked(m_pScreen->switchCorner(static_cast<int>(BottomLeft)));
  ui->m_pCheckBoxCornerBottomRight->setChecked(m_pScreen->switchCorner(static_cast<int>(BottomRight)));
  ui->m_pSpinBoxSwitchCornerSize->setValue(m_pScreen->switchCornerSize());

  ui->m_pCheckBoxCapsLock->setChecked(m_pScreen->fix(CapsLock));
  ui->m_pCheckBoxNumLock->setChecked(m_pScreen->fix(NumLock));
  ui->m_pCheckBoxScrollLock->setChecked(m_pScreen->fix(ScrollLock));
  ui->m_pCheckBoxXTest->setChecked(m_pScreen->fix(XTest));
}

void ScreenSettingsDialog::accept()
{
  if (ui->m_pLineEditName->text().isEmpty()) {
    QMessageBox::warning(
        this, tr("Screen name is empty"),
        tr("The screen name cannot be empty. "
           "Please either fill in a name or cancel the dialog.")
    );
    return;
  }
  if (!ui->m_pLabelNameError->text().isEmpty()) {
    return;
  }

  m_pScreen->setName(ui->m_pLineEditName->text());

  for (int i = 0; i < ui->m_pListAliases->count(); i++) {
    QString alias(ui->m_pListAliases->item(i)->text());
    if (alias == ui->m_pLineEditName->text()) {
      QMessageBox::warning(
          this, tr("Screen name matches alias"),
          tr("The screen name cannot be the same as an alias. "
             "Please either remove the alias or change the screen name.")
      );
      return;
    }
    m_pScreen->addAlias(alias);
  }

  m_pScreen->setModifier(Shift, ui->m_pComboBoxShift->currentIndex());
  m_pScreen->setModifier(Ctrl, ui->m_pComboBoxCtrl->currentIndex());
  m_pScreen->setModifier(Alt, ui->m_pComboBoxAlt->currentIndex());
  m_pScreen->setModifier(Meta, ui->m_pComboBoxMeta->currentIndex());
  m_pScreen->setModifier(Super, ui->m_pComboBoxSuper->currentIndex());

  m_pScreen->setSwitchCorner(TopLeft, ui->m_pCheckBoxCornerTopLeft->isChecked());
  m_pScreen->setSwitchCorner(TopRight, ui->m_pCheckBoxCornerTopRight->isChecked());
  m_pScreen->setSwitchCorner(BottomLeft, ui->m_pCheckBoxCornerBottomLeft->isChecked());
  m_pScreen->setSwitchCorner(BottomRight, ui->m_pCheckBoxCornerBottomRight->isChecked());
  m_pScreen->setSwitchCornerSize(ui->m_pSpinBoxSwitchCornerSize->value());

  m_pScreen->setFix(CapsLock, ui->m_pCheckBoxCapsLock->isChecked());
  m_pScreen->setFix(NumLock, ui->m_pCheckBoxNumLock->isChecked());
  m_pScreen->setFix(ScrollLock, ui->m_pCheckBoxScrollLock->isChecked());
  m_pScreen->setFix(XTest, ui->m_pCheckBoxXTest->isChecked());

  QDialog::accept();
}

void ScreenSettingsDialog::on_m_pButtonAddAlias_clicked()
{
  if (!ui->m_pLineEditAlias->text().isEmpty() &&
      ui->m_pListAliases->findItems(ui->m_pLineEditAlias->text(), Qt::MatchFixedString).isEmpty()) {
    new QListWidgetItem(ui->m_pLineEditAlias->text(), ui->m_pListAliases);
    ui->m_pLineEditAlias->clear();
  }
}

void ScreenSettingsDialog::on_m_pLineEditAlias_textChanged(const QString &text)
{
  ui->m_pButtonAddAlias->setEnabled(!text.isEmpty() && ui->m_pLabelAliasError->text().isEmpty());
}

void ScreenSettingsDialog::on_m_pButtonRemoveAlias_clicked()
{
  QList<QListWidgetItem *> items = ui->m_pListAliases->selectedItems();
  qDeleteAll(items);
}

void ScreenSettingsDialog::on_m_pListAliases_itemSelectionChanged()
{
  ui->m_pButtonRemoveAlias->setEnabled(!ui->m_pListAliases->selectedItems().isEmpty());
}
