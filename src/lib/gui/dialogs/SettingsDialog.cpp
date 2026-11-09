#include "WindowsStartupDialog.h"
#include <QCoreApplication>
#include <QStandardPaths>
/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2025 - 2026 Deskflow Developers
 * SPDX-FileCopyrightText: (C) 2012 Synergy App Ltd
 * SPDX-FileCopyrightText: (C) 2008 Volker Lanz <vl@fidra.de>
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#include "SettingsDialog.h"
#include "StartupRecoveryDialog.h"
#include "common/CandidatePolicy.h"
#include "common/LogLevel.h"
#include "common/PlatformInfo.h"
#include "ui_SettingsDialog.h"

#include "common/I18N.h"
#include "common/Settings.h"
#include "gui/ExperienceStyle.h"
#include "gui/TlsUtility.h"
#include "gui/core/NetworkMonitor.h"
#include "gui/widgets/SettingsDialogButtonBox.h"

#include <QComboBox>
#include <QDir>
#include <QFileDialog>
#include <QFileInfo>
#include <QLabel>
#include <QListWidget>
#include <QMessageBox>
#include <QScrollArea>
#include <QSignalBlocker>
#include <QStyle>
#include <QTabBar>
#include <QToolButton>
#include <QVBoxLayout>

using namespace deskflow::gui;

SettingsDialog::SettingsDialog(
    QWidget *parent, const ServerConfig &serverConfig, std::shared_ptr<startup::Reader> startupReader
)
    : QDialog(parent),
      ui{std::make_unique<Ui::SettingsDialog>()},
      m_serverConfig(serverConfig),
      m_buttonBox{new SettingsDialogButtonBox(this)}
{
  m_startupReader = startupReader ? std::move(startupReader) : startup::makeReadOnlyReader();

  ui->setupUi(this);
  setupPresentation();
  ui->tabWidget->setCurrentIndex(0);

  // these are enabled by the control next to them
  ui->lineCommandEnter->setEnabled(false);
  ui->lineCommandExit->setEnabled(false);

  // set up the language combo
  I18N::reDetectLanguages();
  ui->comboLanguage->addItems(I18N::detectedLanguages());
  ui->comboLanguage->setCurrentText(I18N::toNativeName(I18N::currentLanguage()));

  updateText();

  ui->btnClearAllSettings->setIcon(QIcon::fromTheme(QStringLiteral("edit-clear-all")));

  ui->comboTlsKeyLength->setItemIcon(0, QIcon::fromTheme(QStringLiteral("security-medium")));
  ui->comboTlsKeyLength->setItemIcon(1, QIcon::fromTheme(QIcon::ThemeIcon::SecurityHigh));
  ui->lblTlsCertInfo->setFixedSize(28, 28);

  ui->rbIconMono->setIcon(
      QIcon::fromTheme(
          QStringLiteral("org.deskflow.deskflow-symbolic"),
          QIcon(QStringLiteral(":/icons/deskflow-light/apps/64/org.deskflow.deskflow-symbolic.svg"))
      )
  );
  ui->rbIconColorful->setIcon(
      QIcon::fromTheme(
          QStringLiteral("org.deskflow.deskflow"),
          QIcon(QStringLiteral(":/icons/deskflow-light/apps/64/org.deskflow.deskflow.svg"))
      )
  );

  // force the first tab, since qt creator sets the active tab as the last one
  // the developer was looking at, and it's easy to accidentally save that.
  ui->tabWidget->setCurrentIndex(0);

  // Populate the list of IP addresses
  const auto validAddresses = NetworkMonitor::validAddresses();
  for (const auto &address : validAddresses) {
    QString ipString = address;
    if (ui->comboInterface->findText(ipString) == -1) {
      ui->comboInterface->addItem(ipString, ipString);
    }
  }

  if (const auto interface = Settings::value(Settings::Core::Interface).toString();
      !interface.isEmpty() && (ui->comboInterface->findData(interface) == -1)) {
    ui->comboInterface->addItem(interface, interface);
  }

  loadFromConfig();
  logLevelChanged();

  resize(760, 560);
  setWindowFlags((windowFlags() | Qt::CustomizeWindowHint) & ~Qt::WindowMinMaxButtonsHint);

  setButtonBoxEnabledButtons();
  initConnections();
#ifdef DESKFLOW_UI_REVIEW_BUILD
  auto *reviewNote = new QLabel(tr("Isolated UI review: changes are discarded when this dialog closes."), this);
  reviewNote->setObjectName("settingsReviewBoundary");
  reviewNote->setWordWrap(true);
  reviewNote->setContentsMargins(18, 10, 18, 10);
  qobject_cast<QVBoxLayout *>(layout())->insertWidget(0, reviewNote);
  enforceReviewBoundary();
#endif
}

