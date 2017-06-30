/*
 * synergy -- mouse and keyboard sharing utility
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

ScreenSettingsDialog::ScreenSettingsDialog (QWidget* parent, Screen* pScreen)
    : QDialog (parent, Qt::WindowTitleHint | Qt::WindowSystemMenuHint),
      Ui::ScreenSettingsDialogBase (),
      m_pScreen (pScreen) {
    setupUi (this);

    QRegExp validScreenName ("[a-z0-9\\._-]{,255}", Qt::CaseInsensitive);

    m_pLineEditName->setText (m_pScreen->name ());
    m_pLineEditName->setValidator (
        new QRegExpValidator (validScreenName, m_pLineEditName));
    m_pLineEditName->selectAll ();

    m_pLineEditAlias->setValidator (
        new QRegExpValidator (validScreenName, m_pLineEditName));

    for (int i = 0; i < m_pScreen->aliases ().count (); i++)
        new QListWidgetItem (m_pScreen->aliases ()[i], m_pListAliases);

    m_pComboBoxShift->setCurrentIndex (m_pScreen->modifier (Screen::Shift));
    m_pComboBoxCtrl->setCurrentIndex (m_pScreen->modifier (Screen::Ctrl));
    m_pComboBoxAlt->setCurrentIndex (m_pScreen->modifier (Screen::Alt));
    m_pComboBoxMeta->setCurrentIndex (m_pScreen->modifier (Screen::Meta));
    m_pComboBoxSuper->setCurrentIndex (m_pScreen->modifier (Screen::Super));

    m_pCheckBoxCornerTopLeft->setChecked (
        m_pScreen->switchCorner (Screen::TopLeft));
    m_pCheckBoxCornerTopRight->setChecked (
        m_pScreen->switchCorner (Screen::TopRight));
    m_pCheckBoxCornerBottomLeft->setChecked (
        m_pScreen->switchCorner (Screen::BottomLeft));
    m_pCheckBoxCornerBottomRight->setChecked (
        m_pScreen->switchCorner (Screen::BottomRight));
    m_pSpinBoxSwitchCornerSize->setValue (m_pScreen->switchCornerSize ());

    m_pCheckBoxCapsLock->setChecked (m_pScreen->fix (Screen::CapsLock));
    m_pCheckBoxNumLock->setChecked (m_pScreen->fix (Screen::NumLock));
    m_pCheckBoxScrollLock->setChecked (m_pScreen->fix (Screen::ScrollLock));
    m_pCheckBoxXTest->setChecked (m_pScreen->fix (Screen::XTest));
}

void
ScreenSettingsDialog::accept () {
    if (m_pLineEditName->text ().isEmpty ()) {
        QMessageBox::warning (
            this,
            tr ("Screen name is empty"),
            tr ("The screen name cannot be empty. "
                "Please either fill in a name or cancel the dialog."));
        return;
    }

    m_pScreen->init ();

    m_pScreen->setName (m_pLineEditName->text ());

    for (int i = 0; i < m_pListAliases->count (); i++) {
        QString alias (m_pListAliases->item (i)->text ());
        if (alias == m_pLineEditName->text ()) {
            QMessageBox::warning (
                this,
                tr ("Screen name matches alias"),
                tr ("The screen name cannot be the same as an alias. "
                    "Please either remove the alias or change the screen "
                    "name."));
            return;
        }
        m_pScreen->addAlias (alias);
    }

    m_pScreen->setModifier (Screen::Shift, m_pComboBoxShift->currentIndex ());
    m_pScreen->setModifier (Screen::Ctrl, m_pComboBoxCtrl->currentIndex ());
    m_pScreen->setModifier (Screen::Alt, m_pComboBoxAlt->currentIndex ());
    m_pScreen->setModifier (Screen::Meta, m_pComboBoxMeta->currentIndex ());
    m_pScreen->setModifier (Screen::Super, m_pComboBoxSuper->currentIndex ());

    m_pScreen->setSwitchCorner (Screen::TopLeft,
                                m_pCheckBoxCornerTopLeft->isChecked ());
    m_pScreen->setSwitchCorner (Screen::TopRight,
                                m_pCheckBoxCornerTopRight->isChecked ());
    m_pScreen->setSwitchCorner (Screen::BottomLeft,
                                m_pCheckBoxCornerBottomLeft->isChecked ());
    m_pScreen->setSwitchCorner (Screen::BottomRight,
                                m_pCheckBoxCornerBottomRight->isChecked ());
    m_pScreen->setSwitchCornerSize (m_pSpinBoxSwitchCornerSize->value ());

    m_pScreen->setFix (Screen::CapsLock, m_pCheckBoxCapsLock->isChecked ());
    m_pScreen->setFix (Screen::NumLock, m_pCheckBoxNumLock->isChecked ());
    m_pScreen->setFix (Screen::ScrollLock, m_pCheckBoxScrollLock->isChecked ());
    m_pScreen->setFix (Screen::XTest, m_pCheckBoxXTest->isChecked ());

    QDialog::accept ();
}

void
ScreenSettingsDialog::on_m_pButtonAddAlias_clicked () {
    if (!m_pLineEditAlias->text ().isEmpty () &&
        m_pListAliases
            ->findItems (m_pLineEditAlias->text (), Qt::MatchFixedString)
            .isEmpty ()) {
        new QListWidgetItem (m_pLineEditAlias->text (), m_pListAliases);
        m_pLineEditAlias->clear ();
    }
}

void
ScreenSettingsDialog::on_m_pLineEditAlias_textChanged (const QString& text) {
    m_pButtonAddAlias->setEnabled (!text.isEmpty ());
}

void
ScreenSettingsDialog::on_m_pButtonRemoveAlias_clicked () {
    QList<QListWidgetItem*> items = m_pListAliases->selectedItems ();

    for (int i = 0; i < items.count (); i++)
        delete items[i];
}

void
ScreenSettingsDialog::on_m_pListAliases_itemSelectionChanged () {
    m_pButtonRemoveAlias->setEnabled (
        !m_pListAliases->selectedItems ().isEmpty ());
}
