/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2026 Deskflow Developers
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#include "coordination/CoordinationProtocol.h"

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

  if (message.type == Message::Type::KeyFwd) {
    message.name = object[QStringLiteral("from")].toString().toStdString();
    const QString phase = object[QStringLiteral("phase")].toString();
    if (phase == QStringLiteral("up")) {
      message.keyPhase = Message::KeyPhase::Up;
    } else if (phase == QStringLiteral("repeat")) {
      message.keyPhase = Message::KeyPhase::Repeat;
    } else {
      message.keyPhase = Message::KeyPhase::Down;
    }
    message.keyId = static_cast<uint16_t>(object[QStringLiteral("id")].toInt());
    message.keyMask = static_cast<uint16_t>(object[QStringLiteral("mask")].toInt());
    message.keyButton = static_cast<uint16_t>(object[QStringLiteral("button")].toInt());
    message.keyLang = object[QStringLiteral("lang")].toString().toStdString();
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
    const std::string &from, Message::KeyPhase phase, uint16_t id, uint16_t mask, uint16_t button,
    const std::string &lang, const std::string &token
)
{
  QJsonObject object;
  object[QStringLiteral("t")] = QStringLiteral("keyfwd");
  object[QStringLiteral("from")] = QString::fromStdString(from);
  switch (phase) {
  case Message::KeyPhase::Up:
    object[QStringLiteral("phase")] = QStringLiteral("up");
    break;
  case Message::KeyPhase::Repeat:
    object[QStringLiteral("phase")] = QStringLiteral("repeat");
    break;
  default:
    object[QStringLiteral("phase")] = QStringLiteral("down");
    break;
  }
  object[QStringLiteral("id")] = id;
  object[QStringLiteral("mask")] = mask;
  object[QStringLiteral("button")] = button;
  object[QStringLiteral("lang")] = QString::fromStdString(lang);
  putToken(object, token);
  return serialize(object);
}

std::string encodeStatusReply(
    Role role, const std::string &serverAddress, int64_t seq, double lastSwitchAt, const std::string &name
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
  return reply;
}

} // namespace deskflow::coordination::protocol
