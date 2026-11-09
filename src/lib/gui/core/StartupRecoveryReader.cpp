// SPDX-FileCopyrightText: (C) 2026 Deskflow Developers
// SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
#include "MacStartupExecutor.h"
#include "StartupRecovery.h"
#include <QCoreApplication>
#include <QCryptographicHash>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QProcess>
#include <QRegularExpression>
#ifdef Q_OS_MACOS
#include <unistd.h>
#endif

namespace deskflow::gui::startup {
namespace {
struct ReadResult
{
  int code = -1;
  QByteArray output, error;
};
ReadResult query(const QString &program, const QStringList &args)
{
  QProcess p;
  p.start(program, args, QIODevice::ReadOnly);
  if (!p.waitForFinished(800)) {
    p.kill();
    p.waitForFinished(200);
    return {};
  }
  if (p.exitStatus() != QProcess::NormalExit)
    return {};
  return {p.exitCode(), p.readAllStandardOutput().left(131072), p.readAllStandardError().left(4096)};
}
QByteArray boundedFile(const QString &path, bool &ok)
{
  const QFileInfo info(path);
  ok = !info.isSymLink();
  if (!ok || !info.exists())
    return {};
  QFile f(path);
  ok = info.isFile() && info.size() <= 65536 && f.open(QIODevice::ReadOnly);
  return ok ? f.readAll() : QByteArray{};
}
class ReadOnlyReader final : public Reader
{
public:
  Snapshot read() override
  {
    Snapshot s;
#if defined(Q_OS_MACOS) && !defined(DESKFLOW_LAB_BUILD)
    s.supported = true;
    s.home = QDir::homePath();
    s.domain = QString("gui/%1").arg(getuid());
    s.executable = QCoreApplication::applicationFilePath();
    s.version = QCoreApplication::applicationVersion();
    QFile executable(s.executable);
    QCryptographicHash executableHash(QCryptographicHash::Sha256);
    bool infoReadable = false;
    const auto info = boundedFile(QFileInfo(s.executable).absolutePath() + "/../Info.plist", infoReadable);
    if (!executable.open(QIODevice::ReadOnly) || !executableHash.addData(&executable) || !infoReadable ||
        info.isEmpty() || s.version.isEmpty()) {
      s.error = QStringLiteral("application-identity-unavailable");
      return s;
    }
    s.executableHash = executableHash.result().toHex();
    s.bundleInfoHash = QCryptographicHash::hash(info, QCryptographicHash::Sha256).toHex();
    const auto disabled = query("/bin/launchctl", {"print-disabled", s.domain});
    if (disabled.code != 0) {
      s.error = QStringLiteral("disabled-state-unavailable");
      return s;
    }
    const auto agentRoot = s.home + "/Library/LaunchAgents";
    if (QFileInfo(agentRoot).isSymLink()) {
      s.error = QStringLiteral("unexpected-agent-directory");
      return s;
    }
    const auto script = s.home + "/Library/Application Support/Deskflow/watchdog.zsh";
    for (const auto &label : {original, watchdog, keepawake, candidate}) {
      Service job;
      job.label = label;
      job.path = agentRoot + '/' + label + ".plist";
      bool readable = false;
      job.definition = boundedFile(job.path, readable);
      if (!readable) {
        job.error = QStringLiteral("definition-unreadable");
        s.services.insert(label, job);
        continue;
      }
      const auto match = QRegularExpression('"' + QRegularExpression::escape(label) + "\"\\s*=>\\s*(true|false)")
                             .match(QString::fromUtf8(disabled.output));
      job.disabled = match.hasMatch() && match.captured(1) == "true";
      if (QFileInfo::exists(job.path)) {
        const auto plist = query("/usr/bin/plutil", {"-convert", "json", "-o", "-", job.path});
        QJsonParseError parse;
        const auto doc = QJsonDocument::fromJson(plist.output, &parse);
        const auto object = doc.object();
        const auto args = object.value("ProgramArguments").toArray();
        for (const auto &arg : args)
          if (arg.isString())
            job.arguments << arg.toString();
        QStringList expected;
        if (label == original)
          expected = {"/Applications/Deskflow.app/Contents/MacOS/Deskflow"};
        else if (label == watchdog)
          expected = {"/bin/zsh", script};
        else if (label == keepawake)
          expected = {"/usr/bin/caffeinate", "-di"};
        else
          expected = {s.executable};
        if (plist.code != 0 || parse.error != QJsonParseError::NoError || object.value("Label").toString() != label ||
            job.arguments != expected || args.size() != job.arguments.size())
          job.error = QStringLiteral("unrecognized-service-definition");
        job.login = object.value("RunAtLoad").toBool();
        job.interval = object.value("StartInterval").toInt();
        const auto keep = object.value("KeepAlive");
        job.alwaysAlive = keep.isBool() && keep.toBool();
        const auto conditional = keep.toObject();
        job.crashRecovery = conditional.size() == 1 && conditional.value("SuccessfulExit").isBool() &&
                            !conditional.value("SuccessfulExit").toBool();
        job.login = job.login || job.alwaysAlive || job.crashRecovery;
      }
      const auto status = query("/bin/launchctl", {"print", s.domain + '/' + label});
      if (status.code == 0) {
        job.loaded = true;
        const auto text = QString::fromUtf8(status.output);
        job.running =
            QRegularExpression("^\\s*state = running\\s*$", QRegularExpression::MultilineOption).match(text).hasMatch();
        const auto exit =
            QRegularExpression("^\\s*last exit code = (-?[0-9]+)\\s*$", QRegularExpression::MultilineOption)
                .match(text);
        job.failed = !job.running && exit.hasMatch() && exit.captured(1).toInt() != 0;
        if (job.definition.isEmpty())
          job.error = QStringLiteral("loaded-without-reviewed-file");
      } else if (status.code != 113 || !status.error.contains("Could not find service"))
        job.error = QStringLiteral("service-status-unavailable");
      job.known = job.error.isEmpty();
      s.services.insert(label, job);
    }
    bool readable = false;
    const auto scriptBytes = boundedFile(script, readable);
    if (!readable || (!s.services[watchdog].definition.isEmpty() && scriptBytes.isEmpty()))
      s.error = QStringLiteral("watchdog-backup-unavailable");
    else
      s.watchdogHash = QCryptographicHash::hash(scriptBytes, QCryptographicHash::Sha256).toHex();
    if (s.error.isEmpty())
      s.history = latestStartupOperation(s);
#elif defined(DESKFLOW_LAB_BUILD)
    s.error = QStringLiteral("isolated-review");
#else
    s.error = QStringLiteral("unsupported-platform");
#endif
    return s;
  }
};
} // namespace
std::shared_ptr<Reader> makeReadOnlyReader()
{
  return std::make_shared<ReadOnlyReader>();
}
} // namespace deskflow::gui::startup