void SettingsDialog::setupPresentation()
{
  m_startupRecovery = new QPushButton(tr("Startup & recovery…"), ui->tabGeneral);
  m_startupRecovery->setObjectName("startupRecoverySettings");
  m_startupRecovery->setAutoDefault(false);
  qobject_cast<QVBoxLayout *>(ui->tabGeneral->layout())->addWidget(m_startupRecovery);
  connect(m_startupRecovery, &QPushButton::clicked, this, [this] {
#if defined(Q_OS_WIN) && defined(DESKFLOW_CANDIDATE_BUILD) && !defined(DESKFLOW_LAB_BUILD)
    {
      WindowsStartupDialog dialog(
          winstartup::makeNativeSystem(QCoreApplication::applicationFilePath()),
          QStandardPaths::writableLocation(QStandardPaths::AppLocalDataLocation) + "/startup/previous-task.json", this
      );
      dialog.exec();
      return;
    }
#endif
    StartupRecoveryDialog dialog(
        m_startupReader,
        {Settings::value(deskflow::candidate::pausedKey).toBool(),
         Settings::value(Settings::Gui::AutoStartCore).toBool(), ui->cbPreventSleep->isChecked()},
        this
    );
    connect(&dialog, &StartupRecoveryDialog::requestStartupQuit, this, [this, &dialog] {
      dialog.reject();
      reject();
      Q_EMIT requestStartupQuit();
    });
    dialog.exec();
  });
  auto *elevationHelp = new QLabel(ui->groupService);
  elevationHelp->setObjectName("elevationHelp");
  elevationHelp->setWordWrap(true);
  qobject_cast<QGridLayout *>(ui->groupService->layout())->addWidget(elevationHelp, 2, 0);
  setMinimumSize(640, 480);
  auto *root = qobject_cast<QVBoxLayout *>(layout());
  root->setContentsMargins(0, 0, 0, 0);
  root->setSpacing(0);
  root->removeWidget(ui->tabWidget);
  ui->tabWidget->tabBar()->hide();

  auto *row = new QHBoxLayout;
  row->setContentsMargins(0, 0, 0, 0);
  row->setSpacing(0);
  m_navigation = new QListWidget(this);
  m_navigation->setObjectName("settingsNavigation");
  m_navigation->setFixedWidth(154);
  m_navigation->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
  for (int page : {0, 1, 3, 2, 4}) {
    auto *item = new QListWidgetItem(m_navigation);
    item->setData(Qt::UserRole, page);
    item->setSizeHint(QSize(138, 38));
  }
  row->addWidget(m_navigation);
  auto *content = new QWidget(this);
  content->setObjectName("settingsContent");
  auto *column = new QVBoxLayout(content);
  column->setContentsMargins(20, 18, 20, 16);
  column->setSpacing(12);
  m_pageTitle = new QLabel(content);
  m_pageTitle->setObjectName("settingsPageTitle");
  m_pageTitle->setWordWrap(true);
  m_pageDescription = new QLabel(content);
  m_pageDescription->setObjectName("settingsPageDescription");
  m_pageDescription->setWordWrap(true);
  column->addWidget(m_pageTitle);
  column->addWidget(m_pageDescription);
  column->addSpacing(4);
  column->addWidget(ui->tabWidget, 1);
  row->addWidget(content, 1);
  root->addLayout(row, 1);
  m_buttonBox->setObjectName("settingsFooter");
  m_buttonBox->layout()->setContentsMargins(20, 12, 24, 12);
  root->addWidget(m_buttonBox);

  m_connectionDetailsBody = new QWidget(ui->tabNetwork);
  m_connectionDetailsBody->setObjectName("connectionDetailsBody");
  // Transfer the layout as a whole so its controls are reparented with it.
  // Moving raw layout items alone leaves fields behind the new scroll viewport.
  auto *details = qobject_cast<QVBoxLayout *>(ui->tabNetwork->layout());
  m_connectionDetailsBody->setLayout(details);
  details->setContentsMargins(0, 4, 0, 0);
  details->setSpacing(18);
  for (int i = details->count() - 1; i >= 0; --i)
    if (details->itemAt(i)->spacerItem())
      delete details->takeAt(i);
  auto *network = new QVBoxLayout(ui->tabNetwork);
  ui->horizontalLayout_5->removeWidget(ui->btnTlsRegenCert);
  m_identityHeading = new QLabel(ui->tabNetwork);
  m_identityHeading->setObjectName("identityHeading");
  m_identityHeading->setWordWrap(true);
  m_identityStatus = new QLabel(ui->tabNetwork);
  m_identityStatus->setObjectName("identityStatus");
  m_identityStatus->setWordWrap(true);
  m_identityStatus->setTextFormat(Qt::PlainText);
  auto *connectionSetup = new QPushButton(tr("Computers and pairing…"), ui->tabNetwork);
  connectionSetup->setObjectName("connectionSetup");
  connectionSetup->setAutoDefault(false);
  network->addWidget(connectionSetup, 0, Qt::AlignLeft);
  connect(connectionSetup, &QPushButton::clicked, this, [this] {
    if (isModified() &&
        QMessageBox::question(
            this, tr("Unsaved preferences"), tr("Discard unsaved preferences and open computers and pairing?"),
            QMessageBox::Discard | QMessageBox::Cancel, QMessageBox::Cancel
        ) != QMessageBox::Discard)
      return;
    Q_EMIT requestConnectionSetup();
    reject();
  });
  network->addWidget(m_identityHeading);
  network->addWidget(m_identityStatus);
  m_identityAction = new QHBoxLayout;
  m_identityAction->addWidget(ui->btnTlsRegenCert);
  m_identityAction->addStretch();
  ui->btnTlsRegenCert->setProperty("primaryAction", true);
  ui->btnTlsRegenCert->setIcon(QIcon());
  ui->btnTlsRegenCert->setAutoDefault(false);
  network->addLayout(m_identityAction);
  network->addSpacing(16);
  m_connectionDetails = new QToolButton(ui->tabNetwork);
  m_connectionDetails->setObjectName("connectionDetailsToggle");
  m_connectionDetails->setCheckable(true);
  m_connectionDetails->setArrowType(Qt::RightArrow);
  m_connectionDetails->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
  network->addWidget(m_connectionDetails, 0, Qt::AlignLeft);
  network->addWidget(m_connectionDetailsBody);
  m_connectionDetailsBody->hide();
  connect(m_connectionDetails, &QToolButton::toggled, this, [this](bool open) {
    m_connectionDetailsBody->setVisible(open);
    m_connectionDetails->setArrowType(open ? Qt::DownArrow : Qt::RightArrow);
  });

  for (auto *page : {ui->tabGeneral, ui->tabWindow, ui->tabLog, ui->tabNetwork, ui->tabAdvanced}) {
    auto *body = new QWidget(page);
    body->setObjectName("settingsPageBody");
    auto *old = qobject_cast<QVBoxLayout *>(page->layout());
    for (int i = old->count() - 1; i >= 0; --i)
      if (old->itemAt(i)->spacerItem())
        delete old->takeAt(i);
    old->setContentsMargins(0, 8, 8, 20);
    old->setSpacing(12);
    old->addStretch();
    body->setLayout(old);
    auto *scroll = new QScrollArea(page);
    scroll->setObjectName("settingsPageScroll");
    scroll->viewport()->setObjectName("settingsPageViewport");
    scroll->setFrameShape(QFrame::NoFrame);
    scroll->setWidgetResizable(true);
    scroll->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    scroll->setWidget(body);
    auto *pageLayout = new QVBoxLayout(page);
    pageLayout->setContentsMargins(0, 0, 0, 0);
    pageLayout->addWidget(scroll);
  }
  for (auto *form : findChildren<QFormLayout *>()) {
    form->setRowWrapPolicy(QFormLayout::WrapLongRows);
    form->setVerticalSpacing(12);
    form->setHorizontalSpacing(20);
  }
  connect(m_navigation, &QListWidget::currentRowChanged, this, [this](int row) {
    if (row >= 0)
      ui->tabWidget->setCurrentIndex(m_navigation->item(row)->data(Qt::UserRole).toInt());
  });
  connect(ui->tabWidget, &QTabWidget::currentChanged, this, [this] { updatePageHeading(); });
  updateAppearance();
}

