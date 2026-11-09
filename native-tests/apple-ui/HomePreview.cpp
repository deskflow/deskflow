// SPDX-FileCopyrightText: (C) 2026 Deskflow Developers
// SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
// Inert review executable: only the production view and in-memory state.
#include "gui/widgets/ConnectionHome.h"
#include <QApplication>
#include <QComboBox>
#include <QDir>
#include <QEvent>
#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QTimer>
#include <QTranslator>
#include <QVBoxLayout>
using Home = deskflow::gui::ConnectionHome;
static Home::State scene(const QString &name)
{
  Home::State s;
  s.localName = QStringLiteral("这台 Mac");
  if (name == "first-run" || name == "keyboard-focus")
    return s;
  s.role = name.startsWith("server") ? Home::Role::Share : Home::Role::Receive;
  s.canStart = true;
  s.identityReady = true;
  s.hasAddress = name != "receive" && name != "manual" && name != "invalid-address" && name != "cancel-return";
  if (name == "identity-missing")
    s.identityReady = false;
  if (name == "connecting")
    s.stage = Home::Stage::Connecting;
  if (name == "verifying")
    s.stage = Home::Stage::Verifying;
  if (name == "connected")
    s.stage = Home::Stage::Connected;
  if (name == "paused")
    s.stage = Home::Stage::Paused;
  if (name == "server-listening")
    s.stage = Home::Stage::Listening;
  if (name == "retrying") {
    s.stage = Home::Stage::Retrying;
    s.explanation = QStringLiteral("对方暂时不可达。可以等待恢复，或暂停连接。");
  }
  if (name == "recovering") {
    s.stage = Home::Stage::Recovering;
    s.retrySeconds = 8;
  }
  if (name == "identity-changed") {
    s.stage = Home::Stage::NeedsAction;
    s.explanation = QStringLiteral("对方的身份与保存记录不一致。连接已停止，请先核对身份。");
  }
  return s;
}
int main(int argc, char **argv)
{
  QApplication app(argc, argv);
  app.setApplicationName("Deskflow Design Review");
  QString output, qm = QStringLiteral(":/translations/deskflow_zh_CN.qm");
  const auto args = app.arguments();
  for (int i = 1; i + 1 < args.size(); ++i) {
    if (args[i] == "--screenshots")
      output = args[++i];
    else if (args[i] == "--translation")
      qm = args[++i];
  }
  QTranslator translator;
  if (!qm.isEmpty() && translator.load(qm))
    app.installTranslator(&translator);
  QWidget window;
  window.setWindowTitle("Deskflow");
  window.setMinimumSize(620, 480);
  auto *layout = new QVBoxLayout(&window);
  layout->setContentsMargins(0, 0, 0, 0);
  layout->setSpacing(0);
  auto *notice = new QLabel(QStringLiteral("隔离预览 · 仅模拟状态 · 不执行键鼠控制、网络或身份操作"));
  notice->setObjectName("previewNotice");
  notice->setWordWrap(true);
  notice->setAlignment(Qt::AlignCenter);
  notice->setStyleSheet(
      "QLabel#previewNotice{background:#eef0f3;color:#6d717a;border:0;padding:6px 12px;font-size:11px;}"
  );
  layout->addWidget(notice);
  auto *selector = new QComboBox;
  selector->setObjectName("previewSelector");
  selector->setAccessibleName("Preview scenario");
  const QStringList names = {"first-run",        "receive",          "manual",       "invalid-address",
                             "cancel-return",    "identity-missing", "client-ready", "server-ready",
                             "connecting",       "verifying",        "connected",    "paused",
                             "server-listening", "retrying",         "recovering",   "identity-changed",
                             "keyboard-focus"};
  selector->addItems(names);
  layout->addWidget(selector);
  if (!output.isEmpty()) {
    notice->hide();
    selector->hide();
  }
  auto *home = new Home;
  layout->addWidget(home, 1);
  auto syncTheme = [&](const QPalette &palette) {
    app.setPalette(palette);
    QEvent appearance(QEvent::PaletteChange);
    QApplication::sendEvent(home, &appearance);
    app.processEvents();
  };
  home->setState(scene("first-run"));
  auto setScene = [&](const QString &name) {
    home->setState(scene(name));
    if (name == "manual" || name == "invalid-address" || name == "cancel-return") {
      home->showManualEntry();
      auto *edit = home->findChild<QLineEdit *>("homeAddress");
      if (name == "invalid-address") {
        edit->setText("https://example.invalid/");
        home->findChild<QPushButton *>("homePrimary")->click();
      }
      if (name == "cancel-return") {
        edit->setText("discard.invalid");
        home->findChild<QPushButton *>("homeBack")->click();
      }
    }
    if (name == "keyboard-focus")
      home->findChild<QPushButton *>("homeReceive")->setFocus(Qt::TabFocusReason);
    notice->setText(QStringLiteral("隔离预览 · 模拟状态：%1 · 不执行输入、网络或身份操作").arg(name));
  };
  QObject::connect(selector, &QComboBox::currentTextChanged, &window, setScene);
  QObject::connect(home, &Home::roleChosen, &window, [&](Home::Role role) {
    auto s = scene(role == Home::Role::Share ? "server-ready" : "receive");
    s.identityReady = false;
    home->setState(s);
    notice->setText(QStringLiteral("隔离预览 · 控制方向仅写入内存，未修改任何应用配置"));
  });
  QObject::connect(home, &Home::addressChosen, &window, [&](const QString &) {
    auto s = home->state();
    s.hasAddress = true;
    home->setState(s);
  });
  QObject::connect(home, &Home::identitySetupRequested, &window, [&] {
    notice->setText(QStringLiteral("隔离预览 · 已捕获身份设置意图，未创建身份"));
  });
  QObject::connect(home, &Home::startStopRequested, &window, [&] {
    notice->setText(QStringLiteral("隔离预览 · 已捕获开始/停止意图，未启动任何核心"));
  });
  window.resize(1024, 740);
  window.show();
  if (!output.isEmpty())
    QTimer::singleShot(0, &window, [&] {
      QDir().mkpath(output);
      QJsonArray evidence;
      const auto light = app.palette();
      for (const auto &theme : QStringList{"light", "dark"}) {
        auto p = light;
        if (theme == "dark") {
          p.setColor(QPalette::Window, QColor("#202124"));
          p.setColor(QPalette::WindowText, Qt::white);
          p.setColor(QPalette::Base, QColor("#2c2d31"));
          p.setColor(QPalette::Text, Qt::white);
          p.setColor(QPalette::Button, QColor("#303136"));
          p.setColor(QPalette::ButtonText, Qt::white);
          p.setColor(QPalette::PlaceholderText, QColor("#7e838c"));
        }
        syncTheme(p);
        for (const auto &name : names) {
          home->setState({});
          home->showRoleSelection(); // discard prior preview-only editing state
          home->findChild<QPushButton *>("homeBack")->click();
          selector->setCurrentText(name);
          setScene(name);
          window.resize(760, 560);
          app.processEvents();
          const auto file = theme + "-" + name + ".png";
          const bool ok = window.grab().save(QDir(output).filePath(file));
          evidence.append(
              QJsonObject{
                  {"file", file},
                  {"saved", ok},
                  {"scenario", name},
                  {"theme", theme},
                  {"width", window.width()},
                  {"height", window.height()},
                  {"synthetic_state", true},
                  {"production_view", true}
              }
          );
        }
      }
      syncTheme(light);
      for (const auto size : {QSize(620, 480), QSize(1024, 740)}) {
        selector->setCurrentText("first-run");
        setScene("first-run");
        window.resize(size);
        app.processEvents();
        const auto file = QString("light-first-run-%1x%2.png").arg(size.width()).arg(size.height());
        window.grab().save(QDir(output).filePath(file));
        evidence.append(
            QJsonObject{
                {"file", file},
                {"width", window.width()},
                {"height", window.height()},
                {"synthetic_state", true},
                {"production_view", true}
            }
        );
      }
      for (const auto &name : QStringList{"client-ready", "connected", "server-ready"}) {
        for (const auto size : {QSize(620, 480), QSize(1024, 740)}) {
          selector->setCurrentText(name);
          setScene(name);
          window.resize(size);
          app.processEvents();
          const auto file = QString("light-%1-%2x%3.png").arg(name).arg(size.width()).arg(size.height());
          window.grab().save(QDir(output).filePath(file));
          evidence.append(
              QJsonObject{
                  {"file", file},
                  {"scenario", name},
                  {"width", window.width()},
                  {"height", window.height()},
                  {"synthetic_state", true},
                  {"production_view", true}
              }
          );
        }
      }
      auto dark = light;
      dark.setColor(QPalette::Window, QColor("#202124"));
      dark.setColor(QPalette::WindowText, Qt::white);
      dark.setColor(QPalette::Base, QColor("#2c2d31"));
      dark.setColor(QPalette::Text, Qt::white);
      dark.setColor(QPalette::Button, QColor("#303136"));
      dark.setColor(QPalette::ButtonText, Qt::white);
      dark.setColor(QPalette::PlaceholderText, QColor("#7e838c"));
      syncTheme(dark);
      for (const auto &name : QStringList{"client-ready", "connected"}) {
        selector->setCurrentText(name);
        setScene(name);
        window.resize(1024, 740);
        app.processEvents();
        const auto file = QString("dark-%1-1024x740.png").arg(name);
        window.grab().save(QDir(output).filePath(file));
        evidence.append(
            QJsonObject{
                {"file", file},
                {"scenario", name},
                {"theme", "dark"},
                {"width", window.width()},
                {"height", window.height()},
                {"synthetic_state", true},
                {"production_view", true}
            }
        );
      }
      syncTheme(light);
      QFile record(QDir(output).filePath("screenshots.json"));
      if (!record.open(QIODevice::WriteOnly) || record.write(QJsonDocument(evidence).toJson()) < 0)
        app.exit(2);
      else
        app.quit();
    });
  return app.exec();
}
