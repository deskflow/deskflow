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

#ifdef TEST_ENV
#include <gtest/gtest_prod.h>
#endif

class SerialKey {
    friend bool operator== (SerialKey const&, SerialKey const&);

public:
    explicit SerialKey (Edition edition = kUnregistered);
    explicit SerialKey (std::string serial);

    bool isExpiring (time_t currentTime) const;
    bool isExpired (time_t currentTime) const;
    bool isTrial () const;
    time_t daysLeft (time_t currentTime) const;
    std::string email () const;
    Edition edition () const;
    std::string toString () const;

    static std::string decode (const std::string& serial);
    static Edition parseEdition (const std::string& editionStr);

private:
    bool parse (std::string plainSerial);
    std::string editionString () const;

#ifdef TEST_ENV
private:
    FRIEND_TEST (SerialKeyTests, parse_noParty_invalid);
    FRIEND_TEST (SerialKeyTests, parse_invalidPartsLenghth_invalid);
    FRIEND_TEST (SerialKeyTests, parse_validV1Serial_valid);
    FRIEND_TEST (SerialKeyTests, parse_validV2Serial_valid);
#endif

private:
    std::string m_name;
    std::string m_email;
    std::string m_company;
    unsigned m_userLimit;
    unsigned long long m_warnTime;
    unsigned long long m_expireTime;
    Edition m_edition;
    bool m_trial;
};


inline bool
operator== (SerialKey const& lhs, SerialKey const& rhs) {
    return (lhs.m_name == rhs.m_name) && (lhs.m_email == rhs.m_email) &&
           (lhs.m_company == rhs.m_company) &&
           (lhs.m_userLimit == rhs.m_userLimit) &&
           (lhs.m_warnTime == rhs.m_warnTime) &&
           (lhs.m_expireTime == rhs.m_expireTime) &&
           (lhs.m_edition == rhs.m_edition) && (lhs.m_trial == rhs.m_trial);
}

inline bool
operator!= (SerialKey const& lhs, SerialKey const& rhs) {
    return !(lhs == rhs);
}
