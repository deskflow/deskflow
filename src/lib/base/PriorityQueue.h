/*
 * synergy -- mouse and keyboard sharing utility
 * Copyright (C) 2012-2016 Symless Ltd.
 * Copyright (C) 2003 Chris Schoeneman
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

#include "common/stdvector.h"

#include <algorithm>
#include <functional>

//! A priority queue with an iterator
/*!
This priority queue is the same as a standard priority queue except:
it sorts by std::greater, it has a forward iterator through the elements
(which can appear in any order), and its contents can be swapped.
*/
template <class T, class Container = std::vector<T>,
#if defined(_MSC_VER)
          class Compare = std::greater<Container::value_type>>
#else
          class Compare = std::greater<typename Container::value_type>>
#endif
class PriorityQueue {
public:
    typedef typename Container::value_type value_type;
    typedef typename Container::size_type size_type;
    typedef typename Container::iterator iterator;
    typedef typename Container::const_iterator const_iterator;
    typedef Container container_type;

    PriorityQueue () {
    }
    PriorityQueue (Container& swappedIn) {
        swap (swappedIn);
    }
    ~PriorityQueue () {
    }

    //! @name manipulators
    //@{

    //! Add element
    void
    push (const value_type& v) {
        c.push_back (v);
        std::push_heap (c.begin (), c.end (), comp);
    }

    //! Remove head element
    void
    pop () {
        std::pop_heap (c.begin (), c.end (), comp);
        c.pop_back ();
    }

    //! Erase element
    void
    erase (iterator i) {
        c.erase (i);
        std::make_heap (c.begin (), c.end (), comp);
    }

    //! Get start iterator
    iterator
    begin () {
        return c.begin ();
    }

    //! Get end iterator
    iterator
    end () {
        return c.end ();
    }

    //! Swap contents with another priority queue
    void
    swap (PriorityQueue<T, Container, Compare>& q) {
        c.swap (q.c);
    }

    //! Swap contents with another container
    void
    swap (Container& c2) {
        c.swap (c2);
        std::make_heap (c.begin (), c.end (), comp);
    }

    //@}
    //! @name accessors
    //@{

    //! Returns true if there are no elements
    bool
    empty () const {
        return c.empty ();
    }

    //! Returns the number of elements
    size_type
    size () const {
        return c.size ();
    }

    //! Returns the head element
    const value_type&
    top () const {
        return c.front ();
    }

    //! Get start iterator
    const_iterator
    begin () const {
        return c.begin ();
    }

    //! Get end iterator
    const_iterator
    end () const {
        return c.end ();
    }

    //@}

private:
    Container c;
    Compare comp;
};
