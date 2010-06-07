#if !defined(HOTKEYDIALOG_H)

#define HOTKEYDIALOG_H

#include "ui_HotkeyDialogBase.h"
#include "Hotkey.h"

#include <QDialog>

class HotkeyDialog : public QDialog, public Ui::HotkeyDialogBase
{
	Q_OBJECT

	public:
		HotkeyDialog(QWidget* parent, Hotkey& hotkey);

	public:
		const Hotkey& hotkey() const { return m_Hotkey; }

	protected slots:
		void accept();

	protected:
		const KeySequenceWidget* sequenceWidget() const { return m_pKeySequenceWidgetHotkey; }
		Hotkey& hotkey() { return m_Hotkey; }

	private:
		Hotkey& m_Hotkey;
};

#endif
