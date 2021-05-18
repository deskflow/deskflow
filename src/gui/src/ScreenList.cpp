/*
 * synergy -- mouse and keyboard sharing utility
 * Copyright (C) 2012-2021 Symless Ltd.
 * Copyright (C) 2008 Volker Lanz (vl@fidra.de)
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
    enum{kLEFT, kRIGHT, kTOP, kBOTTOM, kTOP_LEFT, kTOP_RIGHT, kBOTTOM_RIGHT, kBOTTOM_LEFT};
    std::array<int, 8> indexes = { -1 };

    if (serverIndex >= 0 && serverIndex < size)
    {
        indexes[kLEFT] = (serverIndex - 1) % width != width - 1 ? (serverIndex - 1) : -1;;
        indexes[kRIGHT] = (serverIndex + 1) % width != 0 ? (serverIndex + 1) : -1;
        indexes[kTOP] = (serverIndex - width) >= 0 ? (serverIndex - width) : -1;
        indexes[kBOTTOM] = (serverIndex + width) < size ? (serverIndex + width) : -1;
        indexes[kTOP_LEFT] = (indexes[kTOP] != - 1 && indexes[kLEFT] != -1) ? indexes[kTOP] - 1 : -1;
        indexes[kTOP_RIGHT] = (indexes[kTOP] != -1 && indexes[kRIGHT] != -1) ? indexes[kTOP] + 1 : -1;;
        indexes[kBOTTOM_RIGHT] = (indexes[kBOTTOM] != -1 && indexes[kRIGHT] != -1) ? indexes[kBOTTOM] + 1 : -1;;
        indexes[kBOTTOM_LEFT] = (indexes[kBOTTOM] != -1 && indexes[kLEFT] != -1) ? indexes[kBOTTOM] - 1 : -1;;
    }

    return indexes;
}

/**
 * @brief getServerIndex finds server and returns it's index
 * @param screens list to find server
 * @return server index
 */
int getServerIndex(const ScreenList& screens)
{
    int serverIndex = -1;

    for (int i = 0; i < screens.size(); ++i)
    {
        if (screens[i].isServer()){
            serverIndex = i;
            break;
        }
    }

    return serverIndex;
}

} //namespace

ScreenList::ScreenList(int width) :
    QList<Screen>(),
    m_width(width)
{

}

void ScreenList::addScreenByPriority(const Screen& newScreen)
{
    int serverIndex = getServerIndex(*this);
    auto indexes = getNeighborsIndexes(serverIndex, m_width, size());

    bool isAdded = false;
    for (const auto& index : indexes)
    {
        if (index >= 0 && index < size())
        {
            auto& screen = operator[](index);
            if (screen.isNull())
            {
                screen = newScreen;
                isAdded = true;
                break;
            }
        }
    }

    if (!isAdded)
    {
        addScreenToFirstEmpty(newScreen);
    }
}

void ScreenList::addScreenToFirstEmpty(const Screen& newScreen)
{
    for (int i = 0; i < size(); ++i)
    {
        auto& screen = operator[](i);
        if (screen.isNull())
        {
            screen = newScreen;
            break;
        }
    }
}

