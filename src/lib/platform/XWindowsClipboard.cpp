/*
 * barrier -- mouse and keyboard sharing utility
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

#include "platform/XWindowsClipboard.h"

#include "platform/XWindowsClipboardTextConverter.h"
#include "platform/XWindowsClipboardUCS2Converter.h"
#include "platform/XWindowsClipboardUTF8Converter.h"
#include "platform/XWindowsClipboardHTMLConverter.h"
#include "platform/XWindowsClipboardBMPConverter.h"
#include "platform/XWindowsUtil.h"
#include "mt/Thread.h"
#include "arch/Arch.h"
#include "base/Log.h"
#include "base/Stopwatch.h"
#include "common/stdvector.h"

#include <cstdio>
#include <cstring>
#include <X11/Xatom.h>

//
// XWindowsClipboard
//

XWindowsClipboard::XWindowsClipboard(IXWindowsImpl* impl, Display* display,
                Window window, ClipboardID id) :
    m_display(display),
    m_window(window),
    m_id(id),
    m_open(false),
    m_time(0),
    m_owner(false),
    m_timeOwned(0),
    m_timeLost(0)
{
    m_impl = impl;
    // get some atoms
    m_atomTargets         = m_impl->XInternAtom(m_display, "TARGETS", False);
    m_atomMultiple        = m_impl->XInternAtom(m_display, "MULTIPLE", False);
    m_atomTimestamp       = m_impl->XInternAtom(m_display, "TIMESTAMP", False);
    m_atomInteger         = m_impl->XInternAtom(m_display, "INTEGER", False);
    m_atomAtom            = m_impl->XInternAtom(m_display, "ATOM", False);
    m_atomAtomPair        = m_impl->XInternAtom(m_display, "ATOM_PAIR", False);
    m_atomData            = m_impl->XInternAtom(m_display, "CLIP_TEMPORARY",
                                                False);
    m_atomINCR            = m_impl->XInternAtom(m_display, "INCR", False);
    m_atomMotifClipLock   = m_impl->XInternAtom(m_display, "_MOTIF_CLIP_LOCK",
                                                False);
    m_atomMotifClipHeader = m_impl->XInternAtom(m_display, "_MOTIF_CLIP_HEADER",
                                                False);
    m_atomMotifClipAccess = m_impl->XInternAtom(m_display,
                                "_MOTIF_CLIP_LOCK_ACCESS_VALID", False);
    m_atomGDKSelection    = m_impl->XInternAtom(m_display, "GDK_SELECTION",
                                                False);

    // set selection atom based on clipboard id
    switch (id) {
    case kClipboardClipboard:
        m_selection = m_impl->XInternAtom(m_display, "CLIPBOARD", False);
        break;

    case kClipboardSelection:
    default:
        m_selection = XA_PRIMARY;
        break;
    }

    // add converters, most desired first
    m_converters.push_back(new XWindowsClipboardHTMLConverter(m_display,
                                "text/html"));
    m_converters.push_back(new XWindowsClipboardBMPConverter(m_display));
    m_converters.push_back(new XWindowsClipboardUTF8Converter(m_display,
                                "text/plain;charset=UTF-8"));
    m_converters.push_back(new XWindowsClipboardUTF8Converter(m_display,
                                "UTF8_STRING"));
    m_converters.push_back(new XWindowsClipboardUCS2Converter(m_display,
                                "text/plain;charset=ISO-10646-UCS-2"));
    m_converters.push_back(new XWindowsClipboardUCS2Converter(m_display,
                                "text/unicode"));
    m_converters.push_back(new XWindowsClipboardTextConverter(m_display,
                                "text/plain"));
    m_converters.push_back(new XWindowsClipboardTextConverter(m_display,
                                "STRING"));

    // we have no data
    clearCache();
}

XWindowsClipboard::~XWindowsClipboard()
{
    clearReplies();
    clearConverters();
}

void
XWindowsClipboard::lost(Time time)
{
    LOG((CLOG_DEBUG "lost clipboard %d ownership at %d", m_id, time));
    if (m_owner) {
        m_owner    = false;
        m_timeLost = time;
        clearCache();
    }
}

void
XWindowsClipboard::addRequest(Window owner, Window requestor,
                Atom target, ::Time time, Atom property)
{
    // must be for our window and we must have owned the selection
    // at the given time.
    bool success = false;
    if (owner == m_window) {
        LOG((CLOG_DEBUG1 "request for clipboard %d, target %s by 0x%08x (property=%s)", m_selection, XWindowsUtil::atomToString(m_display, target).c_str(), requestor, XWindowsUtil::atomToString(m_display, property).c_str()));
        if (wasOwnedAtTime(time)) {
            if (target == m_atomMultiple) {
                // add a multiple request.  property may not be None
                // according to ICCCM.
                if (property != None) {
                    success = insertMultipleReply(requestor, time, property);
                }
            }
            else {
                addSimpleRequest(requestor, target, time, property);

                // addSimpleRequest() will have already handled failure
                success = true;
            }
        }
        else {
            LOG((CLOG_DEBUG1 "failed, not owned at time %d", time));
        }
    }

    if (!success) {
        // send failure
        LOG((CLOG_DEBUG1 "failed"));
        insertReply(new Reply(requestor, target, time));
    }

    // send notifications that are pending
    pushReplies();
}

bool
XWindowsClipboard::addSimpleRequest(Window requestor,
                Atom target, ::Time time, Atom property)
{
    // obsolete requestors may supply a None property.  in
    // that case we use the target as the property to store
    // the conversion.
    if (property == None) {
        property = target;
    }

    // handle targets
    String data;
    Atom type  = None;
    int format = 0;
    if (target == m_atomTargets) {
        type = getTargetsData(data, &format);
    }
    else if (target == m_atomTimestamp) {
        type = getTimestampData(data, &format);
    }
    else {
        IXWindowsClipboardConverter* converter = getConverter(target);
        if (converter != NULL) {
            IClipboard::EFormat clipboardFormat = converter->getFormat();
            if (m_added[clipboardFormat]) {
                try {
                    data   = converter->fromIClipboard(m_data[clipboardFormat]);
                    format = converter->getDataSize();
                    type   = converter->getAtom();
                }
                catch (...) {
                    // ignore -- cannot convert
                }
            }
        }
    }

    if (type != None) {
        // success
        LOG((CLOG_DEBUG1 "success"));
        insertReply(new Reply(requestor, target, time,
                                property, data, type, format));
        return true;
    }
    else {
        // failure
        LOG((CLOG_DEBUG1 "failed"));
        insertReply(new Reply(requestor, target, time));
        return false;
    }
}

bool
XWindowsClipboard::processRequest(Window requestor,
                ::Time /*time*/, Atom property)
{
    ReplyMap::iterator index = m_replies.find(requestor);
    if (index == m_replies.end()) {
        // unknown requestor window
        return false;
    }
    LOG((CLOG_DEBUG1 "received property %s delete from 0x08%x", XWindowsUtil::atomToString(m_display, property).c_str(), requestor));

    // find the property in the known requests.  it should be the
    // first property but we'll check 'em all if we have to.
    ReplyList& replies = index->second;
    for (ReplyList::iterator index2 = replies.begin();
                                index2 != replies.end(); ++index2) {
        Reply* reply = *index2;
        if (reply->m_replied && reply->m_property == property) {
            // if reply is complete then remove it and start the
            // next one.
            pushReplies(index, replies, index2);
            return true;
        }
    }

    return false;
}