void SettingsDialog::updateAppearance()
{
  if (m_styling)
    return;
  m_styling = true;
  deskflow::gui::applyExperienceStyle(this);
  const bool dark = QApplication::palette().color(QPalette::Window).lightness() < 128;
  const auto sheet =
      QString(
          "QWidget{font-size:12pt;} QDialog#SettingsDialog,QWidget#settingsContent{background:%1;color:%2;}"
          "QLabel,QCheckBox,QRadioButton,QGroupBox{color:%2;}"
          "QLabel#settingsPageTitle{font-size:20pt;font-weight:600;}"
          "QLabel#settingsPageDescription,QLabel#identityStatus{color:%3;}"
          "QLabel#identityHeading{font-size:16pt;font-weight:600;}"
          "QPushButton,QToolButton{font-size:12pt;min-height:20px;}"
          "QWidget#settingsPageBody,QWidget#settingsPageViewport,QScrollArea#settingsPageScroll{background:%1;}"
          "QTabWidget::pane{border:0;background:transparent;}"
          "QListWidget#settingsNavigation{font-size:12pt;border:0;border-right:1px solid %4;background:%5;padding:18px "
          "8px;}"
          "QListWidget#settingsNavigation::item{padding:0 12px;border-radius:6px;color:%2;}"
          "QListWidget#settingsNavigation::item:selected{background:%6;color:%2;}"
          "QListWidget#settingsNavigation::item:focus{border:2px solid #0067d9;}"
          "QGroupBox{border:0;margin-top:18px;padding-top:12px;}"
          "QGroupBox::title{subcontrol-origin:margin;left:0;top:0;}"
          "QCheckBox,QRadioButton{min-height:26px;}"
          "QToolButton#connectionDetailsToggle{border:0;padding:4px 0;background:transparent;}"
          "QToolButton#settingsMoreActions{border:0;border-radius:6px;padding:6px 24px 6px "
          "10px;background:transparent;color:%3;}"
          "QToolButton#settingsMoreActions:hover{background:%6;color:%2;}"
          "QToolButton#settingsMoreActions:focus{border:2px solid #0067d9;padding:4px 22px 4px 8px;}"
          "QToolButton#settingsMoreActions::menu-indicator{image:url(:/icons/deskflow-%7/actions/16/"
          "go-down.svg);width:12px;height:12px;subcontrol-origin:padding;subcontrol-position:right center;}"
          "QWidget#settingsFooter{border-top:1px solid %4;background:%1;}"
      )
          .arg(
              dark ? "#202124" : "#fafafa", dark ? "#f5f5f7" : "#1d1d1f", dark ? "#afb1b8" : "#62656d",
              dark ? "#45464b" : "#d9dadd", dark ? "#27282c" : "#f0f0f2", dark ? "#3b4657" : "#dce8f8",
              dark ? "dark" : "light"
          );
  // Keep native metrics and checked/disabled rendering. Some Qt styles paint
  // empty indicators almost the same color as our dark settings surface.
  const auto indicators =
      dark ? QString(
                 "QCheckBox::indicator:unchecked:enabled{image:url(:/icons/deskflow-dark/status/16/"
                 "checkbox-unchecked.svg);width:%1px;height:%2px;}"
                 "QRadioButton::indicator:unchecked:enabled{image:url(:/icons/deskflow-dark/status/16/"
                 "radio-unchecked.svg);width:%3px;height:%4px;}"
             )
                 .arg(style()->pixelMetric(QStyle::PM_IndicatorWidth, nullptr, ui->cbShowVersion))
                 .arg(style()->pixelMetric(QStyle::PM_IndicatorHeight, nullptr, ui->cbShowVersion))
                 .arg(style()->pixelMetric(QStyle::PM_ExclusiveIndicatorWidth, nullptr, ui->rbAutoHide))
                 .arg(style()->pixelMetric(QStyle::PM_ExclusiveIndicatorHeight, nullptr, ui->rbAutoHide))
           : QString();
  setStyleSheet(styleSheet() + sheet + indicators);
  m_styling = false;
}

void SettingsDialog::updatePageHeading()
{
  const QStringList names = {
      tr("General"), tr("Window behavior"), tr("Logging"), tr("Connection & identity"), tr("Advanced")
  };
  const QStringList navigationNames = {tr("General"), tr("Window"), tr("Logging"), tr("Connection"), tr("Advanced")};
  const QStringList descriptions = {
      tr("Language and everyday preferences."), tr("Choose how Deskflow opens and closes."),
      tr("Keep diagnostic details available when you need them."),
      tr("Prepare this computer for a verified connection."), tr("Optional actions for a custom setup.")
  };
  const int page = ui->tabWidget->currentIndex();
  m_pageTitle->setText(names.value(page));
  m_pageDescription->setText(descriptions.value(page));
  const QSignalBlocker blocker(m_navigation);
  for (int row = 0; row < m_navigation->count(); ++row) {
    auto *item = m_navigation->item(row);
    const int index = item->data(Qt::UserRole).toInt();
    item->setText(navigationNames.at(index));
    if (index == page)
      m_navigation->setCurrentRow(row);
  }
}

void SettingsDialog::changeEvent(QEvent *e)
{
  QDialog::changeEvent(e);
  if (e->type() == QEvent::LanguageChange) {
    ui->retranslateUi(this);
    m_startupRecovery->setText(tr("Startup & recovery…"));
    updateText();
    updateTlsControls();
  }
  if (m_navigation && (e->type() == QEvent::PaletteChange || e->type() == QEvent::ApplicationPaletteChange))
    updateAppearance();
}

