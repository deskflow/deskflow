/*
 * synergy -- mouse and keyboard sharing utility
 * Copyright (C) 2014-2021 Symless Ltd.
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
#include "LanguageManager.h"
#include "base/Log.h"

#include <algorithm>

namespace  {

String vectorToString(const std::vector<String>& vector, const String& delimiter = "")
{
    String string;
    for (const auto& item : vector) {
        if (&item != &vector[0]) {
            string += delimiter;
        }
        string += item;
    }
    return string;
}

} //anonymous namespace

namespace synergy {

namespace languages {

LanguageManager::LanguageManager(const std::vector<String>& localLanguages) :
    m_localLanguages(localLanguages)
{
    LOG((CLOG_INFO "Local languages: %s", vectorToString(m_localLanguages, ", ").c_str()));
}

void LanguageManager::setRemoteLanguages(const String& remoteLanguages)
{
    m_remoteLanguages.clear();
    if (!remoteLanguages.empty()) {
        for (size_t i = 0; i <= remoteLanguages.size() - 2; i +=2) {
            m_remoteLanguages.push_back(remoteLanguages.substr(i, 2));
        }
    }
    LOG((CLOG_INFO "Remote languages: %s", vectorToString(m_remoteLanguages, ", ").c_str()));
}

const std::vector<String>& LanguageManager::getRemoteLanguages() const
{
    return m_remoteLanguages;
}

const std::vector<String>& LanguageManager::getLocalLanguages() const
{
    return m_localLanguages;
}

String LanguageManager::getMissedLanguages() const
{
    String missedLanguages;

    for (const auto& language : m_remoteLanguages) {
        if (!isLanguageInstalled(language)) {
            if (!missedLanguages.empty()) {
                missedLanguages += ", ";
            }
            missedLanguages += language;
        }
    }

    return missedLanguages;
}

String LanguageManager::getSerializedLocalLanguages() const
{
    return  vectorToString(m_localLanguages);
}

bool LanguageManager::isLanguageInstalled(const String& language) const
{
    bool isInstalled = true;

    if (!m_localLanguages.empty()) {
        isInstalled = (std::find(m_localLanguages.begin(), m_localLanguages.end(), language) != m_localLanguages.end());
    }

    return isInstalled;
}

} //namespace languages

} //namespace synergy


