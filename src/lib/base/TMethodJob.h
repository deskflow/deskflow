/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2012 - 2016 Symless Ltd.
 * SPDX-FileCopyrightText: (C) 2002 Chris Schoeneman
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#pragma once

#include "IJob.h"

//! Use a function as a job
/*!
A job class that invokes a member function.
*/
template <class T> class TMethodJob : public IJob
{
public:
  //! run() invokes \c object->method(arg)
  TMethodJob(T *object, void (T::*method)(void *), void *arg = NULL);
  virtual ~TMethodJob();

  // IJob overrides
  virtual void run();

private:
  T *m_object;
  void (T::*m_method)(void *);
  void *m_arg;
};

template <class T>
inline TMethodJob<T>::TMethodJob(T *object, void (T::*method)(void *), void *arg)
    : m_object(object),
      m_method(method),
      m_arg(arg)
{
  // do nothing
}

template <class T> inline TMethodJob<T>::~TMethodJob()
{
  // do nothing
}

template <class T> inline void TMethodJob<T>::run()
{
  if (m_object != NULL) {
    (m_object->*m_method)(m_arg);
  }
}
