/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2025 Chris Rizzitello <sithlord48@gmail.com>
 * SPDX-FileCopyrightText: (C) 2014 - 2016 Synergy App Ltd
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#include "ServerConfigTests.h"

#include "arch/Arch.h"
#include "common/Settings.h"
#include "server/Config.h"

#include <QFile>

class OnlySystemFilter : public InputFilter::Condition
{
public:
  Condition *clone() const override
  {
    return new OnlySystemFilter();
  }
  std::string format() const override
  {
    return "";
  }

  InputFilter::FilterStatus match(const Event &ev) override
  {
    return ev.getType() == EventTypes::System ? InputFilter::FilterStatus::Activate
                                              : InputFilter::FilterStatus::NoMatch;
  }
};

using namespace deskflow::server;

void ServerConfigTests::loadFromSettings()
{
  static Arch arch;
  arch.init();

  const auto settingsFile = QStringLiteral("ServerConfigTests.ini");
  QFile file(settingsFile);
  QVERIFY(file.open(QIODevice::WriteOnly | QIODevice::Truncate));
  file.write(
      "[core]\n"
      "computerName=alpha\n"
      "port=24899\n"
      "\n"
      "[server]\n"
      "enableClipboard=true\n"
      "clipboardSize=3\n"
      "gridWidth=5\n"
      "gridHeight=3\n"
      "\n"
      "[internalConfig]\n"
      "screens\\4\\name=gamma\n"
      "screens\\8\\name=alpha\n"
      "screens\\9\\name=beta\n"
      "screens\\9\\modifierArray\\1\\modifier=6\n"
      "screens\\9\\modifierArray\\size=7\n"
      "screens\\9\\switchCornerArray\\1\\switchCorner=true\n"
      "screens\\9\\switchCornerArray\\size=4\n"
      "screens\\9\\switchCornerSize=25\n"
      "screens\\size=15\n"
      "hotkeys\\1\\keys\\1\\key=16777264\n"
      "hotkeys\\1\\keys\\size=1\n"
      "hotkeys\\1\\actions\\1\\type=3\n"
      "hotkeys\\1\\actions\\1\\switchScreenName=beta\n"
      "hotkeys\\1\\actions\\size=1\n"
      "hotkeys\\size=1\n"
      "\n"
      "[screen_beta]\n"
      "aliases=beta.lan\n"
  );
  file.close();
  Settings::setSettingsFile(settingsFile);

  Config config(nullptr);
  config.loadFromSettings();

  QVERIFY(config.isScreen("alpha"));
  QVERIFY(config.isScreen("beta"));
  QVERIFY(config.isScreen("gamma"));
  QVERIFY(config.isScreen("beta.lan"));
  QCOMPARE(config.getCanonicalName("beta.lan"), "beta");

  QCOMPARE(config.getNeighbor("alpha", Direction::Right, 0.5f, nullptr), "beta");
  QCOMPARE(config.getNeighbor("beta", Direction::Left, 0.5f, nullptr), "alpha");
  QCOMPARE(config.getNeighbor("beta", Direction::Top, 0.5f, nullptr), "gamma");
  QCOMPARE(config.getNeighbor("gamma", Direction::Bottom, 0.5f, nullptr), "beta");
  QVERIFY(!config.hasNeighbor("alpha", Direction::Left));
  QVERIFY(!config.hasNeighbor("alpha", Direction::Top));

  const auto *globalOptions = config.getOptions("");
  QVERIFY(globalOptions != nullptr);
  QCOMPARE(globalOptions->at(kOptionClipboardSharing), 1);
  QCOMPARE(globalOptions->at(kOptionClipboardSharingSize), 3 * 1024);

  const auto *betaOptions = config.getOptions("beta");
  QVERIFY(betaOptions != nullptr);
  QCOMPARE(betaOptions->at(kOptionModifierMapForShift), kKeyModifierIDNull);
  QCOMPARE(betaOptions->at(kOptionScreenSwitchCorners), s_topLeftCornerMask);
  QCOMPARE(betaOptions->at(kOptionScreenSwitchCornerSize), 25);
  QVERIFY(!betaOptions->contains(kOptionModifierMapForControl));

  QCOMPARE(config.getInputFilter()->getNumRules(), 1u);
  const auto rules = config.getInputFilter()->format("");
  QVERIFY(rules.find("switchToScreen(beta)") != std::string::npos);
}

