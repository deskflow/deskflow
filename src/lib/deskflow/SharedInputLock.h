/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2026 Deskflow Developers
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#pragma once

#include <atomic>
#include <chrono>
#include <cstdint>
#include <optional>

namespace deskflow {

// Owned by the input capture thread. Only blocksForwarding() may be read on
// another thread. No operating system calls, real timers, or input synthesis.
class SharedInputLock
{
public:
  using Clock = std::chrono::steady_clock;
  using Time = Clock::time_point;
  using Duration = Clock::duration;
  using Generation = uint64_t;

  enum class Phase
  {
    Open,
    Preparing,
    Locked,
    Releasing
  };

  enum class Reason
  {
    None,
    Escape,
    AutoRelease,
    CaptureLost,
    RemoteUnavailable,
    PrepareTimeout,
    ReleaseTimeout,
    Stopped
  };

  static constexpr auto escapeHold = std::chrono::seconds(3);
  static constexpr auto prepareTimeout = std::chrono::seconds(2);
  static constexpr auto releaseTimeout = std::chrono::seconds(5);

  bool begin(Time now, bool captureHealthy, Duration autoRelease = Duration::zero())
  {
    if (m_phase != Phase::Open || !captureHealthy || autoRelease < Duration::zero()) {
      return false;
    }
    ++m_generation;
    m_phase = Phase::Preparing;
    m_reason = Reason::None;
    m_started = now;
    m_autoRelease = autoRelease;
    m_localReleased = false;
    m_remoteReleased = false;
    m_resumePending = false;
    m_suppressLocal = true;
    m_escapePressed.reset();
    m_blockForwarding.store(true, std::memory_order_release);
    return true;
  }

  void localReleased(Generation generation, Time now)
  {
    acknowledgePreparation(generation, now, m_localReleased);
  }

  void remoteReleased(Generation generation, Time now)
  {
    // A late receipt may make recovery safe, but must never arm an expired
    // lock. Without it, a timed-out remote may still have a synthetic key down.
    if (generation == m_generation && m_phase == Phase::Releasing) {
      m_remoteReleased = true;
      return;
    }
    acknowledgePreparation(generation, now, m_remoteReleased);
  }

  void escape(bool down, bool repeat, Time now)
  {
    if (!down) {
      m_escapePressed.reset();
    } else if ((m_phase == Phase::Preparing || m_phase == Phase::Locked) && !repeat && !m_escapePressed) {
      m_escapePressed = now;
    }
  }

  void update(Time now, bool captureHealthy, bool escapeStillDown)
  {
    if (m_phase == Phase::Open) {
      return;
    }
    if (!captureHealthy) {
      release(Reason::CaptureLost, now);
      // Once capture fails, continuing to claim local suppression is unsafe.
      m_suppressLocal = false;
    }
    if (!escapeStillDown) {
      m_escapePressed.reset();
    }
    if (m_phase == Phase::Preparing && now - m_started >= prepareTimeout) {
      release(Reason::PrepareTimeout, now);
    }
    if (m_phase == Phase::Locked) {
      if (m_escapePressed && now - *m_escapePressed >= escapeHold) {
        release(Reason::Escape, now);
      } else if (m_autoRelease > Duration::zero() && now - m_started >= m_autoRelease) {
        release(Reason::AutoRelease, now);
      }
    }
    if (m_phase == Phase::Releasing && now - m_releasing >= releaseTimeout) {
      m_reason = Reason::ReleaseTimeout;
      m_suppressLocal = false;
    }
  }

  void release(Reason reason, Time now)
  {
    if (m_phase == Phase::Open) {
      return;
    }
    if (m_phase != Phase::Releasing) {
      m_phase = Phase::Releasing;
      m_releasing = now;
      m_reason = reason;
      m_escapePressed.reset();
    } else if (reason == Reason::CaptureLost || reason == Reason::Stopped) {
      m_reason = reason;
    }
    if (reason == Reason::Stopped || reason == Reason::CaptureLost) {
      m_suppressLocal = false;
    }
  }

  // The adapter posts a FIFO event after capture is blocked. It must not
  // reopen forwarding until the consumer acknowledges that queue boundary.
  bool requestResumeBarrier(bool inputsReleased)
  {
    if (m_phase != Phase::Releasing || m_resumePending || !inputsReleased || !m_remoteReleased) {
      return false;
    }
    m_resumePending = true;
    return true;
  }

  bool acknowledgeResume(Generation generation, bool inputsReleased)
  {
    if (generation != m_generation || m_phase != Phase::Releasing || !m_resumePending) {
      return false;
    }
    m_resumePending = false;
    // A new press while the acknowledgement was in flight must not escape.
    if (!inputsReleased) {
      return false;
    }
    m_phase = Phase::Open;
    m_suppressLocal = false;
    m_blockForwarding.store(false, std::memory_order_release);
    return true;
  }

  bool blocksForwarding() const
  {
    return m_blockForwarding.load(std::memory_order_acquire);
  }

  // Only after the previous capture session and its server consumers have
  // stopped. Invalidates callbacks before a screen is enabled again.
  void resetAfterStop()
  {
    ++m_generation;
    m_phase = Phase::Open;
    m_reason = Reason::Stopped;
    m_resumePending = false;
    m_suppressLocal = false;
    m_escapePressed.reset();
    m_blockForwarding.store(false, std::memory_order_release);
  }

  bool suppressesLocal() const
  {
    return m_suppressLocal;
  }

  bool awaitingRemoteRelease() const
  {
    return !m_remoteReleased;
  }

  Phase phase() const
  {
    return m_phase;
  }

  Reason reason() const
  {
    return m_reason;
  }

  Generation generation() const
  {
    return m_generation;
  }

private:
  void acknowledgePreparation(Generation generation, Time now, bool &part)
  {
    if (generation != m_generation || m_phase != Phase::Preparing) {
      return;
    }
    part = true;
    if (now - m_started >= prepareTimeout) {
      release(Reason::PrepareTimeout, now);
      return;
    }
    if (m_localReleased && m_remoteReleased) {
      m_phase = Phase::Locked;
      // Preserve a fresh press observed during preparation, but require the
      // full hold interval after the lock is armed. A preexisting repeat
      // cannot start this timer.
      if (m_escapePressed) {
        m_escapePressed = now;
      }
    }
  }

  std::atomic<bool> m_blockForwarding{false};
  Phase m_phase = Phase::Open;
  Reason m_reason = Reason::None;
  Generation m_generation = 0;
  Time m_started{};
  Time m_releasing{};
  Duration m_autoRelease{};
  std::optional<Time> m_escapePressed;
  bool m_localReleased = false;
  bool m_remoteReleased = false;
  bool m_suppressLocal = false;
  bool m_resumePending = false;
};

} // namespace deskflow
