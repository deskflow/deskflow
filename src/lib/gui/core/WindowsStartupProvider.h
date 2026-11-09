// SPDX-FileCopyrightText: (C) 2026 Deskflow Developers
// SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
#pragma once
#include <QByteArray>
#include <QString>
#include <memory>

namespace deskflow::gui::winstartup {
struct State
{
  bool available = false, exists = false, conflict = false;
  QString userSid, executable, error;
  QByteArray executableHash, definition;
  QByteArray revision() const;
};
struct Options
{
  bool login = false, recovery = false;
};
struct Plan
{
  State before;
  Options options;
  QByteArray definition, digest;
  QString error;
};
enum class Effect
{
  Applied,
  Unchanged,
  Uncertain
};
class System
{
public:
  virtual ~System() = default;
  virtual State read() = 0;
  // Empty definition removes only the exact Candidate task. No process is killed.
  virtual Effect replace(const QByteArray &definition, const QByteArray &expectedRevision) = 0;
};
struct Result
{
  bool success = false, rolledBack = false, uncertain = false;
  QString error;
};
Plan prepare(const State &, Options);
Result apply(const Plan &, const QByteArray &confirmation, System &);
Result applyRecorded(const Plan &, const QByteArray &, System &, const QString &backupFile);
Plan prepareRestore(const State &, const QString &backupFile);
Options observedOptions(const State &);
// Native adapter uses the current user's interactive token, not stored credentials.
// Callers must present the exact preview digest to apply(); tests inject System.
std::unique_ptr<System> makeNativeSystem(const QString &executable);
bool mayLaunch(bool paused, bool resume);
} // namespace deskflow::gui::winstartup
