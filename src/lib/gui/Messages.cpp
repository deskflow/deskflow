/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2024 Symless Ltd.
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#include "Messages.h"

#include "Logger.h"

#include "common/Enums.h"
#include "common/Settings.h"
#include "common/UrlConstants.h"
#include "common/VersionInfo.h"

#include <QCheckBox>
#include <QMessageBox>
#include <QPushButton>
#include <memory>

namespace deskflow::gui::messages {

struct Errors
{
  static std::unique_ptr<QMessageBox> s_criticalMessage;
  static QStringList s_ignoredErrors;
};

std::unique_ptr<QMessageBox> Errors::s_criticalMessage;
QStringList Errors::s_ignoredErrors;

void raiseCriticalDialog()
{
  if (Errors::s_criticalMessage) {
    Errors::s_criticalMessage->raise();
  }
}

void showErrorDialog(const QString &message, const QString &fileLine, QtMsgType type)
{
  auto errorType = QtFatalMsg ? QObject::tr("fatal error") : QObject::tr("error");
  auto title = QStringLiteral("%1 %2").arg(kAppName, errorType);
  auto text = QObject::tr(
                  R"(<p>Please <a href="%1">report a bug</a>)"
                  " and copy/paste the following error:</p><pre>v%2\n%3\n%4</pre>"
  )
                  .arg(kUrlHelp, kVersion, message, fileLine);

  if (type == QtFatalMsg) {
    text.prepend(QObject::tr("<p>Sorry, a fatal error has occurred and the application must now exit.</p>\n"));
    // create a blocking message box for fatal errors, as we want to wait
    // until the dialog is dismissed before aborting the app.
    QMessageBox::critical(nullptr, title, text, QMessageBox::Abort);
    return;
  }

  text.prepend(QObject::tr("<p>Sorry, a critical error has occurred.</p>\n"));
  if (!Errors::s_ignoredErrors.contains(message)) {
    // prevent message boxes piling up by deleting the last one if it exists.
    // if none exists yet, then nothing will happen.
    Errors::s_criticalMessage.reset();

    // as we don't abort for critical messages, create a new non-blocking
    // message box. this is so that we don't block the message handler; if we
    // did, we would prevent new messages from being logged properly.
    // the memory will stay allocated until the app exits, which is acceptable.
    Errors::s_criticalMessage =
        std::make_unique<QMessageBox>(QMessageBox::Critical, title, text, QMessageBox::Ok | QMessageBox::Ignore);

    QObject::connect(
        Errors::s_criticalMessage.get(), &QMessageBox::finished, //
        [message](int result) {
          if (result == QMessageBox::Ignore) {
            Errors::s_ignoredErrors.append(message);
          }
        }
    );
    Errors::s_criticalMessage->open();
  }
}

QString fileLine(const QMessageLogContext &context)
{
  if (!context.file) {
    return {};
  }
  return QStringLiteral("%1:%2").arg(context.file, QString::number(context.line));
}

void messageHandler(QtMsgType type, const QMessageLogContext &context, const QString &message)
{
  const auto fileLine = messages::fileLine(context);
  Logger::instance()->handleMessage(type, fileLine, message);

  if (type == QtFatalMsg || type == QtCriticalMsg) {
    showErrorDialog(message, fileLine, type);
  }

  if (type == QtFatalMsg) {
    // developers: if you hit this line in your debugger, traverse the stack
    // to find the cause of the fatal error. important: crash the app on fatal
    // error to prevent the app being used in a broken state.
    //
    // hint: if you don't want to crash, but still want to show an error
    // message, use `qCritical()` instead of `qFatal()`. you should use
    // fatal errors when the app is in an unrecoverable state; i.e. it cannot
    // function correctly in it's current state and must be restarted.
    abort();
  }
}

void showCloseReminder(QWidget *parent)
{
  auto message = QObject::tr(
                     "<p>%1 will continue to run in the background and can be accessed via the %1 icon in your "
                     "system notifications area. This setting can be disabled.</p>"
  )
                     .arg(kAppName);

#if defined(Q_OS_LINUX)
  message.append(
      QObject::tr(
          "<p>On Linux systems using GNOME 3, the notification area might be disabled. "
          R"(You may need to <a href="%1">enable an extension</a> to see the %2 tray icon.</p>)"
      )
          .arg(kUrlGnomeTrayFix, kAppName)
  );
#endif

  QMessageBox::information(parent, kAppName, message);
}

void showFirstServerStartMessage(QWidget *parent)
{
  QMessageBox::information(
      parent, QObject::tr("%1 Server").arg(kAppName),
      QObject::tr(
          "<p>Great, the %1 server is now running.</p>"
          "<p>Now you can connect your client computers to this server. "
          "You should see a prompt here on the server when a new client tries to connect.</p>"
      )
          .arg(kAppName)
  );
}

void showFirstConnectedMessage(QWidget *parent, bool closeToTray, bool enableService, bool isServer)
{

  auto message = QObject::tr("<p>%1 is now connected!</p>").arg(kAppName);

  if (isServer) {
    message.append(
        QObject::tr(
            "<p>Try moving your mouse to your other computer. Once there, go ahead "
            "and type something.</p>"
            "<p>Don't forget, you can copy and paste between computers too.</p>"
        )
    );
  } else {
    message.append(QObject::tr("<p>Try controlling this computer remotely.</p>"));
  }

  if (!closeToTray && !enableService) {
    message.append(
        QObject::tr(
            "<p>As you do not have the setting enabled to keep %1 running in "
            "the background, you'll need to keep this window open or minimized "
            "to keep %1 running.</p>"
        )
            .arg(kAppName)
    );
  } else {
    message.append(
        QObject::tr(
            "<p>You can now close this window and %1 will continue to run in "
            "the background. This setting can be disabled.</p>"
        )
            .arg(kAppName)
    );
  }

  const auto title = QObject::tr("%1 Connected").arg(kAppName);
  QMessageBox::information(parent, title, message);
}

void showClientConnectError(QWidget *parent, deskflow::client::ErrorType error, const QString &address)
{
  using enum deskflow::client::ErrorType;
  if (error == NoError)
    return;

  auto message = QObject::tr("<p>Failed to connect to the server '%1'.</p>").arg(address);

  if (error == AlreadyConnected) {
    message.append(
        QObject::tr( //
            "<p>A Client with your name is already connected to the server.</p>"
            "Please ensure that you're using a unique name and that only a "
            "single instance of the client process is running.</p>"
        )
    );
  } else if (error == HostnameError) {
    message.append(
        QObject::tr( //
            "<p>Please try to connect to the server using the server IP address "
            "instead of the hostname. </p>"
            "<p>If that doesn't work, please check your TLS and "
            "firewall settings.</p>"
        )
    );
  } else if (error == GenericError) {
    message.append(QObject::tr("<p>Please check your TLS and firewall settings.</p>"));
  } else {
    qFatal("unknown client error");
  }

  auto title = QObject::tr("%1 Connection Error").arg(kAppName);

  if (error != HostnameError) {
    QMessageBox::warning(parent, title, message);
    return;
  }

  auto dialog = QMessageBox(parent);
  dialog.setWindowTitle(title);
  dialog.setText(message);
  dialog.setWindowModality(Qt::ApplicationModal);
  dialog.setIcon(QMessageBox::Information);

  auto cbNoShowAgain = new QCheckBox(QObject::tr("Do not show this message again"));

  QObject::connect(cbNoShowAgain, &QCheckBox::toggled, [](bool enabled) {
    Settings::setValue(Settings::Gui::ShowGenericClientFailureDialog, !enabled);
  });

  dialog.setCheckBox(cbNoShowAgain);
  dialog.setDefaultButton(QMessageBox::Ok);
  dialog.exec();
}

bool showNewClientPrompt(QWidget *parent, const QString &clientName, bool serverRequiresPeerAuth)
{
  if (serverRequiresPeerAuth) {
    // When peer auth is enabled you will be prompted to allow the connection before seeing this dialog.
    // This is why we do not show a dialog with an option to ignore the new client
    QMessageBox::information(
        parent, QObject::tr("%1 - New Client").arg(kAppName),
        QObject::tr("A new client called '%1' has been accepted. You'll need to add it to your server's screen layout.")
            .arg(clientName)
    );
    return true;
  }
  QMessageBox message(parent);
  message.addButton(QObject::tr("Ignore"), QMessageBox::RejectRole);
  message.addButton(QObject::tr("Add client"), QMessageBox::AcceptRole);
  message.setText(QObject::tr("A new client called '%1' wants to connect").arg(clientName));
  message.exec();
  return message.buttonRole(message.clickedButton()) == QMessageBox::AcceptRole;
}

bool showClearSettings(QWidget *parent)
{
  const auto title = QObject::tr("%1 Clear Settings").arg(kAppName);
  const auto message = QObject::tr(
                           "<p>Are you sure you want to clear all settings and restart %1?</p>"
                           "<p>This action cannot be undone.</p>"
  )
                           .arg(kAppName);
  return QMessageBox::question(parent, title, message) == QMessageBox::Yes;
}

void showReadOnlySettings(QWidget *parent, const QString &systemSettingsPath)
{
  const auto title = QObject::tr("%1 Read-only settings").arg(kAppName);
  const auto message = QObject::tr(
                           "<p>Settings are read-only because you only have read access "
                           "to the file:</p><p>%1</p>"
  )
                           .arg(QDir::toNativeSeparators(systemSettingsPath));
  QMessageBox::information(parent, title, message);
}

void showWaylandLibraryError(QWidget *parent)
{
  QMessageBox::critical(
      parent, kAppName,
      QObject::tr(
          "<p>Sorry, while this version of %1 does support Wayland, "
          "this build was not linked with one or more of the required "
          "libraries.</p>"
          "<p>Please either switch to X from your login screen or use a build "
          "that uses the correct libraries.</p>"
          "<p>If you think this is incorrect, please "
          R"(<a href="%2">report a bug</a>.</p>)"
          "<p>Please check the logs for more information.</p>"
      )
          .arg(kAppName, kUrlHelp)
  );
}

bool showUpdateCheckOption(QWidget *parent)
{
  QMessageBox message(parent);
  message.addButton(QObject::tr("No thanks"), QMessageBox::RejectRole);
  const auto checkButton = message.addButton(QObject::tr("Check for updates"), QMessageBox::AcceptRole);
  message.setText(
      QObject::tr(
          "<p>Would you like to check for updates when %1 starts?</p>"
          "<p>Checking for updates requires an Internet connection.</p>"
          "<p>URL: <pre>%2</pre></p>"
      )
          .arg(kAppName, Settings::value(Settings::Gui::UpdateCheckUrl).toString())
  );

  message.exec();
  return message.clickedButton() == checkButton;
}

bool showDaemonOffline(QWidget *parent)
{
  QMessageBox message(parent);
  message.setIcon(QMessageBox::Warning);
  message.setWindowTitle(QObject::tr("Background service offline"));

  message.addButton(QObject::tr("Retry"), QMessageBox::AcceptRole);
  const auto ignore = message.addButton(QObject::tr("Ignore"), QMessageBox::RejectRole);
  const auto disable = message.addButton(QObject::tr("Disable"), QMessageBox::NoRole);

  message.setText(
      QObject::tr(
          "<p>There was a problem finding the %1 background service (daemon).</p>"
          "<p>The background service makes %1 work with UAC prompts and the login screen.</p>"
          "<p>If don't want to use the background service and intentionally stopped it, "
          "you can prevent it's use by disabling this feature.</p>"
          "<p>If you did not stop the background service intentionally, there may be a problem with it. "
          "Please retry or try restarting the %1 service from the Windows services program.</p>"
      )
          .arg(kAppName)
  );
  message.exec();

  if (message.clickedButton() == ignore) {
    return false;
  } else if (message.clickedButton() == disable) {
    Settings::setValue(Settings::Core::ProcessMode, Settings::ProcessMode::Desktop);
    return false;
  }

  return true;
}

} // namespace deskflow::gui::messages
