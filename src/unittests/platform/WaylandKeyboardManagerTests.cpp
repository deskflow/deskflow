/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2026 Applone <app1oneme@gmail.com>
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */
#include "platform/WaylandKeyboardManager.h"
#include <QTemporaryFile>
#include <QTest>

class WaylandKeyboardManagerTests : public QObject
{
  Q_OBJECT
private Q_SLOTS:
  void configurationPreservesVariantsAndOptions()
  {
    QTemporaryFile file;
    QVERIFY(file.open());
    file.write(
        "[Layout]\nLayoutList=us,ru\nVariantList=,phonetic\nModel=pc105\nOptions=grp:alt_shift_toggle,caps:escape\n"
    );
    file.flush();
    const auto value = deskflow::platform::WaylandKeyboardManager::readConfiguration(file.fileName(), {"us", "ru"});
    QVERIFY(value.configured);
    QCOMPARE(value.layouts, std::string("us,ru"));
    QCOMPARE(value.variants, std::string(",phonetic"));
    QCOMPARE(value.model, std::string("pc105"));
    QCOMPARE(value.options, std::string("grp:alt_shift_toggle,caps:escape"));
    const auto reordered = deskflow::platform::WaylandKeyboardManager::readConfiguration(file.fileName(), {"ru", "us"});
    QVERIFY(!reordered.configured);
    QCOMPARE(reordered.layouts, std::string("ru,us"));
    QVERIFY(reordered.variants.empty());
  }
};
QTEST_GUILESS_MAIN(WaylandKeyboardManagerTests)
#include "WaylandKeyboardManagerTests.moc"
