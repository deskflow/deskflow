/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2026 lxf82dev <lxf82dev@gmail.com>
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#include "gui/MacSmoothModeMenu.h"

#include <QDateTime>
#include <QFile>
#include <QFileInfo>
#include <QSignalSpy>
#include <QTemporaryDir>
#include <QTest>

class MacSmoothModeMenuTests : public QObject
{
  Q_OBJECT
private Q_SLOTS:
  void controls_useExistingWorkerProtocol();
  void staleOrMissingWorker_disablesControls();
  void invalidControl_reportsErrorWithoutReplacingLink();
};

namespace {
void writeFile(const QString &path, const QByteArray &contents)
{
  QFile file(path);
  QVERIFY(file.open(QIODevice::WriteOnly));
  QCOMPARE(file.write(contents), contents.size());
}

QByteArray readFile(const QString &path)
{
  QFile file(path);
  if (!file.open(QIODevice::ReadOnly))
    return {};
  return file.readAll();
}

QAction *action(MacSmoothModeMenu &menu, const char *name)
{
  return menu.findChild<QAction *>(name);
}
} // namespace

void MacSmoothModeMenuTests::controls_useExistingWorkerProtocol()
{
  QTemporaryDir temp;
  QVERIFY(temp.isValid());
  const auto control = temp.filePath("control");
  const auto status = temp.filePath("status");
  writeFile(control, "auto\n");
  writeFile(status, "version=1\nmode=smooth\n");
  MacSmoothModeMenu menu(nullptr, control, status);
  auto *automatic = action(menu, "smoothAutomatic");
  auto *airdrop = action(menu, "smoothAirDrop");
  auto *resume = action(menu, "smoothResume");
  QVERIFY(automatic && airdrop && resume);
  QVERIFY(automatic->isEnabled());
  QVERIFY(automatic->isChecked());
  QVERIFY(action(menu, "smoothStatus")->text().contains("suppressed"));
  // Merely constructing/opening the menu must not alter user preferences.
  QCOMPARE(readFile(control), QByteArray("auto\n"));
  automatic->trigger();
  QCOMPARE(readFile(control), QByteArray("disabled\n"));
  QVERIFY(!automatic->isChecked());
  QVERIFY(resume->isEnabled());
  resume->trigger();
  QCOMPARE(readFile(control), QByteArray("auto\n"));
  const auto before = QDateTime::currentSecsSinceEpoch();
  airdrop->trigger();
  const auto pause = readFile(control);
  QVERIFY(pause.startsWith("pause_until="));
  const auto until = pause.mid(12).trimmed().toLongLong();
  QVERIFY(until >= before + 600 && until <= QDateTime::currentSecsSinceEpoch() + 600);
  QVERIFY(airdrop->isChecked());
  QVERIFY(!airdrop->isEnabled());
  QVERIFY(resume->isEnabled());
  QCOMPARE(
      QFile(control).permissions() &
          (QFileDevice::ReadGroup | QFileDevice::WriteGroup | QFileDevice::ReadOther | QFileDevice::WriteOther),
      QFileDevice::Permissions()
  );
  resume->trigger();
  QCOMPARE(readFile(control), QByteArray("auto\n"));
  // Reflect external updates from an existing companion or expired override.
  writeFile(control, "disabled\n");
  menu.refresh();
  QVERIFY(!automatic->isChecked());
  writeFile(control, "pause_until=1\n");
  menu.refresh();
  QVERIFY(airdrop->isEnabled());
  QVERIFY(!airdrop->isChecked());
  QCOMPARE(readFile(control), QByteArray("pause_until=1\n"));
}

void MacSmoothModeMenuTests::staleOrMissingWorker_disablesControls()
{
  QTemporaryDir temp;
  QVERIFY(temp.isValid());
  const auto control = temp.filePath("control");
  const auto status = temp.filePath("status");
  writeFile(control, "auto\n");
  MacSmoothModeMenu menu(nullptr, control, status);
  QVERIFY(!action(menu, "smoothAutomatic")->isEnabled());
  QVERIFY(!action(menu, "smoothAirDrop")->isEnabled());
  writeFile(status, "version=1\nmode=allowed\n");
  menu.refresh();
  QVERIFY(action(menu, "smoothAutomatic")->isEnabled());
  QVERIFY(action(menu, "smoothStatus")->text().contains("AirDrop allowed"));
  QFile file(status);
  QVERIFY(file.open(QIODevice::ReadWrite));
  QVERIFY(file.setFileTime(QDateTime::currentDateTime().addSecs(-60), QFileDevice::FileModificationTime));
  file.close();
  menu.refresh();
  QVERIFY(!action(menu, "smoothAutomatic")->isEnabled());
  QVERIFY(!action(menu, "smoothResume")->isEnabled());
  QVERIFY(action(menu, "smoothStatus")->text().contains("offline"));
  writeFile(status, "version=2\nmode=smooth\n");
  menu.refresh();
  QVERIFY(!action(menu, "smoothAutomatic")->isEnabled());
  QCOMPARE(readFile(control), QByteArray("auto\n"));
}

void MacSmoothModeMenuTests::invalidControl_reportsErrorWithoutReplacingLink()
{
  QTemporaryDir temp;
  QVERIFY(temp.isValid());
  const auto control = temp.filePath("control");
  const auto status = temp.filePath("status");
  const auto target = temp.filePath("unrelated");
  writeFile(control, "auto\n");
  writeFile(status, "version=1\nmode=allowed\n");
  MacSmoothModeMenu menu(nullptr, control, status);
  QSignalSpy errors(&menu, &MacSmoothModeMenu::controlError);
  QVERIFY(QFile::remove(control));
  writeFile(target, "preserve me\n");
  QVERIFY(QFile::link(target, control));
  action(menu, "smoothAutomatic")->trigger();
  QCOMPARE(errors.size(), 1);
  QCOMPARE(readFile(target), QByteArray("preserve me\n"));
  QVERIFY(QFileInfo(control).isSymLink());
  QVERIFY(!action(menu, "smoothAutomatic")->isEnabled());
}

QTEST_MAIN(MacSmoothModeMenuTests)
#include "MacSmoothModeMenuTests.moc"
