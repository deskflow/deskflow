/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2025 Chris Rizzitello <sithlord48@gmail.com>
 * SPDX-FileCopyrightText: (C) 2014 - 2016 Synergy App Ltd
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#include "ServerConfigTests.h"

#include "server/Config.h"

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

void ServerConfigTests::equalityCheck()
{
  Config a(nullptr);
  Config b(nullptr);
  QVERIFY(a.addComputer("computerA"));
  QVERIFY(a != b);

  QVERIFY(b.addComputer("computerB"));
  QVERIFY(a != b);

  QVERIFY(a.addComputer("computerB"));
  QVERIFY(a.addComputer("computerC"));
  QVERIFY(a.connect("computerA", Direction::Bottom, 0.0f, 0.5f, "computerB", 0.5f, 1.0f));
  QVERIFY(a.connect("computerB", Direction::Left, 0.0f, 0.5f, "computerB", 0.5f, 1.0f));
  QVERIFY(b.addComputer("computerA"));
  QVERIFY(b.addComputer("computerC"));
  QVERIFY(b.connect("computerA", Direction::Bottom, 0.0f, 0.5f, "computerB", 0.5f, 1.0f));
  QVERIFY(b.connect("computerB", Direction::Left, 0.0f, 0.5f, "computerB", 0.5f, 1.0f));
  QVERIFY(a.addOption("computerA", kOptionClipboardSharing, 1));
  QVERIFY(b.addOption("computerA", kOptionClipboardSharing, 1));
  QVERIFY(a.addOption(std::string(), kOptionClipboardSharing, 1));
  QVERIFY(b.addOption(std::string(), kOptionClipboardSharing, 1));

  a.getInputFilter()->addFilterRule(InputFilter::Rule{new OnlySystemFilter()});
  b.getInputFilter()->addFilterRule(InputFilter::Rule{new OnlySystemFilter()});
  QVERIFY(a.addAlias("computerA", "aliasA"));
  QVERIFY(b.addAlias("computerA", "aliasA"));
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

  QVERIFY(a.addComputer("computerA"));
  QVERIFY(b.addComputer("computerA"));
  QVERIFY(a.addOption("computerA", kOptionClipboardSharing, 0));
  QVERIFY(b.addOption("computerA", kOptionClipboardSharing, 1));
  QVERIFY(a != b);
}

void ServerConfigTests::equalityCheck_diff_alias()
{
  Config a(nullptr);
  Config b(nullptr);

  QVERIFY(a.addComputer("computerA"));
  QVERIFY(b.addComputer("computerA"));
  QVERIFY(b.addAlias("computerA", "aliasA"));
  QVERIFY(a != b);

  QVERIFY(a.addAlias("computerA", "aliasA"));
  QVERIFY(b.addAlias("computerA", "aliasB"));
  QVERIFY(a != b);
}

void ServerConfigTests::equalityCheck_diff_filters()
{
  Config a(nullptr);
  Config b(nullptr);
  QVERIFY(a.addComputer("computerA"));
  QVERIFY(b.addComputer("computerA"));

  a.getInputFilter()->addFilterRule(InputFilter::Rule{new OnlySystemFilter()});
  QVERIFY(a != b);
}

// TODO FIX
/*
void ServerConfigTests::equalityCheck_diff_address()
{
  Config a(nullptr);
  Config b(nullptr);
  QVERIFY(a.addComputer("computerA"));
  QVERIFY(b.addComputer("computerA"));
  a.setDeskflowAddress(NetworkAddress(8000));
  b.setDeskflowAddress(NetworkAddress(9000));
  QVERIFY(a != b);
}
*/

void ServerConfigTests::equalityCheck_diff_neighbours1()
{
  Config a(nullptr);
  Config b(nullptr);
  QVERIFY(a.addComputer("computerA"));
  QVERIFY(a.addComputer("computerB"));
  QVERIFY(a.connect("computerA", Direction::Bottom, 0.0f, 0.5f, "computerB", 0.5f, 1.0f));
  QVERIFY(b.addComputer("computerA"));
  QVERIFY(b.addComputer("computerB"));
  QVERIFY(a != b);
  QVERIFY(b != a);
}

