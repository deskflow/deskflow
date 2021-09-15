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
#ifndef SCREENLIST_H
#define SCREENLIST_H

#include "Screen.h"

class ScreenList : public QList<Screen>
{
    int m_width = 5;

public:
    explicit ScreenList(int width = 5);

    /**
     * @brief addScreenByPriority adds a new screen according to the following priority:
     * 1.left side of the server
     * 2.right side of the server
     * 3.top
     * 4.down
     * 5.top left-hand diagonally
     * 6.top right-hand diagonally
     * 7.bottom right-hand diagonally
     * 8.bottom left-hand diagonally
     * 9.In case all places from the list have already booked, place in any spare place
     * @param newScreen
     */
    void addScreenByPriority(const Screen& newScreen);

    /**
     * @brief addScreenToFirstEmpty adds screen into the first empty place
     * @param newScreen
     */
    void addScreenToFirstEmpty(const Screen& newScreen);

    /**
     * @brief Returns true if screens are equal
     * @param sc
     */
    bool operator==(const ScreenList& sc) const;
};

#endif // SCREENLIST_H
