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

#if !defined(BASECONFIG_H)

#define BASECONFIG_H

#include <QSettings>
#include <QString>
#include <QVariant>

class BaseConfig
{
	public:
		enum Modifier { DefaultMod = -1, Shift, Ctrl, Alt, Meta, Super, None, NumModifiers };
		enum SwitchCorner { TopLeft, TopRight, BottomLeft, BottomRight, NumSwitchCorners };
		enum Fix { CapsLock, NumLock, ScrollLock, XTest, NumFixes };

	protected:
		BaseConfig() {}
		virtual ~BaseConfig() {}

	protected:
		template<typename T1, typename T2>
		void readSettings(QSettings& settings, T1& array, const QString& arrayName, const T2& deflt)
		{
			int entries = settings.beginReadArray(arrayName + "Array");
			array.clear();
			for (int i = 0; i < entries; i++)
			{
				settings.setArrayIndex(i);
				QVariant v = settings.value(arrayName, deflt);
				array.append(v.value<T2>());
			}
			settings.endArray();
		}

		template<typename T1, typename T2>
		void readSettings(QSettings& settings, T1& array, const QString& arrayName, const T2& deflt, int entries)
		{
			Q_ASSERT(array.size() >= entries);
			settings.beginReadArray(arrayName + "Array");
			for (int i = 0; i < entries; i++)
			{
				settings.setArrayIndex(i);
				QVariant v = settings.value(arrayName, deflt);
				array[i] = v.value<T2>();
			}
			settings.endArray();
		}

		template<typename T>
		void writeSettings(QSettings& settings, const T& array, const QString& arrayName) const
		{
			settings.beginWriteArray(arrayName + "Array");
			for (int i = 0; i < array.size(); i++)
			{
				settings.setArrayIndex(i);
				settings.setValue(arrayName, array[i]);
			}
			settings.endArray();
		}


	public:
		static const char* modifierName(int idx) { return m_ModifierNames[idx]; }
		static const char* fixName(int idx) { return m_FixNames[idx]; }
		static const char* switchCornerName(int idx) { return m_SwitchCornerNames[idx]; }

	private:
		static const char* m_ModifierNames[];
		static const char* m_FixNames[];
		static const char* m_SwitchCornerNames[];
};

#endif
