// SPDX-FileCopyrightText: (C) 2026 Deskflow Developers
// SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
#include "DiagnosticExportDialog.h"
#include "gui/ExperienceStyle.h"
#include <QDateTime>
#include <QDialogButtonBox>
#include <QFile>
#include <QFileDialog>
#include <QFileInfo>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QLabel>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QSet>
#include <QVBoxLayout>

QByteArray DiagnosticExportDialog::sanitized(const QByteArray &snapshot)
{
  if (snapshot.size() > 32768)
    return {};
  QJsonParseError error;
  const auto doc = QJsonDocument::fromJson(snapshot, &error);
  if (error.error != QJsonParseError::NoError || !doc.isObject() || doc["schema"] != 1)
    return {};
  const auto in = doc.object();
  QJsonObject out{{"schema", 1}, {"redacted", true}, {"inputDeliveryVerified", false}, {"latencyMeasured", false}};
  const QSet<QString> phases{"Idle",      "Starting", "Discovering", "Connecting",  "Verifying", "Connected",
                             "Listening", "Retrying", "Stopping",    "NeedsAction", "Paused"};
  if (!phases.contains(in["phase"].toString()))
    return {};
  out["phase"] = in["phase"];
  for (const auto *key : {"server", "tlsConfigured", "peerChecksConfigured", "tlsObserved"}) {
    if (!in[key].isBool())
      return {};
    out[key] = in[key];
  }
  for (const auto *key : {"coreProcessState", "coreConnectionState", "attention"}) {
    const int maximum = QString::fromLatin1(key) == "coreProcessState"      ? 4
                        : QString::fromLatin1(key) == "coreConnectionState" ? 3
                                                                            : 10;
    if (!in[key].isDouble() || in[key].toDouble() != in[key].toInt(-1) || in[key].toInt(-1) < 0 ||
        in[key].toInt() > maximum)
      return {};
    out[key] = in[key];
  }
  QSet<QString> codes{"attempt-starting", "tls-observed",         "tls-reset",         "peer-verification-started",
                      "retry-scheduled",  "client-count-changed", "discovery-started", "discovery-stopped",
                      "user-started",     "user-paused"};
  for (const auto *v : {"Stopped", "Starting", "Started", "Stopping", "RetryPending"})
    codes.insert(QString("process-") + v);
  for (const auto *v : {"Disconnected", "Connecting", "Connected", "Listening"})
    codes.insert(QString("connection-") + v);
  for (const auto *v :
       {"None", "Identity", "SecurityPolicy", "Address", "StartFailed", "Daemon", "DuplicateName", "UnknownClient",
        "Version", "Protocol", "Recovery"})
    codes.insert(QString("attention-") + v);
  QJsonArray events;
  const auto inputEvents = in["events"].toArray();
  for (int i = qMax(0, inputEvents.size() - 64); i < inputEvents.size(); ++i) {
    const auto v = inputEvents[i].toObject();
    const auto code = v["event"].toString();
    if (!codes.contains(code))
      continue;
    QJsonObject event{{"event", code}, {"sequence", events.size() + 1}};
    // Deliberately omit wall-clock timestamps and every unrecognized field.
    events.append(event);
  }
  out["events"] = events;
  return QJsonDocument(out).toJson(QJsonDocument::Indented);
}
bool DiagnosticExportDialog::saveNewFile(const QString &path, const QByteArray &snapshot)
{
  const auto payload = sanitized(snapshot);
  if (payload.isEmpty())
    return false;
  const QFileInfo info(path);
  if (!info.isAbsolute() || info.suffix().toLower() != "json" || info.exists() || info.isSymLink())
    return false;
  QFile file(path);
  if (!file.open(QIODevice::WriteOnly | QIODevice::NewOnly))
    return false;
  if (!file.setPermissions(QFile::ReadOwner | QFile::WriteOwner) || file.write(payload) != payload.size() ||
      !file.flush()) {
    file.close();
    QFile::remove(path);
    return false;
  }
  file.close();
  return true;
}
DiagnosticExportDialog::DiagnosticExportDialog(const QByteArray &snapshot, QWidget *parent)
    : QDialog(parent),
      m_payload(sanitized(snapshot))
{
  deskflow::gui::applyExperienceStyle(this);
  setWindowTitle(tr("Export connection diagnostics"));
  resize(640, 560);
  setMinimumSize(500, 400);
  auto *layout = new QVBoxLayout(this);
  layout->setContentsMargins(24, 24, 24, 20);
  layout->setSpacing(12);
  auto *description = new QLabel(
      tr("Review exactly what will be saved. Only connection state codes and security flags are included. Names, "
         "addresses, fingerprints, file paths, raw logs and input content are excluded."),
      this
  );
  description->setTextFormat(Qt::PlainText);
  description->setWordWrap(true);
  layout->addWidget(description);
  auto *preview = new QPlainTextEdit(QString::fromUtf8(m_payload), this);
  preview->setReadOnly(true);
  preview->setObjectName("diagnosticExportPreview");
  preview->setAccessibleName(tr("Redacted diagnostic preview"));
  layout->addWidget(preview, 1);
  m_result = new QLabel(this);
  m_result->setWordWrap(true);
  m_result->setTextFormat(Qt::PlainText);
  layout->addWidget(m_result);
  auto *buttons = new QDialogButtonBox(QDialogButtonBox::Close, this);
  auto *save = buttons->addButton(tr("Save redacted JSON…"), QDialogButtonBox::ActionRole);
  save->setObjectName("saveRedactedDiagnostics");
  save->setEnabled(!m_payload.isEmpty());
  layout->addWidget(buttons);
  connect(buttons, &QDialogButtonBox::rejected, this, &QDialog::reject);
  connect(save, &QPushButton::clicked, this, [this] {
    const auto path = QFileDialog::getSaveFileName(
        this, tr("Save diagnostics to a new file"), "deskflow-diagnostics.json", tr("JSON files (*.json)")
    );
    if (path.isEmpty())
      return;
    m_result->setText(
        saveNewFile(path, m_payload)
            ? tr("Redacted diagnostics saved. No data was sent anywhere.")
            : tr("Could not save. Choose a new writable JSON filename; existing files are never replaced.")
    );
  });
}
