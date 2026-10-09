/*
 * SPDX-FileCopyrightText: (C) 2026 Deskflow Developers
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */
#include "SendTo.h"
#include "Native.h"
#include "common/Settings.h"
#include "gui/config/ServerConfig.h"

#include <QComboBox>
#include <QDialog>
#include <QDialogButtonBox>
#include <QElapsedTimer>
#include <QFileDialog>
#include <QLabel>
#include <QMenu>
#include <QMessageBox>
#include <QPointer>
#include <QProcess>
#include <QProgressDialog>
#include <QPushButton>
#include <QThread>
#include <QVBoxLayout>
#include <stdexcept>

namespace deskflow::handoff {
SendTo::SendTo(QWidget *parent, const ServerConfig &config, QMenu *fileMenu, QMenu *trayMenu)
    : QObject(parent),
      m_window(parent),
      m_config(config)
{
  auto files = new QAction(tr("Send files to computer…"), this);
  auto window = new QAction(tr("Send current window to computer…"), this);
  for (auto menu : {fileMenu, trayMenu}) {
    menu->addSeparator();
    menu->addAction(files);
    menu->addAction(window);
  }
  connect(files, &QAction::triggered, this, [this] {
    const auto paths = QFileDialog::getOpenFileNames(m_window, tr("Send files"));
    if (!paths.isEmpty())
      chooseComputer({paths, {}, {}});
  });
  connect(window, &QAction::triggered, this, [this] {
    try {
      chooseComputer(currentApplication());
    } catch (const std::exception &error) {
      QMessageBox::warning(m_window, tr("Could not send window"), QString::fromUtf8(error.what()));
    }
  });
  installService([guard = QPointer<SendTo>(this)](Request request) {
    if (guard)
      guard->chooseComputer(std::move(request));
  });
}

SendTo::~SendTo()
{
  for (auto worker : findChildren<QThread *>()) {
    worker->requestInterruption();
    worker->wait();
  }
}

void SendTo::chooseComputer(Request request)
{
  QDialog dialog(m_window);
  dialog.setWindowTitle(tr("Send to computer"));
  auto layout = new QVBoxLayout(&dialog);
  auto destination = new QComboBox(&dialog);
  destination->setEditable(true);
  QStringList names = Settings::value(Settings::Gui::HandoffDestinations).toStringList();
  for (const auto &computer : m_config.computers()) {
    if (!computer.isNull() && computer.name() != Settings::value(Settings::Core::ComputerName).toString())
      names.append(computer.name());
  }
  if (const auto host = Settings::value(Settings::Client::RemoteHost).toString(); !host.isEmpty())
    names.append(host);
  names.removeDuplicates();
  destination->addItems(names);
  layout->addWidget(new QLabel(tr("Computer (SSH alias or user@hostname):"), &dialog));
  layout->addWidget(destination);
  auto help = new QLabel(
      tr("The other Mac needs this build in /Applications/Deskflow.app, Remote Login, "
         "and SSH key login. Connect once in Terminal to verify its host key. Files are copied to Downloads; "
         "the originals stay here. Save documents before sending."),
      &dialog
  );
  help->setWordWrap(true);
  layout->addWidget(help);
  auto buttons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, &dialog);
  buttons->button(QDialogButtonBox::Ok)->setText(tr("Send"));
  layout->addWidget(buttons);
  connect(buttons, &QDialogButtonBox::accepted, &dialog, &QDialog::accept);
  connect(buttons, &QDialogButtonBox::rejected, &dialog, &QDialog::reject);
  if (dialog.exec() != QDialog::Accepted)
    return;
  const auto host = destination->currentText().trimmed();
  if (!validDestination(host)) {
    QMessageBox::warning(m_window, tr("Invalid computer"), tr("Use an SSH host alias or user@hostname."));
    return;
  }
  auto progress = new QProgressDialog(tr("Sending to %1…").arg(host), tr("Cancel"), 0, 0, m_window);
  progress->setWindowModality(Qt::WindowModal);
  progress->setMinimumDuration(0);
  auto worker = QThread::create([this, request, host, progress] {
    QString failure;
    QProcess ssh;
    try {
      ssh.setProgram("/usr/bin/ssh");
      ssh.setArguments(sshArguments(host));
      ssh.start();
      if (!ssh.waitForStarted(10000))
        throw std::runtime_error("Could not start SSH");
      send(ssh, request);
      ssh.closeWriteChannel();
      QElapsedTimer deadline;
      deadline.start();
      while (ssh.state() != QProcess::NotRunning && deadline.elapsed() < 60000) {
        if (QThread::currentThread()->isInterruptionRequested())
          throw std::runtime_error("Transfer cancelled. The destination may already have received it.");
        ssh.waitForFinished(250);
      }
      if (ssh.state() != QProcess::NotRunning || ssh.exitStatus() != QProcess::NormalExit || ssh.exitCode() != 0)
        throw std::runtime_error((QStringLiteral(
                                      "Could not complete handoff. Check Remote Login, SSH keys, "
                                      "and the Deskflow installation on %1.\n%2"
                                  )
                                      .arg(host, QString::fromUtf8(ssh.readAllStandardError()).left(4096)))
                                     .toStdString());
    } catch (const std::exception &error) {
      failure = QString::fromUtf8(error.what());
      ssh.kill();
      ssh.waitForFinished(5000);
    }
    QMetaObject::invokeMethod(
        this,
        [this, host, failure, progress] {
          progress->deleteLater();
          if (failure.isEmpty()) {
            auto hosts = Settings::value(Settings::Gui::HandoffDestinations).toStringList();
            hosts.removeAll(host);
            hosts.prepend(host);
            hosts = hosts.mid(0, 20);
            Settings::setValue(Settings::Gui::HandoffDestinations, hosts);
            QMessageBox::information(m_window, tr("Sent"), tr("Sent to %1.").arg(host));
          } else {
            QMessageBox::warning(m_window, tr("Could not send"), failure);
          }
        },
        Qt::QueuedConnection
    );
  });
  worker->setParent(this);
  connect(progress, &QProgressDialog::canceled, worker, &QThread::requestInterruption);
  connect(worker, &QThread::finished, worker, &QObject::deleteLater);
  worker->start();
}
} // namespace deskflow::handoff
