#include "Hotkey.h"

#include <QSettings>

Hotkey::Hotkey() :
	m_KeySequence(),
	m_Actions()
{
}

QString Hotkey::text() const
{
	QString text = keySequence().toString();

	if (keySequence().isMouseButton())
		return "mousebutton(" + text + ")";

	return "keystroke(" + text + ")";
}

void Hotkey::loadSettings(QSettings& settings)
{
	keySequence().loadSettings(settings);

	actions().clear();
	int num = settings.beginReadArray("actions");
	for (int i = 0; i < num; i++)
	{
		settings.setArrayIndex(i);
		Action a;
		a.loadSettings(settings);
		actions().append(a);
	}

	settings.endArray();
}

void Hotkey::saveSettings(QSettings& settings) const
{
	keySequence().saveSettings(settings);

	settings.beginWriteArray("actions");
	for (int i = 0; i < actions().size(); i++)
	{
		settings.setArrayIndex(i);
		actions()[i].saveSettings(settings);
	}
	settings.endArray();
}

QTextStream& operator<<(QTextStream& outStream, const Hotkey& hotkey)
{
	for (int i = 0; i < hotkey.actions().size(); i++)
		outStream << "\t" << hotkey.text() << " = " << hotkey.actions()[i] << endl;

	return outStream;
}
