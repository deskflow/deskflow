/*
 * synergy -- mouse and keyboard sharing utility
 * Copyright (C) 2024 Symless Ltd.
 *
 * This package is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * found in the file LICENSE that should have accompanied this file.
 *
 * This package is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "messages.h"

#include "Logger.h"
#include "constants.h"
#include "styles.h"

#include <QAction>
#include <QDateTime>
#include <QDir>
#include <QMessageBox>
#include <QTime>
#include <memory>

namespace synergy::gui::messages {

static auto s_criticalMessage = std::unique_ptr<QMessageBox>();
static auto s_ignoredErrors = QStringList();

void showErrorDialog(
    const QString &message, const QMessageLogContext &context, QtMsgType type) {
  auto filename = QFileInfo(context.file).fileName();
  auto contextString = QString("%1:%2").arg(filename).arg(context.line);

  auto title = type == QtFatalMsg ? "Fatal error" : "Critical error";
  QString text;
  if (type == QtFatalMsg) {
    text = "<p>Sorry, a fatal error has occurred and the application must "
           "now exit.</p>";
  } else {
    text = "<p>Sorry, a critical error has occurred.</p>";
  }

  if (kLicensedProduct) {
    text += QString(R"(<p>Please <a href="%1" style="color: %2">contact us</a>)"
                    " and copy/paste the following error:</p>")
                .arg(kUrlContact, kColorSecondary);
  } else {
    text +=
        QString(R"(<p>Please <a href="%1" style="color: %2">report a bug</a>)"
                " and copy/paste the following error:</p>")
            .arg(kUrlBugReport, kColorSecondary);
  }

  text += QString("<pre>%3\n\n%4</pre>").arg(message, contextString);

  if (type == QtFatalMsg) {
    // create a blocking message box for fatal errors, as we want to wait
    // until the dialog is dismissed before aborting the app.
    QMessageBox::critical(nullptr, title, text);
  } else if (!s_ignoredErrors.contains(message)) {
    // prevent message boxes piling up by deleting the last one if it exists.
    // if none exists yet, then nothing will happen.
    s_criticalMessage.reset();

    // as we don't abort for critical messages, create a new non-blocking
    // message box. this is so that we don't block the message handler; if we
    // did, we would prevent new messages from being logged properly.
    // the memory will stay allocated until the app exits, which is acceptable.
    s_criticalMessage = std::make_unique<QMessageBox>(
        QMessageBox::Critical, title, text,
        QMessageBox::Ok | QMessageBox::Ignore);
    s_criticalMessage->open();

    QAction::connect(
        s_criticalMessage.get(), &QMessageBox::finished, //
        [message](int result) {
          if (result == QMessageBox::Ignore) {
            s_ignoredErrors.append(message);
          }
        });
  }
}

void messageHandler(
    QtMsgType type, const QMessageLogContext &context, const QString &message) {

  Logger::instance().handleMessage(type, context, message);

  if (type == QtFatalMsg || type == QtCriticalMsg) {
    showErrorDialog(message, context, type);
  }

  if (type == QtFatalMsg) {
    // developers: if you hit this line in your debugger, traverse the stack
    // to find the cause of the fatal error. important: crash the app on fatal
    // error to prevent the app being used in a broken state.
    // hint: if you don't want to crash, but still want to show an error
    // message, use `qCritical()` instead of `qFatal()`.
    abort();
  }
}

void showCloseReminder(QWidget *parent) {
  QString message =
      "<p>Synergy will continue to run in the background and can be accessed "
      "via the Synergy icon in your system notifications area. This setting "
      "can be disabled.</p>";

#if defined(Q_OS_LINUX)
  message += QString("<p>On some Linux systems such as GNOME 3, the "
                     "notification area might be disabled. "
                     "You may need to "
                     R"(<a href="%1" %2>enable an extension</a>)"
                     " to see the Synergy tray icon.</p>")
                 .arg(kUrlGnomeTrayFix, kStyleLink);
#endif

  QMessageBox::information(parent, "Notification area icon", message);
}

void showFirstRunMessage(
    QWidget *parent, bool closeToTray, bool enableService, bool isServer) {

  auto message = QString("<p>Synergy is now connected!</p>");

  if (isServer) {
    message +=
        "<p>Try moving your mouse to your other computer. Once there, go ahead "
        "and type something.</p>"
        "<p>Don't forget, you can copy and paste between computers too.</p>";
  } else {
    message += "<p>Try controlling this computer remotely.</p>";
  }

  if (!closeToTray && !enableService) {
    message +=
        "<p>As you do not have the setting enabled to keep Synergy running in "
        "the background, you'll need to keep this window open or minimized to "
        "keep Synergy running.</p>";
  } else {
    message +=
        "<p>You can now close this window and Synergy will continue to run in "
        "the background. This setting can be disabled.</p>";
  }

  QMessageBox::information(parent, "Connected", message);
}

void showDevThanks(QWidget *parent, const QString &productName) {
  if (productName.isEmpty()) {
    qFatal("product name not set");
  }

  QMessageBox::information(
      parent, "Thank you!",
      QString("<p>Thanks for using %1.</p>"
              "<p>If you enjoy using this app, you can support the <br />"
              "developers by "
              R"(<a href="%2" style="color: %3")>purchasing a license</a>)"
              " or "
              R"(<a href="%4" style="color: %5")>contributing code</a>.)"
              "</p>"
              "<p>This message will only appear once.</p>")
          .arg(
              productName, kUrlPurchase, kColorSecondary, kUrlGitHub,
              kColorSecondary));
}

} // namespace synergy::gui::messages
