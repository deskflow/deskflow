#ifndef CLIPBOARDTYPES_H
#define CLIPBOARDTYPES_H

#include "BasicTypes.h"

//! Clipboard ID
/*!
Type to hold a clipboard identifier.
*/
typedef UInt8			ClipboardID;

//! @name Clipboard identifiers
//@{
// clipboard identifiers.  kClipboardClipboard is what is normally
// considered the clipboard (e.g. the cut/copy/paste menu items
// affect it).  kClipboardSelection is the selection on those
// platforms that can treat the selection as a clipboard (e.g. X
// windows).  clipboard identifiers must be sequential starting
// at zero.
static const ClipboardID	kClipboardClipboard = 0;
static const ClipboardID	kClipboardSelection = 1;

// the number of clipboards (i.e. one greater than the last clipboard id)
static const ClipboardID	kClipboardEnd       = 2;
//@}

#endif
