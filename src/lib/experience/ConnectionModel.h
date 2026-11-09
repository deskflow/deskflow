// SPDX-FileCopyrightText: (C) 2026 Deskflow Developers
// SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
#pragma once

#include <array>
#include <cstdint>
#include <optional>
#include <string>
#include <vector>

namespace deskflow::experience {
using Millis = std::int64_t; // Monotonic time supplied by the adapter, never wall-clock time.
using Pin = std::array<std::uint8_t, 32>;

struct Endpoint
{
  std::string host;
  std::uint16_t port = 24800;
  std::string interface;
  bool operator==(const Endpoint &) const = default;
};

// Discovery is deliberately unable to supply or modify a certificate pin.
struct Candidate
{
  std::string serviceKey;
  Endpoint endpoint;
  Millis expiresAt = 0;
};

struct TrustedProfile
{
  std::string localId;
  std::string serviceKey; // Unauthenticated rendezvous hint, NOT peer identity.
  Pin expectedPin;
};

enum class ConnectionState
{
  Idle,
  NeedsPairing,
  Discovering,
  Connecting,
  Connected,
  RetryWaiting,
  Paused,
  Offline,
  IdentityBlocked,
  PermissionBlocked
};
enum class Failure
{
  TcpTimeout,
  PeerClosed,
  ResolveFailed,
  IdentityChanged,
  PermissionDenied
};

struct ConnectRequest
{
  std::uint64_t generation;
  Endpoint endpoint;
  Pin expectedPin;
  bool requireTls = true;
};

// This evidence may ONLY be created by a TLS adapter after completing its peer checks.
// Neither a DNS-SD TXT record nor a UI checkbox is TLS evidence.
struct TlsEvidence
{
  bool peerVerified;
  Pin peerPin;
};

// Pure state machine: no sockets, timers, persistent trust writes, or input injection.
// The owner must close/cancel its transport whenever generation() changes. Late results
// are independently rejected here. Untrusted devices require a separate pairing flow.
class ConnectionModel
{
public:
  ConnectionModel(
      std::optional<TrustedProfile> profile, std::vector<std::string> allowedInterfaces, std::uint32_t jitterSeed = 1
  );
  bool observe(const std::string &serviceKey, const Endpoint &endpoint, Millis ttl, Millis now);
  void requestConnect(Millis now);
  void pause();
  void setNetworkAvailable(bool available, Millis now);
  void wake(Millis now);
  std::optional<ConnectRequest> poll(Millis now);
  bool handshake(std::uint64_t generation, const TlsEvidence &evidence);
  bool fail(std::uint64_t generation, Failure reason, Millis now);
  bool recordRtt(std::uint64_t generation, Millis rtt, Millis now);
  std::optional<Millis> rtt(Millis now) const;
  ConnectionState state() const
  {
    return m_state;
  }
  bool allowsInput() const
  {
    return m_state == ConnectionState::Connected;
  }
  std::optional<Millis> retryAt() const
  {
    return m_retryAt;
  }
  std::uint64_t generation() const
  {
    return m_generation;
  }
  std::size_t candidateCount() const
  {
    return m_candidates.size();
  }

private:
  bool blocked() const;
  void invalidate();
  void schedule(Millis now, Millis delay);
  std::optional<TrustedProfile> m_profile;
  std::vector<std::string> m_allowedInterfaces;
  std::vector<Candidate> m_candidates;
  ConnectionState m_state = ConnectionState::Idle;
  bool m_wanted = false;
  bool m_networkAvailable = true;
  std::uint64_t m_generation = 0;
  unsigned m_failures = 0;
  std::uint32_t m_rng;
  std::optional<Millis> m_retryAt;
  std::optional<Millis> m_rtt;
  Millis m_rttAt = 0;
};
} // namespace deskflow::experience
