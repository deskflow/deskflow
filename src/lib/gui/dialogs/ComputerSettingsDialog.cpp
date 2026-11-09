/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2012 Synergy App Ltd
 * SPDX-FileCopyrightText: (C) 2008 Volker Lanz <vl@fidra.de>
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#include "ComputerSettingsDialog.h"
#include "gui/DialogPresentation.h"
#include "ui_ComputerSettingsDialog.h"

#include "gui/config/Computer.h"
#include "validators/AliasValidator.h"
#include "validators/ComputerLineNameValidator.h"
#include "validators/ValidationError.h"

#include <QMessageBox>

using enum ComputerConfig::SwitchCorner;
using enum ComputerConfig::Fix;

ComputerSettingsDialog::~ComputerSettingsDialog() = default;

ComputerSettingsDialog::ComputerSettingsDialog(QWidget *parent, Computer *computer, const ComputerList *computers)
    : QDialog(parent, Qt::WindowTitleHint | Qt::WindowSystemMenuHint),
      ui{std::make_unique<Ui::ComputerSettingsDialog>()},
      m_computer(computer)
{

  ui->setupUi(this);
  deskflow::gui::prepareFormDialog(this, ui->buttonBox);
  new deskflow::gui::ResponsiveFormColumns(this, ui->layoutOptions);
  // Keep both nested columns at the same top edge. Their different natural
  // heights must not distribute spare vertical space above the shorter column.
  ui->layoutOptions->setAlignment(ui->layoutLeftSide, Qt::AlignTop);
  ui->layoutOptions->setAlignment(ui->layoutRightSide, Qt::AlignTop);
  ui->buttonBox->button(QDialogButtonBox::Cancel)->setFocus();

  ui->lineNameEdit->setText(m_computer->name());

  const auto valNameError = new validators::ValidationError(this, ui->lblNameError);
  const auto valName = new validators::ComputerLineNameValidator(ui->lineNameEdit, valNameError, computers);
  ui->lineNameEdit->setValidator(valName);

  const auto valAliasError = new validators::ValidationError(this, ui->lblAliasError);
  const auto valAlias = new validators::AliasValidator(ui->lineAddAlias, valAliasError);
  ui->lineAddAlias->setValidator(valAlias);

  for (int i = 0; i < m_computer->aliases().count(); i++)
    new QListWidgetItem(m_computer->aliases()[i], ui->listAliases);

  ui->comboShift->setCurrentIndex(m_computer->modifier(static_cast<int>(KeyboardModifier::Shift)));
  ui->comboCtrl->setCurrentIndex(m_computer->modifier(static_cast<int>(KeyboardModifier::Ctrl)));
  ui->comboAlt->setCurrentIndex(m_computer->modifier(static_cast<int>(KeyboardModifier::Alt)));
  ui->comboMeta->setCurrentIndex(m_computer->modifier(static_cast<int>(KeyboardModifier::Meta)));
  ui->comboSuper->setCurrentIndex(m_computer->modifier(static_cast<int>(KeyboardModifier::Super)));
  ui->comboAltGr->setCurrentIndex(m_computer->modifier(static_cast<int>(KeyboardModifier::AltGr)));

  ui->chkDeadTopLeft->setChecked(m_computer->switchCorner(static_cast<int>(TopLeft)));
  ui->chkDeadTopRight->setChecked(m_computer->switchCorner(static_cast<int>(TopRight)));
  ui->chkDeadBottomLeft->setChecked(m_computer->switchCorner(static_cast<int>(BottomLeft)));
  ui->chkDeadBottomRight->setChecked(m_computer->switchCorner(static_cast<int>(BottomRight)));
  ui->sbSwitchCornerSize->setValue(m_computer->switchCornerSize());

  ui->chkWeakX11Focus->setChecked(m_computer->weakX11Focus());

  ui->chkFixCapsLock->setChecked(m_computer->fix(CapsLock));
  ui->chkFixNumLock->setChecked(m_computer->fix(NumLock));
  ui->chkFixScrollLock->setChecked(m_computer->fix(ScrollLock));
  ui->chkFixXTest->setChecked(m_computer->fix(XTest));

  connect(ui->buttonBox, &QDialogButtonBox::accepted, this, &ComputerSettingsDialog::accept);
  connect(ui->buttonBox, &QDialogButtonBox::rejected, this, &ComputerSettingsDialog::reject);
  connect(ui->btnAddAlias, &QPushButton::clicked, this, &ComputerSettingsDialog::addAlias);
  connect(ui->btnRemoveAlias, &QPushButton::clicked, this, &ComputerSettingsDialog::removeAlias);
  connect(ui->lineAddAlias, &QLineEdit::textChanged, this, &ComputerSettingsDialog::checkNewAliasName);
  connect(ui->listAliases, &QListWidget::itemSelectionChanged, this, &ComputerSettingsDialog::aliasSelected);
}

