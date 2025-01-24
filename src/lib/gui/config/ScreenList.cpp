/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2012 - 2021 Symless Ltd.
 * SPDX-FileCopyrightText: (C) 2008 Volker Lanz <vl@fidra.de>
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#include "ScreenList.h"

#include <array>

namespace {

/**
 * @brief getNeightborIndexes returns indexes for server neighbors
 * @param serverIndex server index
 * @param width of the grid
 * @param size of the grid
 * @return indexes for server neighbors
 */
std::array<int, 8> getNeighborsIndexes(int serverIndex, int width, int size)
{
  const int UNSET = -1;
  const int LEFT = 0;
  const int RIGHT = 1;
  const int TOP = 2;
  const int BOTTOM = 3;
  const int TOP_LEFT = 4;
  const int TOP_RIGHT = 5;
  const int BOTTOM_RIGHT = 6;
  const int BOTTOM_LEFT = 7;

  std::array<int, 8> indexes = {UNSET};

  if (serverIndex >= 0 && serverIndex < size) {
    indexes[LEFT] = (serverIndex - 1) % width != width - 1 ? (serverIndex - 1) : UNSET;
    indexes[RIGHT] = (serverIndex + 1) % width != 0 ? (serverIndex + 1) : UNSET;
    indexes[TOP] = (serverIndex - width) >= 0 ? (serverIndex - width) : UNSET;
    indexes[BOTTOM] = (serverIndex + width) < size ? (serverIndex + width) : UNSET;
    indexes[TOP_LEFT] = (indexes[TOP] != UNSET && indexes[LEFT] != UNSET) ? indexes[TOP] - 1 : UNSET;
    indexes[TOP_RIGHT] = (indexes[TOP] != UNSET && indexes[RIGHT] != UNSET) ? indexes[TOP] + 1 : UNSET;
    indexes[BOTTOM_RIGHT] = (indexes[BOTTOM] != UNSET && indexes[RIGHT] != UNSET) ? indexes[BOTTOM] + 1 : UNSET;
    indexes[BOTTOM_LEFT] = (indexes[BOTTOM] != UNSET && indexes[LEFT] != UNSET) ? indexes[BOTTOM] - 1 : UNSET;
  }

  return indexes;
}

/**
 * @brief getServerIndex finds server and returns it's index
 * @param screens list to find server
 * @return server index
 */
int getServerIndex(const ScreenList &screens)
{
  int serverIndex = -1;

  for (int i = 0; i < screens.size(); ++i) {
    if (screens[i].isServer()) {
      serverIndex = i;
      break;
    }
  }

  return serverIndex;
}

} // namespace

ScreenList::ScreenList(int width) : QList<Screen>(), m_width(width)
{
}

void ScreenList::addScreenByPriority(const Screen &newScreen)
{
  int serverIndex = getServerIndex(*this);
  auto indexes = getNeighborsIndexes(serverIndex, m_width, static_cast<int>(size()));

  bool isAdded = false;
  for (const auto &index : indexes) {
    if (index >= 0 && index < size()) {
      auto &screen = operator[](index);
      if (screen.isNull()) {
        screen = newScreen;
        isAdded = true;
        break;
      }
    }
  }

  if (!isAdded) {
    addScreenToFirstEmpty(newScreen);
  }
}

void ScreenList::addScreenToFirstEmpty(const Screen &newScreen)
{
  for (int i = 0; i < size(); ++i) {
    auto &screen = operator[](i);
    if (screen.isNull()) {
      screen = newScreen;
      break;
    }
  }
}

bool ScreenList::operator==(const ScreenList &sc) const
{
  return m_width == sc.m_width && QList::operator==(sc);
}
