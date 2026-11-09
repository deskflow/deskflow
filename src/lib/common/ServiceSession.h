// SPDX-FileCopyrightText: (C) 2026 Deskflow Developers
// SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
#pragma once
#include "CandidatePolicy.h"
#include "PeerAuthentication.h"
namespace deskflow::candidate {
// One atomic request binds the file and expected identity to one GUI generation.
// Never persist this transient context or reconstruct it from untrusted logs.
struct ServiceSession
{
  QString settings;
  deskflow::auth::Context context;
  bool valid() const
  {
    return settings.size() < 1024 && settingsPath(settings) && !settings.contains('"') && !settings.contains('\n') &&
           !settings.contains('\r') && context.valid();
  }
  QString encode() const
  {
    return valid() ? deskflow::auth::pack({{"schema", 1}, {"settings", settings}, {"context", context.encode()}})
                   : QString();
  }
  static ServiceSession decode(const QString &encoded)
  {
    const auto o = deskflow::auth::unpack(encoded);
    ServiceSession s{o["settings"].toString(), deskflow::auth::Context::decode(o["context"].toString())};
    return o.size() == 3 && o["schema"] == 1 && s.valid() && s.encode() == encoded ? s : ServiceSession{};
  }
  QString arguments() const
  {
    return valid() ? QString("client --settings \"%1\" --peer-auth-context %2").arg(settings, context.encode())
                   : QString();
  }
};
} // namespace deskflow::candidate