void ComputerSettingsDialog::accept()
{
  if (ui->lineNameEdit->text().isEmpty()) {
    QMessageBox::warning(
        this, tr("Computer name is empty"),
        tr("The computer name cannot be empty. "
           "Please either fill in a name or cancel the dialog.")
    );
    return;
  }
  if (!ui->lblNameError->text().isEmpty()) {
    return;
  }

  // Keep the caller's draft unchanged until every field has been validated.
  Computer draft = *m_computer;
  draft.setName(ui->lineNameEdit->text());
  draft.aliases().clear();

  for (int i = 0; i < ui->listAliases->count(); i++) {
    QString alias(ui->listAliases->item(i)->text());
    if (alias == ui->lineNameEdit->text()) {
      QMessageBox::warning(
          this, tr("Computer name matches alias"),
          tr("The computer name cannot be the same as an alias. "
             "Please either remove the alias or change the computer name.")
      );
      return;
    }
    if (!draft.aliases().contains(alias))
      draft.addAlias(alias);
  }

  draft.setModifier(KeyboardModifier::Shift, ui->comboShift->currentIndex());
  draft.setModifier(KeyboardModifier::Ctrl, ui->comboCtrl->currentIndex());
  draft.setModifier(KeyboardModifier::Alt, ui->comboAlt->currentIndex());
  draft.setModifier(KeyboardModifier::Meta, ui->comboMeta->currentIndex());
  draft.setModifier(KeyboardModifier::Super, ui->comboSuper->currentIndex());
  draft.setModifier(KeyboardModifier::AltGr, ui->comboAltGr->currentIndex());

  draft.setSwitchCorner(TopLeft, ui->chkDeadTopLeft->isChecked());
  draft.setSwitchCorner(TopRight, ui->chkDeadTopRight->isChecked());
  draft.setSwitchCorner(BottomLeft, ui->chkDeadBottomLeft->isChecked());
  draft.setSwitchCorner(BottomRight, ui->chkDeadBottomRight->isChecked());
  draft.setSwitchCornerSize(ui->sbSwitchCornerSize->value());

  draft.setFix(CapsLock, ui->chkFixCapsLock->isChecked());
  draft.setFix(NumLock, ui->chkFixNumLock->isChecked());
  draft.setFix(ScrollLock, ui->chkFixScrollLock->isChecked());
  draft.setFix(XTest, ui->chkFixXTest->isChecked());
  draft.setWeakX11Focus(ui->chkWeakX11Focus->isChecked());

  *m_computer = draft;
  QDialog::accept();
}

void ComputerSettingsDialog::addAlias()
{
  if (!ui->lineAddAlias->text().isEmpty() &&
      ui->listAliases->findItems(ui->lineAddAlias->text(), Qt::MatchFixedString).isEmpty()) {
    new QListWidgetItem(ui->lineAddAlias->text(), ui->listAliases);
    ui->lineAddAlias->clear();
  }
}

void ComputerSettingsDialog::removeAlias() const
{
  QList<QListWidgetItem *> items = ui->listAliases->selectedItems();
  qDeleteAll(items);
}

void ComputerSettingsDialog::checkNewAliasName(const QString &text)
{
  ui->btnAddAlias->setEnabled(!text.isEmpty() && ui->lblAliasError->text().isEmpty());
}

void ComputerSettingsDialog::aliasSelected()
{
  ui->btnRemoveAlias->setEnabled(!ui->listAliases->selectedItems().isEmpty());
}