void SettingsDialog::initConnections() const
{
  connect(m_buttonBox, &SettingsDialogButtonBox::accepted, this, &SettingsDialog::accept);
  connect(m_buttonBox, &SettingsDialogButtonBox::rejected, this, &QDialog::reject);
  connect(m_buttonBox, &SettingsDialogButtonBox::reset, this, &SettingsDialog::loadFromConfig);
  connect(m_buttonBox, &SettingsDialogButtonBox::restoreDefault, this, &SettingsDialog::resetToDefault);

  connect(ui->cbRunEnterCommand, &QCheckBox::toggled, ui->lineCommandEnter, &QLineEdit::setEnabled);
  connect(ui->cbRunExitCommand, &QCheckBox::toggled, ui->lineCommandExit, &QLineEdit::setEnabled);

  connect(ui->groupSecurity, &QGroupBox::toggled, this, &SettingsDialog::updateTlsControlsEnabled);
  connect(ui->groupService, &QGroupBox::toggled, this, &SettingsDialog::updateControls);
  connect(ui->btnClearAllSettings, &QPushButton::clicked, this, &SettingsDialog::resetAllSettings);
  connect(ui->btnTlsRegenCert, &QPushButton::clicked, this, &SettingsDialog::regenCertificates);
  connect(ui->btnTlsCertPath, &QPushButton::clicked, this, &SettingsDialog::browseCertificatePath);
  connect(ui->btnBrowseLog, &QPushButton::clicked, this, &SettingsDialog::browseLogPath);
  connect(ui->groupLogToFile, &QGroupBox::toggled, this, &SettingsDialog::setLogToFile);
  connect(ui->comboLogLevel, &QComboBox::currentIndexChanged, this, &SettingsDialog::logLevelChanged);
#ifndef DESKFLOW_UI_REVIEW_BUILD
  connect(ui->comboLanguage, &QComboBox::currentTextChanged, this, [](const QString &lang) {
    const auto shortName = I18N::nativeTo639Name(lang);
    I18N::setLanguage(shortName);
  });
#endif

  // Connect modifiable controls
  connect(ui->rbIconMono, &QRadioButton::toggled, this, &SettingsDialog::setButtonBoxEnabledButtons);
  connect(ui->sbPort, &QSpinBox::valueChanged, this, &SettingsDialog::setButtonBoxEnabledButtons);
  connect(ui->comboLogLevel, &QComboBox::currentIndexChanged, this, &SettingsDialog::setButtonBoxEnabledButtons);
  connect(ui->comboInterface, &QComboBox::currentIndexChanged, this, &SettingsDialog::setButtonBoxEnabledButtons);
  connect(ui->comboTlsKeyLength, &QComboBox::currentIndexChanged, this, &SettingsDialog::setButtonBoxEnabledButtons);
  connect(ui->comboLanguage, &QComboBox::currentIndexChanged, this, &SettingsDialog::setButtonBoxEnabledButtons);
  connect(ui->rbAutoHide, &QRadioButton::toggled, this, &SettingsDialog::setButtonBoxEnabledButtons);
  connect(ui->cbPreventSleep, &QCheckBox::toggled, this, &SettingsDialog::setButtonBoxEnabledButtons);
  connect(ui->rbCloseToTray, &QRadioButton::toggled, this, &SettingsDialog::setButtonBoxEnabledButtons);
  connect(ui->cbElevateDaemon, &QCheckBox::toggled, this, &SettingsDialog::setButtonBoxEnabledButtons);
  connect(ui->cbAutoUpdate, &QCheckBox::toggled, this, &SettingsDialog::setButtonBoxEnabledButtons);
  connect(ui->cbGuiDebug, &QCheckBox::toggled, this, &SettingsDialog::setButtonBoxEnabledButtons);
  connect(ui->cbShowVersion, &QCheckBox::toggled, this, &SettingsDialog::setButtonBoxEnabledButtons);
  connect(ui->cbRequireClientCert, &QCheckBox::toggled, this, &SettingsDialog::setButtonBoxEnabledButtons);
  connect(ui->groupLogToFile, &QGroupBox::toggled, this, &SettingsDialog::setButtonBoxEnabledButtons);
  connect(ui->groupService, &QGroupBox::toggled, this, &SettingsDialog::setButtonBoxEnabledButtons);
  connect(ui->groupSecurity, &QGroupBox::toggled, this, &SettingsDialog::setButtonBoxEnabledButtons);
  connect(ui->lineLogFilename, &QLineEdit::textChanged, this, &SettingsDialog::setButtonBoxEnabledButtons);
  connect(ui->lineTlsCertPath, &QLineEdit::textChanged, this, &SettingsDialog::setButtonBoxEnabledButtons);
  connect(ui->cbRunEnterCommand, &QCheckBox::toggled, this, &SettingsDialog::setButtonBoxEnabledButtons);
  connect(ui->cbRunExitCommand, &QCheckBox::toggled, this, &SettingsDialog::setButtonBoxEnabledButtons);
  connect(ui->lineCommandEnter, &QLineEdit::textChanged, this, &SettingsDialog::setButtonBoxEnabledButtons);
  connect(ui->lineCommandExit, &QLineEdit::textChanged, this, &SettingsDialog::setButtonBoxEnabledButtons);
  connect(Settings::instance(), &Settings::settingsWritableChanged, this, &SettingsDialog::updateControls);
}

void SettingsDialog::regenCertificates()
{
#ifdef DESKFLOW_UI_REVIEW_BUILD
  return;
#endif
  // Selecting a length is a draft edit; only the explicit generation action
  // needs to publish it before the form is saved.
  updateRequestedKeySize();
  if (TlsUtility::generateCertificate()) {
    QMessageBox::information(this, tr("TLS Certificate Regenerated"), tr("TLS certificate regenerated successfully."));
    const auto certificate = Settings::value(Settings::Security::Certificate).toString();
    updateKeyLengthOnFile(certificate);
    updateTlsControls();
  }
}

void SettingsDialog::browseCertificatePath()
{
#ifdef DESKFLOW_UI_REVIEW_BUILD
  return;
#endif
  QString fileName = QFileDialog::getSaveFileName(
      this, tr("Select a TLS certificate to use..."), ui->lineTlsCertPath->text(), "Cert (*.pem)", nullptr,
      QFileDialog::DontConfirmOverwrite
  );

  if (!fileName.isEmpty()) {
    ui->lineTlsCertPath->setText(fileName);

    if (QFile(fileName).exists()) {
      updateKeyLengthOnFile(fileName);
    } else {
      qDebug("no tls certificate file at: %s", qUtf8Printable(fileName));
    }
  }
}

void SettingsDialog::browseLogPath()
{
#ifdef DESKFLOW_UI_REVIEW_BUILD
  return;
#endif
  QString fileName =
      QFileDialog::getSaveFileName(this, tr("Save log file to..."), ui->lineLogFilename->text(), "Logs (*.log *.txt)");

  if (!fileName.isEmpty()) {
    ui->lineLogFilename->setText(fileName);
  }
}