bool
XWindowsClipboard::destroyRequest(Window requestor)
{
    ReplyMap::iterator index = m_replies.find(requestor);
    if (index == m_replies.end()) {
        // unknown requestor window
        return false;
    }

    // destroy all replies for this window
    clearReplies(index->second);
    m_replies.erase(index);

    // note -- we don't stop watching the window for events because
    // we're called in response to the window being destroyed.

    return true;
}

Window
XWindowsClipboard::getWindow() const
{
    return m_window;
}

Atom
XWindowsClipboard::getSelection() const
{
    return m_selection;
}

bool
XWindowsClipboard::empty()
{
    assert(m_open);

    LOG((CLOG_DEBUG "empty clipboard %d", m_id));

    // assert ownership of clipboard
    m_impl->XSetSelectionOwner(m_display, m_selection, m_window, m_time);
    if (m_impl->XGetSelectionOwner(m_display, m_selection) != m_window) {
        LOG((CLOG_DEBUG "failed to grab clipboard %d", m_id));
        return false;
    }

    // clear all data.  since we own the data now, the cache is up
    // to date.
    clearCache();
    m_cached = true;

    // FIXME -- actually delete motif clipboard items?
    // FIXME -- do anything to motif clipboard properties?

    // save time
    m_timeOwned = m_time;
    m_timeLost  = 0;

    // we're the owner now
    m_owner = true;
    LOG((CLOG_DEBUG "grabbed clipboard %d", m_id));

    return true;
}

void
XWindowsClipboard::add(EFormat format, const String& data)
{
    assert(m_open);
    assert(m_owner);

    LOG((CLOG_DEBUG "add %d bytes to clipboard %d format: %d", data.size(), m_id, format));

    m_data[format]  = data;
    m_added[format] = true;

    // FIXME -- set motif clipboard item?
}

bool
XWindowsClipboard::open(Time time) const
{
    if (m_open) {
        LOG((CLOG_DEBUG "failed to open clipboard: already opened"));
        return false;
    }

    LOG((CLOG_DEBUG "open clipboard %d", m_id));

    // assume not motif
    m_motif = false;

    // lock clipboard
    if (m_id == kClipboardClipboard) {
        if (!motifLockClipboard()) {
            return false;
        }

        // check if motif owns the selection.  unlock motif clipboard
        // if it does not.
        m_motif = motifOwnsClipboard();
        LOG((CLOG_DEBUG1 "motif does %sown clipboard", m_motif ? "" : "not "));
        if (!m_motif) {
            motifUnlockClipboard();
        }
    }

    // now open
    m_open = true;
    m_time = time;

    // be sure to flush the cache later if it's dirty
    m_checkCache = true;

    return true;
}

void
XWindowsClipboard::close() const
{
    assert(m_open);

    LOG((CLOG_DEBUG "close clipboard %d", m_id));

    // unlock clipboard
    if (m_motif) {
        motifUnlockClipboard();
    }

    m_motif = false;
    m_open  = false;
}

IClipboard::Time
XWindowsClipboard::getTime() const
{
    checkCache();
    return m_timeOwned;
}

bool
XWindowsClipboard::has(EFormat format) const
{
    assert(m_open);

    fillCache();
    return m_added[format];
}

String
XWindowsClipboard::get(EFormat format) const
{
    assert(m_open);

    fillCache();
    return m_data[format];
}

void
XWindowsClipboard::clearConverters()
{
    for (ConverterList::iterator index = m_converters.begin();
                                index != m_converters.end(); ++index) {
        delete *index;
    }
    m_converters.clear();
}

