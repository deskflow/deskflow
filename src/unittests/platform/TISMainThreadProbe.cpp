/*
 * SPDX-FileCopyrightText: (C) 2026 Deskflow Developers
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 * Read-only native regression: no event tap, input selection or pasteboard use.
 */
#include "platform/OSXTISMainThread.h"
#include <Carbon/Carbon.h>
#include <algorithm>
#include <atomic>
#include <chrono>
#include <cstdio>
#include <cstring>
#include <memory>
#include <mutex>
#include <stdexcept>
#include <thread>
#include <vector>

using Clock = std::chrono::steady_clock;

static void readInputSources()
{
  auto layout = TISCopyCurrentKeyboardLayoutInputSource();
  auto source = TISCopyCurrentKeyboardInputSource();
  if (layout) {
    TISGetInputSourceProperty(layout, kTISPropertyInputSourceID);
    TISGetInputSourceProperty(layout, kTISPropertyUnicodeKeyLayoutData);
    CFRelease(layout);
  }
  if (source) {
    TISGetInputSourceProperty(source, kTISPropertyInputSourceLanguages);
    CFRelease(source);
  }
  auto sources = TISCreateInputSourceList(nullptr, false);
  if (sources) {
    for (CFIndex i = 0; i < CFArrayGetCount(sources); ++i)
      TISGetInputSourceProperty((TISInputSourceRef)CFArrayGetValueAtIndex(sources, i), kTISPropertyInputSourceID);
    CFRelease(sources);
  }
}

int main(int argc, char **argv)
{
  const bool baseline = argc > 1 && std::strcmp(argv[1], "--baseline-mutex") == 0;
  constexpr int workers = 4, iterations = 2500;
  std::atomic<int> ready{0}, done{0}, calls{0}, wrongThread{0}, caught{0};
  std::atomic<bool> start{false};
  std::mutex legacyMutex;
  std::vector<std::thread> threads;
  std::vector<std::vector<double>> timings(workers);
  withTISOnMainThread([&] {
    withTISOnMainThread([&] {
      if (!pthread_main_np())
        ++wrongThread;
    });
  });
  for (int worker = 0; worker < workers; ++worker) {
    threads.emplace_back([&, worker] {
      ++ready;
      while (!start.load())
        std::this_thread::yield();
      if (!baseline) {
        auto owned = std::make_unique<int>(42);
        withTISOnMainThread([value = std::move(owned), &wrongThread] {
          if (!pthread_main_np() || *value != 42)
            ++wrongThread;
        });
        try {
          withTISOnMainThread([] { throw std::runtime_error("expected"); });
        } catch (const std::runtime_error &) {
          ++caught;
        }
      }
      for (int i = 0; i < iterations; ++i) {
        auto before = Clock::now();
        if (baseline) {
          std::lock_guard lock(legacyMutex);
          readInputSources();
        } else {
          withTISOnMainThread([&] {
            if (!pthread_main_np())
              ++wrongThread;
            readInputSources();
          });
        }
        timings[worker].push_back(std::chrono::duration<double, std::micro>(Clock::now() - before).count());
        ++calls;
      }
      ++done;
    });
  }
  while (ready != workers)
    std::this_thread::yield();
  const auto began = Clock::now();
  start = true;
  int mainReads = 0;
  while (done != workers) {
    // Model AppKit's independent main-thread use, which cannot take our mutex.
    readInputSources();
    ++mainReads;
    CFRunLoopRunInMode(kCFRunLoopDefaultMode, 0.0001, true);
    if (Clock::now() - began > std::chrono::seconds(60)) {
      std::fprintf(stderr, "timeout: main-queue deadlock or unbounded operation\n");
      std::_Exit(2);
    }
  }
  for (auto &thread : threads)
    thread.join();
  std::vector<double> all;
  for (auto &times : timings)
    all.insert(all.end(), times.begin(), times.end());
  std::sort(all.begin(), all.end());
  std::printf(
      "mode=%s worker_operations=%d main_operations=%d wrong_thread=%d exceptions=%d p50_us=%.1f p95_us=%.1f "
      "p99_us=%.1f elapsed_s=%.3f\n",
      baseline ? "legacy-mutex" : "main-thread", calls.load(), mainReads, wrongThread.load(), caught.load(),
      all[all.size() / 2], all[all.size() * 95 / 100], all[all.size() * 99 / 100],
      std::chrono::duration<double>(Clock::now() - began).count()
  );
  return wrongThread == 0 && (baseline || caught == workers) && calls == workers * iterations ? 0 : 1;
}
