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
#pragma once

#include <string>
#include <vector>
#include "SerialKeyData.h"

class SerialKeyParser
{
public:
    SerialKeyParser();
    bool parse(const std::string& plainSerial);
    static std::string decode(const std::string& serial);
    const SerialKeyData& getData() const;

private:
    void parseV1(const std::vector<std::string>& parts);
    void parseV2(const std::vector<std::string>& parts);
    std::vector<std::string> splitToParts(const std::string& plainSerial) const;

    SerialKeyData m_data;
};
