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

#if !defined(BASECONFIG_H)

#define BASECONFIG_H

#include <QSettings>
#include <QString>
#include <QVariant>

class BaseConfig
{
public:
    enum class Modifier {
        DefaultMod = -1,
        Shift,
        Ctrl,
        Alt,
        Meta,
        Super,
        None,
        Count
    };

    enum class SwitchCorner {
        TopLeft,
        TopRight,
        BottomLeft,
        BottomRight,
        Count
    };

    enum class Fix {
        CapsLock,
        NumLock,
        ScrollLock,
        XTest,
        PreserveFocus,
        Count
    };

    protected:
        BaseConfig() {}
        virtual ~BaseConfig() {}

    protected:
        template<class SettingType, class T>
        void readSettings(QSettings& settings, QList<T>& array, const QString& arrayName,
                          const T& deflt)
        {
            int entries = settings.beginReadArray(arrayName + "Array");
            array.clear();
            for (int i = 0; i < entries; i++)
            {
                settings.setArrayIndex(i);
                QVariant v = settings.value(arrayName, static_cast<SettingType>(deflt));
                array.append(static_cast<T>(v.value<SettingType>()));
            }
            settings.endArray();
        }

        template<class SettingType, class T>
        void readSettings(QSettings& settings, QList<T>& array, const QString& arrayName,
                          const T& deflt, int entries)
        {
            Q_ASSERT(array.size() >= entries);
            settings.beginReadArray(arrayName + "Array");
            for (int i = 0; i < entries; i++)
            {
                settings.setArrayIndex(i);
                QVariant v = settings.value(arrayName, static_cast<SettingType>(deflt));
                array[i] = static_cast<T>(v.value<SettingType>());
            }
            settings.endArray();
        }

        template<class SettingType, class T>
        void writeSettings(QSettings& settings, const QList<T>& array,
                           const QString& arrayName) const
        {
            settings.beginWriteArray(arrayName + "Array");
            for (int i = 0; i < array.size(); i++)
            {
                settings.setArrayIndex(i);
                settings.setValue(arrayName, static_cast<SettingType>(array[i]));
            }
            settings.endArray();
        }


    public:
        static const char* modifierName(Modifier idx)
        {
            return m_ModifierNames[static_cast<int>(idx)];
        }
        static const char* fixName(Fix idx)
        {
            return m_FixNames[static_cast<int>(idx)];
        }
        static const char* switchCornerName(SwitchCorner idx)
        {
            return m_SwitchCornerNames[static_cast<int>(idx)];
        }

    private:
        static const char* m_ModifierNames[];
        static const char* m_FixNames[];
        static const char* m_SwitchCornerNames[];
};

#endif
