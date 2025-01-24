/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2012 - 2016 Symless Ltd.
 * SPDX-FileCopyrightText: (C) 2004 Chris Schoeneman
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#pragma once

#include "IEventJob.h"

//! Use a member function as an event job
/*!
An event job class that invokes a member function.
*/
template <class T> class TMethodEventJob : public IEventJob
{
public:
  //! run(event) invokes \c object->method(event, arg)
  TMethodEventJob(T *object, void (T::*method)(const Event &, void *), void *arg = NULL);
  virtual ~TMethodEventJob();

  // IJob overrides
  virtual void run(const Event &);

private:
  T *m_object;
  void (T::*m_method)(const Event &, void *);
  void *m_arg;
};

template <class T>
inline TMethodEventJob<T>::TMethodEventJob(T *object, void (T::*method)(const Event &, void *), void *arg)
    : m_object(object),
      m_method(method),
      m_arg(arg)
{
  // do nothing
}

template <class T> inline TMethodEventJob<T>::~TMethodEventJob()
{
  // do nothing
}

template <class T> inline void TMethodEventJob<T>::run(const Event &event)
{
  if (m_object != NULL) {
    (m_object->*m_method)(event, m_arg);
  }
}
