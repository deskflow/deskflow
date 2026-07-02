/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2026 Deskflow Developers
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#include "coordination/Coordinator.h"

#include "base/Event.h"
#include "base/EventQueue.h"
#include "base/Log.h"
#include "coordination/CoordinationEvents.h"
#include "coordination/CoordinationProtocol.h"
#include "coordination/FleetStateMerge.h"
#include "coordination/KeyboardRelayDecision.h"
#include "coordination/KeyboardRouter.h"
#include "coordination/RelayKeyEvent.h"
#include "base/EventTypes.h"

#include <chrono>
#include <cctype>

namespace deskflow::coordination {

namespace {

const double kHeartbeatIntervalS = 3.0;
const double kDiscoveryWindowS = 30.0;
const double kWorkerTickS = 1.0;
const int kLanProbeTimeoutMs = 700;
const int kWedgeProbeTimeoutMs = 1000;
const int kWedgeProbeEveryTicks = 9;
const int kWedgeStrikesToRestart = 2;

double monotonicSeconds()
{
  using namespace std::chrono;
  return duration<double>(steady_clock::now().time_since_epoch()).count();
}

RelayKeyPhase relayPhaseFromMessage(Message::KeyPhase phase)
{
  switch (phase) {
  case Message::KeyPhase::Up:
    return RelayKeyPhase::Up;
  case Message::KeyPhase::Repeat:
    return RelayKeyPhase::Repeat;
  default:
    return RelayKeyPhase::Down;
  }
}

RelayKeyEvent relayEventFromMessage(const Message &message)
{
  RelayKeyEvent event;
  event.phase = relayPhaseFromMessage(message.keyPhase);
  event.id = message.keyId;
  event.mask = message.keyMask;
  event.button = message.keyButton;
  event.lang = message.keyLang;
  event.from = message.name;
  return event;
}

std::string peerMeshAddress(const std::string &hostName, const FleetState &fleet, const PeerList &peers)
{
  auto matchConfigured = [&](const std::string &name) -> std::string {
    for (const auto &peer : fleet.peers) {
      if (peer.name == name) {
        return peer.lan.empty() ? peer.ip : peer.lan;
      }
    }
    for (const auto &peer : peers) {
      if (peer.name == name) {
        return peer.lan.empty() ? peer.ip : peer.lan;
      }
    }
    return {};
  };

  if (const auto direct = matchConfigured(hostName); !direct.empty()) {
    return direct;
  }
  if (!fleet.server.empty() && (hostName == fleet.server || hostName == fleet.cursorScreen || hostName == fleet.cursorHost)) {
    return matchConfigured(fleet.server);
  }
  return {};
}

} // namespace

Coordinator::Coordinator(CoordinatorConfig config)
    : m_config(std::move(config)),
      m_election(m_config.selfName, m_config.tuning, monotonicSeconds)
{
  m_fleetState.peers.reserve(m_config.peers.size());
  for (const auto &peer : m_config.peers) {
    m_fleetState.peers.push_back(FleetPeer{peer.name, peer.ip, peer.lan});
  }

  m_mesh = std::make_unique<CoordinationMesh>(
      m_config.meshPort, m_config.token,
      [this](const Message &message, const std::function<void(const std::string &)> &reply) {
        onMessage(message, reply);
      }
  );
  m_inputMonitor = createLocalInputMonitor();
  m_keyboardRelay = createKeyboardRelayMonitor();
}

Coordinator::~Coordinator()
{
  stop();
}

bool Coordinator::start()
{
  if (!m_mesh->start()) {
    return false;
  }
  m_inputMonitor->start([this] { onGenuineInput(); });
  m_startedAt = monotonicSeconds();
  m_workerStop = false;
  m_worker = std::thread([this] { workerLoop(); });
  LOG_INFO(
      "coordination: started as \"%s\" with %d peer(s)", m_config.selfName.c_str(),
      static_cast<int>(m_config.peers.size())
  );
  return true;
}

void Coordinator::stop()
{
  {
    std::scoped_lock lock{m_mutex};
    m_workerStop = true;
    m_quit = true;
  }
  m_workerWake.notify_all();
  m_decisionReady.notify_all();
  if (m_worker.joinable()) {
    m_worker.join();
  }
  m_inputMonitor->stop();
  m_keyboardRelay->stop();
  m_mesh->stop();
}

RoleDecision Coordinator::awaitRoleDecision()
{
  std::unique_lock lock{m_mutex};
  m_decisionReady.wait(lock, [this] { return m_hasDecision || m_quit; });
  if (m_quit) {
    return RoleDecision{Role::Init, {}, true};
  }
  m_hasDecision = false;
  return m_decision;
}

void Coordinator::setInterruptCallback(std::function<void()> interrupt)
{
  std::scoped_lock lock{m_mutex};
  m_interrupt = std::move(interrupt);
}

void Coordinator::notifyCursorHere(bool here)
{
  std::scoped_lock lock{m_mutex};
  m_election.setCursorHere(here);
}

void Coordinator::notifyEpochEnded()
{
  // The app exited without a new decision (screen error, transport
  // failure). Re-arm the current role so the epoch loop restarts it;
  // election events can still override at any time.
  std::scoped_lock lock{m_mutex};
  if (m_quit || m_hasDecision) {
    return;
  }
  if (m_election.role() != Role::Init) {
    m_decision = RoleDecision{m_election.role(), m_election.serverAddress(), false};
    m_hasDecision = true;
    m_decisionReady.notify_all();
  }
}

void Coordinator::requestQuit()
{
  std::function<void()> interrupt;
  {
    std::scoped_lock lock{m_mutex};
    m_quit = true;
    interrupt = m_interrupt;
  }
  m_decisionReady.notify_all();
  m_workerWake.notify_all();
  if (interrupt) {
    interrupt();
  }
}

bool Coordinator::hasPendingDecision()
{
  std::scoped_lock lock{m_mutex};
  return m_hasDecision || m_quit;
}

void Coordinator::setEventQueue(IEventQueue *events)
{
  std::scoped_lock lock{m_mutex};
  m_events = events;
}

FleetState Coordinator::fleetSnapshot() const
{
  std::scoped_lock lock{m_mutex};
  return m_fleetState;
}

void Coordinator::postFleetStateEvents(IEventQueue *events, const FleetMergeResult &merge)
{
  if (events == nullptr || !merge.changed) {
    return;
  }
  events->addEvent(Event(EventTypes::CoordinationFleetStateChanged));
  if (merge.topologyBecameReady) {
    events->addEvent(Event(EventTypes::CoordinationTopologyReady));
  }
}

std::vector<FleetPeer> Coordinator::buildFleetPeersLocked()
{
  if (!m_fleetState.peers.empty()) {
    return m_fleetState.peers;
  }
  std::vector<FleetPeer> peers;
  peers.reserve(m_config.peers.size());
  for (const auto &peer : m_config.peers) {
    peers.push_back(FleetPeer{peer.name, peer.ip, peer.lan});
  }
  return peers;
}

void Coordinator::sendFleetLineToPeers(const std::string &line, const PeerList &peers)
{
  for (const auto &peer : peers) {
    if (peer.name == m_config.selfName) {
      continue;
    }
    m_mesh->sendTo(peer.ip, line);
    if (peer.lan != peer.ip) {
      m_mesh->sendTo(peer.lan, line);
    }
  }
}

bool Coordinator::mergeAndBroadcastFleetFragment(const FleetFragment &fragment, bool sendEvenIfUnchanged)
{
  std::string line;
  PeerList peers;
  IEventQueue *events = nullptr;
  FleetMergeResult merge;
  bool sendMesh = false;
  {
    std::scoped_lock lock{m_mutex};
    if (m_config.meshVersion < 2 || m_election.role() != Role::Server) {
      return false;
    }
    if (!fragment.cursorHost.empty()) {
      m_fleetCursorHost = fragment.cursorHost;
    }
    events = m_events;
    merge = applyServerFragment(m_fleetState, fragment);
    sendMesh = sendEvenIfUnchanged || merge.changed;
    if (sendMesh) {
      line = protocol::encodeFleet(fragment, m_config.token);
      peers = m_config.peers;
    }
  }

  postFleetStateEvents(events, merge);

  if (sendMesh) {
    sendFleetLineToPeers(line, peers);
  }
  return merge.changed;
}

void Coordinator::handleHelloMessage(const Message &message, const std::function<void(const std::string &)> &reply)
{
  if (m_config.meshVersion < 2) {
    return;
  }
  LOG_DEBUG(
      "coordination: mesh v2 hello from \"%s\" (v=%d)", message.name.c_str(), message.meshVersion
  );
  reply(protocol::encodeHello(m_config.meshVersion, m_config.selfName, m_config.token));
}

void Coordinator::handleFleetMessage(const Message &message)
{
  if (m_config.meshVersion < 2) {
    return;
  }

  IEventQueue *events = nullptr;
  FleetMergeResult merge;
  int64_t seq = 0;
  size_t linkCount = 0;
  {
    std::scoped_lock lock{m_mutex};
    if (m_election.role() == Role::Server) {
      return;
    }
    events = m_events;
    merge = applyServerFragment(m_fleetState, protocol::fleetFragmentFromMessage(message));
    seq = m_fleetState.seq;
    linkCount = m_fleetState.links.size();
  }
  if (merge.changed) {
    LOG_DEBUG("coordination: fleet snapshot updated (seq=%lld links=%zu)", static_cast<long long>(seq), linkCount);
  }
  postFleetStateEvents(events, merge);
}

void Coordinator::broadcastCursor(const std::string &host)
{
  if (host.empty()) {
    return;
  }
  std::string line;
  PeerList peers;
  {
    std::scoped_lock lock{m_mutex};
    if (m_election.role() != Role::Server) {
      return;
    }
    m_fleetCursorHost = host;
    const int64_t seq = ++m_cursorSeq;
    line = protocol::encodeCursor(host, seq, m_config.token);
    peers = m_config.peers;
  }
  for (const auto &peer : peers) {
    if (peer.name == m_config.selfName) {
      continue;
    }
    m_mesh->sendTo(peer.ip, line);
    if (peer.lan != peer.ip) {
      m_mesh->sendTo(peer.lan, line);
    }
  }
}

void Coordinator::updateCursorHost(const std::string &screenName)
{
  if (screenName.empty()) {
    return;
  }
  if (m_config.meshVersion < 2) {
    broadcastCursor(screenName);
    return;
  }

  FleetFragment fragment;
  {
    std::scoped_lock lock{m_mutex};
    if (m_election.role() != Role::Server) {
      return;
    }
    fragment.server = m_fleetState.server.empty() ? m_config.selfName : m_fleetState.server;
    fragment.seq = ++m_fleetSeq;
    fragment.cursorHost = screenName;
    fragment.cursorScreen = screenName;
    fragment.links = m_fleetState.links;
    fragment.screens = m_fleetState.screens;
    fragment.peers = buildFleetPeersLocked();
  }

  mergeAndBroadcastFleetFragment(fragment, true);
}

void Coordinator::publishFleetTopology(std::vector<FleetLink> links, std::vector<FleetScreen> screens)
{
  if (screens.empty()) {
    return;
  }

  FleetFragment fragment;
  {
    std::scoped_lock lock{m_mutex};
    if (m_config.meshVersion < 2 || m_election.role() != Role::Server) {
      return;
    }

    fragment.server = m_config.selfName;
    fragment.seq = ++m_fleetSeq;
    fragment.cursorHost = m_fleetCursorHost.empty() ? m_config.selfName : m_fleetCursorHost;
    fragment.cursorScreen = fragment.cursorHost;
    fragment.links = std::move(links);
    fragment.screens = std::move(screens);
    fragment.peers = buildFleetPeersLocked();
  }

  const bool changed = mergeAndBroadcastFleetFragment(fragment, false);
  if (changed) {
    const auto snapshot = fleetSnapshot();
    LOG_DEBUG(
        "coordination: published fleet topology (seq=%lld links=%zu)", static_cast<long long>(snapshot.seq),
        snapshot.links.size()
    );
  }
}

void Coordinator::updateKeyboardRelayForRole(Role role)
{
  if (!m_config.keyboardFollowCursor) {
    {
      std::scoped_lock lock{m_mutex};
      m_loggedKeyForward = false;
      m_loggedKeyForwardReceive = false;
    }
    m_keyboardRelay->stop();
    return;
  }
  if (role == Role::Client) {
    {
      std::scoped_lock lock{m_mutex};
      m_loggedKeyForward = false;
      m_loggedKeyForwardReceive = false;
      m_loggedRelayUnknownForward = false;
      // Epoch restart does not call becameClient(); clear stale screen sync.
      m_election.resetCursorScreen();
      m_clientRelayStartedAt = monotonicSeconds();
    }
    // Relay uses ElectionState::cursorHere(), fed by CoordinationScreenEntered/Left.
    // Until the first enter/leave, a boot grace window passes keys locally.
    m_keyboardRelay->start(
        [this] { return relayPassThroughLocal(); },
        [this](Message::KeyPhase phase, KeyID id, KeyModifierMask mask, KeyButton button, const std::string &lang) {
          sendKeyForward(phase, id, mask, button, lang);
        }
    );
    LOG_INFO("coordination: keyboard relay started (client epoch)");
    return;
  }
  {
    std::scoped_lock lock{m_mutex};
    m_loggedKeyForward = false;
    m_loggedKeyForwardReceive = false;
    m_loggedRelayUnknownForward = false;
  }
  // Server epoch: keyboard uses Server::onKeyDown → m_active (not keyfwd relay).
  m_keyboardRelay->stop();
}

void Coordinator::onMessage(const Message &message, const std::function<void(const std::string &)> &reply)
{
  switch (message.type) {
  case Message::Type::Claim: {
    ElectionState::ClaimAction action;
    {
      std::scoped_lock lock{m_mutex};
      action = m_election.onClaim(message.name, message.ip, message.lan, message.seq);
    }
    if (action == ElectionState::ClaimAction::FollowSender) {
      followSender(message);
    }
    break;
  }

  case Message::Type::Promote:
    LOG_INFO("coordination: manual promote received");
    promoteSelf("manual promote");
    break;

  case Message::Type::Status: {
    std::string snapshot;
    {
      std::scoped_lock lock{m_mutex};
      const FleetState *fleet = m_config.meshVersion >= 2 ? &m_fleetState : nullptr;
      snapshot = protocol::encodeStatusReply(
          m_election.role(), m_election.serverAddress(), m_election.seq(), m_election.lastSwitchAt(),
          m_config.selfName, fleet
      );
    }
    reply(snapshot);
    break;
  }

  case Message::Type::Cursor:
    handleCursorMessage(message);
    break;

  case Message::Type::KeyFwd:
  case Message::Type::Key:
    handleKeyForwardMessage(message);
    break;

  case Message::Type::Hello:
    handleHelloMessage(message, reply);
    break;

  case Message::Type::Fleet:
    handleFleetMessage(message);
    break;

  default:
    break;
  }
}

void Coordinator::handleCursorMessage(const Message &message)
{
  if (message.host.empty()) {
    return;
  }
  if (m_config.meshVersion >= 2) {
    // Mesh v2 carries cursor in fleet fragments; legacy cursor heartbeats are v1-only.
    return;
  }
  std::scoped_lock lock{m_mutex};
  m_fleetCursorHost = message.host;
  m_cursorSeq = std::max(m_cursorSeq, message.seq);
}

void Coordinator::handleKeyForwardMessage(const Message &message)
{
  if (message.type == Message::Type::Key && m_config.meshVersion < 2) {
    return;
  }
  if (message.type == Message::Type::KeyFwd && m_config.meshVersion >= 2) {
    return;
  }

  Role role;
  IEventQueue *events = nullptr;
  std::string selfName;
  std::string cursorHost;
  {
    std::scoped_lock lock{m_mutex};
    role = m_election.role();
    events = m_events;
    selfName = m_config.selfName;
    cursorHost = m_fleetState.cursorHost;
  }

  const bool serverEpoch = role == Role::Server;
  const bool clientCursorHost =
      role == Role::Client && m_config.meshVersion >= 2 && cursorHostIsLocal(selfName, cursorHost);
  if (!serverEpoch && !clientCursorHost) {
    return;
  }
  if (serverEpoch && events == nullptr) {
    return;
  }
  if (clientCursorHost && events == nullptr) {
    return;
  }
  if (serverEpoch && !isKnownPeer(message.name)) {
    LOG_DEBUG("coordination: dropping relay key from unknown peer \"%s\"", message.name.c_str());
    return;
  }

  bool logFirst = false;
  {
    std::scoped_lock lock{m_mutex};
    if (!m_loggedKeyForwardReceive) {
      m_loggedKeyForwardReceive = true;
      logFirst = true;
    }
  }
  const char *label = message.type == Message::Type::Key ? "key" : "keyfwd";
  if (logFirst) {
    LOG_INFO("coordination: %s from \"%s\" phase=%d", label, message.name.c_str(), static_cast<int>(message.keyPhase));
  } else {
    LOG_DEBUG("coordination: %s from \"%s\" phase=%d", label, message.name.c_str(), static_cast<int>(message.keyPhase));
  }

  auto *info = new CoordinationKeyForwardInfo(relayEventFromMessage(message));
  events->addEvent(
      Event(EventTypes::CoordinationKeyForward, events->getSystemTarget(), info, Event::EventFlags::DeliverImmediately)
  );
}

void Coordinator::sendKeyForward(
    Message::KeyPhase phase, KeyID id, KeyModifierMask mask, KeyButton button, const std::string &lang
)
{
  const auto relayPhase = relayPhaseFromMessage(phase);
  std::string destination;
  std::string line;
  bool logFirst = false;
  {
    std::scoped_lock lock{m_mutex};
    if (m_election.role() != Role::Client) {
      return;
    }

    if (m_config.meshVersion < 2) {
      destination = m_election.serverAddress();
      line = protocol::encodeKeyFwd(
          m_config.selfName, relayPhase, static_cast<uint16_t>(id), static_cast<uint16_t>(mask), button, lang,
          m_config.token
      );
    } else {
      KeyboardRouteInput input;
      input.selfName = m_config.selfName;
      input.cursorHost = m_fleetState.cursorHost;
      input.cursorHostKnown = !m_fleetState.cursorHost.empty();
      input.secondsSinceRelayStart = monotonicSeconds() - m_clientRelayStartedAt;

      const auto decision = routeKeyboard(input);
      if (decision.route == KeyboardRoute::Local) {
        return;
      }

      destination = peerMeshAddress(decision.forwardHost, m_fleetState, m_config.peers);
      line = protocol::encodeKey(
          m_config.selfName, relayPhase, static_cast<uint16_t>(id), static_cast<uint16_t>(mask), button, lang,
          m_config.token
      );
      if (destination.empty()) {
        destination = m_election.serverAddress();
      }
    }

    if (!m_loggedKeyForward) {
      m_loggedKeyForward = true;
      logFirst = true;
    }
  }
  if (destination.empty()) {
    return;
  }
  if (logFirst) {
    LOG_INFO("coordination: forwarding keyboard to %s", destination.c_str());
  } else {
    LOG_DEBUG("coordination: forwarding keyboard to %s", destination.c_str());
  }
  m_mesh->sendTo(destination, line);
}

bool Coordinator::isKnownPeer(const std::string &name) const
{
  for (const auto &peer : m_config.peers) {
    if (peer.name == name) {
      return true;
    }
  }
  return false;
}

bool Coordinator::relayPassThroughLocal()
{
  std::scoped_lock lock{m_mutex};
  const double elapsed = monotonicSeconds() - m_clientRelayStartedAt;
  if (m_config.meshVersion < 2) {
    const bool known = m_election.cursorScreenKnown();
    const bool passLocal = passKeyToLocalOs(m_election.cursorHere(), known, elapsed);
    if (!passLocal && !known && elapsed >= kCursorRelayBootGraceS && !m_loggedRelayUnknownForward) {
      LOG_INFO("coordination: forwarding keyboard before screen enter/leave sync");
      m_loggedRelayUnknownForward = true;
    }
    return passLocal;
  }

  KeyboardRouteInput input;
  input.selfName = m_config.selfName;
  input.cursorHost = m_fleetState.cursorHost;
  input.cursorHostKnown = !m_fleetState.cursorHost.empty();
  input.secondsSinceRelayStart = elapsed;
  const auto decision = routeKeyboard(input);
  if (decision.route == KeyboardRoute::Forward && !input.cursorHostKnown &&
      elapsed >= kCursorRelayBootGraceS && !m_loggedRelayUnknownForward) {
    LOG_INFO("coordination: forwarding keyboard before fleet cursor sync");
    m_loggedRelayUnknownForward = true;
  }
  return decision.route == KeyboardRoute::Local;
}

std::string Coordinator::fleetCursorHost()
{
  std::scoped_lock lock{m_mutex};
  return m_fleetCursorHost;
}

void Coordinator::onGenuineInput()
{
  bool promote = false;
  {
    std::scoped_lock lock{m_mutex};
    promote = m_election.onLocalInput();
  }
  if (promote) {
    promoteSelf("local input burst");
  }
}

void Coordinator::promoteSelf(const char *reason)
{
  {
    std::scoped_lock lock{m_mutex};
    if (m_quit) {
      return;
    }
    if (m_election.role() == Role::Server) {
      return; // already primary; heartbeats keep claiming
    }
    LOG_INFO("coordination: promoting to server (%s)", reason);
    // The claim broadcast does blocking connects to every peer; hand it
    // to the worker so this thread (often the input monitor's event-tap
    // thread, which macOS disables when stalled) returns immediately.
    m_broadcastPending = true;
  }
  decide(Role::Server, {});
  m_workerWake.notify_all();
}

void Coordinator::followSender(const Message &claim)
{
  // LAN-first: prefer the sender's LAN address when its coordination
  // port answers there; otherwise use the stable address.
  std::string address = claim.ip;
  if (!claim.lan.empty() && claim.lan != claim.ip && m_mesh->probe(claim.lan, kLanProbeTimeoutMs)) {
    address = claim.lan;
  }
  LOG_INFO("coordination: following \"%s\" at %s", claim.name.c_str(), address.c_str());
  decide(Role::Client, address);
}

void Coordinator::decide(Role role, const std::string &serverAddress)
{
  std::function<void()> interrupt;
  {
    std::scoped_lock lock{m_mutex};
    if (m_quit) {
      return;
    }
    if (role == Role::Server) {
      m_election.becameServer();
    } else {
      m_election.becameClient(serverAddress);
    }
    m_decision = RoleDecision{role, serverAddress, false};
    m_hasDecision = true;
    m_wedgeStrikes = 0;
    interrupt = m_interrupt;
  }
  m_decisionReady.notify_all();
  if (interrupt) {
    interrupt();
  }
}

void Coordinator::broadcastClaim()
{
  std::string line;
  {
    std::scoped_lock lock{m_mutex};
    std::string selfIp;
    std::string selfLan;
    for (const auto &peer : m_config.peers) {
      if (peer.name == m_config.selfName) {
        selfIp = peer.ip;
        selfLan = peer.lan;
        break;
      }
    }
    line = protocol::encodeClaim(m_config.selfName, selfIp, selfLan, m_election.nextClaimSeq(), m_config.token);
  }
  for (const auto &peer : m_config.peers) {
    if (peer.name == m_config.selfName) {
      continue;
    }
    m_mesh->sendTo(peer.ip, line);
    if (peer.lan != peer.ip) {
      m_mesh->sendTo(peer.lan, line);
    }
  }
}

void Coordinator::workerLoop()
{
  double lastHeartbeatAt = 0.0;
  int tick = 0;

  while (true) {
    bool broadcastNow = false;
    {
      std::unique_lock lock{m_mutex};
      m_workerWake.wait_for(lock, std::chrono::duration<double>(kWorkerTickS), [this] {
        return m_workerStop || m_broadcastPending;
      });
      if (m_workerStop) {
        return;
      }
      broadcastNow = m_broadcastPending;
      m_broadcastPending = false;
    }
    ++tick;
    const double now = monotonicSeconds();
    if (broadcastNow) {
      broadcastClaim();
      lastHeartbeatAt = now;
    }

    Role role;
    {
      std::scoped_lock lock{m_mutex};
      role = m_election.role();
    }

    if (role == Role::Server) {
      if (now - lastHeartbeatAt >= kHeartbeatIntervalS) {
        lastHeartbeatAt = now;
        broadcastClaim();
        std::string cursorHost;
        PeerList peers;
        std::string selfName;
        std::string token;
        int64_t seq = 0;
        {
          std::scoped_lock lock{m_mutex};
          if (!m_fleetCursorHost.empty()) {
            cursorHost = m_fleetCursorHost;
            seq = ++m_cursorSeq;
            peers = m_config.peers;
            selfName = m_config.selfName;
            token = m_config.token;
          }
        }
        if (!cursorHost.empty()) {
          const auto cursorLine = protocol::encodeCursor(cursorHost, seq, token);
          for (const auto &peer : peers) {
            if (peer.name == selfName) {
              continue;
            }
            m_mesh->sendTo(peer.ip, cursorLine);
            if (peer.lan != peer.ip) {
              m_mesh->sendTo(peer.lan, cursorLine);
            }
          }
        }
      }
      if (tick % kWedgeProbeEveryTicks == 0) {
        // Alive-but-not-accepting detection: the server process can wedge
        // while its accept loop is stuck; restart the epoch if the
        // transport port stops answering locally.
        if (m_mesh->probeDeskflowPort(m_config.deskflowPort, kWedgeProbeTimeoutMs)) {
          m_wedgeStrikes = 0;
        } else if (++m_wedgeStrikes >= kWedgeStrikesToRestart) {
          LOG_WARN("coordination: server transport wedged; restarting server epoch");
          m_wedgeStrikes = 0;
          decide(Role::Server, {});
        }
      }
    } else if (role == Role::Init && now - m_startedAt <= kDiscoveryWindowS) {
      discoverOnce();
    }
  }
}

void Coordinator::discoverOnce()
{
  // Best-effort: ask peers who is serving; the first server claim that
  // arrives via onMessage flips us. (Replies come back on the mesh as
  // unsolicited claims are not sent for status; legacy nodes reply with
  // a status object which we parse here.)
  const std::string line = protocol::encodeStatus(m_config.token);
  for (const auto &peer : m_config.peers) {
    if (peer.name == m_config.selfName) {
      continue;
    }
    const auto replyLine = m_mesh->query(peer.lan.empty() ? peer.ip : peer.lan, line);
    if (replyLine.empty()) {
      continue;
    }
    const auto reply = protocol::decodeStatusReply(replyLine);
    if (reply.valid && reply.role == Role::Server) {
      LOG_INFO("coordination: discovered active server \"%s\"", peer.name.c_str());
      Message claim;
      claim.type = Message::Type::Claim;
      claim.name = peer.name;
      claim.ip = peer.ip;
      claim.lan = peer.lan;
      followSender(claim);
      return;
    }
  }
}

} // namespace deskflow::coordination
