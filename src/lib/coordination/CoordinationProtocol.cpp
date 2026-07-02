/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2026 Deskflow Developers
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#include "coordination/CoordinationProtocol.h"

#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>

namespace deskflow::coordination::protocol {

namespace {

std::string serialize(const QJsonObject &object)
{
  return QJsonDocument(object).toJson(QJsonDocument::Compact).toStdString();
}

void putToken(QJsonObject &object, const std::string &token)
{
  if (!token.empty()) {
    object[QStringLiteral("token")] = QString::fromStdString(token);
  }
}

QString phaseToString(RelayKeyPhase phase)
{
  switch (phase) {
  case RelayKeyPhase::Up:
    return QStringLiteral("up");
  case RelayKeyPhase::Repeat:
    return QStringLiteral("repeat");
  default:
    return QStringLiteral("down");
  }
}

RelayKeyPhase phaseFromString(const QString &phase)
{
  if (phase == QStringLiteral("up")) {
    return RelayKeyPhase::Up;
  }
  if (phase == QStringLiteral("repeat")) {
    return RelayKeyPhase::Repeat;
  }
  return RelayKeyPhase::Down;
}

Message::KeyPhase toMessageKeyPhase(RelayKeyPhase phase)
{
  switch (phase) {
  case RelayKeyPhase::Up:
    return Message::KeyPhase::Up;
  case RelayKeyPhase::Repeat:
    return Message::KeyPhase::Repeat;
  default:
    return Message::KeyPhase::Down;
  }
}

void decodeKeyBody(const QJsonObject &object, Message &message)
{
  message.name = object[QStringLiteral("from")].toString().toStdString();
  message.keyPhase = toMessageKeyPhase(phaseFromString(object[QStringLiteral("phase")].toString()));
  message.keyId = static_cast<uint16_t>(object[QStringLiteral("id")].toInt());
  message.keyMask = static_cast<uint16_t>(object[QStringLiteral("mask")].toInt());
  message.keyButton = static_cast<uint16_t>(object[QStringLiteral("button")].toInt());
  message.keyLang = object[QStringLiteral("lang")].toString().toStdString();
}

std::string encodeKeyMessage(
    const char *type, const std::string &from, RelayKeyPhase phase, uint16_t id, uint16_t mask, uint16_t button,
    const std::string &lang, const std::string &token
)
{
  QJsonObject object;
  object[QStringLiteral("t")] = QString::fromUtf8(type);
  object[QStringLiteral("from")] = QString::fromStdString(from);
  object[QStringLiteral("phase")] = phaseToString(phase);
  object[QStringLiteral("id")] = id;
  object[QStringLiteral("mask")] = mask;
  object[QStringLiteral("button")] = button;
  object[QStringLiteral("lang")] = QString::fromStdString(lang);
  putToken(object, token);
  return serialize(object);
}

FleetPeer decodeFleetPeer(const QJsonObject &object)
{
  FleetPeer peer;
  peer.name = object[QStringLiteral("name")].toString().toStdString();
  peer.ip = object[QStringLiteral("ip")].toString().toStdString();
  peer.lan = object[QStringLiteral("lan")].toString().toStdString();
  return peer;
}

QJsonObject encodeFleetPeer(const FleetPeer &peer)
{
  QJsonObject object;
  object[QStringLiteral("name")] = QString::fromStdString(peer.name);
  object[QStringLiteral("ip")] = QString::fromStdString(peer.ip);
  object[QStringLiteral("lan")] = QString::fromStdString(peer.lan);
  return object;
}

FleetLink decodeFleetLink(const QJsonObject &object)
{
  FleetLink link;
  link.fromScreen = object[QStringLiteral("from")].toString().toStdString();
  link.toScreen = object[QStringLiteral("to")].toString().toStdString();
  link.direction = object[QStringLiteral("dir")].toString().toStdString();
  return link;
}

QJsonObject encodeFleetLink(const FleetLink &link)
{
  QJsonObject object;
  object[QStringLiteral("from")] = QString::fromStdString(link.fromScreen);
  object[QStringLiteral("to")] = QString::fromStdString(link.toScreen);
  object[QStringLiteral("dir")] = QString::fromStdString(link.direction);
  return object;
}

void decodeFleetBody(const QJsonObject &object, FleetFragment &fragment)
{
  fragment.server = object[QStringLiteral("server")].toString().toStdString();
  const auto seqValue = object[QStringLiteral("seq")];
  fragment.seq = seqValue.isString() ? seqValue.toString().toLongLong() : static_cast<int64_t>(seqValue.toDouble());

  if (const auto cursor = object[QStringLiteral("cursor")]; cursor.isObject()) {
    const QJsonObject cursorObject = cursor.toObject();
    fragment.cursorHost = cursorObject[QStringLiteral("host")].toString().toStdString();
    fragment.cursorScreen = cursorObject[QStringLiteral("screen")].toString().toStdString();
  }

  if (const auto peers = object[QStringLiteral("peers")]; peers.isArray()) {
    const QJsonArray peersArray = peers.toArray();
    fragment.peers.reserve(peersArray.size());
    for (const auto &value : peersArray) {
      if (value.isObject()) {
        fragment.peers.push_back(decodeFleetPeer(value.toObject()));
      }
    }
  }

  if (const auto links = object[QStringLiteral("links")]; links.isArray()) {
    const QJsonArray linksArray = links.toArray();
    fragment.links.reserve(linksArray.size());
    for (const auto &value : linksArray) {
      if (value.isObject()) {
        fragment.links.push_back(decodeFleetLink(value.toObject()));
      }
    }
  }

  if (const auto screens = object[QStringLiteral("screens")]; screens.isArray()) {
    const QJsonArray screensArray = screens.toArray();
    fragment.screens.reserve(screensArray.size());
    for (const auto &value : screensArray) {
      if (value.isString()) {
        fragment.screens.push_back({value.toString().toStdString()});
      } else if (value.isObject()) {
        fragment.screens.push_back({value.toObject()[QStringLiteral("name")].toString().toStdString()});
      }
    }
  }
}

} // namespace

Message decode(const std::string &line)
{
  Message message;
  const auto doc = QJsonDocument::fromJson(QByteArray(line.data(), static_cast<int>(line.size())));
  if (!doc.isObject()) {
    return message;
  }
  const QJsonObject object = doc.object();
  const QString type = object[QStringLiteral("t")].toString();

  if (type == QStringLiteral("claim")) {
    message.type = Message::Type::Claim;
  } else if (type == QStringLiteral("promote")) {
    message.type = Message::Type::Promote;
  } else if (type == QStringLiteral("status")) {
    message.type = Message::Type::Status;
  } else if (type == QStringLiteral("cursor")) {
    message.type = Message::Type::Cursor;
  } else if (type == QStringLiteral("keyfwd")) {
    message.type = Message::Type::KeyFwd;
  } else if (type == QStringLiteral("key")) {
    message.type = Message::Type::Key;
  } else if (type == QStringLiteral("hello")) {
    message.type = Message::Type::Hello;
  } else if (type == QStringLiteral("fleet")) {
    message.type = Message::Type::Fleet;
  } else {
    return message;
  }

  message.name = object[QStringLiteral("name")].toString().toStdString();
  message.ip = object[QStringLiteral("ip")].toString().toStdString();
  message.lan = object[QStringLiteral("lan")].toString().toStdString();
  message.host = object[QStringLiteral("host")].toString().toStdString();
  // Legacy senders emit seq as a JSON number; tolerate strings too.
  const auto seqValue = object[QStringLiteral("seq")];
  message.seq = seqValue.isString() ? seqValue.toString().toLongLong() : static_cast<int64_t>(seqValue.toDouble());
  message.token = object[QStringLiteral("token")].toString().toStdString();

  if (message.type == Message::Type::KeyFwd || message.type == Message::Type::Key) {
    decodeKeyBody(object, message);
  }

  if (message.type == Message::Type::Hello) {
    message.meshVersion = object[QStringLiteral("v")].toInt();
  }

  if (message.type == Message::Type::Fleet) {
    decodeFleetBody(object, message.fleet);
  }

  return message;
}

std::string encodeClaim(
    const std::string &name, const std::string &ip, const std::string &lan, int64_t seq, const std::string &token
)
{
  QJsonObject object;
  object[QStringLiteral("t")] = QStringLiteral("claim");
  object[QStringLiteral("name")] = QString::fromStdString(name);
  object[QStringLiteral("ip")] = QString::fromStdString(ip);
  object[QStringLiteral("lan")] = QString::fromStdString(lan);
  object[QStringLiteral("seq")] = static_cast<qint64>(seq);
  putToken(object, token);
  return serialize(object);
}

std::string encodePromote(const std::string &token)
{
  QJsonObject object;
  object[QStringLiteral("t")] = QStringLiteral("promote");
  putToken(object, token);
  return serialize(object);
}

std::string encodeStatus(const std::string &token)
{
  QJsonObject object;
  object[QStringLiteral("t")] = QStringLiteral("status");
  putToken(object, token);
  return serialize(object);
}

std::string encodeCursor(const std::string &host, int64_t seq, const std::string &token)
{
  QJsonObject object;
  object[QStringLiteral("t")] = QStringLiteral("cursor");
  object[QStringLiteral("host")] = QString::fromStdString(host);
  object[QStringLiteral("seq")] = static_cast<qint64>(seq);
  putToken(object, token);
  return serialize(object);
}

std::string encodeKeyFwd(
    const std::string &from, RelayKeyPhase phase, uint16_t id, uint16_t mask, uint16_t button,
    const std::string &lang, const std::string &token
)
{
  return encodeKeyMessage("keyfwd", from, phase, id, mask, button, lang, token);
}

std::string encodeKey(
    const std::string &from, RelayKeyPhase phase, uint16_t id, uint16_t mask, uint16_t button,
    const std::string &lang, const std::string &token
)
{
  return encodeKeyMessage("key", from, phase, id, mask, button, lang, token);
}

std::string encodeHello(int meshVersion, const std::string &name, const std::string &token)
{
  QJsonObject object;
  object[QStringLiteral("t")] = QStringLiteral("hello");
  object[QStringLiteral("v")] = meshVersion;
  object[QStringLiteral("name")] = QString::fromStdString(name);
  putToken(object, token);
  return serialize(object);
}

std::string encodeFleet(const FleetFragment &fragment, const std::string &token)
{
  QJsonObject object;
  object[QStringLiteral("t")] = QStringLiteral("fleet");
  object[QStringLiteral("seq")] = static_cast<qint64>(fragment.seq);
  object[QStringLiteral("server")] = QString::fromStdString(fragment.server);

  QJsonObject cursor;
  cursor[QStringLiteral("host")] = QString::fromStdString(fragment.cursorHost);
  cursor[QStringLiteral("screen")] = QString::fromStdString(fragment.cursorScreen);
  object[QStringLiteral("cursor")] = cursor;

  QJsonArray peers;
  for (const auto &peer : fragment.peers) {
    peers.append(encodeFleetPeer(peer));
  }
  object[QStringLiteral("peers")] = peers;

  QJsonArray links;
  for (const auto &link : fragment.links) {
    links.append(encodeFleetLink(link));
  }
  object[QStringLiteral("links")] = links;

  QJsonArray screens;
  for (const auto &screen : fragment.screens) {
    screens.append(QString::fromStdString(screen.name));
  }
  object[QStringLiteral("screens")] = screens;

  putToken(object, token);
  return serialize(object);
}

FleetFragment fleetFragmentFromMessage(const Message &message)
{
  return message.fleet;
}

std::string encodeStatusReply(
    Role role, const std::string &serverAddress, int64_t seq, double lastSwitchAt, const std::string &name,
    const FleetState *fleet, int meshVersion, const std::vector<std::string> &versionMismatchPeers
)
{
  QJsonObject object;
  object[QStringLiteral("role")] = QString::fromUtf8(roleName(role));
  if (serverAddress.empty()) {
    object[QStringLiteral("server_ip")] = QJsonValue::Null;
  } else {
    object[QStringLiteral("server_ip")] = QString::fromStdString(serverAddress);
  }
  object[QStringLiteral("seq")] = static_cast<qint64>(seq);
  object[QStringLiteral("last_switch")] = lastSwitchAt;
  object[QStringLiteral("name")] = QString::fromStdString(name);
  if (meshVersion > 0) {
    object[QStringLiteral("mesh_version")] = meshVersion;
  }
  if (!versionMismatchPeers.empty()) {
    QJsonArray mismatches;
    for (const auto &peer : versionMismatchPeers) {
      mismatches.append(QString::fromStdString(peer));
    }
    object[QStringLiteral("version_mismatch")] = mismatches;
  }
  if (fleet != nullptr) {
    QJsonObject fleetObject;
    fleetObject[QStringLiteral("seq")] = static_cast<qint64>(fleet->seq);
    if (!fleet->server.empty()) {
      fleetObject[QStringLiteral("server")] = QString::fromStdString(fleet->server);
    }
    if (!fleet->cursorHost.empty()) {
      fleetObject[QStringLiteral("cursor_host")] = QString::fromStdString(fleet->cursorHost);
    }
    if (!fleet->cursorScreen.empty()) {
      fleetObject[QStringLiteral("cursor_screen")] = QString::fromStdString(fleet->cursorScreen);
    }
    QJsonArray peers;
    for (const auto &peer : fleet->peers) {
      peers.append(encodeFleetPeer(peer));
    }
    fleetObject[QStringLiteral("peers")] = peers;
    QJsonArray screens;
    for (const auto &screen : fleet->screens) {
      screens.append(QString::fromStdString(screen.name));
    }
    fleetObject[QStringLiteral("screens")] = screens;
    QJsonArray links;
    for (const auto &link : fleet->links) {
      links.append(encodeFleetLink(link));
    }
    fleetObject[QStringLiteral("links")] = links;
    object[QStringLiteral("fleet")] = fleetObject;
  }
  return serialize(object);
}

StatusReply decodeStatusReply(const std::string &line)
{
  StatusReply reply;
  const auto doc = QJsonDocument::fromJson(QByteArray(line.data(), static_cast<int>(line.size())));
  if (!doc.isObject()) {
    return reply;
  }
  const QJsonObject object = doc.object();
  const QString role = object[QStringLiteral("role")].toString();
  if (role == QStringLiteral("server")) {
    reply.role = Role::Server;
  } else if (role == QStringLiteral("client")) {
    reply.role = Role::Client;
  } else if (role == QStringLiteral("init")) {
    reply.role = Role::Init;
  } else {
    return reply; // not a status reply
  }
  reply.valid = true;
  reply.serverAddress = object[QStringLiteral("server_ip")].toString().toStdString();
  reply.name = object[QStringLiteral("name")].toString().toStdString();
  reply.meshVersion = object[QStringLiteral("mesh_version")].toInt();
  if (const auto mismatches = object[QStringLiteral("version_mismatch")]; mismatches.isArray()) {
    const QJsonArray array = mismatches.toArray();
    reply.versionMismatchPeers.reserve(array.size());
    for (const auto &value : array) {
      if (value.isString()) {
        reply.versionMismatchPeers.push_back(value.toString().toStdString());
      }
    }
  }
  return reply;
}

} // namespace deskflow::coordination::protocol