void ServerConfigTests::equalityCheck()
{
  Config a(nullptr);
  Config b(nullptr);
  QVERIFY(a.addScreen("screenA"));
  QVERIFY(a != b);

  QVERIFY(b.addScreen("screenB"));
  QVERIFY(a != b);

  QVERIFY(a.addScreen("screenB"));
  QVERIFY(a.addScreen("screenC"));
  QVERIFY(a.connect("screenA", Direction::Bottom, 0.0f, 0.5f, "screenB", 0.5f, 1.0f));
  QVERIFY(a.connect("screenB", Direction::Left, 0.0f, 0.5f, "screenB", 0.5f, 1.0f));
  QVERIFY(b.addScreen("screenA"));
  QVERIFY(b.addScreen("screenC"));
  QVERIFY(b.connect("screenA", Direction::Bottom, 0.0f, 0.5f, "screenB", 0.5f, 1.0f));
  QVERIFY(b.connect("screenB", Direction::Left, 0.0f, 0.5f, "screenB", 0.5f, 1.0f));
  QVERIFY(a.addOption("screenA", kOptionClipboardSharing, 1));
  QVERIFY(b.addOption("screenA", kOptionClipboardSharing, 1));
  QVERIFY(a.addOption(std::string(), kOptionClipboardSharing, 1));
  QVERIFY(b.addOption(std::string(), kOptionClipboardSharing, 1));

  a.getInputFilter()->addFilterRule(InputFilter::Rule{new OnlySystemFilter()});
  b.getInputFilter()->addFilterRule(InputFilter::Rule{new OnlySystemFilter()});
  QVERIFY(a.addAlias("screenA", "aliasA"));
  QVERIFY(b.addAlias("screenA", "aliasA"));
  /* TODO Fix linking to the proper libs
  NetworkAddress addr1("localhost", 8080);
  addr1.resolve();
  NetworkAddress addr2("localhost", 8080);
  addr2.resolve();
  a.setDeskflowAddress(addr1);
  b.setDeskflowAddress(addr2);
  */
  QVERIFY(a == b);
}

void ServerConfigTests::equalityCheck_diff_options()
{
  Config a(nullptr);
  Config b(nullptr);

  QVERIFY(a.addScreen("screenA"));
  QVERIFY(b.addScreen("screenA"));
  QVERIFY(a.addOption("screenA", kOptionClipboardSharing, 0));
  QVERIFY(b.addOption("screenA", kOptionClipboardSharing, 1));
  QVERIFY(a != b);
}

void ServerConfigTests::equalityCheck_diff_alias()
{
  Config a(nullptr);
  Config b(nullptr);

  QVERIFY(a.addScreen("screenA"));
  QVERIFY(b.addScreen("screenA"));
  QVERIFY(b.addAlias("screenA", "aliasA"));
  QVERIFY(a != b);

  QVERIFY(a.addAlias("screenA", "aliasA"));
  QVERIFY(b.addAlias("screenA", "aliasB"));
  QVERIFY(a != b);
}

void ServerConfigTests::equalityCheck_diff_filters()
{
  Config a(nullptr);
  Config b(nullptr);
  QVERIFY(a.addScreen("screenA"));
  QVERIFY(b.addScreen("screenA"));

  a.getInputFilter()->addFilterRule(InputFilter::Rule{new OnlySystemFilter()});
  QVERIFY(a != b);
}

// TODO FIX
/*
void ServerConfigTests::equalityCheck_diff_address()
{
  Config a(nullptr);
  Config b(nullptr);
  QVERIFY(a.addScreen("screenA"));
  QVERIFY(b.addScreen("screenA"));
  a.setDeskflowAddress(NetworkAddress(8000));
  b.setDeskflowAddress(NetworkAddress(9000));
  QVERIFY(a != b);
}
*/

void ServerConfigTests::equalityCheck_diff_neighbours1()
{
  Config a(nullptr);
  Config b(nullptr);
  QVERIFY(a.addScreen("screenA"));
  QVERIFY(a.addScreen("screenB"));
  QVERIFY(a.connect("screenA", Direction::Bottom, 0.0f, 0.5f, "screenB", 0.5f, 1.0f));
  QVERIFY(b.addScreen("screenA"));
  QVERIFY(b.addScreen("screenB"));
  QVERIFY(a != b);
  QVERIFY(b != a);
}

void ServerConfigTests::equalityCheck_diff_neighbours2()
{
  Config a(nullptr);
  Config b(nullptr);
  QVERIFY(a.addScreen("screenA"));
  QVERIFY(a.addScreen("screenB"));
  QVERIFY(a.connect("screenA", Direction::Bottom, 0.0f, 0.5f, "screenB", 0.5f, 1.0f));
  QVERIFY(b.addScreen("screenA"));
  QVERIFY(b.addScreen("screenB"));
  QVERIFY(b.connect("screenA", Direction::Bottom, 0.0f, 0.25f, "screenB", 0.25f, 1.0f));
  QVERIFY(a != b);
}

void ServerConfigTests::equalityCheck_diff_neighbours3()
{
  Config a(nullptr);
  Config b(nullptr);
  QVERIFY(a.addScreen("screenA"));
  QVERIFY(a.addScreen("screenB"));
  QVERIFY(a.addScreen("screenC"));
  QVERIFY(a.connect("screenA", Direction::Bottom, 0.0f, 0.5f, "screenB", 0.5f, 1.0f));
  QVERIFY(b.addScreen("screenA"));
  QVERIFY(b.addScreen("screenB"));
  QVERIFY(b.addScreen("screenC"));
  QVERIFY(b.connect("screenA", Direction::Bottom, 0.0f, 0.5f, "screenC", 0.5f, 1.0f));
  QVERIFY(a != b);
}

QTEST_MAIN(ServerConfigTests)
