/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2026 Deskflow Developers
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#pragma once

#include "base/Log.h"

#include <QTest>

class FileDragTests : public QObject
{
  Q_OBJECT
private Q_SLOTS:
  void initTestCase();

  // item list codec
  void itemsRoundTrip();
  void itemsRejectUnsafe_data();
  void itemsRejectUnsafe();
  void itemsRejectBadStructure();
  void itemsRejectTruncated();
  void safePaths_data();
  void safePaths();
  void sanitizeComponent_data();
  void sanitizeComponent();
  void uniqueNameNeverReturnsTaken();

  // wire messages
  void chunkWireFormat();
  void messagesRoundTrip_data();
  void messagesRoundTrip();
  void readRejectsOversizedChunk();
  void readRejectsBadIcon();
  void readRejectsTraversalInBegin();
  void fileDragCodesAreNew();
  void localTransportRoundTrip();
  void readRejectsBadWatchAndOffer();
  void reverseNeedsBothPeersNew_data();
  void reverseNeedsBothPeersNew();

  // staging
  void stagingWritesAndReads();
  void stagingRejectsTraversal();
  void stagingSanitizesAndDeduplicates();
  void stagingChecksFreeSpace();
  void stagingRejectsSizeMismatch();
  void stagingCancelRemovesEverything();
  void stagingDestructorRemovesPartialFiles();
  void stagingReaderWaitsForData();
  void stagingSweepsDeadFolders();

  // moving staged items into a folder
  void moveIntoPlacesFilesAndFolders();
  void moveIntoNeverOverwrites();
  void moveNoReplacePicksFreeName();

  // sending
  void scanBuildsParentsFirst();
  void senderToStagingIsByteIdentical();
  void senderRespectsWindow();
  void senderFailsWhenFileChanges();
  void flowControlShrinksWhenQueueBuilds();
  void flowControlGrowsWhenQuick();

  // crossing the switch edge with a held button
  void switchWithHeldButton_data();
  void switchWithHeldButton();
  void offerNeedsOnlyTheLeftButton();
  void edgeStripFollowsDirectionMask_data();
  void edgeStripFollowsDirectionMask();
  void edgeStripIgnoresOldLeftValue();

  // window placement (macOS: edge windows per display, launcher and catcher windows)
  void edgeStripsLieOnOneDisplayEach();
  void edgeStripsSkipDisplaysAwayFromTheEdge();
  void edgeStripsWithoutDisplayListUseDesktop();
  void squareAroundStaysOnDisplay();
  void flipVerticalIsItsOwnInverse();
  void fileReferencePathsAreRecognised();

  // waiting for one dragged item (macOS file promises)
  void stagingListsTopLevelItems();
  void stagingWaitForItemCoversFolder();
  void stagingWaitForItemFailsOnCancel();
  void stagingWaitForItemRejectsNestedIndex();

  // drag image pixels
  void unpremultiplyRestoresStraightAlpha();

  // client to server: crossing the client's edge with a held button
  void switchFromClientWithHeldButton_data();
  void switchFromClientWithHeldButton();
  void remoteOfferNeedsWatchAndOnlyLeftButton();
  void clientWatchesTheEdgeThatLeadsToTheServer_data();
  void clientWatchesTheEdgeThatLeadsToTheServer();
  void serverWatchesTheEdgeThatLeadsToTheClient();
  void stepAcrossPushesOverTheWatchedEdge();

private:
  Log m_log;
};
