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

#pragma once

#include <string>
#include <ctime>
#include "EditionType.h"
#include "SerialKeyData.h"
#include "SerialKeyType.h"
#include "SerialKeyEdition.h"

#ifdef TEST_ENV
#include <gtest/gtest_prod.h>
#endif

class SerialKey {
    friend bool operator== (SerialKey const&, SerialKey const&);
public:
    explicit SerialKey(Edition edition = kUnregistered);
    explicit SerialKey(const std::string& serial);

    bool                isExpiring(time_t currentTime) const;
    bool                isExpired(time_t currentTime) const;
    bool                isTrial() const;
    bool                isTemporary() const;
    bool                isValid() const;
    time_t              daysLeft(time_t currentTime) const;
    int                 getSpanLeft(time_t time = ::time(0)) const;
    Edition             edition() const;
    std::string         toString() const;

    static std::string  decode(const std::string& serial);

private:
    bool                parse(const std::string& plainSerial);
    void                parseV1(const std::vector<std::string>& parts);
    void                parseV2(const std::vector<std::string>& parts);
    std::vector<std::string> splitToParts(const std::string& plainSerial) const;

#ifdef TEST_ENV
private:
    FRIEND_TEST(SerialKeyTests, parse_noParty_invalid);
    FRIEND_TEST(SerialKeyTests, parse_invalidPartsLenghth_invalid);
    FRIEND_TEST(SerialKeyTests, parse_validV1Serial_valid);
    FRIEND_TEST(SerialKeyTests, parse_validV2Serial_valid);
#endif

private:
    SerialKeyData         m_data;

};


inline bool
operator== (SerialKey const& lhs, SerialKey const& rhs) {
    return (lhs.m_data == rhs.m_data);
}

inline bool
operator!= (SerialKey const& lhs, SerialKey const& rhs) {
    return !(lhs == rhs);
}
