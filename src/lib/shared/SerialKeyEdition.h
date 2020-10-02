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
#include "EditionType.h"

class SerialKeyEdition
{
	friend bool operator== (SerialKeyEdition const&, SerialKeyEdition const&);
public:
	SerialKeyEdition();
	explicit SerialKeyEdition(Edition type);
	explicit SerialKeyEdition(const std::string& type);

	Edition getType() const;
	std::string getName() const;
	std::string getDisplayName() const;

	void setType(Edition type);
	void setType(const std::string& type);

	static const std::string PRO;
	static const std::string BASIC;
	static const std::string BUSINESS;
	static const std::string UNREGISTERED;

private:
	Edition m_Type = kUnregistered;
};

inline bool
operator== (SerialKeyEdition const& lhs, SerialKeyEdition const& rhs) {
	return (lhs.m_Type == rhs.m_Type);
}

inline bool
operator!= (SerialKeyEdition const& lhs, SerialKeyEdition const& rhs) {
	return !(lhs == rhs);
}