IXWindowsClipboardConverter*
XWindowsClipboard::getConverter(Atom target, bool onlyIfNotAdded) const
{
    IXWindowsClipboardConverter* converter = NULL;
    for (ConverterList::const_iterator index = m_converters.begin();
                                index != m_converters.end(); ++index) {
        converter = *index;
        if (converter->getAtom() == target) {
            break;
        }
    }
    if (converter == NULL) {
        LOG((CLOG_DEBUG1 "  no converter for target %s", XWindowsUtil::atomToString(m_display, target).c_str()));
        return NULL;
    }

    // optionally skip already handled targets
    if (onlyIfNotAdded) {
        if (m_added[converter->getFormat()]) {
            LOG((CLOG_DEBUG1 "  skipping handled format %d", converter->getFormat()));
            return NULL;
        }
    }

    return converter;
}

void
XWindowsClipboard::checkCache() const
{
    if (!m_checkCache) {
        return;
    }
    m_checkCache = false;

    // get the time the clipboard ownership was taken by the current
    // owner.
    if (m_motif) {
        m_timeOwned = motifGetTime();
    }
    else {
        m_timeOwned = icccmGetTime();
    }

    // if we can't get the time then use the time passed to us
    if (m_timeOwned == 0) {
        m_timeOwned = m_time;
    }

    // if the cache is dirty then flush it
    if (m_timeOwned != m_cacheTime) {
        clearCache();
    }
}

void
XWindowsClipboard::clearCache() const
{
    const_cast<XWindowsClipboard*>(this)->doClearCache();
}

void
XWindowsClipboard::doClearCache()
{
    m_checkCache = false;
    m_cached     = false;
    for (SInt32 index = 0; index < kNumFormats; ++index) {
        m_data[index]  = "";
        m_added[index] = false;
    }
}

void
XWindowsClipboard::fillCache() const
{
    // get the selection data if not already cached
    checkCache();
    if (!m_cached) {
        const_cast<XWindowsClipboard*>(this)->doFillCache();
    }
}

void
XWindowsClipboard::doFillCache()
{
    if (m_motif) {
        motifFillCache();
    }
    else {
        icccmFillCache();
    }
    m_checkCache = false;
    m_cached     = true;
    m_cacheTime  = m_timeOwned;
}

void
XWindowsClipboard::icccmFillCache()
{
    LOG((CLOG_DEBUG "ICCCM fill clipboard %d", m_id));

    // see if we can get the list of available formats from the selection.
    // if not then use a default list of formats.  note that some clipboard
    // owners are broken and report TARGETS as the type of the TARGETS data
    // instead of the correct type ATOM;  allow either.
    const Atom atomTargets = m_atomTargets;
    Atom target;
    String data;
    if (!icccmGetSelection(atomTargets, &target, &data) ||
        (target != m_atomAtom && target != m_atomTargets)) {
        LOG((CLOG_DEBUG1 "selection doesn't support TARGETS"));
        data = "";
        XWindowsUtil::appendAtomData(data, XA_STRING);
    }

    XWindowsUtil::convertAtomProperty(data);
    const Atom* targets = reinterpret_cast<const Atom*>(data.data()); // TODO: Safe?
    const UInt32 numTargets = data.size() / sizeof(Atom);
    LOG((CLOG_DEBUG "  available targets: %s", XWindowsUtil::atomsToString(m_display, targets, numTargets).c_str()));

    // try each converter in order (because they're in order of
    // preference).
    for (ConverterList::const_iterator index = m_converters.begin();
                                index != m_converters.end(); ++index) {
        IXWindowsClipboardConverter* converter = *index;

        // skip already handled targets
        if (m_added[converter->getFormat()]) {
            continue;
        }

        // see if atom is in target list
        Atom target = None;
        // XXX -- just ask for the converter's target to see if it's
        // available rather than checking TARGETS.  i've seen clipboard
        // owners that don't report all the targets they support.
        target = converter->getAtom();
        /*
        for (UInt32 i = 0; i < numTargets; ++i) {
            if (converter->getAtom() == targets[i]) {
                target = targets[i];
                break;
            }
        }
        */
        if (target == None) {
            continue;
        }

        // get the data
        Atom actualTarget;
        String targetData;
        if (!icccmGetSelection(target, &actualTarget, &targetData)) {
            LOG((CLOG_DEBUG1 "  no data for target %s", XWindowsUtil::atomToString(m_display, target).c_str()));
            continue;
        }

        // add to clipboard and note we've done it
        IClipboard::EFormat format = converter->getFormat();
        m_data[format]  = converter->toIClipboard(targetData);
        m_added[format] = true;
        LOG((CLOG_DEBUG "added format %d for target %s (%u %s)", format, XWindowsUtil::atomToString(m_display, target).c_str(), targetData.size(), targetData.size() == 1 ? "byte" : "bytes"));
    }
}

bool
XWindowsClipboard::icccmGetSelection(Atom target,
                Atom* actualTarget, String* data) const
{
    assert(actualTarget != NULL);
    assert(data         != NULL);

    // request data conversion
    CICCCMGetClipboard getter(m_window, m_time, m_atomData);
    if (!getter.readClipboard(m_display, m_selection,
                                target, actualTarget, data)) {
        LOG((CLOG_DEBUG1 "can't get data for selection target %s", XWindowsUtil::atomToString(m_display, target).c_str()));
        LOGC(getter.m_error, (CLOG_WARN "ICCCM violation by clipboard owner"));
        return false;
    }
    else if (*actualTarget == None) {
        LOG((CLOG_DEBUG1 "selection conversion failed for target %s", XWindowsUtil::atomToString(m_display, target).c_str()));
        return false;
    }
    return true;
}

IClipboard::Time
XWindowsClipboard::icccmGetTime() const
{
    Atom actualTarget;
    String data;
    if (icccmGetSelection(m_atomTimestamp, &actualTarget, &data) &&
        actualTarget == m_atomInteger) {
        Time time = *reinterpret_cast<const Time*>(data.data());
        LOG((CLOG_DEBUG1 "got ICCCM time %d", time));
        return time;
    }
    else {
        // no timestamp
        LOG((CLOG_DEBUG1 "can't get ICCCM time"));
        return 0;
    }
}

