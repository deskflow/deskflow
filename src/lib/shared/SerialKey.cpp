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

#include <fstream>
#include <iostream>
#include <algorithm>
#include <vector>

using namespace std;

SerialKey::SerialKey(std::string serial) :
	m_userLimit(1),
	m_warnTime(0),
	m_expireTime(0),
	m_edition(kBasic),
	m_trial(true),
	m_valid(false)
{
	string plainText = decode(serial);
	if (!plainText.empty()) {
		parse(plainText);
	}
}

bool
SerialKey::isValid(unsigned long long currentTime) const
{
	bool result = false;
	
	if (m_valid) {
		if (m_trial) {
			if (currentTime < m_expireTime) {
				result = true;
			}
		}
		else {
			result = true;
		}
	}
	
	return result;
}

bool
SerialKey::isExpiring(unsigned long long currentTime) const
{
	bool result = false;
	
	if (m_valid) {
		if (m_warnTime < currentTime && currentTime < m_expireTime) {
			result = true;
		}
	}
	
	return result;
}

bool
SerialKey::isExpired(unsigned long long currentTime) const
{
	bool result = false;
	
	if (m_valid) {
		if (currentTime > m_expireTime) {
			result = true;
		}
	}
	
	return result;
}

bool
SerialKey::isTrial() const
{
	return m_trial;
}

int
SerialKey::edition() const
{
	return m_edition;
}

unsigned long long
SerialKey::dayLeft(unsigned long long currentTime) const
{
	unsigned long long timeLeft =  0;
	unsigned long long day = 60 * 60 * 24;
	
	if (m_expireTime > currentTime) {
		timeLeft = m_expireTime - currentTime;
	}

	unsigned long long dayLeft = 0;
	dayLeft = timeLeft % day != 0 ? 1 : 0;
	
	return timeLeft / day + dayLeft;
}

std::string
SerialKey::decode(const std::string& serial) const
{
	static const char* const lut = "0123456789ABCDEF";
	string output;
	size_t len = serial.length();
	if (len & 1) {
		return output;
	}
	
	output.reserve(len / 2);
	for (size_t i = 0; i < len; i += 2) {
		
		char a = serial[i];
		char b = serial[i + 1];
		
		const char* p = std::lower_bound(lut, lut + 16, a);
		const char* q = std::lower_bound(lut, lut + 16, b);
		
		if (*q != b || *p != a) {
			return output;
		}
		
		output.push_back(static_cast<char>(((p - lut) << 4) | (q - lut)));
	}
	
	return output;
}

void
SerialKey::parse(std::string plainSerial)
{
	string parityStart = plainSerial.substr(0, 1);
	string parityEnd = plainSerial.substr(plainSerial.length() - 1, 1);
	
	// check for parity chars { and }, record parity result, then remove them.
	if (parityStart == "{" && parityEnd == "}") {
		plainSerial = plainSerial.substr(1, plainSerial.length() - 2);
		
		// tokenize serialised subscription.
		vector<string> parts;
		std::string::size_type pos = 0;
		bool look = true;
		while (look) {
			std::string::size_type start = pos;
			pos = plainSerial.find(";", pos);
			if (pos == string::npos) {
				pos = plainSerial.length();
				look = false;
			}
			parts.push_back(plainSerial.substr(start, pos - start));
			pos += 1;
		}
		
		if ((parts.size() == 8)
			&& (parts.at(0).find("v1") != string::npos)) {
			// e.g.: {v1;basic;Bob;1;email;company name;1398297600;1398384000}
			m_edition = getEdition(parts.at(1));
			m_name = parts.at(2);
			sscanf(parts.at(3).c_str(), "%d", &m_userLimit);
			m_email = parts.at(4);
			m_company = parts.at(5);
			sscanf(parts.at(6).c_str(), "%lld", &m_warnTime);
			sscanf(parts.at(7).c_str(), "%lld", &m_expireTime);
			m_valid = true;
		}
		else if ((parts.size() == 9)
				 && (parts.at(0).find("v2") != string::npos)) {
			// e.g.: {v2;trial;basic;Bob;1;email;company name;1398297600;1398384000}
			m_trial = parts.at(1) == "trial" ? true : false;
			m_edition = getEdition(parts.at(2));
			m_name = parts.at(3);
			sscanf(parts.at(4).c_str(), "%d", &m_userLimit);
			m_email = parts.at(5);
			m_company = parts.at(6);
			sscanf(parts.at(7).c_str(), "%lld", &m_warnTime);
			sscanf(parts.at(8).c_str(), "%lld", &m_expireTime);
			m_valid = true;
		}
	}
}

Edition
SerialKey::getEdition(std::string editionStr)
{
	Edition e = kBasic;
	if (editionStr == "pro") {
		e = kPro;
	}
	
	return e;
}
