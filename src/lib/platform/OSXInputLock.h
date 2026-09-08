/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2026 Deskflow Developers
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#pragma once

#include <ApplicationServices/ApplicationServices.h>
#include <cstdint>
#include <functional>
#include <memory>

class IEventQueue;

// Uses the primary screen's existing HID tap. All mutable state and AppKit
// resources belong to the main run loop; server acknowledgements are marshalled
// onto it. No second tap or input-grabbing process is installed.
class OSXInputLock
{
public:
  enum class FilterResult
  {
    Pass,
    Suppress,
    LocalOnly
  };

  OSXInputLock(IEventQueue *events, void *target, std::function<void(bool)> restoreState);
  ~OSXInputLock();

  void start(CFMachPortRef tap);
  void stop();
  void request(unsigned autoReleaseSeconds);
  void confirm(uint64_t generation, bool success);
  void resume(uint64_t generation);
  void cancel();
  bool blocksForwarding() const;
  FilterResult filter(CGEventType type, CGEventRef event, bool onScreen);

private:
  struct Impl;
  std::shared_ptr<Impl> m_impl;
};
