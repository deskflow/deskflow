/*
 * synergy -- mouse and keyboard sharing utility
 * Copyright (C) 2012-2016 Symless Ltd.
 * Copyright (C) 2011 Chris Schoeneman
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

#pragma once

//! Log levels
/*!
The logging priority levels in order of highest to lowest priority.
*/
enum ELevel {
    kPRINT = -1, //!< For print only (no file or time)
    kFATAL,      //!< For fatal errors
    kERROR,      //!< For serious errors
    kWARNING,    //!< For minor errors and warnings
    kNOTE,       //!< For messages about notable events
    kINFO,       //!< For informational messages
    kDEBUG,      //!< For important debugging messages
    kDEBUG1,     //!< For verbosity +1 debugging messages
    kDEBUG2,     //!< For verbosity +2 debugging messages
    kDEBUG3,     //!< For verbosity +3 debugging messages
    kDEBUG4,     //!< For verbosity +4 debugging messages
    kDEBUG5      //!< For verbosity +5 debugging messages
};