void SettingsDialog::setLogToFile(bool logToFile)
{
  ui->widgetLogFilename->setEnabled(logToFile);
}

void SettingsDialog::resetAllSettings()
{
#ifdef DESKFLOW_UI_REVIEW_BUILD
  return;
#endif
  auto result = QMessageBox::question(
      this, tr("%1 Clear Settings").arg(kAppName),
      tr("<p>Are you sure you want to clear all settings and restart %1?</p> <p>This action cannot be undone.</p>")
          .arg(kAppName)
  );
  if (result == QMessageBox::Yes)
    Q_EMIT requestRemoveAllSettings();
}

void SettingsDialog::updateText()
{
  updatePageHeading();
  m_navigation->setAccessibleName(tr("Settings categories"));
  ui->cbElevateDaemon->setText(tr("Always run as system"));
  const auto elevationHelp = tr("Allows input at the login screen and UAC prompts.");
  ui->cbElevateDaemon->setAccessibleDescription(elevationHelp);
  findChild<QLabel *>("elevationHelp")->setText(elevationHelp);
  m_connectionDetails->setText(tr("Advanced connection settings"));
  ui->btnTlsCertPath->setText(tr("Choose…"));
  ui->btnBrowseLog->setText(tr("Choose…"));
  ui->btnTlsCertPath->setIcon(QIcon());
  ui->btnBrowseLog->setIcon(QIcon());
  ui->btnTlsCertPath->setAccessibleName(tr("Choose identity file"));
  ui->btnBrowseLog->setAccessibleName(tr("Choose log file location"));
  const auto logLevels = LogLevel::logLevelNames().count();
  const QStringList toolTips = {tr("Required messages"),        tr("Non-fatal errors"), tr("General warnings"),
                                tr("General events [Default]"), tr("Debug entries"),    tr("Verbose debug output")};
  if (ui->comboLogLevel->count() == 0) {
    const auto logLevelOptions = LogLevel::logLevelOptions();
    for (int i = 0; i < logLevels; i++) {
      ui->comboLogLevel->addItem(LogLevel::toString(i));
      ui->comboLogLevel->setItemData(i, logLevelOptions.at(i), Qt::UserRole);
      ui->comboLogLevel->setItemData(i, toolTips.at(i), Qt::ToolTipRole);
    }
  } else {
    for (int i = 0; i < logLevels; i++) {
      ui->comboLogLevel->setItemData(i, LogLevel::toString(i), Qt::DisplayRole);
      ui->comboLogLevel->setItemData(i, toolTips.at(i), Qt::ToolTipRole);
    }
  }
}

void SettingsDialog::accept()
{
#ifdef DESKFLOW_UI_REVIEW_BUILD
  // The real form is reviewable without persisting security/service choices.
  QDialog::accept();
  return;
#endif
  Settings::setValue(Settings::Core::Port, ui->sbPort->value());
  Settings::setValue(Settings::Core::Interface, ui->comboInterface->currentData());
  Settings::setValue(Settings::Log::Level, ui->comboLogLevel->currentData());
  Settings::setValue(Settings::Log::ToFile, ui->groupLogToFile->isChecked());
  Settings::setValue(Settings::Log::File, ui->lineLogFilename->text());
  Settings::setValue(Settings::Daemon::Elevate, ui->cbElevateDaemon->isChecked());
  Settings::setValue(Settings::Gui::Autohide, ui->rbAutoHide->isChecked());
  Settings::setValue(Settings::Gui::AutoUpdateCheck, ui->cbAutoUpdate->isChecked());
  Settings::setValue(Settings::Core::PreventSleep, ui->cbPreventSleep->isChecked());
  Settings::setValue(Settings::Security::Certificate, ui->lineTlsCertPath->text());
  Settings::setValue(Settings::Security::KeySize, ui->comboTlsKeyLength->currentText().toInt());
  Settings::setValue(Settings::Security::TlsEnabled, ui->groupSecurity->isChecked());
  Settings::setValue(Settings::Gui::CloseToTray, ui->rbCloseToTray->isChecked());
  Settings::setValue(Settings::Gui::SymbolicTrayIcon, ui->rbIconMono->isChecked());
  Settings::setValue(Settings::Security::CheckPeers, ui->cbRequireClientCert->isChecked());
  Settings::setValue(Settings::Core::Language, I18N::nativeTo639Name(ui->comboLanguage->currentText()));
  Settings::setValue(Settings::Core::LanguageExplicit, true);
  Settings::setValue(Settings::Log::GuiDebug, ui->cbGuiDebug->isChecked());
  Settings::setValue(Settings::Gui::ShowVersionInTitle, ui->cbShowVersion->isChecked());
  Settings::setValue(Settings::Core::EnableEnterCommand, ui->cbRunEnterCommand->isChecked());
  Settings::setValue(Settings::Core::EnableExitCommand, ui->cbRunExitCommand->isChecked());
  Settings::setValue(Settings::Core::ComputerEnterCommand, ui->lineCommandEnter->text());
  Settings::setValue(Settings::Core::ComputerExitCommand, ui->lineCommandExit->text());

  Settings::ProcessMode mode;
  if (ui->groupService->isChecked())
    mode = Settings::ProcessMode::Service;
  else
    mode = Settings::ProcessMode::Desktop;
  Settings::setValue(Settings::Core::ProcessMode, mode);

  QDialog::accept();
}

