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

#include "synergy/clipboard_types.h"
#include "synergy/IClipboard.h"
#include "common/stdmap.h"
#include "common/stdlist.h"
#include "common/stdvector.h"

#if X_DISPLAY_MISSING
#error X11 is required to build synergy
#else
#include <X11/Xlib.h>
#endif

class IXWindowsClipboardConverter;

//! X11 clipboard implementation
class XWindowsClipboard : public IClipboard {
public:
    /*!
    Use \c window as the window that owns or interacts with the
    clipboard identified by \c id.
    */
    XWindowsClipboard (Display*, Window window, ClipboardID id);
    virtual ~XWindowsClipboard ();

    //! Notify clipboard was lost
    /*!
    Tells clipboard it lost ownership at the given time.
    */
    void lost (Time);

    //! Add clipboard request
    /*!
    Adds a selection request to the request list.  If the given
    owner window isn't this clipboard's window then this simply
    sends a failure event to the requestor.
    */
    void addRequest (Window owner, Window requestor, Atom target, ::Time time,
                     Atom property);

    //! Process clipboard request
    /*!
    Continues processing a selection request.  Returns true if the
    request was handled, false if the request was unknown.
    */
    bool processRequest (Window requestor, ::Time time, Atom property);

    //! Cancel clipboard request
    /*!
    Terminate a selection request.  Returns true iff the request
    was known and handled.
    */
    bool destroyRequest (Window requestor);

    //! Get window
    /*!
    Returns the clipboard's window (passed the c'tor).
    */
    Window getWindow () const;

    //! Get selection atom
    /*!
    Returns the selection atom that identifies the clipboard to X11
    (e.g. XA_PRIMARY).
    */
    Atom getSelection () const;

    // IClipboard overrides
    virtual bool empty ();
    virtual void add (EFormat, const String& data);
    virtual bool open (Time) const;
    virtual void close () const;
    virtual Time getTime () const;
    virtual bool has (EFormat) const;
    virtual String get (EFormat) const;

private:
    // remove all converters from our list
    void clearConverters ();

    // get the converter for a clipboard format.  returns NULL if no
    // suitable converter.  iff onlyIfNotAdded is true then also
    // return NULL if a suitable converter was found but we already
    // have data of the converter's clipboard format.
    IXWindowsClipboardConverter*
    getConverter (Atom target, bool onlyIfNotAdded = false) const;

    // convert target atom to clipboard format
    EFormat getFormat (Atom target) const;

    // add a non-MULTIPLE request.  does not verify that the selection
    // was owned at the given time.  returns true if the conversion
    // could be performed, false otherwise.  in either case, the
    // reply is inserted.
    bool addSimpleRequest (Window requestor, Atom target, ::Time time,
                           Atom property);

    // if not already checked then see if the cache is stale and, if so,
    // clear it.  this has the side effect of updating m_timeOwned.
    void checkCache () const;

    // clear the cache, resetting the cached flag and the added flag for
    // each format.
    void clearCache () const;
    void doClearCache ();

    // cache all formats of the selection
    void fillCache () const;
    void doFillCache ();

    //
    // helper classes
    //

    // read an ICCCM conforming selection
    class CICCCMGetClipboard {
    public:
        CICCCMGetClipboard (Window requestor, Time time, Atom property);
        ~CICCCMGetClipboard ();

        // convert the given selection to the given type.  returns
        // true iff the conversion was successful or the conversion
        // cannot be performed (in which case *actualTarget == None).
        bool readClipboard (Display* display, Atom selection, Atom target,
                            Atom* actualTarget, String* data);

    private:
        bool processEvent (Display* display, XEvent* event);

    private:
        Window m_requestor;
        Time m_time;
        Atom m_property;
        bool m_incr;
        bool m_failed;
        bool m_done;

        // atoms needed for the protocol
        Atom m_atomNone; // NONE, not None
        Atom m_atomIncr;

        // true iff we've received the selection notify
        bool m_reading;

        // the converted selection data
        String* m_data;

        // the actual type of the data.  if this is None then the
        // selection owner cannot convert to the requested type.
        Atom* m_actualTarget;

    public:
        // true iff the selection owner didn't follow ICCCM conventions
        bool m_error;
    };

    // Motif structure IDs
    enum { kMotifClipFormat = 1, kMotifClipItem, kMotifClipHeader };

    // _MOTIF_CLIP_HEADER structure
    class MotifClipHeader {
    public:
        SInt32 m_id; // kMotifClipHeader
        SInt32 m_pad1[3];
        SInt32 m_item;
        SInt32 m_pad2[4];
        SInt32 m_numItems;
        SInt32 m_pad3[3];
        SInt32 m_selectionOwner; // a Window
        SInt32 m_pad4[2];
    };

