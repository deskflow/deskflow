/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2026 Deskflow Developers
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#include <Carbon/Carbon.h>
#include <atomic>
#include <pthread.h>

namespace {
std::atomic<int> s_backgroundCalls{0};
std::atomic<bool> s_missingLayout{false};

TISInputSourceRef checkedCopyKeyboardLayout()
{
  if (!pthread_main_np()) {
    ++s_backgroundCalls;
    return nullptr;
  }
  return s_missingLayout ? nullptr : TISCopyCurrentKeyboardLayoutInputSource();
}

void *checkedGetProperty(TISInputSourceRef source, CFStringRef property)
{
  if (!pthread_main_np()) {
    ++s_backgroundCalls;
    return nullptr;
  }
  return TISGetInputSourceProperty(source, property);
}

CFArrayRef testInputSourceList(CFDictionaryRef, Boolean)
{
  auto source = TISCopyCurrentKeyboardLayoutInputSource();
  if (!source)
    return CFArrayCreate(nullptr, nullptr, 0, &kCFTypeArrayCallBacks);

  // A repeated source maps to group 1, so a fallback to group 0 cannot pass.
  const void *sources[] = {source, source};
  auto groups = CFArrayCreate(nullptr, sources, 2, &kCFTypeArrayCallBacks);
  CFRelease(source);
  return groups;
}

__attribute__((used)) static const struct
{
  const void *replacement;
  const void *original;
} s_inputSourceInterposes[] __attribute__((section("__DATA,__interpose"))) = {
    {reinterpret_cast<const void *>(&checkedCopyKeyboardLayout),
     reinterpret_cast<const void *>(&TISCopyCurrentKeyboardLayoutInputSource)},
    {reinterpret_cast<const void *>(&checkedGetProperty), reinterpret_cast<const void *>(&TISGetInputSourceProperty)},
    {reinterpret_cast<const void *>(&testInputSourceList), reinterpret_cast<const void *>(&TISCreateInputSourceList)}
};
} // namespace

extern "C" int deskflowBackgroundTisCalls()
{
  return s_backgroundCalls.load();
}

extern "C" void deskflowResetTisAudit(bool missingLayout)
{
  s_backgroundCalls = 0;
  s_missingLayout = missingLayout;
}