void SettingsDialog::loadFromConfig()
{
  ui->sbPort->setValue(Settings::value(Settings::Core::Port).toInt());
  ui->comboLogLevel->setCurrentIndex(
      ui->comboLogLevel->findData(Settings::logLevelText(), Qt::UserRole, Qt::MatchFixedString)
  );
  ui->groupLogToFile->setChecked(Settings::value(Settings::Log::ToFile).toBool());
  ui->lineLogFilename->setText(Settings::value(Settings::Log::File).toString());
  ui->cbPreventSleep->setChecked(Settings::value(Settings::Core::PreventSleep).toBool());
  ui->cbElevateDaemon->setChecked(Settings::value(Settings::Daemon::Elevate).toBool());
  ui->cbAutoUpdate->setChecked(Settings::value(Settings::Gui::AutoUpdateCheck).toBool());
  ui->cbGuiDebug->setChecked(Settings::value(Settings::Log::GuiDebug).toBool());
  ui->cbShowVersion->setChecked(Settings::value(Settings::Gui::ShowVersionInTitle).toBool());
  ui->cbRunEnterCommand->setChecked(Settings::value(Settings::Core::EnableEnterCommand).toBool());
  ui->cbRunExitCommand->setChecked(Settings::value(Settings::Core::EnableExitCommand).toBool());
  ui->lineCommandEnter->setText(Settings::value(Settings::Core::ComputerEnterCommand).toString());
  ui->lineCommandExit->setText(Settings::value(Settings::Core::ComputerExitCommand).toString());

  const auto processMode = Settings::value(Settings::Core::ProcessMode).value<Settings::ProcessMode>();
  ui->groupService->setChecked(processMode == Settings::ProcessMode::Service);

  if (!deskflow::platform::isWindows())
    ui->groupService->setVisible(false);

  if (Settings::value(Settings::Gui::SymbolicTrayIcon).toBool())
    ui->rbIconMono->setChecked(true);
  else
    ui->rbIconColorful->setChecked(true);

  const auto autoHide = Settings::value(Settings::Gui::Autohide).toBool();
  ui->rbAutoHide->setChecked(autoHide);
  ui->rbShowOnStart->setChecked(!autoHide);

  const auto closeToTray = Settings::value(Settings::Gui::CloseToTray).toBool();
  ui->rbCloseToTray->setChecked(closeToTray);
  ui->rbExitOnClose->setChecked(!closeToTray);

  ui->lblDebugWarning->setVisible(
      Settings::value(Settings::Log::Level).toInt() > static_cast<int>(LogLevel::Level::Info)
  );

  ui->comboInterface->setCurrentText(Settings::value(Settings::Core::Interface).toString());
  if (ui->comboInterface->currentIndex() <= 0) {
    ui->comboInterface->setCurrentIndex(0);
    m_interfaceSetOnLoad = false;
  } else {
    m_interfaceSetOnLoad = true;
  }

  qDebug() << "load from config done";

  updateControls();
}

void SettingsDialog::updateTlsControls()
{
  const auto certificate = Settings::value(Settings::Security::Certificate).toString();
  if (QFile(certificate).exists()) {
    updateKeyLengthOnFile(certificate);
  }

  ui->comboTlsKeyLength->setCurrentText(Settings::value(Settings::Security::KeySize).toString());

  ui->lineTlsCertPath->setText(certificate);
  const bool identityPresent = QFileInfo::exists(certificate);
  const bool identityValid = identityPresent && TlsUtility::isCertValid(certificate);
  m_identityHeading->setProperty("identityState", !identityPresent ? "missing" : identityValid ? "ready" : "invalid");
  m_identityHeading->setText(
      !identityPresent ? tr("Set up this computer’s identity")
      : identityValid  ? tr("Identity is ready")
                       : tr("Identity needs attention")
  );
  m_identityStatus->setText(
      !identityPresent ? tr("Create a private identity for this computer. You will still verify the other computer "
                            "before sharing control.")
      : identityValid
          ? tr("This computer is ready to verify connections. Saved devices are checked automatically; compare "
               "identities only when pairing a new device.")
          : tr("The selected identity file could not be validated. Review it in advanced connection settings.")
  );
  ui->btnTlsRegenCert->setText(identityPresent ? tr("Replace identity…") : tr("Create identity…"));
  m_identityAction->removeWidget(ui->btnTlsRegenCert);
  m_connectionDetailsBody->layout()->removeWidget(ui->btnTlsRegenCert);
  if (identityPresent)
    qobject_cast<QVBoxLayout *>(m_connectionDetailsBody->layout())
        ->insertWidget(0, ui->btnTlsRegenCert, 0, Qt::AlignLeft);
  else
    m_identityAction->insertWidget(0, ui->btnTlsRegenCert);
  ui->btnTlsRegenCert->setProperty("primaryAction", !identityPresent);
  ui->btnTlsRegenCert->style()->unpolish(ui->btnTlsRegenCert);
  ui->btnTlsRegenCert->style()->polish(ui->btnTlsRegenCert);
  ui->btnTlsRegenCert->show();
  ui->lineTlsCertPath->setToolTip(certificate);
  ui->cbRequireClientCert->setChecked(Settings::value(Settings::Security::CheckPeers).toBool());
  ui->groupSecurity->setChecked(TlsUtility::isEnabled());

  ui->groupSecurity->setEnabled(Settings::isWritable());

  updateTlsControlsEnabled();
}

void SettingsDialog::updateTlsControlsEnabled()
{
  const auto writable = Settings::isWritable();
  const auto tlsChecked = ui->groupSecurity->isChecked();

  auto enabled = writable && tlsChecked;
  ui->lblTlsKeyLength->setEnabled(enabled);
  ui->comboTlsKeyLength->setEnabled(enabled);
  ui->lblTlsCert->setEnabled(enabled);
  ui->widgetTlsCert->setEnabled(enabled);
  ui->btnTlsRegenCert->setEnabled(enabled);
  ui->cbRequireClientCert->setEnabled(enabled && !isClientMode());
  enforceReviewBoundary();
}

void SettingsDialog::enforceReviewBoundary()
{
#ifdef DESKFLOW_UI_REVIEW_BUILD
  for (QWidget *control : QList<QWidget *>{
           ui->btnTlsRegenCert, ui->btnTlsCertPath, ui->btnBrowseLog, ui->btnClearAllSettings, ui->groupService,
           ui->groupLogToFile, ui->comboTlsKeyLength, ui->cbRequireClientCert, ui->cbAutoUpdate, ui->cbPreventSleep,
           ui->cbRunEnterCommand, ui->cbRunExitCommand
       }) {
    control->setEnabled(false);
    control->setToolTip(tr("Unavailable in isolated UI review."));
  }
  ui->groupSecurity->setEnabled(false); // Preserve the displayed checked state in read-only review.
  ui->lineTlsCertPath->setReadOnly(true);
#endif
}

bool SettingsDialog::isClientMode() const
{
  return Settings::value(Settings::Core::CoreMode) == Settings::CoreMode::Client;
}

