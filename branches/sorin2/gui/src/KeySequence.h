#if !defined(KEYSEQUENCE_H)

#define KEYSEQUENCE_H

#include <QList>
#include <QString>

class QSettings;

class KeySequence
{
	public:
		KeySequence();

	public:
		QString toString() const;
		bool appendKey(int modifiers, int key);
		bool appendMouseButton(int button);
		bool isMouseButton() const;
		bool valid() const { return m_IsValid; }
		int modifiers() const { return m_Modifiers; }
		void saveSettings(QSettings& settings) const;
		void loadSettings(QSettings& settings);
		const QList<int>& sequence() const { return m_Sequence; }

	private:
		void setValid(bool b) { m_IsValid = b; }
		void setModifiers(int i) { m_Modifiers = i; }
		QList<int>& sequence() { return m_Sequence; }

	private:
		QList<int> m_Sequence;
		int m_Modifiers;
		bool m_IsValid;

		static QString keyToString(int key);
};

#endif

