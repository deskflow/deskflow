/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2012 - 2016 Symless Ltd.
 * SPDX-FileCopyrightText: (C) 2002 Chris Schoeneman
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#pragma once

#include "deskflow/ClipboardTypes.h"
#include "deskflow/IClipboard.h"

#include <QString>

#include <list>
#include <map>
#include <vector>

#include <X11/Xlib.h>

class IXWindowsClipboardConverter;

//! X11 clipboard implementation
class XWindowsClipboard : public IClipboard
{
public:
  /*!
  Use \c window as the window that owns or interacts with the
  clipboard identified by \c id.
  */
  XWindowsClipboard(Display *, Window window, ClipboardID id);
  XWindowsClipboard(XWindowsClipboard const &) = delete;
  XWindowsClipboard(XWindowsClipboard &&) = delete;
  ~XWindowsClipboard() override;

  XWindowsClipboard &operator=(XWindowsClipboard const &) = delete;
  XWindowsClipboard &operator=(XWindowsClipboard &&) = delete;

  //! Notify clipboard was lost
  /*!
  Tells clipboard it lost ownership at the given time.
  */
  void lost(Time);

  //! Add clipboard request
  /*!
  Adds a selection request to the request list.  If the given
  owner window isn't this clipboard's window then this simply
  sends a failure event to the requestor.
  */
  void addRequest(Window owner, Window requestor, Atom target, ::Time time, Atom property);

  //! Process clipboard request
  /*!
  Continues processing a selection request.  Returns true if the
  request was handled, false if the request was unknown.
  */
  bool processRequest(Window requestor, ::Time time, Atom property);

  //! Cancel clipboard request
  /*!
  Terminate a selection request.  Returns true iff the request
  was known and handled.
  */
  bool destroyRequest(Window requestor);

  //! Get window
  /*!
  Returns the clipboard's window (passed the c'tor).
  */
  Window getWindow() const;

  //! Get selection atom
  /*!
  Returns the selection atom that identifies the clipboard to X11
  (e.g. XA_PRIMARY).
  */
  Atom getSelection() const;

  // IClipboard overrides
  bool empty() override;
  void add(Format, const std::string &data) override;
  bool open(Time) const override;
  void close() const override;
  Time getTime() const override;
  bool has(Format) const override;
  std::string get(Format) const override;

private:
  // remove all converters from our list
  void clearConverters();

  // get the converter for a clipboard format.  returns nullptr if no
  // suitable converter.  iff onlyIfNotAdded is true then also
  // return nullptr if a suitable converter was found but we already
  // have data of the converter's clipboard format.
  IXWindowsClipboardConverter *getConverter(Atom target, bool onlyIfNotAdded = false) const;

  // convert target atom to clipboard format
  Format getFormat(Atom target) const;

  // add a non-MULTIPLE request.  does not verify that the selection
  // was owned at the given time.  returns true if the conversion
  // could be performed, false otherwise.  in either case, the
  // reply is inserted.
  bool addSimpleRequest(Window requestor, Atom target, ::Time time, Atom property);

  // if not already checked then see if the cache is stale and, if so,
  // clear it.  this has the side effect of updating m_timeOwned.
  void checkCache() const;

  // clear the cache, resetting the cached flag and the added flag for
  // each format.
  void clearCache() const;
  void doClearCache();

  // cache all formats of the selection
  void fillCache() const;
  void doFillCache();

protected:
  //
  // helper classes
  //

  // read an ICCCM conforming selection
  class CICCCMGetClipboard
  {
  public:
    CICCCMGetClipboard(Window requestor, Time time, Atom property);
    ~CICCCMGetClipboard() = default;

    // convert the given selection to the given type.  returns
    // true iff the conversion was successful or the conversion
    // cannot be performed (in which case *actualTarget == None).
    bool readClipboard(Display *display, Atom selection, Atom target, Atom *actualTarget, std::string *data);

  private:
    bool processEvent(Display *display, const XEvent *event);

  private:
    Window m_requestor;
    Time m_time;
    Atom m_property;
    bool m_incr = false;
    bool m_failed = false;
    bool m_done = false;

    // atoms needed for the protocol
    Atom m_atomNone = None;
    Atom m_atomIncr = None;

    // true iff we've received the selection notify
    bool m_reading = false;

    // the converted selection data
    std::string *m_data = nullptr;

    // the actual type of the data.  if this is None then the
    // selection owner cannot convert to the requested type.
    Atom *m_actualTarget = nullptr;

    // true iff the selection owner didn't follow ICCCM conventions
    bool m_error = false;

  public:
    bool error() const
    {
      return m_error;
    }
  };

  // Motif structure IDs
  enum class MotifClip : int32_t
  {
    Format = 1,
    Item = 2,
    Header = 3
  };