void SettingsDialog::updateKeyLengthOnFile(const QString &path)
{
  if (!QFile(path).exists()) {
    qCritical("tls certificate file not found: %s", qUtf8Printable(path));
    return;
  }

  auto length = TlsUtility::getCertKeyLength(path);
  auto labelIcon = QPixmap(QIcon::fromTheme(QIcon::ThemeIcon::SecurityLow).pixmap(24, 24));
  if (length == 2048)
    labelIcon = QPixmap(QIcon::fromTheme(QStringLiteral("security-medium")).pixmap(24, 24));
  if (length == 4096)
    labelIcon = QPixmap(QIcon::fromTheme(QIcon::ThemeIcon::SecurityHigh).pixmap(24, 24));

  ui->lblTlsCertInfo->setPixmap(labelIcon);
  ui->lblTlsCertInfo->setToolTip(QStringLiteral("Key length: %1 bits").arg(QString::number(length)));
}

void SettingsDialog::updateControls()
{
  const bool writable = Settings::isWritable();
  const bool serviceChecked = ui->groupService->isChecked();
  const bool logToFile = ui->groupLogToFile->isChecked();

  ui->sbPort->setEnabled(writable);
  ui->comboInterface->setEnabled(writable);
  ui->comboLogLevel->setEnabled(writable);
  ui->groupLogToFile->setEnabled(writable);
  ui->comboLanguage->setEnabled(writable);
  ui->cbShowVersion->setEnabled(writable);
  ui->cbGuiDebug->setEnabled(writable);
  ui->rbIconColorful->setEnabled(writable);
  ui->rbIconMono->setEnabled(writable);
  ui->rbAutoHide->setEnabled(writable);
  ui->rbShowOnStart->setEnabled(writable);
  ui->btnClearAllSettings->setEnabled(writable);
  ui->cbAutoUpdate->setEnabled(writable);
  ui->cbPreventSleep->setEnabled(writable);
  ui->lineTlsCertPath->setEnabled(writable);
  ui->comboTlsKeyLength->setEnabled(writable);
  ui->rbCloseToTray->setEnabled(writable);
  ui->rbExitOnClose->setEnabled(writable);
  ui->cbRunEnterCommand->setEnabled(writable);
  ui->cbRunExitCommand->setEnabled(writable);
  ui->lineCommandEnter->setEnabled(writable && ui->cbRunEnterCommand->isChecked());
  ui->lineCommandExit->setEnabled(writable && ui->cbRunExitCommand->isChecked());

  // Portable mode only ever applies to Windows.
  // Daemon options should only be available on Windows when *not* in portable mode.
  if (!Settings::isPortableMode()) {
    ui->groupService->setEnabled(writable);
    ui->cbElevateDaemon->setEnabled(writable && serviceChecked);
  } else if (ui->groupService->isVisibleTo(ui->tabAdvanced)) {
    ui->groupService->setVisible(false);
  }

  ui->widgetLogFilename->setEnabled(writable && logToFile);

  updateTlsControls();
}

void SettingsDialog::updateRequestedKeySize() const
{
#ifdef DESKFLOW_UI_REVIEW_BUILD
  return;
#endif
  if (ui->comboTlsKeyLength->currentText() == Settings::value(Settings::Security::KeySize).toString())
    return;
  Settings::setValue(Settings::Security::KeySize, ui->comboTlsKeyLength->currentText());
}

void SettingsDialog::logLevelChanged()
{
  ui->lblDebugWarning->setVisible(ui->comboLogLevel->currentIndex() > static_cast<int>(LogLevel::Level::Info));
}

bool SettingsDialog::isModified() const
{
  const auto processMode = Settings::value(Settings::Core::ProcessMode).value<Settings::ProcessMode>();
  const bool ignoreInterface = !m_interfaceSetOnLoad && (ui->comboInterface->currentIndex() == 0);
  const auto storedLanguage = Settings::value(Settings::Core::Language).toString();
  const auto effectiveLanguage = storedLanguage.isEmpty() ? I18N::currentLanguage() : storedLanguage;

  bool modified =
      (ui->sbPort->value() != Settings::value(Settings::Core::Port).toInt()) ||
      (ui->comboLogLevel->currentData().toString().compare(Settings::logLevelText(), Qt::CaseInsensitive) != 0) ||
      (ui->groupLogToFile->isChecked() != Settings::value(Settings::Log::ToFile).toBool()) ||
      (ui->lineLogFilename->text() != Settings::value(Settings::Log::File).toString()) ||
      (ui->rbAutoHide->isChecked() != Settings::value(Settings::Gui::Autohide).toBool()) ||
      (ui->cbPreventSleep->isChecked() != Settings::value(Settings::Core::PreventSleep).toBool()) ||
      (ui->rbCloseToTray->isChecked() != Settings::value(Settings::Gui::CloseToTray).toBool()) ||
      (ui->cbElevateDaemon->isChecked() != Settings::value(Settings::Daemon::Elevate).toBool()) ||
      (ui->cbAutoUpdate->isChecked() != Settings::value(Settings::Gui::AutoUpdateCheck).toBool()) ||
      (ui->cbGuiDebug->isChecked() != Settings::value(Settings::Log::GuiDebug).toBool()) ||
      (ui->cbShowVersion->isChecked() != Settings::value(Settings::Gui::ShowVersionInTitle).toBool()) ||
      (ui->rbIconMono->isChecked() != Settings::value(Settings::Gui::SymbolicTrayIcon).toBool()) ||
      (ui->groupService->isChecked() != (processMode == Settings::ProcessMode::Service)) ||
      (ui->lineTlsCertPath->text() != Settings::value(Settings::Security::Certificate).toString()) ||
      (ui->comboTlsKeyLength->currentText() != Settings::value(Settings::Security::KeySize).toString()) ||
      (ui->groupSecurity->isChecked() != Settings::value(Settings::Security::TlsEnabled).toBool()) ||
      (ui->cbRequireClientCert->isChecked() != Settings::value(Settings::Security::CheckPeers).toBool()) ||
      (ui->cbRunEnterCommand->isChecked() != Settings::value(Settings::Core::EnableEnterCommand).toBool()) ||
      (ui->cbRunExitCommand->isChecked() != Settings::value(Settings::Core::EnableExitCommand).toBool()) ||
      (ui->lineCommandEnter->text() != Settings::value(Settings::Core::ComputerEnterCommand).toString()) ||
      (ui->lineCommandExit->text() != Settings::value(Settings::Core::ComputerExitCommand).toString()) ||
      (I18N::nativeTo639Name(ui->comboLanguage->currentText()) != effectiveLanguage);

  if (!ignoreInterface)
    modified = modified || ui->comboInterface->currentText() != Settings::value(Settings::Core::Interface).toString();
  return modified;
}

