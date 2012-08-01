/* Copyright (C) 2002-2005 RealVNC Ltd.  All Rights Reserved.
 * 
 * This is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 * 
 * This software is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this software; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307,
 * USA.
 */

// -=- Threading.cxx
// Win32 Threading interface implementation

#include <malloc.h>

#include <rdr/Exception.h>
#include <rfb/LogWriter.h>
#include <rfb/util.h>
#include <rfb_win32/Threading.h>

using namespace rfb;

static LogWriter vlog("Threading");

static DWORD threadStorage = TlsAlloc();


inline void logAction(Thread* t, const char* action) {
  vlog.debug("%-16.16s %s(%lx)", action, t->getName(), t);
}

inline void logError(Thread* t, const char* err) {
  vlog.error("%-16.16s %s(%lx):%s", "failed", t->getName(), t, err);
}


DWORD WINAPI
Thread::threadProc(LPVOID lpParameter) {
  Thread* thread = (Thread*) lpParameter;
  TlsSetValue(threadStorage, thread);
  logAction(thread, "started");
  try {
    thread->run();
    logAction(thread, "stopped");
  } catch (rdr::Exception& e) {
    logError(thread, e.str());
  }
  bool deleteThread = false;
  {
    Lock l(thread->mutex);
    thread->state = ThreadStopped;
    thread->sig->signal();
    deleteThread = thread->deleteAfterRun;
  }
  if (deleteThread)
    delete thread;
  return 0;
}

Thread::Thread(const char* name_) : name(strDup(name_ ? name_ : "Unnamed")), sig(0), deleteAfterRun(false) {
  sig = new Condition(mutex);
  cond_event.h = CreateEvent(NULL, TRUE, FALSE, NULL);
  thread.h = CreateThread(NULL, 0, threadProc, this, CREATE_SUSPENDED, &thread_id);
  state = ThreadCreated;
  logAction(this, "created");
}

Thread::Thread(HANDLE thread_, DWORD thread_id_) : name(strDup("Native")), sig(0), deleteAfterRun(false),
  thread(thread_), thread_id(thread_id_) {
  cond_event.h = CreateEvent(NULL, TRUE, FALSE, NULL);
  state = ThreadNative;
  logAction(this, "created");
}

Thread::~Thread() {
  logAction(this, "destroying");
  if (!deleteAfterRun && state != ThreadNative)
    this->join();
  if (sig)
    delete sig;
  logAction(this, "destroyed");
}

void
Thread::run() {
}

void
Thread::start() {
  Lock l(mutex);
  if (state == ThreadCreated) {
    state = ThreadStarted;
    sig->signal();
    ResumeThread(thread);
  }
}

Thread*
Thread::join() {
  if (deleteAfterRun)
    throw rdr::Exception("attempt to join() with deleteAfterRun thread");
  Lock l(mutex);
  if (state == ThreadJoined) {
    logAction(this, "already joined");
  } else {
    logAction(this, "joining");
    while (state == ThreadStarted) {
      sig->wait();
      logAction(this, "checking");
    }
    state = ThreadJoined;
    logAction(this, "joined");
  }
  return this;
}

const char*
Thread::getName() const {
  return name.buf;
}

ThreadState
Thread::getState() const {
  return state;
}

unsigned long
Thread::getThreadId() const {
  return thread_id;
}


Thread*
Thread::self() {
  Thread* thread = (Thread*) TlsGetValue(threadStorage);
  if (!thread) {
    // *** memory leak - could use GetExitCodeThread to lazily detect when
    //     to clean up native thread objects
    thread = new Thread(GetCurrentThread(), GetCurrentThreadId());
    TlsSetValue(threadStorage, thread);
  }
  return thread;
}