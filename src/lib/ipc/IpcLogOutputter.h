/*
 * synergy -- mouse and keyboard sharing utility
 * Copyright (C) 2012-2016 Symless Ltd.
 * Copyright (C) 2012 Nick Bolton
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

#include "arch/Arch.h"
#include "arch/IArchMultithread.h"
#include "base/ILogOutputter.h"
#include "ipc/Ipc.h"

#include <deque>

class IpcServer;
class Event;
class IpcClientProxy;

//! Write log to GUI over IPC
/*!
This outputter writes output to the GUI via IPC.
*/
class IpcLogOutputter : public ILogOutputter {
public:
    /*!
    If \p useThread is \c true, the buffer will be sent using a thread.
    If \p useThread is \c false, then the buffer needs to be sent manually
    using the \c sendBuffer() function.
    */
    IpcLogOutputter (IpcServer& ipcServer, EIpcClientType clientType,
                     bool useThread);
    virtual ~IpcLogOutputter ();

    // ILogOutputter overrides
    virtual void open (const char* title);
    virtual void close ();
    virtual void show (bool showIfEmpty);
    virtual bool write (ELevel level, const char* message);

    //! @name manipulators
    //@{

    //! Notify that the buffer should be sent.
    void notifyBuffer ();

    //! Set the buffer size
    /*!
    Set the maximum size of the buffer to protect memory
    from runaway logging.
    */
    void bufferMaxSize (UInt16 bufferMaxSize);

    //! Set the rate limit
    /*!
    Set the maximum number of \p writeRate for every \p timeRate in seconds.
    */
    void bufferRateLimit (UInt16 writeLimit, double timeLimit);

    //! Send the buffer
    /*!
    Sends a chunk of the buffer to the IPC server, normally called
    when threaded mode is on.
    */
    void sendBuffer ();

    //@}

    //! @name accessors
    //@{

    //! Get the buffer size
    /*!
    Returns the maximum size of the buffer.
    */
    UInt16 bufferMaxSize () const;

    //@}

private:
    void init ();
    void bufferThread (void*);
    String getChunk (size_t count);
    void appendBuffer (const String& text);
    bool isRunning ();

private:
    typedef std::deque<String> Buffer;

    IpcServer& m_ipcServer;
    Buffer m_buffer;
    ArchMutex m_bufferMutex;
    bool m_sending;
    Thread* m_bufferThread;
    bool m_running;
    ArchCond m_notifyCond;
    ArchMutex m_notifyMutex;
    bool m_bufferWaiting;
    IArchMultithread::ThreadID m_bufferThreadId;
    UInt16 m_bufferMaxSize;
    UInt16 m_bufferRateWriteLimit;
    double m_bufferRateTimeLimit;
    UInt16 m_bufferWriteCount;
    double m_bufferRateStart;
    EIpcClientType m_clientType;
    ArchMutex m_runningMutex;
};
