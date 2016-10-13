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

SerialKey::SerialKey(std::string serial) :
	m_userLimit(1),
	m_warnTime(1),
	m_expireTime(1),
	m_trial(true)
{
	m_userLimit = 1;
	m_warnTime = 1;
	m_expireTime = 1;
	m_trial = true;
}
	
bool
SerialKey::isValid(unsigned long long currentTime) const
{
	return true;
}

bool
SerialKey::isExpiring(unsigned long long currentTime) const
{
	return true;
}

bool
SerialKey::isExpired(unsigned long long currentTime) const
{
	return true;
}

bool
SerialKey::isTrial() const
{
	return true;
}

int
SerialKey::edition() const
{
	return 1;
}
