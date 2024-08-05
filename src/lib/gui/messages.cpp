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

#include "constants.h"
#include "styles.h"

#include <QDateTime>
#include <QMessageBox>
#include <QTime>

namespace synergy::gui::messages {

void messageHandler(
    QtMsgType type, const QMessageLogContext &context, const QString &message) {

  auto datetime = QDateTime::currentDateTime().toString("yyyy-MM-ddTHH:mm:ss");
  const auto filename = QString(context.file).split("/").last();
  auto function = context.function ? context.function : "";

  QString typeString;
  auto out = stdout;
  switch (type) {
  case QtDebugMsg:
    typeString = "DEBUG";
    break;
  case QtInfoMsg:
    typeString = "INFO";
    break;
  case QtWarningMsg:
    typeString = "WARNING";
    out = stderr;
    break;
  case QtCriticalMsg:
    typeString = "CRITICAL";
    out = stderr;
    break;
  case QtFatalMsg:
    typeString = "FATAL";
    out = stderr;
    break;
  }

  auto logLine = QString("[%1] %2: %3\n\t%4:%5, %6")
                     .arg(datetime)
                     .arg(typeString)
                     .arg(message)
                     .arg(filename)
                     .arg(context.line)
                     .arg(function);

  auto logLineUtf = logLine.toUtf8();
  auto logLine_c = logLineUtf.constData();
  fprintf(out, "%s\n", logLine_c);

  if (type == QtFatalMsg) {
    auto contextString =
        QString("%1:%2\n%3").arg(filename).arg(context.line).arg(function);

    QMessageBox::critical(
        nullptr, "Fatal error",
        QString("<p>Sorry, a fatal error has occurred "
                "and the application must now exit.</p>"
                "<p>Please "
                R"(<a href="%1" style="color: %2">contact us</a>)"
                " and copy/paste the following error:</p>"
                "<pre>%3\n\n%4</pre>")
            .arg(kUrlContact, kColorSecondary, message, contextString));

    // developers: if you hit this line in your debugger, traverse the stack to
    // find the cause of the fatal error.
    // important: crash the app on fatal error to prevent the app being used in
    // a broken state.
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
      QString(
          "<p>Thanks for using %1.</p>"
          "<p>If you enjoy using this app, you can support the developers by "
          R"(<a href="%1" style="color: %2")>purchasing a license</a>)"
          " or "
          R"(<a href="%3" style="color: %4")>contributing code</a>.)"
          "</p>"
          "<p>This message will only appear once.</p>")
          .arg(
              productName, kUrlPurchase, kColorSecondary, kUrlContribute,
              kColorSecondary));
}

void logVerbose(QString message) {
  const auto enabled = QString(qgetenv("SYNERGY_VERBOSE_LOGGING"));
  if (enabled != "true") {
    return;
  }

  qDebug().noquote() << message;
}

} // namespace synergy::gui::messages