  // _MOTIF_CLIP_HEADER structure
  class MotifClipHeader
  {
  public:
    MotifClip m_id;
    int32_t m_pad1[3];
    int32_t m_item;
    int32_t m_pad2[4];
    int32_t m_numItems;
    int32_t m_pad3[3];
    int32_t m_selectionOwner; // a Window
    int32_t m_pad4[2];
  };

  // Motif clip item structure
  class MotifClipItem
  {
  public:
    MotifClip m_id;
    int32_t m_pad1[5];
    int32_t m_size;
    int32_t m_numFormats;
    int32_t m_numDeletedFormats;
    int32_t m_pad2[6];
  };

  // Motif clip format structure
  class MotifClipFormat
  {
  public:
    MotifClip m_id;
    int32_t m_pad1[6];
    int32_t m_length;
    int32_t m_data;
    int32_t m_type; // an Atom
    int32_t m_pad2[1];
    int32_t m_deleted;
    int32_t m_pad3[4];
  };

  // stores data needed to respond to a selection request
  class Reply
  {
  public:
    Reply(Window, Atom target, ::Time);
    Reply(Window, Atom target, ::Time, Atom property, const std::string &data, Atom type, int format);

  public:
    // information about the request
    Window m_requestor;
    Atom m_target;
    ::Time m_time;
    Atom m_property;

    // true iff we've sent the notification for this reply
    bool m_replied = false;

    // true iff the reply has sent its last message
    bool m_done = false;

    // the data to send and its type and format
    std::string m_data;
    Atom m_type;
    int m_format;

    // index of next byte in m_data to send
    uint32_t m_ptr = 0;
  };
  using ReplyList = std::list<Reply *>;
  using ReplyMap = std::map<Window, ReplyList>;
  using ReplyEventMask = std::map<Window, long>;

  // ICCCM interoperability methods
  void icccmFillCache();
  bool icccmGetSelection(Atom target, Atom *actualTarget, std::string *data) const;
  Time icccmGetTime() const;

  // motif interoperability methods
  bool motifLockClipboard() const;
  void motifUnlockClipboard() const;
  bool motifOwnsClipboard() const;
  void motifFillCache();
  bool motifGetSelection(const MotifClipFormat *, Atom *actualTarget, std::string *data) const;
  Time motifGetTime() const;

  // reply methods
  bool insertMultipleReply(Window, ::Time, Atom);
  void insertReply(Reply *);
  void pushReplies();
  void pushReplies(ReplyMap::iterator &, ReplyList &, ReplyList::iterator);
  bool sendReply(Reply *);
  void clearReplies();
  void clearReplies(ReplyList &);
  void sendNotify(Window requestor, Atom selection, Atom target, Atom property, Time time);
  bool wasOwnedAtTime(::Time) const;

  // data conversion methods
  Atom getTargetsData(std::string &, int *format) const;
  Atom getTimestampData(std::string &, int *format) const;

private:
  using ConverterList = std::vector<IXWindowsClipboardConverter *>;

  Display *m_display;
  Window m_window;
  ClipboardID m_id;
  Atom m_selection;
  mutable bool m_open = false;
  mutable Time m_time = 0;
  bool m_owner = false;
  mutable Time m_timeOwned = 0;
  Time m_timeLost = 0;

  // true iff open and clipboard owned by a motif app
  mutable bool m_motif;

  // the added/cached clipboard data
  mutable bool m_checkCache;
  bool m_cached;
  Time m_cacheTime;
  bool m_added[static_cast<int>(IClipboard::Format::TotalFormats)];
  std::string m_data[static_cast<int>(IClipboard::Format::TotalFormats)];

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
class IXWindowsClipboardConverter : public IInterface
{
public:
  //! @name accessors
  //@{

  //! Get clipboard format
  /*!
  Return the clipboard format this object converts from/to.
  */
  virtual IClipboard::Format getFormat() const = 0;

  //! Get X11 format atom
  /*!
  Return the atom representing the X selection format that
  this object converts from/to.
  */
  virtual Atom getAtom() const = 0;

  //! Get X11 property datum size
  /*!
  Return the size (in bits) of data elements returned by
  toIClipboard().
  */
  virtual int getDataSize() const = 0;

  //! Convert from IClipboard format
  /*!
  Convert from the IClipboard format to the X selection format.
  The input data must be in the IClipboard format returned by
  getFormat().  The return data will be in the X selection
  format returned by getAtom().
  */
  virtual std::string fromIClipboard(const std::string &) const = 0;

  //! Convert to IClipboard format
  /*!
  Convert from the X selection format to the IClipboard format
  (i.e., the reverse of fromIClipboard()).
  */
  virtual std::string toIClipboard(const std::string &) const = 0;

  //@}
};