bool
XWindowsClipboard::motifLockClipboard() const
{
    // fail if anybody owns the lock (even us, so this is non-recursive)
    Window lockOwner = m_impl->XGetSelectionOwner(m_display, m_atomMotifClipLock);
    if (lockOwner != None) {
        LOG((CLOG_DEBUG1 "motif lock owner 0x%08x", lockOwner));
        return false;
    }

    // try to grab the lock
    // FIXME -- is this right?  there's a race condition here --
    // A grabs successfully, B grabs successfully, A thinks it
    // still has the grab until it gets a SelectionClear.
    Time time = XWindowsUtil::getCurrentTime(m_display, m_window);
    m_impl->XSetSelectionOwner(m_display, m_atomMotifClipLock, m_window, time);
    lockOwner = m_impl->XGetSelectionOwner(m_display, m_atomMotifClipLock);
    if (lockOwner != m_window) {
        LOG((CLOG_DEBUG1 "motif lock owner 0x%08x", lockOwner));
        return false;
    }

    LOG((CLOG_DEBUG1 "locked motif clipboard"));
    return true;
}

void
XWindowsClipboard::motifUnlockClipboard() const
{
    LOG((CLOG_DEBUG1 "unlocked motif clipboard"));

    // fail if we don't own the lock
    Window lockOwner = m_impl->XGetSelectionOwner(m_display, m_atomMotifClipLock);
    if (lockOwner != m_window) {
        return;
    }

    // release lock
    Time time = XWindowsUtil::getCurrentTime(m_display, m_window);
    m_impl->XSetSelectionOwner(m_display, m_atomMotifClipLock, None, time);
}

bool
XWindowsClipboard::motifOwnsClipboard() const
{
    // get the current selection owner
    // FIXME -- this can't be right.  even if the window is destroyed
    // Motif will still have a valid clipboard.  how can we tell if
    // some other client owns CLIPBOARD?
    Window owner = m_impl->XGetSelectionOwner(m_display, m_selection);
    if (owner == None) {
        return false;
    }

    // get the Motif clipboard header property from the root window
    Atom target;
    SInt32 format;
    String data;
    Window root = RootWindow(m_display, DefaultScreen(m_display));
    if (!XWindowsUtil::getWindowProperty(m_display, root,
                                m_atomMotifClipHeader,
                                &data, &target, &format, False)) {
        return false;
    }

    // check the owner window against the current clipboard owner
    if (data.size() >= sizeof(MotifClipHeader)) {
        MotifClipHeader header;
        std::memcpy (&header, data.data(), sizeof(header));
        if ((header.m_id == kMotifClipHeader) &&
            (static_cast<Window>(header.m_selectionOwner) == owner)) {
            return true;
        }
    }

    return false;
}

void
XWindowsClipboard::motifFillCache()
{
    LOG((CLOG_DEBUG "Motif fill clipboard %d", m_id));

    // get the Motif clipboard header property from the root window
    Atom target;
    SInt32 format;
    String data;
    Window root = RootWindow(m_display, DefaultScreen(m_display));
    if (!XWindowsUtil::getWindowProperty(m_display, root,
                                m_atomMotifClipHeader,
                                &data, &target, &format, False)) {
        return;
    }

    MotifClipHeader header;
    if (data.size() < sizeof(header)) { // check that the header is okay
        return;
    }
    std::memcpy (&header, data.data(), sizeof(header));
    if (header.m_id != kMotifClipHeader || header.m_numItems < 1) {
        return;
    }

    // get the Motif item property from the root window
    char name[18 + 20];
    sprintf(name, "_MOTIF_CLIP_ITEM_%d", header.m_item);
    Atom atomItem = m_impl->XInternAtom(m_display, name, False);
    data = "";
    if (!XWindowsUtil::getWindowProperty(m_display, root,
                                atomItem, &data,
                                &target, &format, False)) {
        return;
    }

    MotifClipItem item;
    if (data.size() < sizeof(item)) { // check that the item is okay
        return;
    }
    std::memcpy (&item, data.data(), sizeof(item));
    if (item.m_id != kMotifClipItem ||
        item.m_numFormats - item.m_numDeletedFormats < 1) {
        return;
    }

    // format list is after static item structure elements
    const SInt32 numFormats = item.m_numFormats - item.m_numDeletedFormats;
    const SInt32* formats   = reinterpret_cast<const SInt32*>(item.m_size +
                                static_cast<const char*>(data.data()));

    // get the available formats
    typedef std::map<Atom, String> MotifFormatMap;
    MotifFormatMap motifFormats;
    for (SInt32 i = 0; i < numFormats; ++i) {
        // get Motif format property from the root window
        sprintf(name, "_MOTIF_CLIP_ITEM_%d", formats[i]);
        Atom atomFormat = m_impl->XInternAtom(m_display, name, False);
        String data;
        if (!XWindowsUtil::getWindowProperty(m_display, root,
                                    atomFormat, &data,
                                    &target, &format, False)) {
            continue;
        }

        // check that the format is okay
        MotifClipFormat motifFormat;
        if (data.size() < sizeof(motifFormat)) {
            continue;
        }
        std::memcpy (&motifFormat, data.data(), sizeof(motifFormat));
        if (motifFormat.m_id != kMotifClipFormat ||
            motifFormat.m_length < 0 ||
            motifFormat.m_type == None ||
            motifFormat.m_deleted != 0) {
            continue;
        }

        // save it
        motifFormats.insert(std::make_pair(motifFormat.m_type, data));
    }
    //const UInt32 numMotifFormats = motifFormats.size();

    // try each converter in order (because they're in order of
    // preference).
    for (ConverterList::const_iterator index = m_converters.begin();
                                index != m_converters.end(); ++index) {
        IXWindowsClipboardConverter* converter = *index;

        // skip already handled targets
        if (m_added[converter->getFormat()]) {
            continue;
        }

        // see if atom is in target list
        MotifFormatMap::const_iterator index2 =
                                motifFormats.find(converter->getAtom());
        if (index2 == motifFormats.end()) {
            continue;
        }

        // get format
        MotifClipFormat motifFormat;
        std::memcpy (&motifFormat, index2->second.data(), sizeof(motifFormat));
        const Atom target = motifFormat.m_type;

        // get the data (finally)
        Atom actualTarget;
        String targetData;
        if (!motifGetSelection(&motifFormat, &actualTarget, &targetData)) {
            LOG((CLOG_DEBUG1 "  no data for target %s", XWindowsUtil::atomToString(m_display, target).c_str()));
            continue;
        }

        // add to clipboard and note we've done it
        IClipboard::EFormat format = converter->getFormat();
        m_data[format]  = converter->toIClipboard(targetData);
        m_added[format] = true;
        LOG((CLOG_DEBUG "added format %d for target %s", format, XWindowsUtil::atomToString(m_display, target).c_str()));
    }
}

