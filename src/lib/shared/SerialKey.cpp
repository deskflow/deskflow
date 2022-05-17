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
#include "SerialKeyEdition.h"

SerialKey::SerialKey(Edition edition):
    m_data(edition)
{
}

SerialKey::SerialKey(const std::string& serial) :
    m_data(serial)
{
    std::string plainText = decode(m_data.m_key);

    if (!parse(plainText)) {
        throw std::runtime_error ("Invalid serial key");
    }
}

bool
SerialKey::isExpiring(time_t currentTime) const
{
    bool result = false;

    if (isTemporary()) {
        unsigned long long currentTimeAsLL = static_cast<unsigned long long>(currentTime);
        if ((m_data.m_warnTime <= currentTimeAsLL) && (currentTimeAsLL < m_data.m_expireTime)) {
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
        if (m_data.m_expireTime <= currentTimeAsLL) {
            result = true;
        }
    }

    return result;
}

bool
SerialKey::isTrial() const
{
    return m_data.m_keyType.isTrial();
}

bool
SerialKey::isTemporary() const
{
    return m_data.m_keyType.isTemporary();
}

bool
SerialKey::isValid() const
{
    bool Valid = true;

    if (!m_data.m_edition.isValid() || isExpired(::time(0)))
    {
        Valid = false;
    }

    return Valid;
}

Edition
SerialKey::edition() const
{
    return m_data.m_edition.getType();
}

std::string
SerialKey::toString() const
{
    return m_data.m_key;
}

time_t
SerialKey::daysLeft(time_t currentTime) const
{
    unsigned long long timeLeft =  0;
    unsigned long long const day = 60 * 60 * 24;

    unsigned long long currentTimeAsLL = static_cast<unsigned long long>(currentTime);
    if (currentTimeAsLL < m_data.m_expireTime) {
        timeLeft = m_data.m_expireTime - currentTimeAsLL;
    }

    unsigned long long daysLeft = 0;
    daysLeft = timeLeft % day != 0 ? 1 : 0;

    return timeLeft / day + daysLeft;
}

int
SerialKey::getSpanLeft(time_t time) const
{
    int result{-1};

    if (isTemporary() && !isExpired(time)){
        auto timeLeft = (m_data.m_expireTime - time) * 1000;

        if (timeLeft < INT_MAX){
            result = static_cast<int>(timeLeft);
        }
        else{
            result = INT_MAX;
        }
    }

    return result;
}

std::string
SerialKey::decode(const std::string& serial)
{
    static const char* const lut = "0123456789ABCDEF";
    std::string output;
    size_t len = serial.length();
    if (len & 1) {
        return output;
    }

    output.reserve(len / 2);
    for (size_t i = 0; i < len; i += 2) {

        char a = serial[i];
        char b = serial[i + 1];

        const char* p = std::lower_bound(lut, lut + 16, a);
        const char* q = std::lower_bound(lut, lut + 16, b);

        if (*q != b || *p != a) {
            return output;
        }

        output.push_back(static_cast<char>(((p - lut) << 4) | (q - lut)));
    }

    return output;
}

bool
SerialKey::parse(const std::string& plainSerial)
{
    bool valid = false;
    const auto parts = splitToParts(plainSerial);

    if ((parts.size() == 8) && (parts.at(0).find("v1") != std::string::npos)) {
        parseV1(parts);
        valid = true;
    }
    else if ((parts.size() == 9) && (parts.at(0).find("v2") != std::string::npos)) {
        parseV2(parts);
        valid = true;
    }

    return valid;
}

void
SerialKey::parseV1(const std::vector<std::string>& parts)
{
    // e.g.: {v1;basic;Bob;1;email;company name;1398297600;1398384000}
    m_data.m_edition.setType(parts.at(1));
    m_data.m_name = parts.at(2);
    sscanf(parts.at(3).c_str(), "%d", &m_data.m_userLimit);
    m_data.m_email = parts.at(4);
    m_data.m_company = parts.at(5);
    sscanf(parts.at(6).c_str(), "%lld", &m_data.m_warnTime);
    sscanf(parts.at(7).c_str(), "%lld", &m_data.m_expireTime);
}

void
SerialKey::parseV2(const std::vector<std::string>& parts)
{
    // e.g.: {v2;trial;basic;Bob;1;email;company name;1398297600;1398384000}
    m_data.m_keyType.setKeyType(parts.at(1));
    m_data.m_edition.setType(parts.at(2));
    m_data.m_name = parts.at(3);
    sscanf(parts.at(4).c_str(), "%d", &m_data.m_userLimit);
    m_data.m_email = parts.at(5);
    m_data.m_company = parts.at(6);
    sscanf(parts.at(7).c_str(), "%lld", &m_data.m_warnTime);
    sscanf(parts.at(8).c_str(), "%lld", &m_data.m_expireTime);
}

std::vector<std::string>
SerialKey::splitToParts(const std::string& plainSerial) const
{
    // tokenize serialised subscription.
    std::vector<std::string> parts;

    if (!plainSerial.empty()) {
        std::string parityStart = plainSerial.substr(0, 1);
        std::string parityEnd = plainSerial.substr(plainSerial.length() - 1, 1);

        // check for parity chars { and }, record parity result, then remove them.
        if (parityStart == "{" && parityEnd == "}") {
            const auto serialData = plainSerial.substr(1, plainSerial.length() - 2);

            std::string::size_type pos = 0;
            bool look = true;
            while (look) {
                std::string::size_type start = pos;
                pos = serialData.find(";", pos);
                if (pos == std::string::npos) {
                    pos = serialData.length();
                    look = false;
                }
                parts.push_back(serialData.substr(start, pos - start));
                pos += 1;
            }
        }
    }

    return parts;
}
