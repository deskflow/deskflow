/*
 * synergy -- mouse and keyboard sharing utility
 * Copyright (C) 2022 Symless Ltd.
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

#include "SerialKeyParser.h"

void
SerialKeyParser::setKey(const std::string& key)
{
    m_data.key = key;
}

void
SerialKeyParser::setType(const std::string& type)
{
    m_data.keyType.setKeyType(type);
}

void
SerialKeyParser::setEdition(const std::string& edition)
{
    m_data.edition.setType(edition);
    if (m_data.keyType.isMaintenance() && m_data.edition.getType() == Edition::kBasic) {
        m_data.edition.setType(kLite);
    }
    else if (m_data.keyType.isMaintenance() && m_data.edition.getType() == Edition::kPro) {
        m_data.edition.setType(kUltimate);
    }
}

void
SerialKeyParser::setWarningTime(const std::string& warnTime)
{
    sscanf(warnTime.c_str(), "%lld", &m_data.warnTime);
}

void
SerialKeyParser::setExpirationTime(const std::string& expTime)
{
    sscanf(expTime.c_str(), "%lld", &m_data.expireTime);
}

std::string
SerialKeyParser::decode(const std::string& serial) const
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

        const auto p = std::lower_bound(lut, lut + 16, a);
        const auto q = std::lower_bound(lut, lut + 16, b);

        if (*q != b || *p != a) {
            return output;
        }

        output.push_back(static_cast<char>(((p - lut) << 4) | (q - lut)));
    }

    return output;
}

bool
SerialKeyParser::parse(const std::string& plainSerial)
{
    bool valid = false;
    auto key = decode(plainSerial);
    const auto parts = splitToParts(key);

    if ((parts.size() == 8) && (parts.at(0).find("v1") != std::string::npos)) {
        setKey(plainSerial);
        parseV1(parts);
        valid = true;
    }
    else if ((parts.size() == 9) && (parts.at(0).find("v2") != std::string::npos)) {
        setKey(plainSerial);
        parseV2(parts);
        valid = true;
    }

    return valid;
}

const SerialKeyData&
SerialKeyParser::getData() const
{
    return m_data;
}

void
SerialKeyParser::parseV1(const std::vector<std::string>& parts)
{
    // e.g.: {v1;basic;Bob;1;email;company name;1398297600;1398384000}
    setEdition(parts.at(1));
    setWarningTime(parts.at(6));
    setExpirationTime(parts.at(7));
}

void
SerialKeyParser::parseV2(const std::vector<std::string>& parts)
{
    // e.g.: {v2;trial;basic;Bob;1;email;company name;1398297600;1398384000}
    setType(parts.at(1));
    setEdition(parts.at(2));
    setWarningTime(parts.at(7));
    setExpirationTime(parts.at(8));
}

std::vector<std::string>
SerialKeyParser::splitToParts(const std::string& plainSerial) const
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