bool SettingsDialog::isDefault() const
{
  const auto processMode = Settings::defaultValue(Settings::Core::ProcessMode).value<Settings::ProcessMode>();
  const auto logLevelIndex =
      static_cast<int>(LogLevel::fromOption(Settings::defaultValue(Settings::Log::Level).toString()));

  return (
      (ui->sbPort->value() == Settings::defaultValue(Settings::Core::Port).toInt()) &&
      (ui->comboLogLevel->currentIndex() == logLevelIndex) &&
      (ui->groupLogToFile->isChecked() == Settings::defaultValue(Settings::Log::ToFile).toBool()) &&
      (ui->lineLogFilename->text() == Settings::defaultValue(Settings::Log::File).toString()) &&
      (ui->rbAutoHide->isChecked() == Settings::defaultValue(Settings::Gui::Autohide).toBool()) &&
      (ui->cbPreventSleep->isChecked() == Settings::defaultValue(Settings::Core::PreventSleep).toBool()) &&
      (ui->rbCloseToTray->isChecked() == Settings::defaultValue(Settings::Gui::CloseToTray).toBool()) &&
      (ui->cbElevateDaemon->isChecked() == Settings::defaultValue(Settings::Daemon::Elevate).toBool()) &&
      (ui->cbAutoUpdate->isChecked() == Settings::defaultValue(Settings::Gui::AutoUpdateCheck).toBool()) &&
      (ui->cbGuiDebug->isChecked() == Settings::defaultValue(Settings::Log::GuiDebug).toBool()) &&
      (ui->cbShowVersion->isChecked() == Settings::defaultValue(Settings::Gui::ShowVersionInTitle).toBool()) &&
      (ui->rbIconMono->isChecked() == Settings::defaultValue(Settings::Gui::SymbolicTrayIcon).toBool()) &&
      (ui->groupService->isChecked() == (processMode == Settings::ProcessMode::Service)) &&
      (ui->comboInterface->currentIndex() == 0) &&
      (ui->lineTlsCertPath->text() == Settings::defaultValue(Settings::Security::Certificate).toString()) &&
      (ui->comboTlsKeyLength->currentText() == Settings::defaultValue(Settings::Security::KeySize).toString()) &&
      (ui->groupSecurity->isChecked() == Settings::defaultValue(Settings::Security::TlsEnabled).toBool()) &&
      (ui->cbRequireClientCert->isChecked() == Settings::defaultValue(Settings::Security::CheckPeers).toBool()) &&
      (ui->lineCommandEnter->text() == Settings::defaultValue(Settings::Core::ComputerEnterCommand).toString()) &&
      (ui->lineCommandExit->text() == Settings::defaultValue(Settings::Core::ComputerExitCommand).toString()) &&
      (ui->cbRunEnterCommand->isChecked() == Settings::defaultValue(Settings::Core::EnableEnterCommand).toBool()) &&
      (ui->cbRunExitCommand->isChecked() == Settings::defaultValue(Settings::Core::EnableExitCommand).toBool()) &&
      (ui->comboLanguage->currentText() == "English")
  );
}

void SettingsDialog::resetToDefault()
{
  ui->sbPort->setValue(Settings::defaultValue(Settings::Core::Port).toInt());
  ui->comboLogLevel->setCurrentIndex(
      static_cast<int>(LogLevel::fromOption(Settings::defaultValue(Settings::Log::Level).toString()))
  );
  ui->groupLogToFile->setChecked(Settings::defaultValue(Settings::Log::ToFile).toBool());
  ui->lineLogFilename->setText(Settings::defaultValue(Settings::Log::File).toString());
  ui->cbPreventSleep->setChecked(Settings::defaultValue(Settings::Core::PreventSleep).toBool());
  ui->cbElevateDaemon->setChecked(Settings::defaultValue(Settings::Daemon::Elevate).toBool());
  ui->cbAutoUpdate->setChecked(Settings::defaultValue(Settings::Gui::AutoUpdateCheck).toBool());
  ui->cbGuiDebug->setChecked(Settings::defaultValue(Settings::Log::GuiDebug).toBool());
  ui->cbShowVersion->setChecked(Settings::defaultValue(Settings::Gui::ShowVersionInTitle).toBool());
  ui->cbRunEnterCommand->setChecked(Settings::defaultValue(Settings::Core::EnableEnterCommand).toBool());
  ui->cbRunExitCommand->setChecked(Settings::defaultValue(Settings::Core::EnableExitCommand).toBool());
  ui->lineCommandEnter->setText(Settings::defaultValue(Settings::Core::ComputerEnterCommand).toString());
  ui->lineCommandExit->setText(Settings::defaultValue(Settings::Core::ComputerExitCommand).toString());

  const auto autoHide = Settings::defaultValue(Settings::Gui::Autohide).toBool();
  ui->rbCloseToTray->setChecked(autoHide);
  ui->rbExitOnClose->setChecked(!autoHide);

  const auto closeToTray = Settings::defaultValue(Settings::Gui::CloseToTray).toBool();
  ui->rbCloseToTray->setChecked(closeToTray);
  ui->rbExitOnClose->setChecked(!closeToTray);

  const auto processMode = Settings::defaultValue(Settings::Core::ProcessMode).value<Settings::ProcessMode>();
  ui->groupService->setChecked(processMode == Settings::ProcessMode::Service);

  if (!deskflow::platform::isWindows())
    ui->groupService->setVisible(false);

  if (Settings::defaultValue(Settings::Gui::SymbolicTrayIcon).toBool())
    ui->rbIconMono->setChecked(true);
  else
    ui->rbIconColorful->setChecked(true);

  ui->lblDebugWarning->setVisible(false);

  ui->comboInterface->setCurrentIndex(0);

  qDebug() << "reset to default values";
  updateControls();
  setButtonBoxEnabledButtons();
}

void SettingsDialog::setButtonBoxEnabledButtons() const
{
  const bool modified = isModified();
  m_buttonBox->enableSave(modified);
  m_buttonBox->enableReset(modified);
  m_buttonBox->enableRestoreDefaults(!isDefault());
}

SettingsDialog::~SettingsDialog() = default;
