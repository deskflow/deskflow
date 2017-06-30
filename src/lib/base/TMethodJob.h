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

#include "IJob.h"

//! Use a function as a job
/*!
A job class that invokes a member function.
*/
template <class T>
class TMethodJob : public IJob {
public:
    //! run() invokes \c object->method(arg)
    TMethodJob (T* object, void (T::*method) (void*), void* arg = NULL);
    virtual ~TMethodJob ();

    // IJob overrides
    virtual void run ();

private:
    T* m_object;
    void (T::*m_method) (void*);
    void* m_arg;
};

template <class T>
inline TMethodJob<T>::TMethodJob (T* object, void (T::*method) (void*),
                                  void* arg)
    : m_object (object), m_method (method), m_arg (arg) {
    // do nothing
}

template <class T>
inline TMethodJob<T>::~TMethodJob () {
    // do nothing
}

template <class T>
inline void
TMethodJob<T>::run () {
    if (m_object != NULL) {
        (m_object->*m_method) (m_arg);
    }
}
