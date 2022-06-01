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
#include "SerialKeyType.h"
#include "SerialKeyEdition.h"

/**
 * @brief The SerialKeyData struct
 * This is DTO which stores key data
 */
struct SerialKeyData {
    std::string           key;            // Encoded serial key as a string
    SerialKeyEdition      edition;        // Serial key edition
    SerialKeyType         keyType;        // Serial key type
    unsigned long long    warnTime   = 0; // Warning time
    unsigned long long    expireTime = 0; // Expiration time

    /**
     * @brief SerialKeyData constructor
     * @param key encoded key
     */
    explicit SerialKeyData(const std::string& key) :
        key(key)
    {
    }

    /**
     * @brief SerialKeyData default constructor
     * @param key edition. Unregistered by default
     */
    explicit SerialKeyData(Edition edition = kUnregistered) :
        edition(edition)
    {
    }
};

inline bool
operator== (SerialKeyData const& lhs, SerialKeyData const& rhs) {
    return  (lhs.key == rhs.key) &&
            (lhs.warnTime == rhs.warnTime) &&
            (lhs.expireTime == rhs.expireTime) &&
            (lhs.edition == rhs.edition) &&
            (lhs.keyType == rhs.keyType);
}
