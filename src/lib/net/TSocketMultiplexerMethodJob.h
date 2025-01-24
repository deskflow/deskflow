/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2012 - 2016 Symless Ltd.
 * SPDX-FileCopyrightText: (C) 2002 Chris Schoeneman
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#pragma once

#include "arch/Arch.h"
#include "net/ISocketMultiplexerJob.h"

//! Use a method as a socket multiplexer job
/*!
A socket multiplexer job class that invokes a member function.
*/
template <class T> class TSocketMultiplexerMethodJob : public ISocketMultiplexerJob
{
public:
  typedef ISocketMultiplexerJob *(T::*Method)(ISocketMultiplexerJob *, bool, bool, bool);

  //! run() invokes \c object->method(arg)
  TSocketMultiplexerMethodJob(T *object, Method method, ArchSocket socket, bool readable, bool writeable);
  TSocketMultiplexerMethodJob(TSocketMultiplexerMethodJob const &) = delete;
  TSocketMultiplexerMethodJob(TSocketMultiplexerMethodJob &&) = delete;
  virtual ~TSocketMultiplexerMethodJob();

  TSocketMultiplexerMethodJob &operator=(TSocketMultiplexerMethodJob const &) = delete;
  TSocketMultiplexerMethodJob &operator=(TSocketMultiplexerMethodJob &&) = delete;

  // IJob overrides
  virtual ISocketMultiplexerJob *run(bool readable, bool writable, bool error);
  virtual ArchSocket getSocket() const;
  virtual bool isReadable() const;
  virtual bool isWritable() const;

private:
  T *m_object;
  Method m_method;
  ArchSocket m_socket;
  bool m_readable;
  bool m_writable;
  void *m_arg;
};

template <class T>
inline TSocketMultiplexerMethodJob<T>::TSocketMultiplexerMethodJob(
    T *object, Method method, ArchSocket socket, bool readable, bool writable
)
    : m_object(object),
      m_method(method),
      m_socket(ARCH->copySocket(socket)),
      m_readable(readable),
      m_writable(writable)
{
  // do nothing
}

template <class T> inline TSocketMultiplexerMethodJob<T>::~TSocketMultiplexerMethodJob()
{
  ARCH->closeSocket(m_socket);
}

template <class T> inline ISocketMultiplexerJob *TSocketMultiplexerMethodJob<T>::run(bool read, bool write, bool error)
{
  if (m_object != NULL) {
    return (m_object->*m_method)(this, read, write, error);
  }
  return NULL;
}

template <class T> inline ArchSocket TSocketMultiplexerMethodJob<T>::getSocket() const
{
  return m_socket;
}

template <class T> inline bool TSocketMultiplexerMethodJob<T>::isReadable() const
{
  return m_readable;
}

template <class T> inline bool TSocketMultiplexerMethodJob<T>::isWritable() const
{
  return m_writable;
}
