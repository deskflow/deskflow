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
    /**
     * @brief ~SerialKeyParser destructor
     */
    virtual ~SerialKeyParser(){}
    /**
     * @brief parse serial key
     * @param plainSerial encoded serial key
     * @return true if parsed
     */
    virtual bool parse(const std::string& plainSerial);

    /**
     * @brief getData getter for serial key data
     * @return serial key data as DTO
     */
    const SerialKeyData& getData() const;

    /**
     * @brief setKey setter to set encoded key
     * @param key encoded serial key
     */
    void setKey(const std::string& key);

    /**
     * @brief setType setter for serial type
     * @param type serial key type
     */
    void setType(const std::string& type);

    /**
     * @brief setEdition setter for serial edition
     * @param edition serial key edition
     */
    void setEdition(const std::string& edition);

    /**
     * @brief setWarningTime setter for warning time
     * @param warnTime warning time for key as a string
     */
    void setWarningTime(const std::string& warnTime);

    /**
     * @brief setExpirationTime setter for expiration date
     * @param expTime expiration data as a string
     */
    void setExpirationTime(const std::string& expTime);

private:
    /**
     * @brief decode serial key from encoded string
     * @param encoded serial key as a string
     * @return decoded serial key as a string
     */
    std::string decode(const std::string& serial) const;

    /**
     * @brief parseV1 parse serial key version 1
     * @param parts of serial key version 1
     */
    void parseV1(const std::vector<std::string>& parts);

    /**
     * @brief parseV2 parse serial key version 2
     * @param parts parts of serial key version 2
     */
    void parseV2(const std::vector<std::string>& parts);

    /**
     * @brief splitToParts splits decoded serial key to parts
     * @param plainSerial decoded serial key as a string
     * @return splitted serial key
     */
    std::vector<std::string> splitToParts(const std::string& plainSerial) const;

    SerialKeyData m_data{}; //serial key data
};
