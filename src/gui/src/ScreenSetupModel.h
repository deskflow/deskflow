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

#if !defined(SCREENSETUPMODEL__H)

#define SCREENSETUPMODEL__H

#include <QAbstractTableModel>
#include <QList>
#include <QString>
#include <QStringList>

#include "Screen.h"

class ScreenSetupView;
class ServerConfigDialog;

class ScreenSetupModel : public QAbstractTableModel {
    Q_OBJECT

    friend class ScreenSetupView;
    friend class ServerConfigDialog;

public:
    ScreenSetupModel (ScreenList& screens, int numColumns, int numRows);

public:
    static const QString&
    mimeType () {
        return m_MimeType;
    }
    QVariant data (const QModelIndex& index, int role = Qt::DisplayRole) const;
    int
    rowCount () const {
        return m_NumRows;
    }
    int
    columnCount () const {
        return m_NumColumns;
    }
    int
    rowCount (const QModelIndex&) const {
        return rowCount ();
    }
    int
    columnCount (const QModelIndex&) const {
        return columnCount ();
    }
    Qt::DropActions supportedDropActions () const;
    Qt::ItemFlags flags (const QModelIndex& index) const;
    QStringList mimeTypes () const;
    QMimeData* mimeData (const QModelIndexList& indexes) const;

protected:
    bool dropMimeData (const QMimeData* data, Qt::DropAction action, int row,
                       int column, const QModelIndex& parent);
    const Screen&
    screen (const QModelIndex& index) const {
        return screen (index.column (), index.row ());
    }
    Screen&
    screen (const QModelIndex& index) {
        return screen (index.column (), index.row ());
    }
    const Screen&
    screen (int column, int row) const {
        return m_Screens[row * m_NumColumns + column];
    }
    Screen&
    screen (int column, int row) {
        return m_Screens[row * m_NumColumns + column];
    }

private:
    ScreenList& m_Screens;
    const int m_NumColumns;
    const int m_NumRows;

    static const QString m_MimeType;
};

#endif
