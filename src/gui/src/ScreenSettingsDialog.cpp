/*
 * barrier -- mouse and keyboard sharing utility
 * Copyright (C) 2012-2016 Symless Ltd.
 * Copyright (C) 2008 Volker Lanz (vl@fidra.de)
 * 
 * This package is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * found in the file LICENSE that should have accompanied this file.
 * 
 * This package is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "ScreenSettingsDialog.h"
#include "Screen.h"

#include <QtCore>
#include <QtGui>
#include <QMessageBox>

static const QRegExp ValidScreenName("[a-z0-9\\._-]{,255}", Qt::CaseInsensitive);

static QString check_name_param(QString name)
{
    // after internationalization happens the default name "Unnamed" might
    // be translated with spaces (or other chars). let's replace the spaces
    // with dashes and just give up if that doesn't pass the regexp
    name.replace(' ', '-');
    if (ValidScreenName.exactMatch(name))
        return name;
    return "";
}

ScreenSettingsDialog::ScreenSettingsDialog(QWidget* parent, Screen* pScreen) :
    QDialog(parent, Qt::WindowTitleHint | Qt::WindowSystemMenuHint),
    Ui::ScreenSettingsDialogBase(),
    m_pScreen(pScreen)
{
    setupUi(this);

    m_pLineEditName->setText(check_name_param(m_pScreen->name()));
    m_pLineEditName->setValidator(new QRegExpValidator(ValidScreenName, m_pLineEditName));
    m_pLineEditName->selectAll();

    m_pLineEditAlias->setValidator(new QRegExpValidator(ValidScreenName, m_pLineEditName));

    for (int i = 0; i < m_pScreen->aliases().count(); i++)
        new QListWidgetItem(m_pScreen->aliases()[i], m_pListAliases);

    m_pComboBoxShift->setCurrentIndex(static_cast<int>(m_pScreen->modifier(Screen::Modifier::Shift)));
    m_pComboBoxCtrl->setCurrentIndex(static_cast<int>(m_pScreen->modifier(Screen::Modifier::Ctrl)));
    m_pComboBoxAlt->setCurrentIndex(static_cast<int>(m_pScreen->modifier(Screen::Modifier::Alt)));
    m_pComboBoxMeta->setCurrentIndex(static_cast<int>(m_pScreen->modifier(Screen::Modifier::Meta)));
    m_pComboBoxSuper->setCurrentIndex(static_cast<int>(m_pScreen->modifier(Screen::Modifier::Super)));

    m_pCheckBoxCornerTopLeft->setChecked(m_pScreen->switchCorner(Screen::SwitchCorner::TopLeft));
    m_pCheckBoxCornerTopRight->setChecked(m_pScreen->switchCorner(Screen::SwitchCorner::TopRight));
    m_pCheckBoxCornerBottomLeft->setChecked(m_pScreen->switchCorner(Screen::SwitchCorner::BottomLeft));
    m_pCheckBoxCornerBottomRight->setChecked(m_pScreen->switchCorner(Screen::SwitchCorner::BottomRight));
    m_pSpinBoxSwitchCornerSize->setValue(m_pScreen->switchCornerSize());

    m_pCheckBoxCapsLock->setChecked(m_pScreen->fix(Screen::Fix::CapsLock));
    m_pCheckBoxNumLock->setChecked(m_pScreen->fix(Screen::Fix::NumLock));
    m_pCheckBoxScrollLock->setChecked(m_pScreen->fix(Screen::Fix::ScrollLock));
    m_pCheckBoxXTest->setChecked(m_pScreen->fix(Screen::Fix::XTest));
    m_pCheckBoxPreserveFocus->setChecked(m_pScreen->fix(Screen::Fix::PreserveFocus));
}

void ScreenSettingsDialog::accept()
{
    if (m_pLineEditName->text().isEmpty())
    {
        QMessageBox::warning(
            this, tr("Screen name is empty"),
            tr("The screen name cannot be empty. "
               "Please either fill in a name or cancel the dialog."));
        return;
    }

    m_pScreen->init();

    m_pScreen->setName(m_pLineEditName->text());

    for (int i = 0; i < m_pListAliases->count(); i++)
    {
        QString alias(m_pListAliases->item(i)->text());
        if (alias == m_pLineEditName->text())
        {
            QMessageBox::warning(
                this, tr("Screen name matches alias"),
                tr("The screen name cannot be the same as an alias. "
                   "Please either remove the alias or change the screen name."));
            return;
        }
        m_pScreen->addAlias(alias);
    }

    m_pScreen->setModifier(Screen::Modifier::Shift,
                           static_cast<Screen::Modifier>(m_pComboBoxShift->currentIndex()));
    m_pScreen->setModifier(Screen::Modifier::Ctrl,
                           static_cast<Screen::Modifier>(m_pComboBoxCtrl->currentIndex()));
    m_pScreen->setModifier(Screen::Modifier::Alt,
                           static_cast<Screen::Modifier>(m_pComboBoxAlt->currentIndex()));
    m_pScreen->setModifier(Screen::Modifier::Meta,
                           static_cast<Screen::Modifier>(m_pComboBoxMeta->currentIndex()));
    m_pScreen->setModifier(Screen::Modifier::Super,
                           static_cast<Screen::Modifier>(m_pComboBoxSuper->currentIndex()));

    m_pScreen->setSwitchCorner(Screen::SwitchCorner::TopLeft, m_pCheckBoxCornerTopLeft->isChecked());
    m_pScreen->setSwitchCorner(Screen::SwitchCorner::TopRight, m_pCheckBoxCornerTopRight->isChecked());
    m_pScreen->setSwitchCorner(Screen::SwitchCorner::BottomLeft, m_pCheckBoxCornerBottomLeft->isChecked());
    m_pScreen->setSwitchCorner(Screen::SwitchCorner::BottomRight, m_pCheckBoxCornerBottomRight->isChecked());
    m_pScreen->setSwitchCornerSize(m_pSpinBoxSwitchCornerSize->value());

    m_pScreen->setFix(Screen::Fix::CapsLock, m_pCheckBoxCapsLock->isChecked());
    m_pScreen->setFix(Screen::Fix::NumLock, m_pCheckBoxNumLock->isChecked());
    m_pScreen->setFix(Screen::Fix::ScrollLock, m_pCheckBoxScrollLock->isChecked());
    m_pScreen->setFix(Screen::Fix::XTest, m_pCheckBoxXTest->isChecked());
    m_pScreen->setFix(Screen::Fix::PreserveFocus, m_pCheckBoxPreserveFocus->isChecked());

    QDialog::accept();
}

void ScreenSettingsDialog::on_m_pButtonAddAlias_clicked()
{
    if (!m_pLineEditAlias->text().isEmpty() && m_pListAliases->findItems(m_pLineEditAlias->text(), Qt::MatchFixedString).isEmpty())
    {
        new QListWidgetItem(m_pLineEditAlias->text(), m_pListAliases);
        m_pLineEditAlias->clear();
    }
}

void ScreenSettingsDialog::on_m_pLineEditAlias_textChanged(const QString& text)
{
    m_pButtonAddAlias->setEnabled(!text.isEmpty());
}

void ScreenSettingsDialog::on_m_pButtonRemoveAlias_clicked()
{
    QList<QListWidgetItem*> items = m_pListAliases->selectedItems();

    for (int i = 0; i < items.count(); i++)
        delete items[i];
}

void ScreenSettingsDialog::on_m_pListAliases_itemSelectionChanged()
{
    m_pButtonRemoveAlias->setEnabled(!m_pListAliases->selectedItems().isEmpty());
}

