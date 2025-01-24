/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2012 Symless Ltd.
 * SPDX-FileCopyrightText: (C) 2008 Volker Lanz <vl@fidra.de>
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#include "ScreenSettingsDialog.h"
#include "ui_ScreenSettingsDialog.h"

#include "gui/config/Screen.h"
#include "gui/styles.h"
#include "gui/validators/AliasValidator.h"
#include "gui/validators/ScreenNameValidator.h"
#include "gui/validators/ValidationError.h"

#include <QMessageBox>

using namespace deskflow::gui;
using enum ScreenConfig::Modifier;
using enum ScreenConfig::SwitchCorner;
using enum ScreenConfig::Fix;

ScreenSettingsDialog::~ScreenSettingsDialog() = default;

ScreenSettingsDialog::ScreenSettingsDialog(QWidget *parent, Screen *pScreen, const ScreenList *pScreens)
    : QDialog(parent, Qt::WindowTitleHint | Qt::WindowSystemMenuHint),
      ui_{std::make_unique<Ui::ScreenSettingsDialog>()},
      m_pScreen(pScreen)
{

  ui_->setupUi(this);

  ui_->m_pLabelAliasError->setStyleSheet(kStyleErrorActiveLabel);
  ui_->m_pLabelNameError->setStyleSheet(kStyleErrorActiveLabel);

  ui_->m_pLineEditName->setText(m_pScreen->name());
  ui_->m_pLineEditName->setValidator(new validators::ScreenNameValidator(
      ui_->m_pLineEditName, new validators::ValidationError(this, ui_->m_pLabelNameError), pScreens
  ));
  ui_->m_pLineEditName->selectAll();

  ui_->m_pLineEditAlias->setValidator(new validators::AliasValidator(
      ui_->m_pLineEditAlias, new validators::ValidationError(this, ui_->m_pLabelAliasError)
  ));

  for (int i = 0; i < m_pScreen->aliases().count(); i++)
    new QListWidgetItem(m_pScreen->aliases()[i], ui_->m_pListAliases);

  ui_->m_pComboBoxShift->setCurrentIndex(m_pScreen->modifier(static_cast<int>(Shift)));
  ui_->m_pComboBoxCtrl->setCurrentIndex(m_pScreen->modifier(static_cast<int>(Ctrl)));
  ui_->m_pComboBoxAlt->setCurrentIndex(m_pScreen->modifier(static_cast<int>(Alt)));
  ui_->m_pComboBoxMeta->setCurrentIndex(m_pScreen->modifier(static_cast<int>(Meta)));
  ui_->m_pComboBoxSuper->setCurrentIndex(m_pScreen->modifier(static_cast<int>(Super)));

  ui_->m_pCheckBoxCornerTopLeft->setChecked(m_pScreen->switchCorner(static_cast<int>(TopLeft)));
  ui_->m_pCheckBoxCornerTopRight->setChecked(m_pScreen->switchCorner(static_cast<int>(TopRight)));
  ui_->m_pCheckBoxCornerBottomLeft->setChecked(m_pScreen->switchCorner(static_cast<int>(BottomLeft)));
  ui_->m_pCheckBoxCornerBottomRight->setChecked(m_pScreen->switchCorner(static_cast<int>(BottomRight)));
  ui_->m_pSpinBoxSwitchCornerSize->setValue(m_pScreen->switchCornerSize());

  ui_->m_pCheckBoxCapsLock->setChecked(m_pScreen->fix(CapsLock));
  ui_->m_pCheckBoxNumLock->setChecked(m_pScreen->fix(NumLock));
  ui_->m_pCheckBoxScrollLock->setChecked(m_pScreen->fix(ScrollLock));
  ui_->m_pCheckBoxXTest->setChecked(m_pScreen->fix(XTest));
}