bool
XWindowsClipboard::motifGetSelection(const MotifClipFormat* format,
                            Atom* actualTarget, String* data) const
{
    // if the current clipboard owner and the owner indicated by the
    // motif clip header are the same then transfer via a property on
    // the root window, otherwise transfer as a normal ICCCM client.
    if (!motifOwnsClipboard()) {
        return icccmGetSelection(format->m_type, actualTarget, data);
    }

    // use motif way
    // FIXME -- this isn't right.  it'll only work if the data is
    // already stored on the root window and only if it fits in a
    // property.  motif has some scheme for transferring part by
    // part that i don't know.
    char name[18 + 20];
    sprintf(name, "_MOTIF_CLIP_ITEM_%d", format->m_data);
       Atom target = m_impl->XInternAtom(m_display, name, False);
    Window root = RootWindow(m_display, DefaultScreen(m_display));
    return XWindowsUtil::getWindowProperty(m_display, root,
                                target, data,
                                actualTarget, NULL, False);
}

IClipboard::Time
XWindowsClipboard::motifGetTime() const
{
    return icccmGetTime();
}

bool
XWindowsClipboard::insertMultipleReply(Window requestor,
                ::Time time, Atom property)
{
    // get the requested targets
    Atom target;
    SInt32 format;
    String data;
    if (!XWindowsUtil::getWindowProperty(m_display, requestor,
                                property, &data, &target, &format, False)) {
        // can't get the requested targets
        return false;
    }

    // fail if the requested targets isn't of the correct form
    if (format != 32 || target != m_atomAtomPair) {
        return false;
    }

    // data is a list of atom pairs:  target, property
    XWindowsUtil::convertAtomProperty(data);
    const Atom* targets = reinterpret_cast<const Atom*>(data.data());
    const UInt32 numTargets = data.size() / sizeof(Atom);

    // add replies for each target
    bool changed = false;
    for (UInt32 i = 0; i < numTargets; i += 2) {
        const Atom target   = targets[i + 0];
        const Atom property = targets[i + 1];
        if (!addSimpleRequest(requestor, target, time, property)) {
            // note that we can't perform the requested conversion
            XWindowsUtil::replaceAtomData(data, i, None);
            changed = true;
        }
    }

    // update the targets property if we changed it
    if (changed) {
        XWindowsUtil::setWindowProperty(m_display, requestor,
                                property, data.data(), data.size(),
                                target, format);
    }

    // add reply for MULTIPLE request
    insertReply(new Reply(requestor, m_atomMultiple,
                                time, property, String(), None, 32));

    return true;
}

void
XWindowsClipboard::insertReply(Reply* reply)
{
    assert(reply != NULL);

    // note -- we must respond to requests in order if requestor,target,time
    // are the same, otherwise we can use whatever order we like with one
    // exception:  each reply in a MULTIPLE reply must be handled in order
    // as well.  those replies will almost certainly not share targets so
    // we can't simply use requestor,target,time as map index.
    //
    // instead we'll use just the requestor.  that's more restrictive than
    // necessary but we're guaranteed to do things in the right order.
    // note that we could also include the time in the map index and still
    // ensure the right order.  but since that'll just make it harder to
    // find the right reply when handling property notify events we stick
    // to just the requestor.

    const bool newWindow = (m_replies.count(reply->m_requestor) == 0);
    m_replies[reply->m_requestor].push_back(reply);

    // adjust requestor's event mask if we haven't done so already.  we
    // want events in case the window is destroyed or any of its
    // properties change.
    if (newWindow) {
        // note errors while we adjust event masks
        bool error = false;
        {
            XWindowsUtil::ErrorLock lock(m_display, &error);

            // get and save the current event mask
            XWindowAttributes attr;
            m_impl->XGetWindowAttributes(m_display, reply->m_requestor, &attr);
            m_eventMasks[reply->m_requestor] = attr.your_event_mask;

            // add the events we want
            m_impl->XSelectInput(m_display, reply->m_requestor, attr.your_event_mask |
                                    StructureNotifyMask | PropertyChangeMask);
        }

        // if we failed then the window has already been destroyed
        if (error) {
            m_replies.erase(reply->m_requestor);
            delete reply;
        }
    }
}

