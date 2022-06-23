/*
 * synergy -- mouse and keyboard sharing utility
 * Copyright (C) 2016 Symless Ltd.
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

#include "SerialKey.h"

#include <vector>
#include <stdexcept>
#include <climits>
#include "SerialKeyEdition.h"
#include "SerialKeyParser.h"

SerialKey::SerialKey(Edition edition):
    m_data(edition)
{
}

SerialKey::SerialKey(const std::string& serial) :
    m_data(serial)
{
    SerialKeyParser parser;

    if (parser.parse(serial)) {
        m_data = parser.getData();
    }
    else {
        throw std::runtime_error ("Invalid serial key");
    }
}

bool
SerialKey::isExpiring(time_t currentTime) const
{
    bool result = false;

    if (isTemporary()) {
        unsigned long long currentTimeAsLL = static_cast<unsigned long long>(currentTime);
        if ((m_data.warnTime <= currentTimeAsLL) && (currentTimeAsLL < m_data.expireTime)) {
            result = true;
        }
    }

    return result;
}

bool
SerialKey::isExpired(time_t currentTime) const
{
    bool result = false;

    if (isTemporary()) {
        unsigned long long currentTimeAsLL = static_cast<unsigned long long>(currentTime);
        if (m_data.expireTime <= currentTimeAsLL) {
            result = true;
        }
    }

    return result;
}

bool
SerialKey::isTrial() const
{
    return m_data.keyType.isTrial();
}

bool
SerialKey::isTemporary() const
{
    return (m_data.keyType.isTemporary() && !m_data.edition.isChina());
}

bool
SerialKey::isMaintenance() const
{
    return (m_data.keyType.isMaintenance() || m_data.edition.isChina());
}

bool
SerialKey::isValid() const
{
    bool Valid = true;

    if (!m_data.edition.isValid() || isExpired(::time(nullptr)))
    {
        Valid = false;
    }

    return Valid;
}

Edition
SerialKey::edition() const
{
    return m_data.edition.getType();
}

const std::string&
SerialKey::toString() const
{
    return m_data.key;
}

time_t
SerialKey::daysLeft(time_t currentTime) const
{
    unsigned long long timeLeft =  0;
    unsigned long long const day = 60 * 60 * 24;

    unsigned long long currentTimeAsLL = static_cast<unsigned long long>(currentTime);
    if (currentTimeAsLL < m_data.expireTime) {
        timeLeft = m_data.expireTime - currentTimeAsLL;
    }

    unsigned long long daysLeft = 0;
    daysLeft = timeLeft % day != 0 ? 1 : 0;

    return timeLeft / day + daysLeft;
}

time_t
SerialKey::getExpiration() const
{
    return m_data.expireTime;
}

int
SerialKey::getSpanLeft(time_t time) const
{
    int result{-1};

    if (isTemporary() && !isExpired(time)){
        auto timeLeft = (m_data.expireTime - time) * 1000;

        if (timeLeft < INT_MAX){
            result = static_cast<int>(timeLeft);
        }
        else{
            result = INT_MAX;
        }
    }

    return result;
}
