// SPDX-FileCopyrightText: (C) 2026 Deskflow Developers
// SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
#pragma once

#include <QByteArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QRegularExpression>
#include <QString>
#include <atomic>
#include <mutex>
#include <optional>

namespace deskflow::auth {
// This is correlation metadata, never a trust database or a TLS bypass.
inline QString fingerprint(QString value)
{
  static const QRegularExpression plain("\\A[0-9a-fA-F]{64}\\z");
  static const QRegularExpression colon("\\A(?:[0-9a-fA-F]{2}:){31}[0-9a-fA-F]{2}\\z");
  if (!plain.match(value).hasMatch() && !colon.match(value).hasMatch())
    return {};
  return value.remove(':').toLower();
}
inline bool identifier(const QString &s)
{
  static const QRegularExpression re("\\A[0-9a-f]{32}\\z");
  return re.match(s).hasMatch();
}
inline quint64 counter(const QJsonValue &v)
{
  if (!v.isString())
    return 0;
  bool ok = false;
  auto n = v.toString().toULongLong(&ok);
  return ok && n && QString::number(n) == v.toString() ? n : 0;
}
inline QString pack(const QJsonObject &o)
{
  return QString::fromLatin1(QJsonDocument(o)
                                 .toJson(QJsonDocument::Compact)
                                 .toBase64(QByteArray::Base64UrlEncoding | QByteArray::OmitTrailingEquals));
}
inline QJsonObject unpack(const QString &s)
{
  static const QRegularExpression chars("\\A[A-Za-z0-9_-]+\\z");
  if (s.size() > 2048 || !chars.match(s).hasMatch())
    return {};
  const auto bytes =
      QByteArray::fromBase64(s.toLatin1(), QByteArray::Base64UrlEncoding | QByteArray::AbortOnBase64DecodingErrors);
  const auto d = QJsonDocument::fromJson(bytes);
  if (!d.isObject() || pack(d.object()) != s)
    return {};
  return d.object();
}
struct Context
{
  QString session, expected;
  quint64 generation = 0;
  bool valid() const
  {
    return identifier(session) && generation && (expected.isEmpty() || fingerprint(expected) == expected);
  }
  bool operator==(const Context &) const = default;
  QJsonObject json() const
  {
    return {{"schema", 1}, {"session", session}, {"generation", QString::number(generation)}, {"expected", expected}};
  }
  QString encode() const
  {
    return valid() ? pack(json()) : QString();
  }
  static Context decode(const QString &s)
  {
    const auto o = unpack(s);
    Context c{o["session"].toString(), o["expected"].toString(), counter(o["generation"])};
    return o.size() == 4 && o["schema"] == 1 && o["expected"].isString() && c.valid() ? c : Context{};
  }
};
enum class Outcome
{
  Started,
  Verified,
  WrongIdentity,
  Untrusted,
  MissingCertificate,
  SecurityDisabled,
  Cancelled
};
inline QString code(Outcome o)
{
  switch (o) {
  case Outcome::Started:
    return "started";
  case Outcome::Verified:
    return "verified";
  case Outcome::WrongIdentity:
    return "wrong-identity";
  case Outcome::Untrusted:
    return "untrusted";
  case Outcome::MissingCertificate:
    return "missing-certificate";
  case Outcome::SecurityDisabled:
    return "security-disabled";
  case Outcome::Cancelled:
    return "cancelled";
  }
  return {};
}
struct Report
{
  Context context;
  quint64 sequence = 0;
  Outcome outcome = Outcome::Cancelled;
  QString observed;
  bool valid() const
  {
    if (!context.valid() || !sequence)
      return false;
    if (!observed.isEmpty() && fingerprint(observed) != observed)
      return false;
    if (outcome == Outcome::Verified)
      return !observed.isEmpty() && (context.expected.isEmpty() || context.expected == observed);
    if (outcome == Outcome::WrongIdentity)
      return !context.expected.isEmpty() && !observed.isEmpty() && context.expected != observed;
    return outcome != Outcome::Untrusted || !observed.isEmpty();
  }
  QString encode() const
  {
    if (!valid())
      return {};
    auto o = context.json();
    o["sequence"] = QString::number(sequence);
    o["outcome"] = code(outcome);
    o["observed"] = observed;
    return pack(o);
  }
  static std::optional<Report> decode(const QString &s)
  {
    auto o = unpack(s);
    if (o.size() != 7 || o["schema"] != 1 || !o["expected"].isString() || !o["observed"].isString())
      return {};
    std::optional<Outcome> outcome;
    for (auto x :
         {Outcome::Started, Outcome::Verified, Outcome::WrongIdentity, Outcome::Untrusted, Outcome::MissingCertificate,
          Outcome::SecurityDisabled, Outcome::Cancelled})
      if (o["outcome"] == code(x))
        outcome = x;
    if (!outcome)
      return {};
    Report r{
        {o["session"].toString(), o["expected"].toString(), counter(o["generation"])},
        counter(o["sequence"]),
        *outcome,
        o["observed"].toString()
    };
    return r.valid() ? std::optional(r) : std::nullopt;
  }
};
// Called by SecureSocket only after its TLS handshake, with the actual peer
// certificate digest and the existing FingerprintDatabase decision.
inline Outcome evaluate(const Context &context, const QString &observed, bool tls, bool peerChecks, bool trusted)
{
  const auto fp = fingerprint(observed);
  if (!tls || !peerChecks)
    return Outcome::SecurityDisabled;
  if (fp.isEmpty())
    return Outcome::MissingCertificate;
  if (!context.expected.isEmpty() && (!context.valid() || fp != context.expected))
    return Outcome::WrongIdentity;
  return trusted ? Outcome::Verified : Outcome::Untrusted;
}
// GUI gate. Started must precede a terminal event; each sequence is consumed
// once. A terminal rejection prevents later events from reviving this run.
class Gate
{
public:
  bool begin(const Context &context)
  {
    cancel();
    if (!context.valid())
      return false;
    m_context = context;
    m_active = true;
    return true;
  }
  void cancel()
  {
    m_active = false;
    m_sequence = 0;
    m_terminal = true;
  }
  // Close this socket's sequence, while permitting a later Started sequence
  // in the same core process. A late terminal report cannot revive it.
  void endAttempt()
  {
    m_terminal = true;
  }
  bool consume(const Report &r)
  {
    if (!m_active || !r.valid() || r.context != m_context)
      return false;
    if (r.outcome == Outcome::Started) {
      if (r.sequence <= m_sequence)
        return false;
      m_sequence = r.sequence;
      m_terminal = false;
      return true;
    }
    if (r.sequence != m_sequence || m_terminal)
      return false;
    m_terminal = true;
    if (r.outcome != Outcome::Verified)
      m_active = false;
    return true;
  }
  const Context &context() const
  {
    return m_context;
  }

private:
  Context m_context;
  quint64 m_sequence = 0;
  bool m_active = false, m_terminal = true;
};
// Immutable per core process. No environment variable, settings or remote IPC
// can replace it. A new GUI process launch must explicitly provide a new value.
class Runtime
{
public:
  static bool activate(const Context &context)
  {
    std::lock_guard lock(m_mutex);
    if (m_context || !context.valid())
      return false;
    m_context = context;
    return true;
  }
  static Report capture()
  {
    std::lock_guard lock(m_mutex);
    return m_context ? Report{*m_context, ++m_sequence, Outcome::Started, {}} : Report{};
  }
  static void reject()
  {
    m_rejected = true;
  }
  static bool retryAllowed()
  {
    return !m_rejected.load();
  }

private:
  inline static std::mutex m_mutex;
  inline static std::optional<Context> m_context;
  inline static quint64 m_sequence = 0;
  inline static std::atomic_bool m_rejected{false};
};
} // namespace deskflow::auth
Q_DECLARE_METATYPE(deskflow::auth::Context)
Q_DECLARE_METATYPE(deskflow::auth::Report)
