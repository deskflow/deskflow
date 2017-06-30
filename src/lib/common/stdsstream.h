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

#include "common/stdpre.h"

#if HAVE_SSTREAM || !defined(__GNUC__) || (__GNUC__ >= 3)

#include <sstream>

#elif defined(__GNUC_MINOR__) && (__GNUC_MINOR__ >= 95)
// g++ 2.95 didn't ship with sstream.  the following is a backport
// by Magnus Fromreide of the sstream in g++ 3.0.

/* This is part of libio/iostream, providing -*- C++ -*- input/output.
Copyright (C) 2012-2016 Symless Ltd.
Copyright (C) 2000 Free Software Foundation

This file is part of the GNU IO Library.  This library is free
software; you can redistribute it and/or modify it under the
terms of the GNU General Public License as published by the
Free Software Foundation; either version 2, or (at your option)
any later version.

This library is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this library; see the file LICENSE.  If not, write to the Free
Software Foundation, 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.

As a special exception, if you link this library with files
compiled with a GNU compiler to produce an executable, this does not cause
the resulting executable to be covered by the GNU General Public License.
This exception does not however invalidate any other reasons why
the executable file might be covered by the GNU General Public License. */

/* Written by Magnus Fromreide (magfr@lysator.liu.se). */
/* seekoff and ideas for overflow is largely borrowed from libstdc++-v3 */

#include <iostream.h>
#include <streambuf.h>
#include <string>

namespace std {
class stringbuf : public streambuf {
public:
    typedef char char_type;
    typedef int int_type;
    typedef streampos pos_type;
    typedef streamoff off_type;

    explicit stringbuf (int which = ios::in | ios::out)
        : streambuf (),
          mode (static_cast<ios::open_mode> (which)),
          stream (NULL),
          stream_len (0) {
        stringbuf_init ();
    }

    explicit stringbuf (const string& str, int which = ios::in | ios::out)
        : streambuf (),
          mode (static_cast<ios::open_mode> (which)),
          stream (NULL),
          stream_len (0) {
        if (mode & (ios::in | ios::out)) {
            stream_len = str.size ();
            stream     = new char_type[stream_len];
            str.copy (stream, stream_len);
        }
        stringbuf_init ();
    }

    virtual ~stringbuf () {
        delete[] stream;
    }

    string
    str () const {
        if (pbase () != 0)
            return string (stream, pptr () - pbase ());
        else
            return string ();
    }

    void
    str (const string& str) {
        delete[] stream;
        stream_len = str.size ();
        stream     = new char_type[stream_len];
        str.copy (stream, stream_len);
        stringbuf_init ();
    }

protected:
    // The buffer is already in gptr, so if it ends then it is out of data.
    virtual int
    underflow () {
        return EOF;
    }

    virtual int
    overflow (int c = EOF) {
        int res;
        if (mode & ios::out) {
            if (c != EOF) {
                streamsize old_stream_len = stream_len;
                stream_len += 1;
                char_type* new_stream = new char_type[stream_len];
                memcpy (new_stream, stream, old_stream_len);
                delete[] stream;
                stream = new_stream;
                stringbuf_sync (gptr () - eback (), pptr () - pbase ());
                sputc (c);
                res = c;
            } else
                res = EOF;
        } else
            res = 0;
        return res;
    }

    virtual streambuf*
    setbuf (char_type* s, streamsize n) {
        if (n != 0) {
            delete[] stream;
            stream = new char_type[n];
            memcpy (stream, s, n);
            stream_len = n;
            stringbuf_sync (0, 0);
        }
        return this;
    }

