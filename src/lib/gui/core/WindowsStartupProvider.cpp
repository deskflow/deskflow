// SPDX-FileCopyrightText: (C) 2026 Deskflow Developers
// SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
#include "WindowsStartupProvider.h"
#include <QCryptographicHash>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QJsonDocument>
#include <QJsonObject>
#include <QRegularExpression>
#include <QSaveFile>
#include <QSettings>
#include <QXmlStreamReader>
#include <QXmlStreamWriter>
#include <algorithm>
#ifdef Q_OS_WIN
#include <sddl.h>
#include <taskschd.h>
#include <vector>
#include <windows.h>
#include <wrl/client.h>
#endif

namespace deskflow::gui::winstartup {
namespace {
QByteArray hash(const QByteArray &v)
{
  return QCryptographicHash::hash(v, QCryptographicHash::Sha256).toHex();
}
constexpr auto owner = "DeskflowCandidate/user-startup/v1";
QByteArray normalized(const QByteArray &xml)
{
  if (xml.isEmpty())
    return {};
  QXmlStreamReader r(QString::fromUtf8(xml));
  QStringList path, fields;
  const QMap<QString, QString> defaults{
      {"Task/Settings/AllowStartOnDemand", "true"},
      {"Task/Settings/Enabled", "true"},
      {"Task/Settings/Hidden", "false"},
      {"Task/Settings/WakeToRun", "false"},
      {"Task/Settings/Priority", "7"},
      {"Task/Settings/StartWhenAvailable", "false"},
      {"Task/Settings/RunOnlyIfNetworkAvailable", "false"},
      {"Task/Settings/RunOnlyIfIdle", "false"},
      {"Task/Settings/AllowHardTerminate", "true"},
      {"Task/Settings/IdleSettings/StopOnIdleEnd", "true"},
      {"Task/Settings/IdleSettings/RestartOnIdle", "false"},
      {"Task/Settings/UseUnifiedSchedulingEngine", "false"},
      {"Task/Settings/DisallowStartOnRemoteAppSession", "false"}
  };
  while (!r.atEnd()) {
    r.readNext();
    if (r.isStartElement()) {
      path.append(r.name().toString());
      const auto key = path.join('/');
      if (r.name() == QStringLiteral("Exec") || r.name() == QStringLiteral("Principal") ||
          r.name() == QStringLiteral("LogonTrigger"))
        fields << key + "#";
      for (const auto &attribute : r.attributes())
        if (!(path.size() == 1 && attribute.name() == QStringLiteral("version")))
          fields << key + "@" + attribute.name().toString() + "=" + attribute.value().toString();
    } else if (r.isEndElement())
      path.removeLast();
    else if (r.isCharacters() && !r.isWhitespace()) {
      const auto key = path.join('/'), value = r.text().toString();
      if (key == "Task/RegistrationInfo/Date" || key == "Task/RegistrationInfo/Author")
        continue;
      if (defaults.contains(key) && defaults[key] == value)
        continue;
      fields << key + "=" + value;
    }
  }
  if (r.hasError())
    return "invalid";
  std::sort(fields.begin(), fields.end());
  return fields.join('\n').toUtf8();
}
bool owned(const QByteArray &xml)
{
  QXmlStreamReader r(QString::fromUtf8(xml));
  bool marker = false;
  while (!r.atEnd()) {
    r.readNext();
    if (r.isStartElement() && r.name() == QStringLiteral("URI"))
      marker = r.readElementText() == owner;
  }
  return !r.hasError() && marker;
}
QByteArray definition(const State &s, Options o)
{
  if (!o.login)
    return {};
  QByteArray xml;
  QXmlStreamWriter w(&xml);
  w.writeStartDocument();
  w.writeStartElement("Task");
  w.writeAttribute("version", "1.2");
  w.writeDefaultNamespace("http://schemas.microsoft.com/windows/2004/02/mit/task");
  w.writeStartElement("RegistrationInfo");
  w.writeTextElement("URI", owner);
  w.writeEndElement();
  w.writeStartElement("Triggers");
  w.writeStartElement("LogonTrigger");
  w.writeTextElement("Enabled", "true");
  w.writeTextElement("UserId", s.userSid);
  w.writeEndElement();
  w.writeEndElement();
  w.writeStartElement("Principals");
  w.writeStartElement("Principal");
  w.writeAttribute("id", "User");
  w.writeTextElement("UserId", s.userSid);
  w.writeTextElement("LogonType", "InteractiveToken");
  w.writeTextElement("RunLevel", "LeastPrivilege");
  w.writeEndElement();
  w.writeEndElement();
  w.writeStartElement("Settings");
  w.writeTextElement("MultipleInstancesPolicy", "IgnoreNew");
  w.writeTextElement("DisallowStartIfOnBatteries", "false");
  w.writeTextElement("StopIfGoingOnBatteries", "false");
  w.writeTextElement("ExecutionTimeLimit", "PT0S");
  if (o.recovery) {
    w.writeStartElement("RestartOnFailure");
    w.writeTextElement("Interval", "PT1M");
    w.writeTextElement("Count", "3");
    w.writeEndElement();
  }
  w.writeEndElement();
  w.writeStartElement("Actions");
  w.writeAttribute("Context", "User");
  w.writeStartElement("Exec");
  w.writeTextElement("Command", s.executable);
  w.writeTextElement("Arguments", "--managed-start");
  w.writeTextElement("WorkingDirectory", QFileInfo(s.executable).absolutePath());
  w.writeEndElement();
  w.writeEndElement();
  w.writeEndElement();
  w.writeEndDocument();
  return xml;
}
} // namespace
QByteArray State::revision() const
{
  return hash(QJsonDocument(
                  QJsonObject{
                      {"sid", userSid},
                      {"exe", executable},
                      {"hash", QString::fromLatin1(executableHash)},
                      {"definition", QString::fromLatin1(hash(definition))},
                      {"exists", exists},
                      {"conflict", conflict},
                      {"available", available},
                      {"error", error}
                  }
  ).toJson(QJsonDocument::Compact));
}
Plan prepare(const State &s, Options o)
{
  Plan p;
  p.before = s;
  p.options = o;
  if (!s.available || !s.error.isEmpty())
    p.error = "startup-status-unavailable";
  else if (s.conflict || (s.exists && !owned(s.definition)))
    p.error = "existing-startup-conflict";
  else if (!QRegularExpression("^S-1-[0-9-]+$").match(s.userSid).hasMatch() || !QFileInfo(s.executable).isAbsolute() ||
           s.executableHash.size() != 64)
    p.error = "application-identity-unavailable";
  else if (o.recovery && !o.login)
    p.error = "recovery-requires-login";
  if (!p.error.isEmpty())
    return p;
  p.definition = definition(s, o);
  p.digest = hash(s.revision() + p.definition);
  return p;
}
Result apply(const Plan &p, const QByteArray &confirmation, System &system)
{
  const auto canonical = prepare(p.before, p.options);
  if (!canonical.error.isEmpty() || canonical.definition != p.definition || canonical.digest != p.digest)
    return {false, false, false, "invalid-startup-plan"};
  if (!p.error.isEmpty() || p.digest.isEmpty() || confirmation != p.digest ||
      p.digest != hash(p.before.revision() + p.definition))
    return {false, false, false, "confirmation-required"};
  const auto before = system.read();
  if (before.revision() != p.before.revision())
    return {false, false, false, "startup-state-changed"};
  if (normalized(before.definition) == normalized(p.definition))
    return {true, false, false, {}};
  const auto effect = system.replace(p.definition, before.revision());
  if (effect == Effect::Uncertain)
    return {false, false, true, "startup-result-uncertain"};
  auto after = system.read();
  if (effect == Effect::Applied && after.available && after.error.isEmpty() &&
      normalized(after.definition) == normalized(p.definition))
    return {true, false, false, {}};
  if (effect == Effect::Unchanged && after.revision() == before.revision())
    return {false, false, false, "startup-change-failed-no-change"};
  // Never overwrite an intervening or unknown change with a guessed rollback.
  if (!after.available || !after.error.isEmpty() || normalized(after.definition) != normalized(p.definition))
    return {false, false, true, "startup-readback-mismatch"};
  const auto restored = system.replace(before.definition, after.revision());
  after = system.read();
  const bool ok = restored == Effect::Applied && after.revision() == before.revision();
  return {false, ok, !ok, ok ? "startup-change-rolled-back" : "startup-rollback-unverified"};
}
Options observedOptions(const State &s)
{
  return {!s.definition.isEmpty(), s.definition.contains("RestartOnFailure")};
}
Result applyRecorded(const Plan &p, const QByteArray &confirmation, System &system, const QString &backupFile)
{
  const auto canonical = prepare(p.before, p.options);
  if (!canonical.error.isEmpty() || canonical.digest != confirmation || canonical.definition != p.definition ||
      system.read().revision() != p.before.revision())
    return {false, false, false, "startup-state-changed"};
  if (normalized(p.before.definition) == normalized(p.definition))
    return {true, false, false, {}};
  const QFileInfo target(backupFile);
  if (!target.isAbsolute() || target.isSymLink() || QFileInfo(target.absolutePath()).isSymLink() ||
      !QDir().mkpath(target.absolutePath()))
    return {false, false, false, "startup-backup-unavailable"};
  const QJsonObject record{
      {"schema", 1},
      {"sid", p.before.userSid},
      {"exe", p.before.executable},
      {"exeHash", QString::fromLatin1(p.before.executableHash)},
      {"before", QString::fromLatin1(p.before.definition.toBase64())},
      {"after", QString::fromLatin1(p.definition.toBase64())}
  };
  QSaveFile file(backupFile);
  const auto bytes = QJsonDocument(record).toJson(QJsonDocument::Compact);
  if (!file.open(QIODevice::WriteOnly) || file.write(bytes) != bytes.size() || !file.commit())
    return {false, false, false, "startup-backup-failed"};
  return apply(p, confirmation, system);
}
Plan prepareRestore(const State &current, const QString &backupFile)
{
  Plan rejected;
  rejected.error = "startup-backup-not-restorable";
  QFile f(backupFile);
  if (QFileInfo(f).isSymLink() || f.size() > 262144 || !f.open(QIODevice::ReadOnly))
    return rejected;
  const auto record = QJsonDocument::fromJson(f.readAll()).object();
  if (record.size() != 6 || record["schema"].toInt() != 1 || record["sid"].toString() != current.userSid ||
      record["exe"].toString() != current.executable ||
      record["exeHash"].toString().toLatin1() != current.executableHash)
    return rejected;
  const auto previous = QByteArray::fromBase64(record["before"].toString().toLatin1());
  const auto expected = QByteArray::fromBase64(record["after"].toString().toLatin1());
  if (normalized(current.definition) != normalized(expected))
    return rejected;
  State old = current;
  old.definition = previous;
  old.exists = !previous.isEmpty();
  old.conflict = false;
  auto plan = prepare(current, observedOptions(old));
  if (!plan.error.isEmpty() || normalized(plan.definition) != normalized(previous))
    return rejected;
  return plan;
}
bool mayLaunch(bool paused, bool resume)
{
  return !paused && resume;
}

#ifdef Q_OS_WIN
namespace {
using Microsoft::WRL::ComPtr;
struct Bstr
{
  BSTR value;
  explicit Bstr(const QString &s) : value(SysAllocString(reinterpret_cast<const wchar_t *>(s.utf16())))
  {
  }
  ~Bstr()
  {
    SysFreeString(value);
  }
  operator BSTR() const
  {
    return value;
  }
};
struct Session
{
  HRESULT init = CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED), error = E_FAIL;
  ComPtr<ITaskService> service;
  ComPtr<ITaskFolder> root;
  Session()
  {
    if (FAILED(init) && init != RPC_E_CHANGED_MODE) {
      error = init;
      return;
    }
    error = CoCreateInstance(CLSID_TaskScheduler, nullptr, CLSCTX_INPROC_SERVER, IID_PPV_ARGS(&service));
    if (FAILED(error))
      return;
    VARIANT v;
    VariantInit(&v);
    error = service->Connect(v, v, v, v);
    if (SUCCEEDED(error))
      error = service->GetFolder(Bstr("\\"), &root);
  }
  ~Session()
  {
    root.Reset();
    service.Reset();
    if (SUCCEEDED(init))
      CoUninitialize();
  }
};
QString sid()
{
  HANDLE token = nullptr;
  if (!OpenProcessToken(GetCurrentProcess(), TOKEN_QUERY, &token))
    return {};
  DWORD size = 0;
  GetTokenInformation(token, TokenUser, nullptr, 0, &size);
  std::vector<char> bytes(size);
  const bool ok = GetTokenInformation(token, TokenUser, bytes.data(), size, &size);
  CloseHandle(token);
  if (!ok)
    return {};
  LPWSTR text = nullptr;
  if (!ConvertSidToStringSidW(reinterpret_cast<TOKEN_USER *>(bytes.data())->User.Sid, &text))
    return {};
  const auto result = QString::fromWCharArray(text);
  LocalFree(text);
  return result;
}
QString task(const QString &user)
{
  return "DeskflowCandidate-" + user;
}
QString failure(HRESULT value)
{
  return QString("task-scheduler-0x%1").arg(quint32(value), 8, 16, QChar('0'));
}
class NativeSystem final : public System
{
  QString executable;

public:
  explicit NativeSystem(QString path) : executable(std::move(path))
  {
  }
  State read() override
  {
    State s;
    s.executable = executable;
    s.userSid = sid();
    QFile f(executable);
    QCryptographicHash digest(QCryptographicHash::Sha256);
    if (s.userSid.isEmpty() || QFileInfo(executable).isSymLink() || !f.open(QIODevice::ReadOnly) ||
        !digest.addData(&f)) {
      s.error = "application-identity-unavailable";
      return s;
    }
    s.executableHash = digest.result().toHex();
    QSettings run("HKEY_CURRENT_USER\\Software\\Microsoft\\Windows\\CurrentVersion\\Run", QSettings::NativeFormat);
    for (const auto &name : {QString("Deskflow"), QString("DeskflowCandidate")})
      if (run.contains(name))
        s.conflict = true;
    if (run.status() != QSettings::NoError) {
      s.error = "legacy-login-status-unavailable";
      return s;
    }
    SC_HANDLE manager = OpenSCManagerW(nullptr, nullptr, SC_MANAGER_CONNECT);
    if (!manager) {
      s.error = "legacy-service-status-unavailable";
      return s;
    }
    SC_HANDLE legacy = OpenServiceW(manager, L"Deskflow", SERVICE_QUERY_STATUS);
    if (legacy) {
      s.conflict = true;
      CloseServiceHandle(legacy);
    } else if (GetLastError() != ERROR_SERVICE_DOES_NOT_EXIST) {
      CloseServiceHandle(manager);
      s.error = "legacy-service-status-unavailable";
      return s;
    }
    CloseServiceHandle(manager);
    Session session;
    if (FAILED(session.error)) {
      s.error = failure(session.error);
      return s;
    }
    for (const auto &name : {QString("Deskflow"), QString("DeskflowWatchdog")}) {
      ComPtr<IRegisteredTask> old;
      const auto code = session.root->GetTask(Bstr(name), &old);
      if (SUCCEEDED(code))
        s.conflict = true;
      else if (code != HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND)) {
        s.error = "legacy-task-status-unavailable";
        return s;
      }
    }
    ComPtr<IRegisteredTask> job;
    auto hr = session.root->GetTask(Bstr(task(s.userSid)), &job);
    if (hr == HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND)) {
      s.available = true;
      return s;
    }
    if (FAILED(hr)) {
      s.error = failure(hr);
      return s;
    }
    BSTR xml = nullptr;
    hr = job->get_Xml(&xml);
    if (FAILED(hr)) {
      s.error = failure(hr);
      return s;
    }
    s.definition = QString::fromWCharArray(xml).toUtf8();
    SysFreeString(xml);
    s.exists = true;
    s.available = true;
    s.conflict = s.conflict || !owned(s.definition) ||
                 (normalized(s.definition) != normalized(definition(s, observedOptions(s))));
    return s;
  }
  Effect replace(const QByteArray &xml, const QByteArray &expectedRevision) override
  {
    const auto before = read();
    if (before.revision() != expectedRevision || !before.available || !before.error.isEmpty() || before.conflict ||
        (!xml.isEmpty() && !owned(xml)))
      return Effect::Unchanged;
    Session session;
    if (FAILED(session.error))
      return Effect::Unchanged;
    HRESULT hr;
    if (xml.isEmpty()) {
      if (!before.exists)
        return Effect::Applied;
      hr = session.root->DeleteTask(Bstr(task(before.userSid)), 0);
    } else {
      ComPtr<IRegisteredTask> job;
      VARIANT user, password, sddl;
      VariantInit(&user);
      VariantInit(&password);
      VariantInit(&sddl);
      user.vt = VT_BSTR;
      user.bstrVal = SysAllocString(reinterpret_cast<const wchar_t *>(before.userSid.utf16()));
      hr = session.root->RegisterTask(
          Bstr(task(before.userSid)), Bstr(QString::fromUtf8(xml)), TASK_CREATE_OR_UPDATE, user, password,
          TASK_LOGON_INTERACTIVE_TOKEN, sddl, &job
      );
      VariantClear(&user);
    }
    if (SUCCEEDED(hr))
      return Effect::Applied;
    const auto after = read();
    return after.revision() == before.revision() ? Effect::Unchanged : Effect::Uncertain;
  }
};
} // namespace
std::unique_ptr<System> makeNativeSystem(const QString &exe)
{
  return std::make_unique<NativeSystem>(exe);
}
#else
std::unique_ptr<System> makeNativeSystem(const QString &)
{
  return {};
}
#endif
} // namespace deskflow::gui::winstartup
