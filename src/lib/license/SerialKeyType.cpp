/*
 * synergy -- mouse and keyboard sharing utility
 * Copyright (C) 2015 Synergy Ltd.
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

#include "SerialKeyType.h"

const std::string SerialKeyType::Trial = "trial";
const std::string SerialKeyType::Subscription = "subscription";

SerialKeyType::SerialKeyType() {}

void SerialKeyType::setKeyType(const std::string &Type) {
  m_isTrial = (Type == SerialKeyType::Trial);
  m_isTimeLimited = (m_isTrial || (Type == SerialKeyType::Subscription));
}

bool SerialKeyType::isTrial() const { return m_isTrial; }

bool SerialKeyType::isTimeLimited() const { return m_isTimeLimited; }
