#include "HotkeyDialog.h"

#include <QtCore>
#include <QtGui>

HotkeyDialog::HotkeyDialog (QWidget* parent, Hotkey& hotkey) :
	QDialog(parent, Qt::WindowTitleHint | Qt::WindowSystemMenuHint),
	Ui::HotkeyDialogBase(),
	m_Hotkey(hotkey)
{
	setupUi(this);

	m_pKeySequenceWidgetHotkey->setText(m_Hotkey.text());
}

void HotkeyDialog::accept()
{
	if (!sequenceWidget()->valid())
		return;

	hotkey().setKeySequence(sequenceWidget()->keySequence());
	QDialog::accept();
}
