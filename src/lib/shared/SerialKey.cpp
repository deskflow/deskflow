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

#include <fstream>
#include <iostream>
#include <algorithm>
#include <vector>
#include <climits>
#include <sstream>
#include <iomanip>
#include <stdexcept>
#include "SerialKeyEdition.h"

using namespace std;
static std::string hexEncode (std::string const& str);

SerialKey::SerialKey(Edition edition):
    m_userLimit(1),
    m_warnTime(ULLONG_MAX),
    m_expireTime(ULLONG_MAX),
    m_edition(edition)
{
}

SerialKey::SerialKey(std::string serial) :
    m_userLimit(1),
    m_warnTime(0),
    m_expireTime(0),
    m_edition(kBasic)
{
    string plainText = decode(serial);
    bool valid = false;
    if (!plainText.empty()) {
        valid = parse(plainText);
    }
    if (!valid) {
        throw std::runtime_error ("Invalid serial key");
    }
}

bool
SerialKey::isExpiring(time_t currentTime) const
{
    bool result = false;

    if (isTemporary()) {
        unsigned long long currentTimeAsLL = static_cast<unsigned long long>(currentTime);
        if ((m_warnTime <= currentTimeAsLL) && (currentTimeAsLL < m_expireTime)) {
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
        if (m_expireTime <= currentTimeAsLL) {
            result = true;
        }
    }

    return result;
}

bool
SerialKey::isTrial() const
{
    return m_KeyType.isTrial();
}

bool
SerialKey::isTemporary() const
{
    return m_KeyType.isTemporary();
}

bool
SerialKey::isValid() const
{
    bool Valid = true;

    if (m_edition.getType() == kUnregistered || isExpired(::time(0)))
    {
        Valid = false;
    }

    return Valid;
}

Edition
SerialKey::edition() const
{
    return m_edition.getType();
}

static std::string
hexEncode (std::string const& str) {
    std::ostringstream oss;
    for (size_t i = 0; i < str.size(); ++i) {
        unsigned c = str[i];
        c %= 256;
        oss << std::setfill('0') << std::hex << std::setw(2)
            << std::uppercase;
        oss << c;
    }
    return oss.str();
}

std::string
SerialKey::toString() const
{
    std::ostringstream oss;
    oss << "{";
    if (isTemporary()) {
        if (isTrial()){
            oss << "v2;" << SerialKeyType::TRIAL << ";";
        }
        else{
            oss << "v2;" << SerialKeyType::SUBSCRIPTION << ";";
        }
    } else {
        oss << "v1;";
    }
    oss << m_edition.getName() << ";";
    oss << m_name << ";";
    oss << m_userLimit << ";";
    oss << m_email << ";";
    oss << m_company << ";";
    oss << (isTemporary() ? m_warnTime : 0) << ";";
    oss << (isTemporary() ? m_expireTime : 0);
    oss << "}";
    return hexEncode(oss.str());
}

time_t
SerialKey::daysLeft(time_t currentTime) const
{
    unsigned long long timeLeft =  0;
    unsigned long long const day = 60 * 60 * 24;

    unsigned long long currentTimeAsLL = static_cast<unsigned long long>(currentTime);
    if (currentTimeAsLL < m_expireTime) {
        timeLeft = m_expireTime - currentTimeAsLL;
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
        auto timeLeft = (m_expireTime - time) * 1000;

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
SerialKey::email() const
{
    return m_email;
}

std::string
SerialKey::decode(const std::string& serial)
{
    static const char* const lut = "0123456789ABCDEF";
    string output;
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
SerialKey::parse(std::string plainSerial)
{
    string parityStart = plainSerial.substr(0, 1);
    string parityEnd = plainSerial.substr(plainSerial.length() - 1, 1);

    bool valid = false;

    // check for parity chars { and }, record parity result, then remove them.
    if (parityStart == "{" && parityEnd == "}") {
        plainSerial = plainSerial.substr(1, plainSerial.length() - 2);

        // tokenize serialised subscription.
        vector<string> parts;
        std::string::size_type pos = 0;
        bool look = true;
        while (look) {
            std::string::size_type start = pos;
            pos = plainSerial.find(";", pos);
            if (pos == string::npos) {
                pos = plainSerial.length();
                look = false;
            }
            parts.push_back(plainSerial.substr(start, pos - start));
            pos += 1;
        }

        if ((parts.size() == 8)
                && (parts.at(0).find("v1") != string::npos)) {
            // e.g.: {v1;basic;Bob;1;email;company name;1398297600;1398384000}
            m_edition.setType(parts.at(1));
            m_name = parts.at(2);
            sscanf(parts.at(3).c_str(), "%d", &m_userLimit);
            m_email = parts.at(4);
            m_company = parts.at(5);
            sscanf(parts.at(6).c_str(), "%lld", &m_warnTime);
            sscanf(parts.at(7).c_str(), "%lld", &m_expireTime);
            valid = true;
        }
        else if ((parts.size() == 9)
                 && (parts.at(0).find("v2") != string::npos)) {
            // e.g.: {v2;trial;basic;Bob;1;email;company name;1398297600;1398384000}
            m_KeyType.setKeyType(parts.at(1));
            m_edition.setType(parts.at(2));
            m_name = parts.at(3);
            sscanf(parts.at(4).c_str(), "%d", &m_userLimit);
            m_email = parts.at(5);
            m_company = parts.at(6);
            sscanf(parts.at(7).c_str(), "%lld", &m_warnTime);
            sscanf(parts.at(8).c_str(), "%lld", &m_expireTime);
            valid = true;
        }
    }

    return valid;
}