    // Motif clip item structure
    class MotifClipItem {
    public:
        SInt32 m_id; // kMotifClipItem
        SInt32 m_pad1[5];
        SInt32 m_size;
        SInt32 m_numFormats;
        SInt32 m_numDeletedFormats;
        SInt32 m_pad2[6];
    };

    // Motif clip format structure
    class MotifClipFormat {
    public:
        SInt32 m_id; // kMotifClipFormat
        SInt32 m_pad1[6];
        SInt32 m_length;
        SInt32 m_data;
        SInt32 m_type; // an Atom
        SInt32 m_pad2[1];
        SInt32 m_deleted;
        SInt32 m_pad3[4];
    };

    // stores data needed to respond to a selection request
    class Reply {
    public:
        Reply (Window, Atom target, ::Time);
        Reply (Window, Atom target, ::Time, Atom property, const String& data,
               Atom type, int format);

    public:
        // information about the request
        Window m_requestor;
        Atom m_target;
        ::Time m_time;
        Atom m_property;

        // true iff we've sent the notification for this reply
        bool m_replied;

        // true iff the reply has sent its last message
        bool m_done;

        // the data to send and its type and format
        String m_data;
        Atom m_type;
        int m_format;

        // index of next byte in m_data to send
        UInt32 m_ptr;
    };
    typedef std::list<Reply*> ReplyList;
    typedef std::map<Window, ReplyList> ReplyMap;
    typedef std::map<Window, long> ReplyEventMask;

    // ICCCM interoperability methods
    void icccmFillCache ();
    bool
    icccmGetSelection (Atom target, Atom* actualTarget, String* data) const;
    Time icccmGetTime () const;

    // motif interoperability methods
    bool motifLockClipboard () const;
    void motifUnlockClipboard () const;
    bool motifOwnsClipboard () const;
    void motifFillCache ();
    bool motifGetSelection (const MotifClipFormat*, Atom* actualTarget,
                            String* data) const;
    Time motifGetTime () const;

    // reply methods
    bool insertMultipleReply (Window, ::Time, Atom);
    void insertReply (Reply*);
    void pushReplies ();
    void pushReplies (ReplyMap::iterator&, ReplyList&, ReplyList::iterator);
    bool sendReply (Reply*);
    void clearReplies ();
    void clearReplies (ReplyList&);
    void sendNotify (Window requestor, Atom selection, Atom target,
                     Atom property, Time time);
    bool wasOwnedAtTime (::Time) const;

    // data conversion methods
    Atom getTargetsData (String&, int* format) const;
    Atom getTimestampData (String&, int* format) const;

private:
    typedef std::vector<IXWindowsClipboardConverter*> ConverterList;

    Display* m_display;
    Window m_window;
    ClipboardID m_id;
    Atom m_selection;
    mutable bool m_open;
    mutable Time m_time;
    bool m_owner;
    mutable Time m_timeOwned;
    Time m_timeLost;

    // true iff open and clipboard owned by a motif app
    mutable bool m_motif;

    // the added/cached clipboard data
    mutable bool m_checkCache;
    bool m_cached;
    Time m_cacheTime;
    bool m_added[kNumFormats];
    String m_data[kNumFormats];

    // conversion request replies
    ReplyMap m_replies;
    ReplyEventMask m_eventMasks;

    // clipboard format converters
    ConverterList m_converters;

    // atoms we'll need
    Atom m_atomTargets;
    Atom m_atomMultiple;
    Atom m_atomTimestamp;
    Atom m_atomInteger;
    Atom m_atomAtom;
    Atom m_atomAtomPair;
    Atom m_atomData;
    Atom m_atomINCR;
    Atom m_atomMotifClipLock;
    Atom m_atomMotifClipHeader;
    Atom m_atomMotifClipAccess;
    Atom m_atomGDKSelection;
};

//! Clipboard format converter interface
/*!
This interface defines the methods common to all X11 clipboard format
converters.
*/
class IXWindowsClipboardConverter : public IInterface {
public:
    //! @name accessors
    //@{

    //! Get clipboard format
    /*!
    Return the clipboard format this object converts from/to.
    */
    virtual IClipboard::EFormat getFormat () const = 0;

    //! Get X11 format atom
    /*!
    Return the atom representing the X selection format that
    this object converts from/to.
    */
    virtual Atom getAtom () const = 0;

    //! Get X11 property datum size
    /*!
    Return the size (in bits) of data elements returned by
    toIClipboard().
    */
    virtual int getDataSize () const = 0;

    //! Convert from IClipboard format
    /*!
    Convert from the IClipboard format to the X selection format.
    The input data must be in the IClipboard format returned by
    getFormat().  The return data will be in the X selection
    format returned by getAtom().
    */
    virtual String fromIClipboard (const String&) const = 0;

    //! Convert to IClipboard format
    /*!
    Convert from the X selection format to the IClipboard format
    (i.e., the reverse of fromIClipboard()).
    */
    virtual String toIClipboard (const String&) const = 0;

    //@}
};