void ServerConfigTests::equalityCheck_diff_neighbours2()
{
  Config a(nullptr);
  Config b(nullptr);
  QVERIFY(a.addComputer("computerA"));
  QVERIFY(a.addComputer("computerB"));
  QVERIFY(a.connect("computerA", Direction::Bottom, 0.0f, 0.5f, "computerB", 0.5f, 1.0f));
  QVERIFY(b.addComputer("computerA"));
  QVERIFY(b.addComputer("computerB"));
  QVERIFY(b.connect("computerA", Direction::Bottom, 0.0f, 0.25f, "computerB", 0.25f, 1.0f));
  QVERIFY(a != b);
}

void ServerConfigTests::equalityCheck_diff_neighbours3()
{
  Config a(nullptr);
  Config b(nullptr);
  QVERIFY(a.addComputer("computerA"));
  QVERIFY(a.addComputer("computerB"));
  QVERIFY(a.addComputer("computerC"));
  QVERIFY(a.connect("computerA", Direction::Bottom, 0.0f, 0.5f, "computerB", 0.5f, 1.0f));
  QVERIFY(b.addComputer("computerA"));
  QVERIFY(b.addComputer("computerB"));
  QVERIFY(b.addComputer("computerC"));
  QVERIFY(b.connect("computerA", Direction::Bottom, 0.0f, 0.5f, "computerC", 0.5f, 1.0f));
  QVERIFY(a != b);
}

void ServerConfigTests::holeLink_addAndGet_roundTrips()
{
  using deskflow::HoleLocation;

  Config config(nullptr);
  QVERIFY(config.addComputer("server"));
  QVERIFY(config.addComputer("client1"));
  QVERIFY(config.addComputer("client2"));

  QVERIFY(config.addHoleLink("server", HoleLocation::BottomLeft, "client1"));
  QVERIFY(config.addHoleLink("server", HoleLocation::TopRight, "client2"));

  const auto links = config.getHoleLinks("server");
  QCOMPARE(links.size(), 2);
  QCOMPARE(links[0].location, HoleLocation::BottomLeft);
  QCOMPARE(links[0].dstName, "client1");
  QCOMPARE(links[1].location, HoleLocation::TopRight);
  QCOMPARE(links[1].dstName, "client2");

  // a computer with no hole links registered should report none
  QVERIFY(config.getHoleLinks("client1").empty());
}

void ServerConfigTests::holeLink_unknownComputer_fails()
{
  using deskflow::HoleLocation;

  Config config(nullptr);
  QVERIFY(config.addComputer("server"));

  QVERIFY(!config.addHoleLink("server", HoleLocation::BottomLeft, "noSuchComputer"));
  QVERIFY(!config.addHoleLink("noSuchComputer", HoleLocation::BottomLeft, "server"));
  QVERIFY(config.getHoleLinks("server").empty());
}

void ServerConfigTests::holeLink_doesNotCreateNeighbor()
{
  using deskflow::HoleLocation;

  Config config(nullptr);
  QVERIFY(config.addComputer("server"));
  QVERIFY(config.addComputer("client1"));
  QVERIFY(config.addHoleLink("server", HoleLocation::BottomLeft, "client1"));

  // a hole link is only a pending request -- it must not be usable as a
  // neighbor until the server resolves it against the hole's real position
  QVERIFY(!config.hasNeighbor("server", Direction::Left));
  QVERIFY(!config.hasNeighbor("server", Direction::Bottom));
}

void ServerConfigTests::equalityCheck_diff_holeLinks()
{
  using deskflow::HoleLocation;

  Config a(nullptr);
  Config b(nullptr);
  QVERIFY(a.addComputer("computerA"));
  QVERIFY(a.addComputer("computerB"));
  QVERIFY(a.addHoleLink("computerA", HoleLocation::BottomLeft, "computerB"));
  QVERIFY(b.addComputer("computerA"));
  QVERIFY(b.addComputer("computerB"));
  QVERIFY(a != b);

  QVERIFY(b.addHoleLink("computerA", HoleLocation::BottomLeft, "computerB"));
  QVERIFY(a == b);
}

void ServerConfigTests::restartServerOnZoneChange_defaultsFalse()
{
  // the full "section: options" text-parsing path (Config::read() /
  // operator>>) isn't exercised anywhere else in this suite: it
  // unconditionally constructs a NetworkAddress, which asserts unless the
  // global Arch singleton has been initialized by main() first -- not
  // something a unit test should bootstrap just for this. The actual
  // "restartServerOnZoneChange = ..." line parsing added to
  // readSectionOptions() is exercised manually instead.
  Config config(nullptr);
  QVERIFY(!config.restartServerOnZoneChange());
}

QTEST_MAIN(ServerConfigTests)
