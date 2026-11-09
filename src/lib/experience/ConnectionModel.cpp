// SPDX-FileCopyrightText: (C) 2026 Deskflow Developers
// SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
#include "ConnectionModel.h"

#include <algorithm>
#include <limits>
#include <stdexcept>

namespace deskflow::experience {
namespace {
Millis addTime(Millis now, Millis delay)
{
  if (now < 0 || delay < 0)
    throw std::invalid_argument("monotonic time must be nonnegative");
  return now > std::numeric_limits<Millis>::max() - delay ? std::numeric_limits<Millis>::max() : now + delay;
}
bool samePin(const Pin &a, const Pin &b)
{
  unsigned difference = 0;
  for (std::size_t i = 0; i < a.size(); ++i)
    difference |= a[i] ^ b[i];
  return difference == 0;
}
bool validHost(const std::string &host)
{
  return !host.empty() && host.size() <= 253 && std::all_of(host.begin(), host.end(), [](unsigned char c) {
    return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || (c >= '0' && c <= '9') || c == '.' || c == '-' ||
           c == ':' || c == '%' || c == '_';
  });
}
} // namespace

ConnectionModel::ConnectionModel(
    std::optional<TrustedProfile> profile, std::vector<std::string> allowedInterfaces, std::uint32_t jitterSeed
)
    : m_profile(std::move(profile)),
      m_allowedInterfaces(std::move(allowedInterfaces)),
      m_rng(jitterSeed)
{
  if (m_profile && (m_profile->localId.empty() || m_profile->serviceKey.empty() || m_profile->serviceKey.size() > 128))
    throw std::invalid_argument("invalid trusted profile");
}

bool ConnectionModel::observe(const std::string &serviceKey, const Endpoint &endpoint, Millis ttl, Millis now)
{
  if (now < 0 || ttl <= 0 || !validHost(endpoint.host) || endpoint.port == 0 || serviceKey.empty() ||
      serviceKey.size() > 128 ||
      std::find(m_allowedInterfaces.begin(), m_allowedInterfaces.end(), endpoint.interface) ==
          m_allowedInterfaces.end())
    return false;
  if (m_profile && serviceKey != m_profile->serviceKey)
    return false;
  std::erase_if(m_candidates, [now, &endpoint](const Candidate &c) {
    return c.expiresAt <= now || c.endpoint == endpoint;
  });
  if (m_candidates.size() == 8)
    m_candidates.erase(m_candidates.begin());
  m_candidates.push_back({serviceKey, endpoint, addTime(now, std::min<Millis>(ttl, 300000))});
  if (m_wanted && m_networkAvailable && m_state == ConnectionState::Discovering)
    schedule(now, 250); // coalesce discovery/wake bursts, rather than starting parallel attempts
  return true;
}

bool ConnectionModel::blocked() const
{
  return m_state == ConnectionState::IdentityBlocked || m_state == ConnectionState::PermissionBlocked;
}
void ConnectionModel::invalidate()
{
  ++m_generation;
  m_retryAt.reset();
  m_rtt.reset();
}
void ConnectionModel::schedule(Millis now, Millis delay)
{
  m_retryAt = addTime(now, delay);
  m_state = ConnectionState::RetryWaiting;
}
void ConnectionModel::requestConnect(Millis now)
{
  if (blocked() || m_state == ConnectionState::Connecting || m_state == ConnectionState::Connected)
    return;
  m_wanted = true;
  if (!m_profile) {
    m_state = ConnectionState::NeedsPairing;
    return;
  }
  if (!m_networkAvailable) {
    m_state = ConnectionState::Offline;
    return;
  }
  schedule(now, 0);
}
void ConnectionModel::pause()
{
  m_wanted = false;
  const bool wasBlocked = blocked();
  invalidate();
  if (!wasBlocked)
    m_state = ConnectionState::Paused;
}
void ConnectionModel::setNetworkAvailable(bool available, Millis now)
{
  if (available == m_networkAvailable)
    return;
  m_networkAvailable = available;
  const bool wasBlocked = blocked();
  invalidate();
  if (wasBlocked || !m_wanted)
    return;
  if (!available)
    m_state = ConnectionState::Offline;
  else if (m_profile)
    schedule(now, 250);
  else
    m_state = ConnectionState::NeedsPairing;
}
void ConnectionModel::wake(Millis now)
{
  if (!m_wanted || blocked() || !m_networkAvailable || !m_profile)
    return;
  invalidate();
  schedule(now, 250);
}
std::optional<ConnectRequest> ConnectionModel::poll(Millis now)
{
  if (now < 0 || !m_wanted || !m_networkAvailable || !m_profile || blocked() ||
      m_state == ConnectionState::Connecting || m_state == ConnectionState::Connected ||
      (m_retryAt && now < *m_retryAt))
    return std::nullopt;
  std::erase_if(m_candidates, [now](const Candidate &c) { return c.expiresAt <= now; });
  if (m_candidates.empty()) {
    m_retryAt.reset();
    m_state = ConnectionState::Discovering;
    return std::nullopt;
  }
  invalidate();
  m_state = ConnectionState::Connecting;
  return ConnectRequest{m_generation, m_candidates.back().endpoint, m_profile->expectedPin, true};
}
bool ConnectionModel::handshake(std::uint64_t generation, const TlsEvidence &evidence)
{
  if (generation != m_generation || !m_wanted || m_state != ConnectionState::Connecting || !m_profile)
    return false;
  if (!evidence.peerVerified || !samePin(evidence.peerPin, m_profile->expectedPin)) {
    invalidate();
    m_state = ConnectionState::IdentityBlocked;
    return false;
  }
  m_state = ConnectionState::Connected;
  m_failures = 0;
  return true;
}
bool ConnectionModel::fail(std::uint64_t generation, Failure reason, Millis now)
{
  if (generation != m_generation || !m_wanted ||
      (m_state != ConnectionState::Connecting && m_state != ConnectionState::Connected))
    return false;
  invalidate();
  if (reason == Failure::IdentityChanged || reason == Failure::PermissionDenied) {
    m_state =
        reason == Failure::IdentityChanged ? ConnectionState::IdentityBlocked : ConnectionState::PermissionBlocked;
    return true;
  }
  constexpr Millis delays[] = {1000, 2000, 4000, 8000, 16000, 30000};
  const auto delay = delays[std::min(m_failures, 5u)];
  m_failures = std::min(m_failures + 1, 6u);
  m_rng = m_rng * 1664525u + 1013904223u; // jitter only, not used for security or identity
  schedule(now, std::min<Millis>(30000, delay * (90 + m_rng % 21) / 100));
  return true;
}
bool ConnectionModel::recordRtt(std::uint64_t generation, Millis rtt, Millis now)
{
  if (generation != m_generation || !allowsInput() || rtt < 0 || rtt > 60000 || now < 0)
    return false;
  m_rtt = rtt;
  m_rttAt = now;
  return true;
}
std::optional<Millis> ConnectionModel::rtt(Millis now) const
{
  return allowsInput() && m_rtt && now >= m_rttAt && now - m_rttAt <= 10000 ? m_rtt : std::nullopt;
}
} // namespace deskflow::experience
