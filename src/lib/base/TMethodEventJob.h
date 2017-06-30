/*
 * synergy -- mouse and keyboard sharing utility
 * Copyright (C) 2012-2016 Symless Ltd.
 * Copyright (C) 2004 Chris Schoeneman
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

#include "IEventJob.h"

//! Use a member function as an event job
/*!
An event job class that invokes a member function.
*/
template <class T>
class TMethodEventJob : public IEventJob {
public:
    //! run(event) invokes \c object->method(event, arg)
    TMethodEventJob (T* object, void (T::*method) (const Event&, void*),
                     void* arg = NULL);
    virtual ~TMethodEventJob ();

    // IJob overrides
    virtual void run (const Event&);

private:
    T* m_object;
    void (T::*m_method) (const Event&, void*);
    void* m_arg;
};

template <class T>
inline TMethodEventJob<T>::TMethodEventJob (
    T* object, void (T::*method) (const Event&, void*), void* arg)
    : m_object (object), m_method (method), m_arg (arg) {
    // do nothing
}

template <class T>
inline TMethodEventJob<T>::~TMethodEventJob () {
    // do nothing
}

template <class T>
inline void
TMethodEventJob<T>::run (const Event& event) {
    if (m_object != NULL) {
        (m_object->*m_method) (event, m_arg);
    }
}
