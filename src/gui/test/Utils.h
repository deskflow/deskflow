/*  barrier -- mouse and keyboard sharing utility
    Copyright (C) 2021 Povilas Kanapickas <povilas@radix.lt>

    This package is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License
    found in the file LICENSE that should have accompanied this file.

    This package is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#ifndef BARRIER_GUI_TEST_UTILS_H
#define BARRIER_GUI_TEST_UTILS_H

#include <QtCore/QFile>
#include <QtCore/QTemporaryFile>

struct TestKey
{
    int key = 0;
    int modifier = Qt::NoModifier;

    TestKey(int key, int modifier) : key{key}, modifier{modifier} {}
};

inline QString getTemporaryFilename()
{
    QTemporaryFile temp_file;
    temp_file.open();
    return temp_file.fileName();
}

#endif // BARRIER_GUI_TEST_UTILS_H
