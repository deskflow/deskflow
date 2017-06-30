/*
 * synergy -- mouse and keyboard sharing utility
 * Copyright (C) 2013-2016 Symless Ltd.
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

#pragma once

#include <QString>
#include <QVector>
#include <QComboBox>

class SynergyLocale {
    class Language {
    public:
        Language () {
        }
        Language (const QString& IetfCode, const QString& name)
            : m_IetfCode (IetfCode), m_Name (name) {
        }

    public:
        QString m_IetfCode;
        QString m_Name;
    };

public:
    SynergyLocale ();
    void fillLanguageComboBox (QComboBox* comboBox);

private:
    void loadLanguages ();
    void addLanguage (const QString& IetfCode, const QString& name);

private:
    QVector<Language> m_Languages;
};
