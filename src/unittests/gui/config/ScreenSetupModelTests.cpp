/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2026 Mikhail Slyusarev <slyusarevmikhail@gmail.com>
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#include "ScreenSetupModelTests.h"

#include "gui/ScreenSetupModel.h"
#include "gui/config/ScreenList.h"

#include <QMimeData>

namespace {

// exposes the protected drop entry point used by the view
class TestModel : public ScreenSetupModel
{
public:
  using ScreenSetupModel::dropMimeData;
  using ScreenSetupModel::screen;
  using ScreenSetupModel::ScreenSetupModel;
};

// mimics ScreenSetupView::dropEvent: build the drag payload and drop on the
// TRUE target cell. The view resolves the cell with columnAt()/rowAt(), which
// are span-agnostic, so the target is NOT collapsed to a covering screen's
// anchor. Returns true if accepted.
bool drag(TestModel &m, int srcCol, int srcRow, int dstCol, int dstRow)
{
  QByteArray payload;
  QDataStream out(&payload, QIODevice::WriteOnly);
  out << srcCol << srcRow << m.screen(srcCol, srcRow);
  QMimeData mime;
  mime.setData(ScreenSetupModel::mimeType(), payload);

  return m.dropMimeData(&mime, Qt::MoveAction, -1, -1, m.index(dstRow, dstCol));
}

// every non-empty cell must be claimed by exactly one screen
bool hasOverlap(const ScreenList &list, int cols)
{
  const int rows = static_cast<int>(list.size()) / cols;
  for (int r = 0; r < rows; r++)
    for (int c = 0; c < cols; c++) {
      int claims = 0;
      for (int i = 0; i < list.size(); i++) {
        const auto &s = list[i];
        if (s.isNull())
          continue;
        const int ac = i % cols, ar = i / cols;
        if (c >= ac && c < ac + s.width() && r >= ar && r < ar + s.height())
          claims++;
      }
      if (claims > 1)
        return true;
    }
  return false;
}

} // namespace

void ScreenSetupModelTests::moveKeepsLayoutConsistent()
{
  ScreenList list(5);
  for (int i = 0; i < 15; i++)
    list.append(Screen());
  TestModel m(list, 5, 3);

  Screen wide("wide");
  wide.setWidth(3);
  m.screen(1, 2) = wide; // anchor (1,2), covers cols 1..3
  m.screen(0, 0) = Screen("solo");

  // move the wide screen around; the layout must never overlap
  QVERIFY(drag(m, 1, 2, 1, 1)); // up a row
  QVERIFY(!hasOverlap(list, 5));
  QVERIFY(drag(m, 1, 1, 2, 0)); // up and right
  QVERIFY(!hasOverlap(list, 5));
  QVERIFY(drag(m, 2, 0, 0, 2)); // back down to the left
  QVERIFY(!hasOverlap(list, 5));

  // the wide screen kept its span and is anchored where it was dropped
  QCOMPARE(m.screen(0, 2).name(), QStringLiteral("wide"));
  QCOMPARE(m.screen(0, 2).width(), 3);
}

void ScreenSetupModelTests::invalidSpanDropRejected()
{
  ScreenList list(5);
  for (int i = 0; i < 15; i++)
    list.append(Screen());
  TestModel m(list, 5, 3);

  Screen tall("tall");
  tall.setHeight(2);
  m.screen(4, 0) = tall; // anchor (4,0), covers rows 0..1
  m.screen(0, 2) = Screen("solo");

  // swapping solo onto the tall screen would push tall off the bottom row;
  // the drop must be rejected and the layout left untouched
  QVERIFY(!drag(m, 0, 2, 4, 1));
  QVERIFY(!hasOverlap(list, 5));
  QCOMPARE(m.screen(4, 0).name(), QStringLiteral("tall"));
  QCOMPARE(m.screen(0, 2).name(), QStringLiteral("solo"));
}

void ScreenSetupModelTests::spanNudgesIntoOwnFootprint()
{
  ScreenList list(5);
  for (int i = 0; i < 15; i++)
    list.append(Screen());
  TestModel m(list, 5, 3);

  Screen wide("wide");
  wide.setWidth(3);
  m.screen(0, 0) = wide; // anchor (0,0), covers cols 0..2

  // nudge one cell right: target (1,0) is INSIDE the screen's own span. The old
  // span-collapsed drop resolved this back to the anchor and rejected it as a
  // self-drop; the fix must let the screen shift by a single cell.
  QVERIFY(drag(m, 0, 0, 1, 0));
  QVERIFY(!hasOverlap(list, 5));
  QVERIFY(m.screen(0, 0).isNull());
  QCOMPARE(m.screen(1, 0).name(), QStringLiteral("wide"));
  QCOMPARE(m.screen(1, 0).width(), 3); // span preserved, now covers cols 1..3

  // shift into the right corner: anchor (2,0) covers cols 2..4
  QVERIFY(drag(m, 1, 0, 2, 0));
  QVERIFY(!hasOverlap(list, 5));
  QCOMPARE(m.screen(2, 0).name(), QStringLiteral("wide"));

  // one cell further would push cols 3..5 off the 5-wide grid -> rejected
  QVERIFY(!drag(m, 2, 0, 3, 0));
  QCOMPARE(m.screen(2, 0).name(), QStringLiteral("wide"));
}

void ScreenSetupModelTests::singleCellsSwap()
{
  ScreenList list(5);
  for (int i = 0; i < 15; i++)
    list.append(Screen());
  TestModel m(list, 5, 3);

  m.screen(0, 0) = Screen("a");
  m.screen(1, 0) = Screen("b");

  // dropping a onto an occupied cell swaps the two single-cell screens
  QVERIFY(drag(m, 0, 0, 1, 0));
  QVERIFY(!hasOverlap(list, 5));
  QCOMPARE(m.screen(1, 0).name(), QStringLiteral("a"));
  QCOMPARE(m.screen(0, 0).name(), QStringLiteral("b"));
}

void ScreenSetupModelTests::crossingSpansRejected()
{
  ScreenList list(5);
  for (int i = 0; i < 15; i++)
    list.append(Screen());
  TestModel m(list, 5, 3);

  Screen wide("wide");
  wide.setWidth(2);
  m.screen(0, 1) = wide;           // anchor (col0,row1), covers (0,1) and (1,1)
  m.screen(1, 0) = Screen("tall"); // 1x1 at (col1,row0)

  // growing "tall" down to height 2 makes it cover (1,0) and (1,1). (1,1) is a
  // covered (non-anchor) cell of "wide" as well, so the two spans cross there.
  // A wide screen and a tall screen overlapping like this must be rejected --
  // each span only ever covers raw-empty cells, so the per-span "is this raw
  // cell null" check misses it; validation has to be occupancy-based.
  QVERIFY(!m.trySetSpan(1, 0, 1, 2));
  QVERIFY(!hasOverlap(list, 5));
  QCOMPARE(m.screen(1, 0).height(), 1); // unchanged
}

QTEST_MAIN(ScreenSetupModelTests)
