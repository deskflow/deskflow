/*
 * synergy -- mouse and keyboard sharing utility
 * Copyright (C) 2012 Synergy Si Ltd.
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
 * 
 * In addition, as a special exception, the copyright holders give
 * permission to link the code of portions of this program with the OpenSSL
 * library.
 * You must obey the GNU General Public License in all respects for all of
 * the code used other than OpenSSL. If you modify file(s) with this
 * exception, you may extend this exception to your version of the file(s),
 * but you are not obligated to do so. If you do not wish to do so, delete
 * this exception statement from your version. If you delete this exception
 * statement from all source files in the program, then also delete it here.
 */

#if !defined(ACTION_H)

#define ACTION_H

#include "KeySequence.h"

#include <QString>
#include <QStringList>
#include <QList>

class ActionDialog;
class QSettings;
class QTextStream;

class Action
{
	friend class ActionDialog;
	friend QTextStream& operator<<(QTextStream& outStream, const Action& action);

	public:
		enum ActionType { keyDown, keyUp, keystroke, switchToScreen, switchInDirection, lockCursorToScreen, mouseDown, mouseUp, mousebutton };
		enum SwitchDirection { switchLeft, switchRight, switchUp, switchDown };
		enum LockCursorMode { lockCursorToggle, lockCursonOn, lockCursorOff  };

	public:
		Action();

	public:
		QString text() const;
		const KeySequence& keySequence() const { return m_KeySequence; }
		void loadSettings(QSettings& settings);
		void saveSettings(QSettings& settings) const;
		int type() const { return m_Type; }
		const QStringList& typeScreenNames() const { return m_TypeScreenNames; }
		const QString& switchScreenName() const { return m_SwitchScreenName; }
		int switchDirection() const { return m_SwitchDirection; }
		int lockCursorMode() const { return m_LockCursorMode; }
		bool activeOnRelease() const { return m_ActiveOnRelease; }
		bool haveScreens() const { return m_HasScreens; }

	protected:
		KeySequence& keySequence() { return m_KeySequence; }
		void setKeySequence(const KeySequence& seq) { m_KeySequence = seq; }
		void setType(int t) { m_Type = t; }
		QStringList& typeScreenNames() { return m_TypeScreenNames; }
		void setSwitchScreenName(const QString& n) { m_SwitchScreenName = n; }
		void setSwitchDirection(int d) { m_SwitchDirection = d; }
		void setLockCursorMode(int m) { m_LockCursorMode = m; }
		void setActiveOnRelease(bool b) { m_ActiveOnRelease = b; }
		void setHaveScreens(bool b) { m_HasScreens = b; }

	private:
		KeySequence m_KeySequence;
		int m_Type;
		QStringList m_TypeScreenNames;
		QString m_SwitchScreenName;
		int m_SwitchDirection;
		int m_LockCursorMode;
		bool m_ActiveOnRelease;
		bool m_HasScreens;

		static const char* m_ActionTypeNames[];
		static const char* m_SwitchDirectionNames[];
		static const char* m_LockCursorModeNames[];
};

typedef QList<Action> ActionList;

QTextStream& operator<<(QTextStream& outStream, const Action& action);

#endif
