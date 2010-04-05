#if !defined(HOTKEY_H)

#define HOTKEY_H

#include <QString>
#include <QList>
#include <QTextStream>

#include "Action.h"
#include "KeySequence.h"

class HotkeyDialog;
class ServerConfigDialog;
class QSettings;

class Hotkey
{
	friend class HotkeyDialog;
	friend class ServerConfigDialog;
	friend QTextStream& operator<<(QTextStream& outStream, const Hotkey& hotkey);

	public:
		Hotkey();

	public:
		QString text() const;
		const KeySequence& keySequence() const { return m_KeySequence; }
		const ActionList& actions() const { return m_Actions; }

		void loadSettings(QSettings& settings);
		void saveSettings(QSettings& settings) const;

	protected:
		KeySequence& keySequence() { return m_KeySequence; }
		void setKeySequence(const KeySequence& seq) { m_KeySequence = seq; }
		ActionList& actions() { return m_Actions; }


	private:
		KeySequence m_KeySequence;
		ActionList m_Actions;
};

typedef QList<Hotkey> HotkeyList;

QTextStream& operator<<(QTextStream& outStream, const Hotkey& hotkey);

#endif
