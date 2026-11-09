// SPDX-FileCopyrightText: (C) 2026 Deskflow Developers
// SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
#include "IdentityBindings.h"
#include <QJsonArray>
#include <QJsonDocument>
#include <QRegularExpression>

namespace deskflow::gui {
QString IdentityBindings::normalizedFingerprint(const QString &value)
{
  static const QRegularExpression plain("\\A[0-9a-fA-F]{64}\\z");
  static const QRegularExpression colon("\\A(?:[0-9a-fA-F]{2}:){31}[0-9a-fA-F]{2}\\z");
  if (!plain.match(value).hasMatch() && !colon.match(value).hasMatch())
    return {};
  return QString(value).remove(':').toLower();
}
bool IdentityBindings::validLabel(const QString &s)
{
  if (s.trimmed().isEmpty() || s.toUcs4().size() > 80)
    return false;
  for (auto c : s.toUcs4()) {
    const auto type = QChar::category(c);
    if (type == QChar::Other_Control || type == QChar::Other_Format || type == QChar::Other_Surrogate ||
        type == QChar::Separator_Line || type == QChar::Separator_Paragraph || c == 0xfffd)
      return false;
  }
  return true;
}
bool IdentityBindings::validEndpoint(const Endpoint &e)
{
  if (e.port < 1 || e.port > 65535 || e.host.size() > 254)
    return false;
  static const QRegularExpression local("\\A(?:[a-zA-Z0-9](?:[a-zA-Z0-9-]{0,61}[a-zA-Z0-9])?\\.)+local\\z");
  if (local.match(e.host).hasMatch())
    return true;
  const auto parts = e.host.split('.');
  if (parts.size() != 4)
    return false;
  int n[4];
  for (int i = 0; i < 4; ++i) {
    bool ok;
    n[i] = parts[i].toInt(&ok);
    if (!ok || n[i] < 0 || n[i] > 255 || QString::number(n[i]) != parts[i])
      return false;
  }
  return (n[0] == 10 || (n[0] == 192 && n[1] == 168) || (n[0] == 172 && n[1] >= 16 && n[1] <= 31) ||
          (n[0] == 169 && n[1] == 254)) &&
         n[3] != 0 && n[3] != 255;
}
bool IdentityBindings::load(const QByteArray &bytes)
{
  if (bytes.size() > 32768)
    return false;
  QJsonParseError error;
  const auto doc = QJsonDocument::fromJson(bytes, &error);
  if (error.error != QJsonParseError::NoError || !doc.isObject() || doc.object().size() != 2 || doc["schema"] != 1 ||
      !doc["devices"].isArray())
    return false;
  const auto array = doc["devices"].toArray();
  if (array.size() > 16)
    return false;
  QMap<QString, Device> next;
  for (const auto &v : array) {
    if (!v.isObject())
      return false;
    const auto o = v.toObject();
    if (o.size() != 4 || !o["port"].isDouble() || o["port"].toDouble() != o["port"].toInt(-1))
      return false;
    const auto fp = normalizedFingerprint(o["sha256"].toString());
    Device d{fp, o["label"].toString(), {o["host"].toString(), o["port"].toInt(-1)}};
    if (fp.isEmpty() || !validLabel(d.label) || !validEndpoint(d.endpoint) || next.contains(fp))
      return false;
    next.insert(fp, d);
  }
  m_devices = next;
  cancel();
  return true;
}
QByteArray IdentityBindings::encode() const
{
  QJsonArray a;
  for (const auto &d : m_devices)
    a.append(
        QJsonObject{{"sha256", d.sha256}, {"label", d.label}, {"host", d.endpoint.host}, {"port", d.endpoint.port}}
    );
  return QJsonDocument(QJsonObject{{"schema", 1}, {"devices", a}}).toJson(QJsonDocument::Compact);
}
quint64 IdentityBindings::begin(const Endpoint &endpoint, const QString &expected)
{
  cancel();
  m_attempt = {m_generation, endpoint, normalizedFingerprint(expected), {}, Review::Invalid};
  if (!validEndpoint(endpoint) ||
      (!expected.isEmpty() && (m_attempt.expected.isEmpty() || !m_devices.contains(m_attempt.expected)))) {
    cancel();
    return 0;
  }
  return m_generation;
}
IdentityBindings::Review IdentityBindings::observe(quint64 generation, const QString &fingerprint)
{
  if (!generation || generation != m_attempt.generation || !validEndpoint(m_attempt.endpoint))
    return Review::Invalid;
  const auto fp = normalizedFingerprint(fingerprint);
  // A second differing observation invalidates confirmation; do not conflate sessions.
  if (m_attempt.observed != fp) {
    m_confirmed = false;
    m_label.clear();
  }
  m_attempt.observed = fp;
  if (fp.isEmpty())
    return m_attempt.review = Review::Invalid;
  if (!m_attempt.expected.isEmpty() && fp != m_attempt.expected)
    return m_attempt.review = Review::ChangedIdentity;
  return m_attempt.review = m_devices.contains(fp) ? Review::KnownIdentity : Review::VerifyNew;
}
bool IdentityBindings::confirm(quint64 generation, const QString &label, bool compared)
{
  if (!generation || generation != m_attempt.generation || !compared || !validLabel(label) ||
      m_attempt.review != Review::VerifyNew || m_devices.size() >= 16)
    return false;
  m_label = label;
  m_confirmed = true;
  return true;
}
bool IdentityBindings::authenticated(quint64 generation, const QString &fingerprint, bool tls, bool peers)
{
  const auto fp = normalizedFingerprint(fingerprint);
  if (!generation || generation != m_attempt.generation || !tls || !peers || fp.isEmpty() || fp != m_attempt.observed ||
      m_attempt.review == Review::ChangedIdentity || m_attempt.review == Review::Invalid ||
      (!m_devices.contains(fp) && !m_confirmed))
    return false;
  auto label = m_devices.contains(fp) ? m_devices[fp].label : m_label;
  if (m_devices.contains(fp) && label == m_devices[fp].endpoint.host.left(80))
    label = m_attempt.endpoint.host.left(80);
  m_devices[fp] = {fp, label, m_attempt.endpoint};
  cancel();
  return true;
}
void IdentityBindings::cancel()
{
  ++m_generation;
  m_attempt = {};
  m_confirmed = false;
  m_label.clear();
  m_authGate.cancel();
}
bool IdentityBindings::rename(const QString &value, const QString &label)
{
  const auto fp = normalizedFingerprint(value);
  if (!m_devices.contains(fp) || !validLabel(label))
    return false;
  m_devices[fp].label = label;
  return true;
}
QList<IdentityBindings::Device> IdentityBindings::atEndpoint(const Endpoint &endpoint) const
{
  QList<Device> result;
  if (!validEndpoint(endpoint))
    return result;
  for (const auto &device : m_devices)
    if (device.endpoint == endpoint)
      result.append(device);
  return result;
}
std::optional<IdentityBindings::Device>
IdentityBindings::selectedDevice(const Endpoint &endpoint, const QString &selected) const
{
  const auto matches = atEndpoint(endpoint);
  if (!selected.isEmpty()) {
    const auto fingerprint = normalizedFingerprint(selected);
    for (const auto &device : matches)
      if (device.sha256 == fingerprint)
        return device;
    return {};
  }
  return matches.size() == 1 ? std::optional(matches.first()) : std::nullopt;
}
bool IdentityBindings::updateDefaultLabel(const QString &value, const QString &reportedName)
{
  const auto fp = normalizedFingerprint(value);
  if (!m_devices.contains(fp) || !validLabel(reportedName))
    return false;
  auto &device = m_devices[fp];
  if (device.label != device.endpoint.host.left(80) || device.label == reportedName)
    return false;
  device.label = reportedName;
  return true;
}
bool IdentityBindings::beginAuthentication(const deskflow::auth::Context &context, const Endpoint &endpoint)
{
  if (!context.valid() || !begin(endpoint, context.expected))
    return false;
  return m_authGate.begin(context);
}
bool IdentityBindings::consumeAuthentication(const deskflow::auth::Report &report)
{
  if (!m_authGate.consume(report))
    return false;
  if (report.outcome == deskflow::auth::Outcome::Started)
    return false;
  const auto generation = m_attempt.generation;
  observe(generation, report.observed);
  if (report.outcome != deskflow::auth::Outcome::Verified) {
    m_confirmed = false;
    return false;
  }
  return authenticated(generation, report.observed, true, true);
}
bool IdentityBindings::beginReviewedAuthentication(
    const deskflow::auth::Context &context, const Endpoint &endpoint, const QString &label
)
{
  // Called only after explicit UI comparison AND successful existing trust-DB
  // write. This pending approval is memory-only and is consumed by one launch.
  if (!context.valid() || context.expected.isEmpty() || !validLabel(label) || !begin(endpoint))
    return false;
  m_attempt.expected = context.expected;
  observe(m_attempt.generation, context.expected);
  if (m_attempt.review == Review::VerifyNew && !confirm(m_attempt.generation, label, true))
    return false;
  return m_authGate.begin(context);
}
bool IdentityBindings::forget(const QString &fp)
{
  cancel();
  return m_devices.remove(normalizedFingerprint(fp)) > 0;
}
} // namespace deskflow::gui
