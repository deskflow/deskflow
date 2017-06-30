/*
 * synergy -- mouse and keyboard sharing utility
 * Copyright (C) 2012-2016 Symless Ltd.
 * Copyright (C) 2002 Chris Schoeneman
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

#include "mt/Thread.h"
#include "base/ILogOutputter.h"
#include "base/String.h"
#include "common/basic_types.h"
#include "common/stddeque.h"

#include <list>
#include <fstream>

//! Stop traversing log chain outputter
/*!
This outputter performs no output and returns false from \c write(),
causing the logger to stop traversing the outputter chain.  Insert
this to prevent already inserted outputters from writing.
*/
class StopLogOutputter : public ILogOutputter {
public:
    StopLogOutputter ();
    virtual ~StopLogOutputter ();

    // ILogOutputter overrides
    virtual void open (const char* title);
    virtual void close ();
    virtual void show (bool showIfEmpty);
    virtual bool write (ELevel level, const char* message);
};

//! Write log to console
/*!
This outputter writes output to the console.  The level for each
message is ignored.
*/
class ConsoleLogOutputter : public ILogOutputter {
public:
    ConsoleLogOutputter ();
    virtual ~ConsoleLogOutputter ();

    // ILogOutputter overrides
    virtual void open (const char* title);
    virtual void close ();
    virtual void show (bool showIfEmpty);
    virtual bool write (ELevel level, const char* message);
    virtual void flush ();
};

//! Write log to file
/*!
This outputter writes output to the file.  The level for each
message is ignored.
*/

class FileLogOutputter : public ILogOutputter {
public:
    FileLogOutputter (const char* logFile);
    virtual ~FileLogOutputter ();

    // ILogOutputter overrides
    virtual void open (const char* title);
    virtual void close ();
    virtual void show (bool showIfEmpty);
    virtual bool write (ELevel level, const char* message);

    void setLogFilename (const char* title);

private:
    std::string m_fileName;
};

//! Write log to system log
/*!
This outputter writes output to the system log.
*/
class SystemLogOutputter : public ILogOutputter {
public:
    SystemLogOutputter ();
    virtual ~SystemLogOutputter ();

    // ILogOutputter overrides
    virtual void open (const char* title);
    virtual void close ();
    virtual void show (bool showIfEmpty);
    virtual bool write (ELevel level, const char* message);
};

//! Write log to system log only
/*!
Creating an object of this type inserts a StopLogOutputter followed
by a SystemLogOutputter into Log.  The destructor removes those
outputters.  Add one of these to any scope that needs to write to
the system log (only) and restore the old outputters when exiting
the scope.
*/
class SystemLogger {
public:
    SystemLogger (const char* title, bool blockConsole);
    ~SystemLogger ();

private:
    ILogOutputter* m_syslog;
    ILogOutputter* m_stop;
};

//! Save log history
/*!
This outputter records the last N log messages.
*/
class BufferedLogOutputter : public ILogOutputter {
private:
    typedef std::deque<String> Buffer;

public:
    typedef Buffer::const_iterator const_iterator;

    BufferedLogOutputter (UInt32 maxBufferSize);
    virtual ~BufferedLogOutputter ();

    //! @name accessors
    //@{

    //! Get start of buffer
    const_iterator begin () const;

    //! Get end of buffer
    const_iterator end () const;

    //@}

    // ILogOutputter overrides
    virtual void open (const char* title);
    virtual void close ();
    virtual void show (bool showIfEmpty);
    virtual bool write (ELevel level, const char* message);

private:
    UInt32 m_maxBufferSize;
    Buffer m_buffer;
};

//! Write log to message box
/*!
The level for each message is ignored.
*/
class MesssageBoxLogOutputter : public ILogOutputter {
public:
    MesssageBoxLogOutputter ();
    virtual ~MesssageBoxLogOutputter ();

    // ILogOutputter overrides
    virtual void open (const char* title);
    virtual void close ();
    virtual void show (bool showIfEmpty);
    virtual bool write (ELevel level, const char* message);
};
