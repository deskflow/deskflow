/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2026 Deskflow Developers
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#pragma once

#include <QObject>

class CoordinationProtocolTests : public QObject
{
  Q_OBJECT
private Q_SLOTS:
  void claimRoundTrip();
  void promoteRoundTrip();
  void statusRoundTrip();
  void decodesLegacyCoordinatorClaim();
  void toleratesStringSequenceNumbers();
  void malformedInputIsInvalid();
  void statusReplyMatchesLegacyShape();
  void statusReplyIncludesFleetSnapshot();
  void peerListParsing();
  void cursorRoundTrip();
  void keyFwdRoundTrip();
  void keyRoundTrip();
  void keyFwdPhasesDecode();
  void helloRoundTrip();
  void fleetRoundTrip();
};