    virtual pos_type
    seekoff (off_type off, ios::seek_dir way, int which = ios::in | ios::out) {
        pos_type ret  = pos_type (off_type (-1));
        bool testin   = which & ios::in && mode & ios::in;
        bool testout  = which & ios::out && mode & ios::out;
        bool testboth = testin && testout && way != ios::cur;

        if (stream_len && ((testin != testout) || testboth)) {
            char_type* beg  = stream;
            char_type* curi = NULL;
            char_type* curo = NULL;
            char_type* endi = NULL;
            char_type* endo = NULL;

            if (testin) {
                curi = gptr ();
                endi = egptr ();
            }
            if (testout) {
                curo = pptr ();
                endo = epptr ();
            }

            off_type newoffi = 0;
            off_type newoffo = 0;
            if (way == ios::beg) {
                newoffi = beg - curi;
                newoffo = beg - curo;
            } else if (way == ios::end) {
                newoffi = endi - curi;
                newoffo = endo - curo;
            }

            if (testin && newoffi + off + curi - beg >= 0 &&
                endi - beg >= newoffi + off + curi - beg) {
                gbump (newoffi + off);
                ret = pos_type (newoffi + off + curi);
            }
            if (testout && newoffo + off + curo - beg >= 0 &&
                endo - beg >= newoffo + off + curo - beg) {
                pbump (newoffo + off);
                ret = pos_type (newoffo + off + curo);
            }
        }
        return ret;
    }

    virtual pos_type
    seekpos (pos_type sp, int which = ios::in | ios::out) {
        pos_type ret = seekoff (sp, ios::beg, which);
        return ret;
    }

private:
    void
    stringbuf_sync (streamsize i, streamsize o) {
        if (mode & ios::in)
            setg (stream, stream + i, stream + stream_len);
        if (mode & ios::out) {
            setp (stream, stream + stream_len);
            pbump (o);
        }
    }
    void
    stringbuf_init () {
        if (mode & ios::ate)
            stringbuf_sync (0, stream_len);
        else
            stringbuf_sync (0, 0);
    }

private:
    ios::open_mode mode;
    char_type* stream;
    streamsize stream_len;
};

class istringstream : public istream {
public:
    typedef char char_type;
    typedef int int_type;
    typedef streampos pos_type;
    typedef streamoff off_type;

    explicit istringstream (int which = ios::in)
        : istream (&sb), sb (which | ios::in) {
    }

    explicit istringstream (const string& str, int which = ios::in)
        : istream (&sb), sb (str, which | ios::in) {
    }

    stringbuf*
    rdbuf () const {
        return const_cast<stringbuf*> (&sb);
    }

    string
    str () const {
        return rdbuf ()->str ();
    }
    void
    str (const string& s) {
        rdbuf ()->str (s);
    }

private:
    stringbuf sb;
};

class ostringstream : public ostream {
public:
    typedef char char_type;
    typedef int int_type;
    typedef streampos pos_type;
    typedef streamoff off_type;

    explicit ostringstream (int which = ios::out)
        : ostream (&sb), sb (which | ios::out) {
    }

    explicit ostringstream (const string& str, int which = ios::out)
        : ostream (&sb), sb (str, which | ios::out) {
    }

    stringbuf*
    rdbuf () const {
        return const_cast<stringbuf*> (&sb);
    }

    string
    str () const {
        return rdbuf ()->str ();
    }

    void
    str (const string& s) {
        rdbuf ()->str (s);
    }

private:
    stringbuf sb;
};

class stringstream : public iostream {
public:
    typedef char char_type;
    typedef int int_type;
    typedef streampos pos_type;
    typedef streamoff off_type;

    explicit stringstream (int which = ios::out | ios::in)
        : iostream (&sb), sb (which) {
    }

    explicit stringstream (const string& str, int which = ios::out | ios::in)
        : iostream (&sb), sb (str, which) {
    }

    stringbuf*
    rdbuf () const {
        return const_cast<stringbuf*> (&sb);
    }

    string
    str () const {
        return rdbuf ()->str ();
    }

    void
    str (const string& s) {
        rdbuf ()->str (s);
    }

private:
    stringbuf sb;
};
};

#else /* not g++ 2.95 and no <sstream> */

#error "Standard C++ library is missing required sstream header."

#endif /* not g++ 2.95 and no <sstream> */

#include "common/stdpost.h"
#include "common/stdistream.h"
