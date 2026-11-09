// SPDX-FileCopyrightText: (C) 2026 Deskflow Developers
// SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
// Real .ui/widgets, synthetic state, no MainWindow constructor/core/IPC/settings.
#include "gui/widgets/RecentServersButton.h"
#include "gui/widgets/StatusBar.h"
#include "ui_MainWindow.h"

#include <QApplication>
#include <QDir>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QMenu>
#include <QTimer>
#include <iostream>

int main(int argc, char **argv)
{
  QApplication app(argc, argv);
  if (argc != 3) {
    std::cerr << "usage: preview OUTPUT_DIRECTORY light|dark\n";
    return 2;
  }
  app.setStyle("Fusion");
  if (QString::fromLocal8Bit(argv[2]) == QStringLiteral("dark")) {
    QPalette palette;
    palette.setColor(QPalette::Window, QColor("#202329"));
    palette.setColor(QPalette::WindowText, QColor("#e7e9ee"));
    palette.setColor(QPalette::Base, QColor("#16191e"));
    palette.setColor(QPalette::Text, QColor("#e7e9ee"));
    palette.setColor(QPalette::Button, QColor("#2b3038"));
    palette.setColor(QPalette::ButtonText, QColor("#e7e9ee"));
    palette.setColor(QPalette::Highlight, QColor("#5c9ded"));
    palette.setColor(QPalette::HighlightedText, QColor("#101820"));
    app.setPalette(palette);
  }
  const QDir output(QString::fromLocal8Bit(argv[1]));
  if (!output.exists())
    return 3;

  QMainWindow window;
  Ui::MainWindow ui;
  ui.setupUi(&window);
  ui.lblComputerName->setText(QStringLiteral("Mac · example"));
  ui.lineEditName->hide();
  ui.lblIpAddresses->hide();
  ui.serverOptions->hide();
  ui.lblNoMode->hide();
  ui.rbModeClient->setChecked(true);
  ui.clientOptions->show();
  ui.lineHostname->setText(QStringLiteral("example-server.invalid"));
  ui.btnToggleCore->setText(QStringLiteral("Disconnect"));
  ui.btnToggleCore->setEnabled(true);
  ui.btnRestartCore->setEnabled(true);
  auto *recent = new RecentServersButton(&window);
  ui.clientOptions->layout()->addWidget(recent);
  recent->setConnectionAllowed(true);
  recent->beginAttempt(QStringLiteral("example-server.invalid"), 24800, true);
  recent->secureSocket(true);
  recent->connectionStateChanged(ConnectionState::Connected);
  auto *status = new StatusBar(&window);
  status->setBtnFingerprintVisible(false); // SVG renderer is outside the QtBase-only toolchain.
  window.setStatusBar(status);
  window.resize(971, 240);
  window.show(); // QT_QPA_PLATFORM=offscreen is enforced by the runner.

  QTimer::singleShot(0, &app, [&] {
    status->setStatus(ConnectionState::Connected, ProcessState::Started, false);
    app.processEvents();
    if (!window.grab().save(output.filePath(QStringLiteral("connected.png"))))
      app.exit(4);
    status->setStatus(ConnectionState::Connecting, ProcessState::Started, false);
    status->setConnectionInterval(5);
    app.processEvents();
    if (!window.grab().save(output.filePath(QStringLiteral("retry.png"))))
      app.exit(4);
    QJsonArray bounds;
    for (QWidget *widget :
         {static_cast<QWidget *>(ui.lineHostname), static_cast<QWidget *>(recent),
          static_cast<QWidget *>(ui.btnToggleCore), static_cast<QWidget *>(status)}) {
      const auto position = widget->mapTo(&window, QPoint());
      bounds.append(
          QJsonObject{
              {"widget", widget->objectName()},
              {"x", position.x()},
              {"y", position.y()},
              {"width", widget->width()},
              {"height", widget->height()},
              {"minimumHintWidth", widget->minimumSizeHint().width()}
          }
      );
    }
    std::cout << QJsonDocument(
                     QJsonObject{
                         {"kind", "actual Qt .ui and widgets; synthetic state"},
                         {"full_application_started", false},
                         {"window_width", window.width()},
                         {"window_height", window.height()},
                         {"device_pixel_ratio", window.devicePixelRatio()},
                         {"bounds", bounds}
                     }
    )
                     .toJson()
                     .toStdString();
    app.quit();
  });
  return app.exec();
}
