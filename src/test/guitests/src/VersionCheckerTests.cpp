/*
 * synergy -- mouse and keyboard sharing utility
 * Copyright (C) 2012-2016 Symless Ltd.
 * Copyright (C) 2012 Nick Bolton
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

#include "VersionCheckerTests.h"
#include "VersionChecker.cpp"
#include "../../gui/tmp/release/moc_VersionChecker.cpp"

#include <QtTest/QTest>

void
VersionCheckerTests::compareVersions () {
    VersionChecker versionChecker;

    // compare majors
    QCOMPARE (versionChecker.compareVersions ("1.0.0", "2.0.0"), 1);
    QCOMPARE (versionChecker.compareVersions ("2.0.0", "1.0.0"), -1);
    QCOMPARE (versionChecker.compareVersions ("1.0.0", "1.0.0"), 0);
    QCOMPARE (versionChecker.compareVersions ("1.4.8", "2.4.7"), 1);
    QCOMPARE (versionChecker.compareVersions ("2.4.7", "1.4.8"), -1);

    // compare minors
    QCOMPARE (versionChecker.compareVersions ("1.3.0", "1.4.0"), 1);
    QCOMPARE (versionChecker.compareVersions ("1.4.0", "1.3.0"), -1);
    QCOMPARE (versionChecker.compareVersions ("1.4.0", "1.4.0"), 0);
    QCOMPARE (versionChecker.compareVersions ("1.3.8", "1.4.7"), 1);
    QCOMPARE (versionChecker.compareVersions ("1.4.7", "1.3.8"), -1);

    // compare revs
    QCOMPARE (versionChecker.compareVersions ("1.4.7", "1.4.8"), 1);
    QCOMPARE (versionChecker.compareVersions ("1.4.8", "1.4.7"), -1);
    QCOMPARE (versionChecker.compareVersions ("1.4.7", "1.4.7"), 0);
}