void
XWindowsClipboard::pushReplies()
{
    // send the first reply for each window if that reply hasn't
    // been sent yet.
    for (ReplyMap::iterator index = m_replies.begin();
                                index != m_replies.end(); ) {
        assert(!index->second.empty());
        ReplyList::iterator listit = index->second.begin();
        while (listit != index->second.end()) {
            if (!(*listit)->m_replied)
                break;
            ++listit;
        }
        if (listit != index->second.end() && !(*listit)->m_replied) {
            pushReplies(index, index->second, listit);
        }
        else {
            ++index;
        }
    }
}

void
XWindowsClipboard::pushReplies(ReplyMap::iterator& mapIndex,
                ReplyList& replies, ReplyList::iterator index)
{
    Reply* reply = *index;
    while (sendReply(reply)) {
        // reply is complete.  discard it and send the next reply,
        // if any.
        index = replies.erase(index);
        delete reply;
        if (index == replies.end()) {
            break;
        }
        reply = *index;
    }

    // if there are no more replies in the list then remove the list
    // and stop watching the requestor for events.
    if (replies.empty()) {
        XWindowsUtil::ErrorLock lock(m_display);
        Window requestor = mapIndex->first;
        m_impl->XSelectInput(m_display, requestor, m_eventMasks[requestor]);
        m_replies.erase(mapIndex++);
        m_eventMasks.erase(requestor);
    }
    else {
        ++mapIndex;
    }
}

bool
XWindowsClipboard::sendReply(Reply* reply)
{
    assert(reply != NULL);

    // bail out immediately if reply is done
    if (reply->m_done) {
        LOG((CLOG_DEBUG1 "clipboard: finished reply to 0x%08x,%d,%d", reply->m_requestor, reply->m_target, reply->m_property));
        return true;
    }

    // start in failed state if property is None
    bool failed = (reply->m_property == None);
    if (!failed) {
        LOG((CLOG_DEBUG1 "clipboard: setting property on 0x%08x,%d,%d", reply->m_requestor, reply->m_target, reply->m_property));

        // send using INCR if already sending incrementally or if reply
        // is too large, otherwise just send it.
        const UInt32 maxRequestSize = 3 * XMaxRequestSize(m_display);
        const bool useINCR = (reply->m_data.size() > maxRequestSize);

        // send INCR reply if incremental and we haven't replied yet
        if (useINCR && !reply->m_replied) {
            UInt32 size = reply->m_data.size();
            if (!XWindowsUtil::setWindowProperty(m_display,
                                reply->m_requestor, reply->m_property,
                                &size, 4, m_atomINCR, 32)) {
                failed = true;
            }
        }

        // send more INCR reply or entire non-incremental reply
        else {
            // how much more data should we send?
            UInt32 size = reply->m_data.size() - reply->m_ptr;
            if (size > maxRequestSize)
                size = maxRequestSize;

            // send it
            if (!XWindowsUtil::setWindowProperty(m_display,
                                reply->m_requestor, reply->m_property,
                                reply->m_data.data() + reply->m_ptr,
                                size,
                                reply->m_type, reply->m_format)) {
                failed = true;
            }
            else {
                reply->m_ptr += size;

                // we've finished the reply if we just sent the zero
                // size incremental chunk or if we're not incremental.
                reply->m_done = (size == 0 || !useINCR);
            }
        }
    }

    // if we've failed then delete the property and say we're done.
    // if we haven't replied yet then we can send a failure notify,
    // otherwise we've failed in the middle of an incremental
    // transfer;  i don't know how to cancel that so i'll just send
    // the final zero-length property.
    // FIXME -- how do you gracefully cancel an incremental transfer?
    if (failed) {
        LOG((CLOG_DEBUG1 "clipboard: sending failure to 0x%08x,%d,%d", reply->m_requestor, reply->m_target, reply->m_property));
        reply->m_done = true;
        if (reply->m_property != None) {
            XWindowsUtil::ErrorLock lock(m_display);
            m_impl->XDeleteProperty(m_display, reply->m_requestor, reply->m_property);
        }

        if (!reply->m_replied) {
            sendNotify(reply->m_requestor, m_selection,
                                reply->m_target, None,
                                reply->m_time);

            // don't wait for any reply (because we're not expecting one)
            return true;
        }
        else {
            static const char dummy = 0;
            XWindowsUtil::setWindowProperty(m_display,
                                reply->m_requestor, reply->m_property,
                                &dummy,
                                0,
                                reply->m_type, reply->m_format);

            // wait for delete notify
            return false;
        }
    }

    // send notification if we haven't yet
    if (!reply->m_replied) {
        LOG((CLOG_DEBUG1 "clipboard: sending notify to 0x%08x,%d,%d", reply->m_requestor, reply->m_target, reply->m_property));
        reply->m_replied = true;

        // dump every property on the requestor window to the debug2
        // log.  we've seen what appears to be a bug in lesstif and
        // knowing the properties may help design a workaround, if
        // it becomes necessary.
        if (CLOG->getFilter() >= kDEBUG2) {
            XWindowsUtil::ErrorLock lock(m_display);
            int n;
            Atom* props = m_impl->XListProperties(m_display, reply->m_requestor,
                                                  &n);
            LOG((CLOG_DEBUG2 "properties of 0x%08x:", reply->m_requestor));
            for (int i = 0; i < n; ++i) {
                Atom target;
                String data;
                char* name = m_impl->XGetAtomName(m_display, props[i]);
                if (!XWindowsUtil::getWindowProperty(m_display,
                                reply->m_requestor,
                                props[i], &data, &target, NULL, False)) {
                    LOG((CLOG_DEBUG2 "  %s: <can't read property>", name));
                }
                else {
                    // if there are any non-ascii characters in string
                    // then print the binary data.
                    static const char* hex = "0123456789abcdef";
                    for (String::size_type j = 0; j < data.size(); ++j) {
                        if (data[j] < 32 || data[j] > 126) {
                            String tmp;
                            tmp.reserve(data.size() * 3);
                            for (j = 0; j < data.size(); ++j) {
                                unsigned char v = (unsigned char)data[j];
                                tmp += hex[v >> 16];
                                tmp += hex[v & 15];
                                tmp += ' ';
                            }
                            data = tmp;
                            break;
                        }
                    }
                    char* type = m_impl->XGetAtomName(m_display, target);
                    LOG((CLOG_DEBUG2 "  %s (%s): %s", name, type, data.c_str()));
                    if (type != NULL) {
                        m_impl->XFree(type);
                    }
                }
                if (name != NULL) {
                    m_impl->XFree(name);
                }
            }
            if (props != NULL) {
                m_impl->XFree(props);
            }
        }

        sendNotify(reply->m_requestor, m_selection,
                                reply->m_target, reply->m_property,
                                reply->m_time);
    }

    // wait for delete notify
    return false;
}

