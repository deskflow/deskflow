/*
 * synergy -- mouse and keyboard sharing utility
 * Copyright (C) 2015 Synergy Si Ltd.
 *
 * This package is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * found in the file COPYING that should have accompanied this file.
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

#include <QString>

class Fingerprint
{
private:
	Fingerprint(const QString& filename);

public:
	void trust(const QString& fingerprintText);
	bool exists(const QString& fingerprintText);

public:
	static Fingerprint local();
	static Fingerprint trustedServers();
	static Fingerprint trustedClients();

	static QString localFingerprint();
	static bool localFingerprintExists();

private:
	QString m_Filename;
};
