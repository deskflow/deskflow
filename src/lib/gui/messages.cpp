/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2024 Symless Ltd.
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#include "messages.h"

#include "Logger.h"
#include "common/constants.h"
#include "constants.h"
#include "env_vars.h"
#include "styles.h"

#include <QAction>
#include <QDateTime>
#include <QDir>
#include <QMessageBox>
#include <QPushButton>
#include <QTime>
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
  auto title = type == QtFatalMsg ? "Fatal error" : "Critical error";
  QString text;
  if (type == QtFatalMsg) {
    text = "<p>Sorry, a fatal error has occurred and the application must "
           "now exit.</p>";
  } else {
    text = "<p>Sorry, a critical error has occurred.</p>";
  }

  text += QString(R"(<p>Please <a href="%1" style="color: %2">report a bug</a>)"
                  " and copy/paste the following error:</p>")
              .arg(kUrlHelp, kColorSecondary);

  text += QString("<pre>v%1\n%2\n%3</pre>").arg(kVersion, message, fileLine);

  if (type == QtFatalMsg) {
    // create a blocking message box for fatal errors, as we want to wait
    // until the dialog is dismissed before aborting the app.
    QMessageBox critical(QMessageBox::Critical, title, text, QMessageBox::Abort);
    critical.exec();
  } else if (!Errors::s_ignoredErrors.contains(message)) {
    // prevent message boxes piling up by deleting the last one if it exists.
    // if none exists yet, then nothing will happen.
    Errors::s_criticalMessage.reset();

    // as we don't abort for critical messages, create a new non-blocking
    // message box. this is so that we don't block the message handler; if we
    // did, we would prevent new messages from being logged properly.
    // the memory will stay allocated until the app exits, which is acceptable.
    Errors::s_criticalMessage =
        std::make_unique<QMessageBox>(QMessageBox::Critical, title, text, QMessageBox::Ok | QMessageBox::Ignore);

    Errors::s_criticalMessage->open();

    QAction::connect(
        Errors::s_criticalMessage.get(), &QMessageBox::finished, //
        [message](int result) {
          if (result == QMessageBox::Ignore) {
            Errors::s_ignoredErrors.append(message);
          }
        }
    );
  }
}

QString fileLine(const QMessageLogContext &context)
{
  if (!context.file) {
    return "";
  }
  return QString("%1:%2").arg(context.file).arg(context.line);
}

void messageHandler(QtMsgType type, const QMessageLogContext &context, const QString &message)
{
  const auto fileLine = messages::fileLine(context);
  Logger::instance().handleMessage(type, fileLine, message);

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
  QString message = QString("<p>%1 will continue to run in the background and can be accessed "
                            "via the %1 icon in your system notifications area. This "
                            "setting "
                            "can be disabled.</p>")
                        .arg(kAppName);

#if defined(Q_OS_LINUX)
  message += QString("<p>On some Linux systems such as GNOME 3, the "
                     "notification area might be disabled. "
                     "You may need to "
                     R"(<a href="%1" %2>enable an extension</a>)"
                     " to see the %3 tray icon.</p>")
                 .arg(kUrlGnomeTrayFix, kStyleLink, kAppName);
#endif

  QMessageBox::information(parent, "Notification area icon", message);
}

void showFirstServerStartMessage(QWidget *parent)
{
  QMessageBox::information(
      parent, "Server is running",
      QString("<p>Great, the %1 server is now running.</p>"
              "<p>Now you can connect your client computers to this server. "
              "You should see a prompt here on the server when a new client "
              "tries to connect.</p>")
          .arg(kAppName)
  );
}

void showFirstConnectedMessage(QWidget *parent, bool closeToTray, bool enableService, bool isServer)
{

  auto message = QString("<p>%1 is now connected!</p>").arg(kAppName);

  if (isServer) {
    message += "<p>Try moving your mouse to your other computer. Once there, go ahead "
               "and type something.</p>"
               "<p>Don't forget, you can copy and paste between computers too.</p>";
  } else {
    message += "<p>Try controlling this computer remotely.</p>";
  }

  if (!closeToTray && !enableService) {
    message += QString("<p>As you do not have the setting enabled to keep %1 running in "
                       "the background, you'll need to keep this window open or minimized "
                       "to keep %1 running.</p>")
                   .arg(kAppName);
  } else {
    message += QString("<p>You can now close this window and %1 will continue to run in "
                       "the background. This setting can be disabled.</p>")
                   .arg(kAppName);
  }

  QMessageBox::information(parent, "Connected", message);
}