void
XWindowsClipboard::clearReplies()
{
    for (ReplyMap::iterator index = m_replies.begin();
                                index != m_replies.end(); ++index) {
        clearReplies(index->second);
    }
    m_replies.clear();
    m_eventMasks.clear();
}

void
XWindowsClipboard::clearReplies(ReplyList& replies)
{
    for (ReplyList::iterator index = replies.begin();
                                index != replies.end(); ++index) {
        delete *index;
    }
    replies.clear();
}

void
XWindowsClipboard::sendNotify(Window requestor,
                Atom selection, Atom target, Atom property, Time time)
{
    XEvent event;
    event.xselection.type      = SelectionNotify;
    event.xselection.display   = m_display;
    event.xselection.requestor = requestor;
    event.xselection.selection = selection;
    event.xselection.target    = target;
    event.xselection.property  = property;
    event.xselection.time      = time;
    XWindowsUtil::ErrorLock lock(m_display);
    m_impl->XSendEvent(m_display, requestor, False, 0, &event);
}

bool
XWindowsClipboard::wasOwnedAtTime(::Time time) const
{
    // not owned if we've never owned the selection
    checkCache();
    if (m_timeOwned == 0) {
        return false;
    }

    // if time is CurrentTime then return true if we still own the
    // selection and false if we do not.  else if we still own the
    // selection then get the current time, otherwise use
    // m_timeLost as the end time.
    Time lost = m_timeLost;
    if (m_timeLost == 0) {
        if (time == CurrentTime) {
            return true;
        }
        else {
            lost = XWindowsUtil::getCurrentTime(m_display, m_window);
        }
    }
    else {
        if (time == CurrentTime) {
            return false;
        }
    }

    // compare time to range
    Time duration = lost - m_timeOwned;
    Time when     = time - m_timeOwned;
    return (/*when >= 0 &&*/ when <= duration);
}

Atom
XWindowsClipboard::getTargetsData(String& data, int* format) const
{
    assert(format != NULL);

    // add standard targets
    XWindowsUtil::appendAtomData(data, m_atomTargets);
    XWindowsUtil::appendAtomData(data, m_atomMultiple);
    XWindowsUtil::appendAtomData(data, m_atomTimestamp);

    // add targets we can convert to
    for (ConverterList::const_iterator index = m_converters.begin();
                                index != m_converters.end(); ++index) {
        IXWindowsClipboardConverter* converter = *index;

        // skip formats we don't have
        if (m_added[converter->getFormat()]) {
            XWindowsUtil::appendAtomData(data, converter->getAtom());
        }
    }

    *format = 32;
    return m_atomAtom;
}

Atom
XWindowsClipboard::getTimestampData(String& data, int* format) const
{
    assert(format != NULL);

    checkCache();
    XWindowsUtil::appendTimeData(data, m_timeOwned);
    *format = 32;
    return m_atomInteger;
}


//
// XWindowsClipboard::CICCCMGetClipboard
//

XWindowsClipboard::CICCCMGetClipboard::CICCCMGetClipboard(
                Window requestor, Time time, Atom property) :
    m_requestor(requestor),
    m_time(time),
    m_property(property),
    m_incr(false),
    m_failed(false),
    m_done(false),
    m_reading(false),
    m_data(NULL),
    m_actualTarget(NULL),
    m_error(false)
{
    // do nothing
}

XWindowsClipboard::CICCCMGetClipboard::~CICCCMGetClipboard()
{
    // do nothing
}

