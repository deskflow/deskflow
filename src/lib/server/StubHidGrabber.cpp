/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2026 Deskflow Developers
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#include "server/HidPassthrough.h"

#include "base/Log.h"

namespace deskflow::server {

namespace {

//! Honest placeholder: pass-through is macOS-only so far.
/*!
Windows needs an exclusive CreateFile on the vendor HID collection and
Linux an EVIOCGRAB/hidraw grab (docs/hid-passthrough.md); both fail-safe
the same way (handles release on process exit). Until implemented, the
feature reports failure instead of pretending.
*/
class StubHidGrabber : public IHidGrabber
{
public:
  bool start(std::vector<HidDeviceSelector>, AttachCallback, FrameCallback, DetachCallback) override
  {
    LOG_WARN("hid passthrough is not implemented on this platform yet");
    return false;
  }
  void setSeized(bool) override
  {
    // do nothing
  }
  void stop() override
  {
    // do nothing
  }
};

} // namespace

std::unique_ptr<IHidGrabber> createHidGrabber()
{
  return std::make_unique<StubHidGrabber>();
}

} // namespace deskflow::server
