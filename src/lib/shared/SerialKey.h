/*
 * synergy -- mouse and keyboard sharing utility
 * Copyright (C) 2015 Synergy Seamless Inc.
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

class SerialKey {
public:
	SerialKey(std::string serial);
	
	bool				isValid(unsigned long long currentTime) const;
	bool				isExpiring(unsigned long long currentTime) const;
	bool				isExpired(unsigned long long currentTime) const;
	bool				isTrial() const;
	int					edition() const;
		
private:
	std::string			m_name;
	std::string			m_type;
	std::string			m_email;
	std::string			m_company;
	int					m_userLimit;
	int					m_warnTime;
	int					m_expireTime;
	bool				m_trial;
};