bool
XWindowsClipboard::CICCCMGetClipboard::readClipboard(Display* display,
                Atom selection, Atom target, Atom* actualTarget, String* data)
{
    assert(actualTarget != NULL);
    assert(data         != NULL);

    LOG((CLOG_DEBUG1 "request selection=%s, target=%s, window=%x", XWindowsUtil::atomToString(display, selection).c_str(), XWindowsUtil::atomToString(display, target).c_str(), m_requestor));

    m_atomNone = XInternAtom(display, "NONE", False);
    m_atomIncr = XInternAtom(display, "INCR", False);

    // save output pointers
    m_actualTarget = actualTarget;
    m_data         = data;

    // assume failure
    *m_actualTarget = None;
    *m_data         = "";

    // delete target property
    XDeleteProperty(display, m_requestor, m_property);

    // select window for property changes
    XWindowAttributes attr;
    XGetWindowAttributes(display, m_requestor, &attr);
    XSelectInput(display, m_requestor,
                                attr.your_event_mask | PropertyChangeMask);

    // request data conversion
    XConvertSelection(display, selection, target,
                                m_property, m_requestor, m_time);

    // synchronize with server before we start following timeout countdown
    XSync(display, False);

    // Xlib inexplicably omits the ability to wait for an event with
    // a timeout.  (it's inexplicable because there's no portable way
    // to do it.)  we'll poll until we have what we're looking for or
    // a timeout expires.  we use a timeout so we don't get locked up
    // by badly behaved selection owners.
    XEvent xevent;
    std::vector<XEvent> events;
    Stopwatch timeout(false);    // timer not stopped, not triggered
    static const double s_timeout = 0.25;    // FIXME -- is this too short?
    bool noWait = false;
    while (!m_done && !m_failed) {
        // fail if timeout has expired
        if (timeout.getTime() >= s_timeout) {
            m_failed = true;
            break;
        }

        // process events if any otherwise sleep
        if (noWait || XPending(display) > 0) {
            while (!m_done && !m_failed && (noWait || XPending(display) > 0)) {
                XNextEvent(display, &xevent);
                if (!processEvent(display, &xevent)) {
                    // not processed so save it
                    events.push_back(xevent);
                }
                else {
                    // reset timer since we've made some progress
                    timeout.reset();

                    // don't sleep anymore, just block waiting for events.
                    // we're assuming here that the clipboard owner will
                    // complete the protocol correctly.  if we continue to
                    // sleep we'll get very bad performance.
                    noWait = true;
                }
            }
        }
        else {
            ARCH->sleep(0.01);
        }
    }

    // put unprocessed events back
    for (UInt32 i = events.size(); i > 0; --i) {
        XPutBackEvent(display, &events[i - 1]);
    }

    // restore mask
    XSelectInput(display, m_requestor, attr.your_event_mask);

    // return success or failure
    LOG((CLOG_DEBUG1 "request %s after %fs", m_failed ? "failed" : "succeeded", timeout.getTime()));
    return !m_failed;
}

bool
XWindowsClipboard::CICCCMGetClipboard::processEvent(
                Display* display, XEvent* xevent)
{
    // process event
    switch (xevent->type) {
    case DestroyNotify:
        if (xevent->xdestroywindow.window == m_requestor) {
            m_failed = true;
            return true;
        }

        // not interested
        return false;

    case SelectionNotify:
        if (xevent->xselection.requestor == m_requestor) {
            // done if we can't convert
            if (xevent->xselection.property == None ||
                xevent->xselection.property == m_atomNone) {
                m_done = true;
                return true;
            }

            // proceed if conversion successful
            else if (xevent->xselection.property == m_property) {
                m_reading = true;
                break;
            }
        }

        // otherwise not interested
        return false;

    case PropertyNotify:
        // proceed if conversion successful and we're receiving more data
        if (xevent->xproperty.window == m_requestor &&
            xevent->xproperty.atom   == m_property &&
            xevent->xproperty.state  == PropertyNewValue) {
            if (!m_reading) {
                // we haven't gotten the SelectionNotify yet
                return true;
            }
            break;
        }

        // otherwise not interested
        return false;

    default:
        // not interested
        return false;
    }

    // get the data from the property
    Atom target;
    const String::size_type oldSize = m_data->size();
    if (!XWindowsUtil::getWindowProperty(display, m_requestor,
                                m_property, m_data, &target, NULL, True)) {
        // unable to read property
        m_failed = true;
        return true;
    }

    // note if incremental.  if we're already incremental then the
    // selection owner is busted.  if the INCR property has no size
    // then the selection owner is busted.
    if (target == m_atomIncr) {
        if (m_incr) {
            m_failed = true;
            m_error  = true;
        }
        else if (m_data->size() == oldSize) {
            m_failed = true;
            m_error  = true;
        }
        else {
            m_incr   = true;

            // discard INCR data
            *m_data = "";
        }
    }

    // handle incremental chunks
    else if (m_incr) {
        // if first incremental chunk then save target
        if (oldSize == 0) {
            LOG((CLOG_DEBUG1 "  INCR first chunk, target %s", XWindowsUtil::atomToString(display, target).c_str()));
            *m_actualTarget = target;
        }

        // secondary chunks must have the same target
        else {
            if (target != *m_actualTarget) {
                LOG((CLOG_WARN "  INCR target mismatch"));
                m_failed = true;
                m_error  = true;
            }
        }

        // note if this is the final chunk
        if (m_data->size() == oldSize) {
            LOG((CLOG_DEBUG1 "  INCR final chunk: %d bytes total", m_data->size()));
            m_done = true;
        }
    }

    // not incremental;  save the target.
    else {
        LOG((CLOG_DEBUG1 "  target %s", XWindowsUtil::atomToString(display, target).c_str()));
        *m_actualTarget = target;
        m_done          = true;
    }

    // this event has been processed
    LOGC(!m_incr, (CLOG_DEBUG1 "  got data, %d bytes", m_data->size()));
    return true;
}


//
// XWindowsClipboard::Reply
//

XWindowsClipboard::Reply::Reply(Window requestor, Atom target, ::Time time) :
    m_requestor(requestor),
    m_target(target),
    m_time(time),
    m_property(None),
    m_replied(false),
    m_done(false),
    m_data(),
    m_type(None),
    m_format(32),
    m_ptr(0)
{
    // do nothing
}

XWindowsClipboard::Reply::Reply(Window requestor, Atom target, ::Time time,
                Atom property, const String& data, Atom type, int format) :
    m_requestor(requestor),
    m_target(target),
    m_time(time),
    m_property(property),
    m_replied(false),
    m_done(false),
    m_data(data),
    m_type(type),
    m_format(format),
    m_ptr(0)
{
    // do nothing
}
