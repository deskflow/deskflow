// SPDX-FileCopyrightText: (C) 2026 Deskflow Developers
// SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
// Actual production widgets/.ui with synthetic typed events. No MainWindow,
// CoreProcess, network, IPC, Settings singleton or input capture is constructed.
#include "gui/dialogs/DesktopLayoutEditor.h"
#include "gui/widgets/ConnectionDiagnostics.h"
#include "gui/widgets/RecentServersButton.h"
#include "ui_MainWindow.h"
#include <QApplication>
#include <QDir>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QTimer>
#include <QTranslator>
#include <iostream>

int main(int argc, char **argv)
{
  if (qEnvironmentVariable("QT_QPA_PLATFORM") != "offscreen")
    return 2;
  QApplication app(argc, argv);
  QTranslator applicationTranslation, qtTranslation;
  if (qEnvironmentVariable("DESKFLOW_PREVIEW_LANGUAGE") == "zh_CN") {
    const auto directory = qEnvironmentVariable("DESKFLOW_PREVIEW_TRANSLATIONS");
    if (!applicationTranslation.load(directory + "/deskflow_zh_CN.qm") ||
        !qtTranslation.load(directory + "/qt_zh_CN.qm"))
      return 6;
    app.installTranslator(&applicationTranslation);
    app.installTranslator(&qtTranslation);
  }
  if (argc != 3)
    return 3;
  app.setStyle("Fusion");
  if (QString::fromLocal8Bit(argv[2]) == "dark") {
    QPalette p;
    p.setColor(QPalette::Window, QColor("#202630"));
    p.setColor(QPalette::WindowText, QColor("#e7edf5"));
    p.setColor(QPalette::Base, QColor("#18202c"));
    p.setColor(QPalette::Text, QColor("#e7edf5"));
    p.setColor(QPalette::Button, QColor("#303b4b"));
    p.setColor(QPalette::ButtonText, QColor("#e7edf5"));
    p.setColor(QPalette::Highlight, QColor("#72a8f6"));
    p.setColor(QPalette::HighlightedText, QColor("#111c2c"));
    p.setColor(QPalette::Disabled, QPalette::ButtonText, QColor("#8791a2"));
    p.setColor(QPalette::Disabled, QPalette::Text, QColor("#8791a2"));
    app.setPalette(p);
  }
  const QDir output(QString::fromLocal8Bit(argv[1]));
  if (!output.exists())
    return 4;
  QMainWindow window;
  Ui::MainWindow ui;
  ui.setupUi(&window);
  ui.lblComputerName->setText("Mac · example");
  ui.lineEditName->hide();
  ui.lblIpAddresses->hide();
  ui.serverOptions->hide();
  ui.lblNoMode->hide();
  ui.rbModeClient->setChecked(true);
  ui.clientOptions->show();
  ui.lineHostname->setText("workstation.local");
  ui.btnToggleCore->setText(QCoreApplication::translate("MainWindow", "Disconnect"));
  ui.btnToggleCore->setEnabled(true);
  ui.btnRestartCore->setEnabled(true);
  auto *recent = new RecentServersButton(&window);
  ui.clientOptions->layout()->addWidget(recent);
  recent->setConnectionAllowed(true);
  recent->beginAttempt("workstation.local", 24800, true);
  recent->secureSocket(true);
  recent->connectionStateChanged(deskflow::core::ConnectionState::Connected);
  auto *diagnostics = new ConnectionDiagnostics(&window);
  qobject_cast<QBoxLayout *>(ui.topLevelWidget->layout())->insertWidget(1, diagnostics);
  window.setWindowTitle("Deskflow · isolated UI review");
  window.resize(960, 430);
  window.show();
  QJsonArray images;
  auto capture = [&](QWidget &widget, const QString &name) {
    app.processEvents();
    const auto png = widget.grab();
    if (!png.save(output.filePath(name + ".png")))
      return false;
    images.append(
        QJsonObject{
            {"file", name + ".png"},
            {"width", widget.width()},
            {"height", widget.height()},
            {"pixelWidth", png.width()},
            {"pixelHeight", png.height()},
            {"dpr", png.devicePixelRatio()}
        }
    );
    return true;
  };
  QTimer::singleShot(0, &app, [&] {
    using deskflow::core::ProcessState;
    using deskflow::core::ConnectionState;
    diagnostics->beginAttempt(false, "workstation.local", 24800, true, true);
    diagnostics->processStateChanged(ProcessState::Started);
    diagnostics->connectionStateChanged(ConnectionState::Connecting);
    diagnostics->secureSocket(true);
    diagnostics->connectionStateChanged(ConnectionState::Connected);
    if (!capture(window, "connection-connected")) {
      app.exit(5);
      return;
    }
    diagnostics->connectionStateChanged(ConnectionState::Disconnected);
    diagnostics->retryIn(5);
    if (!capture(window, "connection-retry")) {
      app.exit(5);
      return;
    }
    diagnostics->requireAttention(ConnectionDiagnostics::Attention::Identity);
    if (!capture(window, "connection-identity")) {
      app.exit(5);
      return;
    }
    diagnostics->beginAttempt(false, QString(220, 'a') + ".example.invalid", 25000, true, true);
    diagnostics->processStateChanged(ProcessState::Started);
    diagnostics->connectionRefused(deskflow::core::ConnectionRefusal::AlreadyConnected);
    diagnostics->findChild<QPushButton *>("diagnosticDetailsButton")->setChecked(true);
    window.resize(720, 650);
    if (!capture(window, "connection-long-details")) {
      app.exit(5);
      return;
    }
    DesktopLayoutEditor editor({{"Mac", 0, 0, 1440, 900}, {"Windows", 1440, 180, 1920, 1080}});
    editor.show();
    if (!capture(editor, "layout-aligned")) {
      app.exit(5);
      return;
    }
    editor.moveDesktop("Windows", 700, 100, false);
    if (!capture(editor, "layout-invalid")) {
      app.exit(5);
      return;
    }
    editor.undo();
    editor.reject();
    DesktopLayoutEditor longLayout(
        {{std::string(100, 'm') + "-Mac", -1440, -200, 1440, 900}, {"Windows-125-percent", 0, 0, 2048, 1152}}
    );
    longLayout.resize(710, 650);
    longLayout.show();
    if (!capture(longLayout, "layout-long-scaled")) {
      app.exit(5);
      return;
    }
    std::cout << QJsonDocument(
                     QJsonObject{
                         {"kind", "actual Qt widgets and .ui; synthetic typed events"},
                         {"full_application_started", false},
                         {"settings_singleton_started", false},
                         {"images", images}
                     }
    )
                     .toJson()
                     .toStdString();
    app.quit();
  });
  return app.exec();
}
