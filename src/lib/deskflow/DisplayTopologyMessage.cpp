// SPDX-FileCopyrightText: (C) 2026 Deskflow Developers
// SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
#include "DisplayTopologyMessage.h"
#include "DeskflowException.h"
#include "ProtocolUtil.h"
#include "common/DesktopWorkspace.h"
#include <QJsonDocument>
#include <QJsonObject>
#include <QRegularExpression>
namespace deskflow::display {
void writeTopologyMessage(IStream *stream, const QString &device, const Topology &topology, const QByteArray &workspace)
{
  auto bytes = topology.encode();
  // Schema-1 topology readers ignore unknown optional object fields. Keep the
  // same frame/version and omit this read-only extension when unavailable.
  const auto snapshot = WorkspaceSnapshot::decode(workspace);
  if (!bytes.isEmpty() && snapshot && snapshot->devices.contains(device) && snapshot->topologies.contains(device) &&
      snapshot->topologies[device].encode() == bytes) {
    auto object = QJsonDocument::fromJson(bytes).object();
    object["workspace"] = QJsonDocument::fromJson(workspace).object();
    const auto extended = QJsonDocument(object).toJson(QJsonDocument::Compact);
    if (extended.size() <= 16384)
      bytes = extended;
  }
  const auto name = device.toStdString(), payload = bytes.toStdString();
  ProtocolUtil::writef(stream, topologyMessage, &name, &payload);
}
Message readTopologyMessage(IStream *stream, const QString &expectedDevice)
{
  std::string name, payload;
  if (!ProtocolUtil::readf(stream, "%s%s", &name, &payload) || name.size() > 255 || payload.size() > 16384)
    throw BadClientException();
  const auto device = QString::fromStdString(name);
  static const QRegularExpression validName("^[a-zA-Z0-9_.-]{1,255}$");
  if (!validName.match(device).hasMatch() || (!expectedDevice.isEmpty() && device != expectedDevice))
    throw BadClientException();
  const auto topology = Topology::decode(QByteArray::fromStdString(payload));
  if (!payload.empty() && !topology)
    throw BadClientException();
  QByteArray workspace;
  const auto object = QJsonDocument::fromJson(QByteArray::fromStdString(payload)).object();
  if (object.contains("workspace")) {
    if (!object["workspace"].isObject())
      throw BadClientException();
    workspace = QJsonDocument(object["workspace"].toObject()).toJson(QJsonDocument::Compact);
    const auto snapshot = WorkspaceSnapshot::decode(workspace);
    if (!snapshot || !snapshot->devices.contains(device) || !topology || !snapshot->topologies.contains(device) ||
        snapshot->topologies[device].encode() != topology->encode())
      throw BadClientException();
  }
  return {device, topology, workspace};
}
} // namespace deskflow::display
