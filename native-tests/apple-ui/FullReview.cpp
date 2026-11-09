// SPDX-FileCopyrightText: (C) 2026 Deskflow Developers
// SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
// Real MainWindow and dialogs; execution is disabled in the linked Lab build.
#include "common/I18N.h"
#include "common/LabExecutionPolicy.h"
#include "common/Settings.h"
#include "gui/LabRun.h"
#include "gui/MainWindow.h"
#include "gui/StyleUtils.h"
#include <QApplication>
#include <QComboBox>
#include <QJsonDocument>
#include <QJsonObject>
#include <QLabel>
#include <QStyleHints>
#include <QTimer>
#include <QToolBar>
#include <QUuid>

#if !defined(DESKFLOW_UI_REVIEW_BUILD) || !defined(DESKFLOW_LAB_BUILD) || defined(DESKFLOW_CANDIDATE_BUILD)
#error Full UI review must only link the inert Lab configuration
#endif

int main(int argc, char **argv)
{
  static_assert(!deskflow::lab::kProbeNetworkExecutionEnabled && !deskflow::lab::kCandidateExecutionEnabled);
  QCoreApplication::setApplicationName("DeskflowLab");
  QCoreApplication::setOrganizationName("DeskflowLab");
  QCoreApplication::setApplicationVersion("0.20.8");
  const auto path =
      QStringLiteral(DESKFLOW_LAB_RUNS_ROOT) + "/ui-review-" + QUuid::createUuid().toString(QUuid::WithoutBraces);
  QByteArray executable = argv[0], rootFlag = "--lab-root", rootPath = path.toUtf8(), languageFlag = "--lab-language",
             language = "zh_CN";
  char *prepareArgs[] = {executable.data(), rootFlag.data(), rootPath.data(), languageFlag.data(), language.data()};
  if (deskflow::gui::lab::prepare(5, prepareArgs).isEmpty())
    return 2;
  QApplication app(argc, argv);
  Settings::setValue(Settings::Core::CoreMode, Settings::CoreMode::None);
  Settings::setValue(Settings::Client::RemoteHost, QString());
  Settings::setValue(Settings::Gui::CloseToTray, false);
  I18N::currentLanguage();
  deskflow::gui::updateIconTheme();
  MainWindow window;
  window.setWindowTitle("Deskflow UI Review 0.20.8 — 离线界面验收");
  auto *review = new QToolBar(&window);
  review->setObjectName("uiReviewToolbar");
  review->setMovable(false);
  review->setFloatable(false);
  auto *notice = new QLabel(QStringLiteral("离线验收 · 无网络、身份或输入；仅独立临时配置"), review);
  notice->setObjectName("uiReviewBoundary");
  notice->setWordWrap(true);
  notice->setMargin(8);
  review->addWidget(notice);
  auto *locale = new QComboBox(review);
  locale->setObjectName("reviewLanguage");
  locale->setAccessibleName("Review language");
  locale->addItem(QStringLiteral("中文"), "zh_CN");
  locale->addItem("English", "en");
  review->addWidget(locale);
  QObject::connect(locale, &QComboBox::currentIndexChanged, &window, [locale, notice] {
    I18N::setLanguage(locale->currentData().toString());
    notice->setText(
        locale->currentData() == "en"
            ? QStringLiteral("Offline review · no network, identity or input · isolated local profile")
            : QStringLiteral("离线验收 · 无网络、身份或输入；仅独立临时配置")
    );
  });
  const auto originalPalette = app.palette();
  auto *theme = new QComboBox(review);
  theme->setObjectName("reviewAppearance");
  theme->setAccessibleName("Review appearance");
  theme->addItems({"System", "Light", "Dark"});
  review->addWidget(theme);
  QObject::connect(theme, &QComboBox::currentIndexChanged, &window, [&app, theme, originalPalette] {
    auto palette = originalPalette;
    if (theme->currentIndex() != 0) {
      const bool dark = theme->currentIndex() == 2;
      palette.setColor(QPalette::Window, dark ? QColor("#202124") : QColor("#fafafa"));
      palette.setColor(QPalette::WindowText, dark ? Qt::white : Qt::black);
      palette.setColor(QPalette::Base, dark ? QColor("#2c2d31") : Qt::white);
      palette.setColor(QPalette::Text, dark ? Qt::white : Qt::black);
      palette.setColor(QPalette::Button, dark ? QColor("#303136") : QColor("#ffffff"));
      palette.setColor(QPalette::ButtonText, dark ? Qt::white : Qt::black);
    }
    app.setPalette(palette);
    deskflow::gui::updateIconTheme();
  });
  window.addToolBar(Qt::TopToolBarArea, review);
  window.resize(760, 600);
  window.open();
  QTimer::singleShot(0, &window, [&app, path] {
    // Read platform hints only. Do not override keyboard navigation or OS preferences.
    QFile file(path + "/ui-review-runtime.json");
    const QJsonObject data{
        {"version", "0.20.8"},
        {"platform", QApplication::platformName()},
        {"tab_focus_behavior", int(app.styleHints()->tabFocusBehavior())},
        {"settings", Settings::settingsFile()},
        {"synthetic_profile", true},
        {"input_disabled", true},
        {"network_execution_disabled", true},
        {"identity_and_trust_writes_disabled", true}
    };
    if (file.open(QIODevice::WriteOnly))
      file.write(QJsonDocument(data).toJson());
  });
  return app.exec();
}