void ScreenSettingsDialog::accept()
{
  if (ui_->m_pLineEditName->text().isEmpty()) {
    QMessageBox::warning(
        this, tr("Screen name is empty"),
        tr("The screen name cannot be empty. "
           "Please either fill in a name or cancel the dialog.")
    );
    return;
  } else if (!ui_->m_pLabelNameError->text().isEmpty()) {
    return;
  }

  m_pScreen->init();

  m_pScreen->setName(ui_->m_pLineEditName->text());

  for (int i = 0; i < ui_->m_pListAliases->count(); i++) {
    QString alias(ui_->m_pListAliases->item(i)->text());
    if (alias == ui_->m_pLineEditName->text()) {
      QMessageBox::warning(
          this, tr("Screen name matches alias"),
          tr("The screen name cannot be the same as an alias. "
             "Please either remove the alias or change the screen name.")
      );
      return;
    }
    m_pScreen->addAlias(alias);
  }

  m_pScreen->setModifier(static_cast<int>(Shift), ui_->m_pComboBoxShift->currentIndex());
  m_pScreen->setModifier(static_cast<int>(Ctrl), ui_->m_pComboBoxCtrl->currentIndex());
  m_pScreen->setModifier(static_cast<int>(Alt), ui_->m_pComboBoxAlt->currentIndex());
  m_pScreen->setModifier(static_cast<int>(Meta), ui_->m_pComboBoxMeta->currentIndex());
  m_pScreen->setModifier(static_cast<int>(Super), ui_->m_pComboBoxSuper->currentIndex());

  m_pScreen->setSwitchCorner(static_cast<int>(TopLeft), ui_->m_pCheckBoxCornerTopLeft->isChecked());
  m_pScreen->setSwitchCorner(static_cast<int>(TopRight), ui_->m_pCheckBoxCornerTopRight->isChecked());
  m_pScreen->setSwitchCorner(static_cast<int>(BottomLeft), ui_->m_pCheckBoxCornerBottomLeft->isChecked());
  m_pScreen->setSwitchCorner(static_cast<int>(BottomRight), ui_->m_pCheckBoxCornerBottomRight->isChecked());
  m_pScreen->setSwitchCornerSize(ui_->m_pSpinBoxSwitchCornerSize->value());

  m_pScreen->setFix(static_cast<int>(CapsLock), ui_->m_pCheckBoxCapsLock->isChecked());
  m_pScreen->setFix(static_cast<int>(NumLock), ui_->m_pCheckBoxNumLock->isChecked());
  m_pScreen->setFix(static_cast<int>(ScrollLock), ui_->m_pCheckBoxScrollLock->isChecked());
  m_pScreen->setFix(static_cast<int>(XTest), ui_->m_pCheckBoxXTest->isChecked());

  QDialog::accept();
}

void ScreenSettingsDialog::on_m_pButtonAddAlias_clicked()
{
  if (!ui_->m_pLineEditAlias->text().isEmpty() &&
      ui_->m_pListAliases->findItems(ui_->m_pLineEditAlias->text(), Qt::MatchFixedString).isEmpty()) {
    new QListWidgetItem(ui_->m_pLineEditAlias->text(), ui_->m_pListAliases);
    ui_->m_pLineEditAlias->clear();
  }
}

void ScreenSettingsDialog::on_m_pLineEditAlias_textChanged(const QString &text)
{
  ui_->m_pButtonAddAlias->setEnabled(!text.isEmpty() && ui_->m_pLabelAliasError->text().isEmpty());
}

void ScreenSettingsDialog::on_m_pButtonRemoveAlias_clicked()
{
  QList<QListWidgetItem *> items = ui_->m_pListAliases->selectedItems();

  for (int i = 0; i < items.count(); i++)
    delete items[i];
}

void ScreenSettingsDialog::on_m_pListAliases_itemSelectionChanged()
{
  ui_->m_pButtonRemoveAlias->setEnabled(!ui_->m_pListAliases->selectedItems().isEmpty());
}