void showClientConnectError(QWidget *parent, ClientError error, const QString &address)
{
  using enum ClientError;

  auto message = QString("<p>The connection to server '%1' didn't work.</p>").arg(address);

  if (error == AlreadyConnected) {
    message += //
        "<p>Two of your client computers have the same name or there are "
        "two instances of the client process running.</p>"
        "<p>Please ensure that you're using a unique name and that only a "
        "single client process is running.</p>";
  } else if (error == HostnameError) {
    message += //
        "<p>Please try to connect to the server using the server IP address "
        "instead of the hostname. </p>"
        "<p>If that doesn't work, please check your TLS and "
        "firewall settings.</p>";
  } else if (error == GenericError) {
    message += //
        "<p>Please check your TLS and firewall settings.</p>";
  } else {
    qFatal("unknown client error");
  }

  QMessageBox dialog(parent);
  dialog.addButton(QObject::tr("Close"), QMessageBox::RejectRole);
  dialog.setText(message);
  dialog.exec();
}

NewClientPromptResult showNewClientPrompt(QWidget *parent, const QString &clientName, bool serverRequiresPeerAuth)
{
  using enum NewClientPromptResult;

  if (serverRequiresPeerAuth) {
    // When peer auth is enabled you will be prompted to allow the connection before seeing this dialog.
    // This is why we do not show a dialog with an option to ignore the new client
    QMessageBox::information(
        parent, QString("New Client"),
        QString("A new client called '%1' has been accepted. You'll need to add it to your server's screen layout.")
            .arg(clientName)
    );
    return Add;
  } else {
    QMessageBox message(parent);
    const QPushButton *ignore = message.addButton("Ignore", QMessageBox::RejectRole);
    const QPushButton *add = message.addButton("Add client", QMessageBox::AcceptRole);
    message.setText(QString("A new client called '%1' wants to connect").arg(clientName));
    message.exec();
    if (message.clickedButton() == add) {
      return Add;
    } else if (message.clickedButton() == ignore) {
      return Ignore;
    } else {
      qFatal("no expected dialog button was clicked");
      abort();
    }
  }
}

bool showClearSettings(QWidget *parent)
{
  QMessageBox message(parent);
  message.addButton(QObject::tr("Cancel"), QMessageBox::RejectRole);
  const auto clear = message.addButton(QObject::tr("Clear settings"), QMessageBox::AcceptRole);
  message.setText(QString("<p>Are you sure you want to clear all settings and restart %1?</p>"
                          "<p>This action cannot be undone.</p>")
                      .arg(kAppName));
  message.exec();

  return message.clickedButton() == clear;
}

void showReadOnlySettings(QWidget *parent, const QString &systemSettingsPath)
{
  QString nativePath = QDir::toNativeSeparators(systemSettingsPath);
  QMessageBox::information(
      parent, "Read-only settings",
      QString("<p>Settings are read-only because you only have read access "
              "to the file:</p><p>%1</p>")
          .arg(nativePath)
  );
}

void showWaylandLibraryError(QWidget *parent)
{
  QMessageBox::critical(
      parent, "Library problem",
      QString("<p>Sorry, while this version of %1 does support Wayland, "
              "this build was not linked with one or more of the required "
              "libraries.</p>"
              "<p>Please either switch to X from your login screen or use a build "
              "that uses the correct libraries.</p>"
              "<p>If you think this is incorrect, please "
              R"(<a href="%2" style="color: %3">report a bug</a>.</p>)"
              "<p>Please check the logs for more information.</p>")
          .arg(kAppName, kUrlHelp, kColorSecondary)
  );
}

bool showUpdateCheckOption(QWidget *parent)
{
  QMessageBox message(parent);
  message.addButton(QObject::tr("No thanks"), QMessageBox::RejectRole);
  const auto checkButton = message.addButton(QObject::tr("Check for updates"), QMessageBox::AcceptRole);
  message.setText(QString("<p>Would you like to check for updates when %1 starts?</p>"
                          "<p>Checking for updates requires an Internet connection.</p>"
                          "<p>URL: <pre>%2</pre></p>")
                      .arg(kAppName, env_vars::versionUrl()));

  message.exec();
  return message.clickedButton() == checkButton;
}

} // namespace deskflow::gui::messages
