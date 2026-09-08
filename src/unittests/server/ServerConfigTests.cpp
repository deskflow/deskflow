/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2025 Chris Rizzitello <sithlord48@gmail.com>
 * SPDX-FileCopyrightText: (C) 2014 - 2016 Symless Ltd.
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#include "ServerConfigTests.h"

#include "server/Config.h"

#include <sstream>

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

void ServerConfigTests::inputLockAction_data()
{
  QTest::addColumn<QString>("action");
  QTest::addColumn<bool>("valid");
  QTest::newRow("no deadline") << "lockInput()" << true;
  QTest::newRow("short test deadline") << "lockInput(30)" << true;
  QTest::newRow("maximum deadline") << "lockInput(300)" << true;
  QTest::newRow("zero") << "lockInput(0)" << false;
  QTest::newRow("negative") << "lockInput(-1)" << false;
  QTest::newRow("fraction") << "lockInput(1.5)" << false;
  QTest::newRow("too large") << "lockInput(301)" << false;
  QTest::newRow("overflow") << "lockInput(999999999999999999999)" << false;
  QTest::newRow("extra argument") << "lockInput(10,20)" << false;
  QTest::newRow("invalid text") << "lockInput(cat)" << false;
  QTest::newRow("missing close") << "lockInput(30" << false;
}

void ServerConfigTests::inputLockAction()
{
  QFETCH(QString, action);
  QFETCH(bool, valid);
  const auto text =
      "section: screens\n  mac:\nend\nsection: options\n  keystroke(Control+Alt+Super+l) = " + action.toStdString() +
      "\nend\n";
  std::istringstream input(text);
  Config config(nullptr);
  if (valid) {
    input >> config;
    std::ostringstream output;
    output << config;
    QVERIFY(output.str().find(action.toStdString()) != std::string::npos);
    Config copy(config);
    QCOMPARE(copy, config);
  } else {
    QVERIFY_THROWS_EXCEPTION(ServerConfigReadException, input >> config);
  }
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
