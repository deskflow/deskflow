/* Copyright (C) 2002-2005 RealVNC Ltd.  All Rights Reserved.
 * 
 * This is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 * 
 * This software is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this software; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307,
 * USA.
 */

// -=- rfbUpdateTracker.cpp
//
// Tracks updated regions and a region-copy event, too
//

#include <assert.h>

#include <rfb/UpdateTracker.h>
#include <rfb/LogWriter.h>

using namespace rfb;

static LogWriter vlog("UpdateTracker");


// -=- ClippingUpdateTracker

void ClippingUpdateTracker::add_changed(const Region &region) {
  ut->add_changed(region.intersect(clipRect));
}

void ClippingUpdateTracker::add_copied(const Region &dest, const Point &delta) {
  // Clip the destination to the display area
  Region clipdest = dest.intersect(clipRect);
  if (clipdest.is_empty())  return;

  // Clip the source to the screen
  Region tmp = clipdest;
  tmp.translate(delta.negate());
  tmp.assign_intersect(clipRect);
  if (!tmp.is_empty()) {
    // Translate the source back to a destination region
    tmp.translate(delta);

    // Pass the copy region to the child tracker
    ut->add_copied(tmp, delta);
  }

  // And add any bits that we had to remove to the changed region
  tmp = clipdest.subtract(tmp);
  if (!tmp.is_empty())
    ut->add_changed(tmp);
}

// SimpleUpdateTracker

SimpleUpdateTracker::SimpleUpdateTracker(bool use_copyrect) {
  copy_enabled = use_copyrect;
}

SimpleUpdateTracker::~SimpleUpdateTracker() {
}

void SimpleUpdateTracker::enable_copyrect(bool enable) {
  if (!enable && copy_enabled) {
    add_changed(copied);
    copied.clear();
  }
  copy_enabled=enable;
}

void SimpleUpdateTracker::add_changed(const Region &region) {
  changed.assign_union(region);
}

void SimpleUpdateTracker::add_copied(const Region &dest, const Point &delta) {
  // Do we support copyrect?
  if (!copy_enabled) {
    add_changed(dest);
    return;
  }

  // Is there anything to do?
  if (dest.is_empty()) return;

  // Calculate whether any of this copy can be treated as a continuation
  // of an earlier one
  Region src = dest;
  src.translate(delta.negate());
  Region overlap = src.intersect(copied);

  if (overlap.is_empty()) {
    // There is no overlap

    Rect newbr = dest.get_bounding_rect();
    Rect oldbr = copied.get_bounding_rect();
    if (oldbr.area() > newbr.area()) {
      // Old copyrect is (probably) bigger - use it
      changed.assign_union(dest);
    } else {
      // New copyrect is probably bigger
      // Use the new one
      // But be careful not to copy stuff that still needs
      // to be updated.
      Region invalid_src = src.intersect(changed);
      invalid_src.translate(delta);
      changed.assign_union(invalid_src);
      changed.assign_union(copied);
      copied = dest;
      copy_delta = delta;
    }
    return;
  }

  Region invalid_src = overlap.intersect(changed);
  invalid_src.translate(delta);
  changed.assign_union(invalid_src);
  
  overlap.translate(delta);

  Region nonoverlapped_copied = dest.union_(copied).subtract(overlap);
  changed.assign_union(nonoverlapped_copied);

  copied = overlap;
  copy_delta = copy_delta.translate(delta);

  return;
}

void SimpleUpdateTracker::subtract(const Region& region) {
  copied.assign_subtract(region);
  changed.assign_subtract(region);
}

void SimpleUpdateTracker::getUpdateInfo(UpdateInfo* info, const Region& clip)
{
  copied.assign_subtract(changed);
  info->changed = changed.intersect(clip);
  info->copied = copied.intersect(clip);
  info->copy_delta = copy_delta;
}

void SimpleUpdateTracker::copyTo(UpdateTracker* to) const {
  if (!copied.is_empty())
    to->add_copied(copied, copy_delta);
  if (!changed.is_empty())
    to->add_changed(changed);
}
