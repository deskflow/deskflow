/*
 * synergy -- mouse and keyboard sharing utility
 * Copyright (C) 2012-2016 Symless Ltd.
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

#if !defined(TRASHSCREENWIDGET__H)

#define TRASHSCREENWIDGET__H

#include <QLabel>

class QWidget;
class QDragEnterEvent;
class QDropEvent;

class TrashScreenWidget : public QLabel {
    Q_OBJECT

public:
    TrashScreenWidget (QWidget* parent) : QLabel (parent) {
    }

public:
    void dragEnterEvent (QDragEnterEvent* event);
    void dropEvent (QDropEvent* event);
};

#endif
