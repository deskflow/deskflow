/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2025 - 2026 Chris Rizzitello <sithlord48@gmail.com>
 * SPDX-FileCopyrightText: (C) 2012 - 2016 Synergy App Ltd
 * SPDX-FileCopyrightText: (C) 2008 Volker Lanz <vl@fidra.de>
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#include "ServerConfigDialog.h"
#include "ui_ServerConfigDialog.h"

#include "common/Constants.h"
#include "common/NetworkProtocol.h"
#include "common/PlatformInfo.h"
#include "common/Settings.h"
#include "dialogs/ActionDialog.h"
#include "dialogs/ComputerSettingsDialog.h"
#include "dialogs/DesktopLayoutEditor.h"
#include "dialogs/HotkeyDialog.h"
#include "gui/DialogPresentation.h"
#include "gui/ExperienceStyle.h"
#include "gui/widgets/SettingsDialogButtonBox.h"

#include <QComboBox>
#include <QEvent>
#include <QFileDialog>
#include <QInputDialog>
#include <QLabel>
#include <QLineEdit>
#include <QMessageBox>
#include <QPainter>
#include <QPushButton>
#include <QScrollArea>
#include <QSignalBlocker>
#include <QStackedWidget>
#include <QStyledItemDelegate>
#include <QTabBar>

namespace {
QPixmap computerSymbol(const QColor &color)
{
  QPixmap display(28, 28);
  display.fill(Qt::transparent);
  QPainter painter(&display);
  painter.setRenderHint(QPainter::Antialiasing);
  painter.setPen(QPen(color, 1.5));
  painter.drawRoundedRect(QRectF(3, 3, 22, 16), 2, 2);
  painter.drawLine(QPointF(14, 19), QPointF(14, 24));
  painter.drawLine(QPointF(8, 24), QPointF(20, 24));
  return display;
}
class CompatibilityComputerDelegate final : public QStyledItemDelegate
{
public:
  using QStyledItemDelegate::QStyledItemDelegate;

protected:
  void initStyleOption(QStyleOptionViewItem *option, const QModelIndex &index) const override
  {
    QStyledItemDelegate::initStyleOption(option, index);
    if (index.data(Qt::DisplayRole).toString().isEmpty())
      return;
    option->icon = QIcon(computerSymbol(option->palette.color(QPalette::Text)));
    option->decorationSize = QSize(28, 28);
    option->decorationPosition = QStyleOptionViewItem::Top;
    option->displayAlignment = Qt::AlignCenter;
  }
};
} // namespace

using enum ComputerConfig::SwitchCorner;

ServerConfigDialog::ServerConfigDialog(QWidget *parent, ServerConfig &config)
    : QDialog(parent, Qt::WindowTitleHint | Qt::WindowSystemMenuHint),
      ui{std::make_unique<Ui::ServerConfigDialog>()},
      m_columns{Settings::value(Settings::Server::GridWidth).toInt()},
      m_rows{Settings::value(Settings::Server::GridHeight).toInt()},
      m_originalServerConfig(config),
      m_originalServerConfigIsExternal(config.useExternalConfig()),
      m_originalServerConfigUsesExternalFile(config.configFile()),
      m_serverConfig(config),
      m_computerSetupModel(m_serverConfig.computers(), m_columns, m_rows),
      m_buttonBox{new SettingsDialogButtonBox(this)}
{
  ui->setupUi(this);
  ui->tabWidget->setCurrentIndex(0);
  layout()->addWidget(m_buttonBox);
  auto *layoutActions = new QHBoxLayout;
  m_editLayout = new QPushButton(tr("Arrange computers…"), this);
  m_editLayout->setObjectName("editDesktopLayout");
  m_restoreGrid = new QPushButton(tr("Restore grid…"), this);
  m_restoreGrid->setObjectName("restoreGridLayout");
  layoutActions->addWidget(m_editLayout);
  layoutActions->addWidget(m_restoreGrid);
  layoutActions->addStretch();
  auto *computerLayout = qobject_cast<QBoxLayout *>(ui->tabComputers->layout());
  computerLayout->insertLayout(0, layoutActions);
  m_layoutSummary = new QLabel(this);
  m_layoutSummary->setWordWrap(true);
  m_layoutSummary->setTextFormat(Qt::PlainText);
  computerLayout->insertWidget(1, m_layoutSummary);
  m_deviceControls = new QWidget(this);
  auto *deviceRow = new QHBoxLayout(m_deviceControls);
  deviceRow->setContentsMargins(0, 0, 0, 0);
  m_devicePicker = new QComboBox(this);
  m_devicePicker->setObjectName("layoutDevicePicker");
  m_devicePicker->setAccessibleName(tr("Computer to manage"));
  deviceRow->addWidget(m_devicePicker, 1);
  auto *addDevice = new QPushButton(tr("Add computer…"), this);
  auto *renameDevice = new QPushButton(tr("Rename…"), this);
  auto *removeDevice = new QPushButton(tr("Remove…"), this);
  renameDevice->setObjectName("renameLayoutClient");
  removeDevice->setObjectName("removeLayoutClient");
  deviceRow->addWidget(addDevice);
  deviceRow->addWidget(renameDevice);
  deviceRow->addWidget(removeDevice);
  computerLayout->insertWidget(2, m_deviceControls);
  connect(addDevice, &QPushButton::clicked, this, [this] { addClient(); });
  connect(renameDevice, &QPushButton::clicked, this, [this] { manageLayoutClient(false); });
  connect(removeDevice, &QPushButton::clicked, this, [this] { manageLayoutClient(true); });

  loadFromConfig();

  ui->btnBrowseConfigFile->setIcon(QIcon::fromTheme(QIcon::ThemeIcon::DocumentOpen));
  ui->btnBrowseConfigFile->setText(tr("Choose…"));

  if (!deskflow::platform::isWindows())
    ui->cbWin32KeepForeground->setVisible(false);
  initConnections();
  setupPresentation();
}

ServerConfigDialog::~ServerConfigDialog() = default;

void ServerConfigDialog::setupPresentation()
{
  auto *root = qobject_cast<QVBoxLayout *>(layout());
  root->setContentsMargins(24, 20, 24, 16);
  root->setSpacing(16);
  root->removeWidget(ui->tabWidget);
  m_pages = new QStackedWidget(this);
  m_pages->setObjectName("serverConfigPages");
  root->insertWidget(0, m_pages, 1);
  auto *monitorRow = new QHBoxLayout;
  m_monitorStatus = new QLabel(this);
  m_monitorStatus->setObjectName("monitorLayoutStatus");
  m_monitorStatus->setWordWrap(true);
  m_monitorStatus->setTextFormat(Qt::PlainText);
  m_monitorToggle = new QPushButton(tr("Arrange individual displays"), this);
  m_monitorToggle->setObjectName("toggleMonitorLayout");
  auto *reviewDisplays = new QPushButton(tr("Review detected displays"), this);
  reviewDisplays->setObjectName("reviewDetectedDisplays");
  monitorRow->addWidget(m_monitorStatus, 1);
  monitorRow->addWidget(m_monitorToggle);
  monitorRow->addWidget(reviewDisplays);
  root->insertLayout(0, monitorRow);
  connect(m_monitorToggle, &QPushButton::clicked, this, [this] {
    if (m_monitorMode) {
      m_serverConfig.clearMonitorLayout();
      m_monitorMode = false;
      reloadLayoutDraft();
      return;
    }
    m_monitorDraft = m_serverConfig.monitorLayoutDraft();
    if (!m_monitorDraft.error.isEmpty())
      return;
    m_monitorMode = true;
    m_serverConfig.setMonitorLayout(m_monitorDraft);
    reloadLayoutDraft();
  });
  connect(reviewDisplays, &QPushButton::clicked, this, [this] {
    auto detected = m_serverConfig.monitorLayoutDraft(true);
    if (!detected.error.isEmpty()) {
      m_monitorStatus->setText(detected.error);
      return;
    }
    m_monitorDraft = std::move(detected);
    m_serverConfig.setMonitorLayout(m_monitorDraft);
    m_monitorMode = true;
    reloadLayoutDraft();
  });

  m_layoutEditor = new DesktopLayoutEditor(m_serverConfig.desktopLayout().desktops, m_pages);
  m_layoutEditor->setObjectName("serverArrangement");
  m_layoutEditor->embedInSettings();
  auto *arrangementScroll = new QScrollArea(m_pages);
  arrangementScroll->setObjectName("arrangementScroll");
  arrangementScroll->setWidgetResizable(true);
  arrangementScroll->setFrameShape(QFrame::NoFrame);
  arrangementScroll->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
  arrangementScroll->setWidget(m_layoutEditor);
  m_pages->addWidget(arrangementScroll);
  m_addLayoutComputer = new QPushButton(tr("Add computer…"), this);
  m_addLayoutComputer->setObjectName("arrangementAddComputer");
  auto *advanced = new QPushButton(tr("Advanced settings…"), this);
  advanced->setObjectName("advancedServerSettings");
  m_layoutEditor->addSecondaryAction(m_addLayoutComputer);
  m_layoutEditor->addSecondaryAction(advanced);

  auto *advancedPage = new QWidget(m_pages);
  auto *advancedLayout = new QVBoxLayout(advancedPage);
  advancedLayout->setContentsMargins(0, 0, 0, 0);
  advancedLayout->setSpacing(16);
  m_backToArrangement = new QPushButton(tr("Back to arrangement"), advancedPage);
  m_backToArrangement->setObjectName("backToArrangement");
  advancedLayout->addWidget(m_backToArrangement, 0, Qt::AlignLeft);
  ui->tabWidget->setTabText(0, tr("Compatibility grid"));
  ui->tabWidget->tabBar()->setExpanding(false);
  ui->computerSetupView->setIconSize(QSize(28, 28));
  ui->computerSetupView->setItemDelegate(new CompatibilityComputerDelegate(ui->computerSetupView));
  advancedLayout->addWidget(ui->tabWidget, 1);
  m_pages->addWidget(advancedPage);
  for (int i = 0; i < ui->tabWidget->count(); ++i) {
    if (auto *pageLayout = ui->tabWidget->widget(i)->layout()) {
      pageLayout->setContentsMargins(16, 16, 16, 16);
      pageLayout->setSpacing(12);
    }
  }
  connect(advanced, &QPushButton::clicked, this, &ServerConfigDialog::showAdvancedSettings);
  connect(m_backToArrangement, &QPushButton::clicked, this, &ServerConfigDialog::editDesktopLayout);
  connect(m_addLayoutComputer, &QPushButton::clicked, this, [this] {
    if (applyLayoutDraft())
      addClient();
  });
  connect(m_layoutEditor, &DesktopLayoutEditor::draftChanged, this, [this] {
    applyLayoutDraft();
    setButtonBoxEnabledButtons();
  });
  connect(m_layoutEditor, &DesktopLayoutEditor::saveRequested, this, &ServerConfigDialog::save);
  connect(m_layoutEditor, &DesktopLayoutEditor::cancelRequested, this, &ServerConfigDialog::cancel);
  connect(&m_computerSetupModel, &ComputerSetupModel::computersChanged, this, &ServerConfigDialog::reloadLayoutDraft);
  reloadLayoutDraft();
  if (m_serverConfig.useExternalConfig() || !m_serverConfig.desktopLayoutError().isEmpty())
    m_pages->setCurrentIndex(1);
  setWindowTitle(tr("Computer arrangement"));
  resize(820, 700);
  setMinimumSize(680, 620);
  updateAppearance();
  updateLayoutControls();
  setButtonBoxEnabledButtons();
}

void ServerConfigDialog::updateAppearance()
{
  deskflow::gui::applyExperienceStyle(this);
  const bool dark = palette().color(QPalette::Window).lightness() < 128;
  setStyleSheet(
      styleSheet() +
      QString(
          "QTabWidget::pane{border:1px solid %1;border-radius:6px;}"
          "QTabBar::tab{padding:10px 16px;margin-right:4px;border:0;border-bottom:2px solid transparent;}"
          "QTabBar::tab:selected{border-bottom:2px solid %2;font-weight:600;}"
          "QToolButton#settingsMoreActions{padding:6px 10px;border:0;color:%3;}"
      )
          .arg(dark ? "#45464b" : "#d9dadd", dark ? "#0a84ff" : "#0067d9", dark ? "#f5f5f7" : "#1d1d1f")
  );
  // Compatibility drag targets remain functional but no longer dominate the page.
  for (auto *target : {static_cast<QLabel *>(ui->lblNewComputer), static_cast<QLabel *>(ui->lblRemoveComputer)}) {
    target->setFixedSize(40, 40);
    target->setAlignment(Qt::AlignCenter);
    target->setFrameShape(QFrame::NoFrame);
  }
  ui->lblRemoveComputer->setPixmap(QIcon::fromTheme("user-trash").pixmap(QSize(24, 24)));
  ui->lblNewComputer->setPixmap(computerSymbol(palette().color(QPalette::WindowText)));
}

void ServerConfigDialog::changeEvent(QEvent *event)
{
  QDialog::changeEvent(event);
  if (m_pages && event->type() == QEvent::PaletteChange)
    updateAppearance();
}

bool ServerConfigDialog::applyLayoutDraft()
{
  if (m_advancedOnly)
    return true;
  if (!m_layoutEditor || !m_layoutEditor->editingEnabled() || ui->groupExternalConfig->isChecked())
    return true;
  const auto &draft = m_layoutEditor->desktops();
  if (m_monitorMode) {
    if (!m_monitorDraft.apply(draft)) {
      m_layoutEditor->setMonitorEdges({});
      m_layoutEditor->setRoutingError(m_monitorDraft.validationError);
      return false;
    }
    QVector<QLineF> edges;
    for (const auto &route : m_monitorDraft.layout.routes(m_monitorDraft.topologies))
      edges.append(route.segment);
    m_layoutEditor->setMonitorEdges(std::move(edges));
    m_serverConfig.setMonitorLayout(m_monitorDraft);
    return true;
  }
  if (!deskflow::gui::DesktopLayout::draftError(draft).isEmpty())
    return false;
  // Undoing every edit must also retain the original grid representation.
  if (draft == m_loadedLayout && !m_loadedLayoutWasCustom) {
    if (m_serverConfig.hasDesktopLayout())
      m_serverConfig.restoreGridLayout();
    return true;
  }
  if (draft == m_serverConfig.desktopLayout().desktops)
    return true;
  return m_serverConfig.setDesktopLayout(draft);
}

void ServerConfigDialog::reloadLayoutDraft()
{
  if (!m_layoutEditor)
    return;
  m_monitorMode = m_serverConfig.hasMonitorLayout();
  m_monitorDraft = m_serverConfig.monitorLayoutDraft();
  const QSignalBlocker editorBlock(m_layoutEditor);
  const auto desktops = m_monitorMode && m_monitorDraft.error.isEmpty() ? m_monitorDraft.desktops()
                                                                        : m_serverConfig.desktopLayout().desktops;
  m_loadedLayout = desktops;
  m_loadedLayoutWasCustom = m_monitorMode || m_serverConfig.hasDesktopLayout();
  m_layoutEditor->setDesktops(desktops);
  m_layoutEditor->setMonitorMode(m_monitorMode && m_monitorDraft.error.isEmpty());
  QMap<QString, QString> details;
  if (m_monitorMode && m_monitorDraft.error.isEmpty())
    for (const auto &p : m_monitorDraft.layout.placements) {
      const auto m = m_monitorDraft.topologies.constFind(p.device)->find(p.monitor);
      details[deskflow::gui::MonitorLayoutDraft::token(p)] =
          tr("Detected: %1 × %2 pixels · %3 × %4 logical units · physical size unknown")
              .arg(m->pixels.width())
              .arg(m->pixels.height())
              .arg(m->logical.width())
              .arg(m->logical.height());
    }
  m_layoutEditor->setMonitorDetails(std::move(details));
  QVector<QLineF> edges;
  if (m_monitorMode && m_monitorDraft.error.isEmpty())
    for (const auto &route : m_monitorDraft.layout.routes(m_monitorDraft.topologies))
      edges.append(route.segment);
  m_layoutEditor->setMonitorEdges(std::move(edges));
  m_monitorStatus->setText(
      m_monitorDraft.error.isEmpty() ? tr("Detected display sizes. Disconnected computers use their last received "
                                          "snapshot; the core verifies it again before use.")
                                     : m_monitorDraft.error
  );
  m_monitorToggle->setText(m_monitorMode ? tr("Use compatibility layout") : tr("Arrange individual displays"));
  m_monitorToggle->setEnabled(
      Settings::isWritable() && !ui->groupExternalConfig->isChecked() &&
      (m_monitorMode || m_monitorDraft.error.isEmpty())
  );
  updateLayoutControls();
  setButtonBoxEnabledButtons();
}

void ServerConfigDialog::showAdvancedSettings()
{
  if (!applyLayoutDraft()) {
    m_layoutEditor->setFocus();
    return;
  }
  m_pages->setCurrentIndex(1);
  ui->tabWidget->setCurrentIndex(1); // Hotkeys first; the legacy grid is explicitly secondary.
  updateLayoutControls();
  m_backToArrangement->setFocus();
}

void ServerConfigDialog::showAdvancedOnly()
{
  if (m_advancedOnly)
    return;
  m_advancedOnly = true;
  m_pages->setCurrentIndex(1);
  ui->tabWidget->setCurrentIndex(1);
  // Modern workspace owns arrangement. Retain per-computer options here,
  // without presenting a second editable layout that can diverge from it.
  auto *page = new QWidget(ui->tabWidget);
  page->setObjectName("computerOptionsPage");
  auto *form = new QVBoxLayout(page);
  form->setContentsMargins(16, 16, 16, 16);
  form->setSpacing(12);
  auto *description = new QLabel(
      tr("Choose a computer to edit aliases, modifier keys and corner behavior. Screen arrangement is managed in My "
         "desktop."),
      page
  );
  description->setWordWrap(true);
  form->addWidget(description);
  auto *picker = new QComboBox(page);
  picker->setObjectName("computerOptionsPicker");
  form->addWidget(picker);
  for (int i = 0; i < m_serverConfig.computers().size(); ++i) {
    const auto &computer = m_serverConfig.computers()[i];
    if (!computer.isNull())
      picker->addItem(computer.name(), i);
  }
  auto *edit = new QPushButton(tr("Computer options…"), page);
  edit->setObjectName("editComputerOptions");
  form->addWidget(edit, 0, Qt::AlignLeft);
  auto *reason = new QLabel(
      tr("An external configuration file controls these options. Edit that file or explicitly switch configuration "
         "mode."),
      page
  );
  reason->setWordWrap(true);
  form->addWidget(reason);
  form->addStretch();
  auto enabled = [this, picker, edit, reason] {
    const bool external = ui->groupExternalConfig->isChecked();
    picker->setEnabled(!external);
    edit->setEnabled(!external && picker->count() > 0 && Settings::isWritable());
    reason->setVisible(external);
  };
  connect(ui->groupExternalConfig, &QGroupBox::toggled, page, enabled);
  enabled();
  connect(edit, &QPushButton::clicked, this, [this, picker] {
    const auto index = picker->currentData().toInt();
    if (index < 0 || index >= m_serverConfig.computers().size())
      return;
    ComputerSettingsDialog options(this, &m_serverConfig.computers()[index], &m_serverConfig.computers());
    // Renaming a workspace identity belongs to its explicit identity flow.
    options.findChild<QLineEdit *>("lineNameEdit")->setReadOnly(true);
    if (options.exec() == QDialog::Accepted)
      setButtonBoxEnabledButtons();
  });
  ui->tabWidget->removeTab(0);
  ui->tabComputers->hide();
  ui->tabWidget->insertTab(0, page, tr("Computers"));
  ui->tabWidget->setCurrentIndex(0);
  m_backToArrangement->hide();
  m_monitorToggle->hide();
  m_monitorStatus->hide();
  m_editLayout->hide();
  m_restoreGrid->hide();
  findChild<QPushButton *>("reviewDetectedDisplays")->hide();
  m_layoutEditor->setEditingEnabled(false);
  for (int index = 1; index < ui->tabWidget->count(); ++index) {
    auto *content = ui->tabWidget->widget(index);
    const auto title = ui->tabWidget->tabText(index);
    ui->tabWidget->removeTab(index);
    auto *scroll = new QScrollArea(ui->tabWidget);
    scroll->setObjectName(QString("sharingOptionsScroll%1").arg(index));
    scroll->setWidgetResizable(true);
    scroll->setFrameShape(QFrame::NoFrame);
    scroll->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    scroll->setWidget(content);
    ui->tabWidget->insertTab(index, scroll, title);
  }
  for (auto *group : findChildren<QGroupBox *>())
    if (group->layout()) {
      group->layout()->setContentsMargins(14, 20, 14, 14);
      group->layout()->setSpacing(12);
    }
  const bool dark = palette().color(QPalette::Window).lightness() < 128;
  setStyleSheet(
      styleSheet() + QString(
                         "QGroupBox{border:1px solid "
                         "%1;border-radius:7px;margin-top:12px;}QGroupBox::title{subcontrol-origin:margin;left:12px;"
                         "padding:0 4px;}QListWidget{border:1px solid %1;border-radius:5px;}"
                     )
                         .arg(dark ? "#45464b" : "#d9dadd")
  );
  for (auto *button : m_buttonBox->findChildren<QPushButton *>())
    new deskflow::gui::FormButtonMetrics(button);
  setMinimumSize(620, 480);
  resize(760, 640);
  setWindowTitle(tr("Advanced sharing settings"));
}

bool ServerConfigDialog::addClient(const QString &clientName)
{
  return addComputer(clientName, true);
}

void ServerConfigDialog::save()
{
  if (!Settings::isWritable())
    return;
  if (!applyLayoutDraft())
    return;
  if (!ui->groupExternalConfig->isChecked() && !m_serverConfig.desktopLayoutError().isEmpty()) {
    QMessageBox::warning(this, tr("Layout needs attention"), m_serverConfig.desktopLayoutError());
    return;
  }
  if (ui->groupExternalConfig->isChecked() && !QFile::exists(ui->lineConfigFile->text())) {

    auto selectedButton = QMessageBox::warning(
        this, "Filename invalid", "Please select a valid configuration file.", QMessageBox::Ok | QMessageBox::Ignore
    );

    if (selectedButton != QMessageBox::Ok || !browseConfigFile()) {
      return;
    }
  }
  const auto previousConfig = m_originalServerConfig;
  const bool resetClipboardState = Settings::value(Settings::Server::EnableClipboard).toBool() != m_enableClipboard;
  // Keep MainWindow consistent with notifications from the successful commit.
  // On failure both its config and the persisted values remain unchanged.
  setOriginalServerConfig(serverConfig());
  const bool saved = Settings::saveConfig([this] {
    Settings::setValue(Settings::Server::Protocol, networkProtocolToOption(m_protocol));
    Settings::setValue(Settings::Server::EnableClipboard, m_enableClipboard);
    Settings::setValue(Settings::Server::ClipboardSize, m_clipboardSize);
    Settings::setValue(Settings::Server::EnableHeartbeat, m_enableHeartbeat);
    Settings::setValue(Settings::Server::Heartbeat, m_heartbeatRate);
    Settings::setValue(Settings::Server::EnableSwitchDelay, m_enableSwitchDelay);
    Settings::setValue(Settings::Server::SwitchDelay, m_switchDelay);
    Settings::setValue(Settings::Server::DefaultLockToComputerState, m_defaultLockToComputerState);
    Settings::setValue(Settings::Server::DisableLockToComputer, m_disableLockToComputer);
    Settings::setValue(Settings::Server::EnableSwitchDoubleTap, m_enableSwitchDoubleTap);
    Settings::setValue(Settings::Server::SwitchDoubleTap, m_switchDoubleTap);
    Settings::setValue(Settings::Server::RelativeMouseMoves, m_relativeMouseMoves);
    Settings::setValue(Settings::Server::Win32KeepForeground, m_win32keepForeground);
    Settings::setValue(Settings::Server::ExternalConfig, ui->groupExternalConfig->isChecked());
    Settings::setValue(Settings::Server::ExternalConfigFile, ui->lineConfigFile->text());

    QStringList computerNames;
    const auto computerList = m_computerSetupModel.m_computers;
    for (const auto &computer : computerList) {
      const auto &computerName = computer.name();
      if (computerName.isEmpty())
        continue;
      computerNames.append(QStringLiteral("computer_%1").arg(computerName));
      Settings::setValue(Settings::Computer::Aliases.arg(computerName), computer.aliases());
      Settings::setValue(Settings::Computer::Name.arg(computerName), computerName);
    }
    Settings::removeUnknownComputers(computerNames);
    m_originalServerConfig.commit();
  });
  if (!saved) {
    setOriginalServerConfig(previousConfig);
    QMessageBox::warning(
        this, tr("Unable to save configuration"),
        tr("The configuration could not be saved. Your previous layout is unchanged. Check the file permissions and "
           "available disk space, then try again.")
    );
    return;
  }
  if (resetClipboardState) {
    Settings::setValue(Settings::Server::XdpRestoreToken, QString());
    Settings::setValue(Settings::Server::XdpClipboardRetried, false);
  }
  QDialog::accept();
}

void ServerConfigDialog::cancel()
{
  QDialog::reject();
}

void ServerConfigDialog::addHotkey()
{
  Hotkey hotkey;
  HotkeyDialog dlg(this, hotkey);
  if (dlg.exec() == QDialog::Accepted) {
    serverConfig().hotkeys().append(hotkey);
    ui->listHotkeys->addItem(hotkey.text());
    setButtonBoxEnabledButtons();
  }
}

void ServerConfigDialog::editHotkey()
{
  int row = ui->listHotkeys->currentRow();
  if (row < 0 || row >= serverConfig().hotkeys().size()) {
    qDebug() << "Attempt to editing out of bounds hotkey row: " << row;
    return;
  }

  Hotkey &hotkey = serverConfig().hotkeys()[row];
  HotkeyDialog dlg(this, hotkey);
  if (dlg.exec() == QDialog::Accepted) {
    ui->listHotkeys->currentItem()->setText(hotkey.text());
    setButtonBoxEnabledButtons();
  }
}

void ServerConfigDialog::removeHotkey()
{
  int row = ui->listHotkeys->currentRow();
  if (row < 0 || row >= serverConfig().hotkeys().size()) {
    qDebug() << "Attempt to remove out of bounds hotkey row: " << row;
    return;
  }

  serverConfig().hotkeys().removeAt(row);
  ui->listActions->clear();
  delete ui->listHotkeys->item(row);
  setButtonBoxEnabledButtons();
}

void ServerConfigDialog::listHotkeysSelectionChanged(const QItemSelection &selected, const QItemSelection &)
{
  bool itemsSelected = !selected.isEmpty();
  ui->btnEditHotkey->setEnabled(itemsSelected);
  ui->btnRemoveHotkey->setEnabled(itemsSelected);
  ui->btnNewAction->setEnabled(itemsSelected);

  if (itemsSelected && !serverConfig().hotkeys().isEmpty()) {
    ui->listActions->clear();
    const Hotkey &hotkey = serverConfig().hotkeys().at(selected.indexes().first().row());
    for (const Action &action : hotkey.actions())
      ui->listActions->addItem(action.text());
  }
}

void ServerConfigDialog::addAction()
{
  int row = ui->listHotkeys->currentRow();
  if (row < 0 || row >= serverConfig().hotkeys().size()) {
    qDebug() << "Attempt to add action to out of bounds hotkey row: " << row;
    return;
  }

  Hotkey &hotkey = serverConfig().hotkeys()[row];
  Action action;
  ActionDialog dlg(this, serverConfig(), hotkey, action);
  if (dlg.exec() == QDialog::Accepted) {
    hotkey.addAction(action);
    ui->listActions->addItem(action.text());
    setButtonBoxEnabledButtons();
  }
}

void ServerConfigDialog::editAction()
{
  int hotkeyRow = ui->listHotkeys->currentRow();
  if (hotkeyRow < 0 || hotkeyRow >= serverConfig().hotkeys().size()) {
    qDebug() << "Attempt to edit action from out of bounds hotkey row: " << hotkeyRow;
    return;
  }
  Hotkey &hotkey = serverConfig().hotkeys()[hotkeyRow];

  int actionRow = ui->listActions->currentRow();
  if (actionRow < 0 || actionRow >= hotkey.actions().size()) {
    qDebug() << "Attempt to remove out of bounds action row: " << actionRow;
    return;
  }
  Action &action = hotkey.actionAt(actionRow);

  ActionDialog dlg(this, serverConfig(), hotkey, action);
  if (dlg.exec() == QDialog::Accepted) {
    ui->listActions->currentItem()->setText(action.text());
    setButtonBoxEnabledButtons();
  }
}

void ServerConfigDialog::removeAction()
{
  int hotkeyRow = ui->listHotkeys->currentRow();
  if (hotkeyRow < 0 || hotkeyRow >= serverConfig().hotkeys().size()) {
    qDebug() << "Attempt to remove action from out of bounds hotkey row: " << hotkeyRow;
    return;
  }
  Hotkey &hotkey = serverConfig().hotkeys()[hotkeyRow];

  int actionRow = ui->listActions->currentRow();
  if (actionRow < 0 || actionRow >= hotkey.actions().size()) {
    qDebug() << "Attempt to remove out of bounds action row: " << actionRow;
    return;
  }

  hotkey.removeActionAt(actionRow);
  delete ui->listActions->currentItem();
  setButtonBoxEnabledButtons();
}

void ServerConfigDialog::toggleClipboard(bool enabled)
{
  if (m_enableClipboard == enabled)
    return;

  m_enableClipboard = enabled;

  ui->sbClipboardSizeLimit->setEnabled(enabled);
  if (enabled && !ui->sbClipboardSizeLimit->value()) {
    m_clipboardSize = Settings::defaultValue(Settings::Server::ClipboardSize).toUInt();
    ui->sbClipboardSizeLimit->setValue(m_clipboardSize ? m_clipboardSize : 1);
  }
  setButtonBoxEnabledButtons();
}

void ServerConfigDialog::setClipboardLimit(int limit)
{
  if (m_clipboardSize == limit)
    return;

  m_clipboardSize = limit;
  setButtonBoxEnabledButtons();
}

void ServerConfigDialog::toggleHeartbeat(bool enabled)
{
  m_enableHeartbeat = enabled;
  ui->sbHeartbeat->setEnabled(enabled);
  setButtonBoxEnabledButtons();
}

void ServerConfigDialog::setHeartbeat(int rate)
{
  if (rate == m_heartbeatRate)
    return;
  m_heartbeatRate = rate;
  setButtonBoxEnabledButtons();
}

void ServerConfigDialog::toggleRelativeMouseMoves(bool enabled)
{
  if (m_relativeMouseMoves == enabled)
    return;
  m_relativeMouseMoves = enabled;
  setButtonBoxEnabledButtons();
}

void ServerConfigDialog::toggleProtocol()
{
  m_protocol = ui->rbProtocolBarrier->isChecked() ? NetworkProtocol::Barrier : NetworkProtocol::Synergy;
  setButtonBoxEnabledButtons();
}

void ServerConfigDialog::listActionsSelectionChanged(const QItemSelection &selected, const QItemSelection &)
{
  bool enabled = !selected.isEmpty();
  ui->btnEditAction->setEnabled(enabled);
  ui->btnRemoveAction->setEnabled(enabled);
}

void ServerConfigDialog::toggleSwitchDoubleTap(bool enable)
{
  m_enableSwitchDoubleTap = enable;
  ui->sbSwitchDoubleTap->setEnabled(enable);
  setButtonBoxEnabledButtons();
}

void ServerConfigDialog::setSwitchDoubleTap(int within)
{
  if (m_switchDoubleTap == within)
    return;
  m_switchDoubleTap = within;
  setButtonBoxEnabledButtons();
}

void ServerConfigDialog::toggleSwitchDelay(bool enable)
{
  m_enableSwitchDelay = enable;
  ui->sbSwitchDelay->setEnabled(enable);
  setButtonBoxEnabledButtons();
}

void ServerConfigDialog::setSwitchDelay(int delay)
{
  if (m_switchDelay == delay)
    return;
  m_switchDelay = delay;
  setButtonBoxEnabledButtons();
}

void ServerConfigDialog::toggleDefaultLockToComputerState(bool state)
{
  if (m_defaultLockToComputerState == state)
    return;
  m_defaultLockToComputerState = state;
  setButtonBoxEnabledButtons();
}

void ServerConfigDialog::toggleLockToComputer(bool disabled)
{
  if (m_disableLockToComputer == disabled)
    return;
  m_disableLockToComputer = disabled;
  setButtonBoxEnabledButtons();
}

void ServerConfigDialog::toggleWin32Foreground(bool enabled)
{
  if (m_win32keepForeground == enabled)
    return;
  m_win32keepForeground = enabled;
  setButtonBoxEnabledButtons();
}

void ServerConfigDialog::addClient()
{
  addComputer("", false);
}

void ServerConfigDialog::onComputerRemoved()
{
  ui->lblNewComputer->setEnabled(true);
  setButtonBoxEnabledButtons();
}

void ServerConfigDialog::toggleExternalConfig(bool checked)
{
  ui->widgetExternalConfigControls->setEnabled(checked);
  ui->tabWidget->setTabEnabled(0, !checked);
  ui->tabWidget->setTabEnabled(1, !checked);
  updateLayoutControls();
  setButtonBoxEnabledButtons();
}

bool ServerConfigDialog::browseConfigFile()
{
  //: %1 is replaced with the application names
  //: (*.conf) and (*.*) should not be translated
  const auto deskflowConfigFilter = tr("%1 Configurations (*.conf);;All files (*.*)");

  QString fileName =
      QFileDialog::getOpenFileName(this, tr("Browse for a config file"), "", deskflowConfigFilter.arg(kAppName));

  if (!fileName.isEmpty()) {
    ui->lineConfigFile->setText(fileName);
    setServerConfig();
    return true;
  }

  return false;
}

void ServerConfigDialog::loadFromConfig()
{
  m_protocol = Settings::networkProtocol();
  m_enableHeartbeat = Settings::value(Settings::Server::EnableHeartbeat).toBool();
  m_heartbeatRate = Settings::value(Settings::Server::Heartbeat).toInt();
  m_relativeMouseMoves = Settings::value(Settings::Server::RelativeMouseMoves).toBool();
  m_win32keepForeground = Settings::value(Settings::Server::Win32KeepForeground).toBool();
  m_enableSwitchDelay = Settings::value(Settings::Server::EnableSwitchDelay).toBool();
  m_switchDelay = Settings::value(Settings::Server::SwitchDelay).toInt();
  m_enableSwitchDoubleTap = Settings::value(Settings::Server::EnableSwitchDoubleTap).toBool();
  m_switchDoubleTap = Settings::value(Settings::Server::SwitchDoubleTap).toInt();
  m_defaultLockToComputerState = Settings::value(Settings::Server::DefaultLockToComputerState).toBool();
  m_disableLockToComputer = Settings::value(Settings::Server::DisableLockToComputer).toBool();
  m_enableClipboard = Settings::value(Settings::Server::EnableClipboard).toBool();
  m_clipboardSize = Settings::value(Settings::Server::ClipboardSize).toUInt();

  ui->lineConfigFile->setText(serverConfig().configFile());
  ui->groupExternalConfig->setChecked(serverConfig().useExternalConfig());

  refreshControls();

  ui->listHotkeys->clear();
  for (const Hotkey &hotkey : std::as_const(serverConfig().hotkeys()))
    ui->listHotkeys->addItem(hotkey.text());

  ui->computerSetupView->setModel(&m_computerSetupModel);

  auto &computers = serverConfig().computers();
  auto server = std::ranges::find_if(computers, [this](const Computer &computer) {
    return (computer.name() == serverConfig().getServerName());
  });

  if (server == computers.end()) {
    Computer serverComputer(serverConfig().getServerName());
    serverComputer.markAsServer();
    model().computer(m_columns / 2, m_rows / 2) = serverComputer;
  } else {
    server->markAsServer();
  }
  updateControls();
}

void ServerConfigDialog::editDesktopLayout()
{
  if (!m_pages || ui->groupExternalConfig->isChecked() || !m_serverConfig.desktopLayoutError().isEmpty())
    return;
  m_pages->setCurrentIndex(0);
  m_layoutEditor->findChild<LayoutCanvas *>("layoutCanvas")->setFocus();
}

void ServerConfigDialog::manageLayoutClient(bool remove)
{
  if (!Settings::isWritable() || !m_serverConfig.hasDesktopLayout())
    return;
  const auto name = m_devicePicker->currentText();
  QString error;
  if (remove) {
    if (QMessageBox::question(
            this, tr("Remove computer?"),
            tr("Remove %1 from this draft? Saved settings remain unchanged until you save.").arg(name)
        ) != QMessageBox::Yes)
      return;
    error = m_serverConfig.removeLayoutClient(name);
  } else {
    bool ok = false;
    const auto renamed =
        QInputDialog::getText(this, tr("Rename computer"), tr("Computer name"), QLineEdit::Normal, name, &ok);
    if (!ok)
      return;
    error = m_serverConfig.renameLayoutClient(name, renamed);
  }
  if (!error.isEmpty())
    QMessageBox::warning(this, tr("Layout needs attention"), error);
  else
    reloadLayoutDraft();
  updateLayoutControls();
  // The grid model references the same draft list; keep its display current.
  Q_EMIT m_computerSetupModel.dataChanged(
      m_computerSetupModel.index(0, 0), m_computerSetupModel.index(m_rows - 1, m_columns - 1)
  );
  ui->listHotkeys->clear();
  for (const auto &hotkey : std::as_const(m_serverConfig).hotkeys())
    ui->listHotkeys->addItem(hotkey.text());
  setButtonBoxEnabledButtons();
}

void ServerConfigDialog::restoreGridLayout()
{
  if (!Settings::isWritable() || !m_serverConfig.hasDesktopLayout())
    return;
  if (QMessageBox::question(
          this, tr("Restore grid layout?"),
          tr("Replace the custom alignment and layout display names with the original grid below? Cancel server "
             "settings to keep your saved layout.")
      ) != QMessageBox::Yes)
    return;
  m_serverConfig.restoreGridLayout();
  reloadLayoutDraft();
  updateLayoutControls();
  setButtonBoxEnabledButtons();
}

void ServerConfigDialog::updateLayoutControls() const
{
  const bool custom = m_serverConfig.hasDesktopLayout();
  if (auto *review = findChild<QPushButton *>("reviewDetectedDisplays"))
    review->setEnabled(Settings::isWritable() && !ui->groupExternalConfig->isChecked());
  const bool writable = Settings::isWritable();
  m_editLayout->setEnabled(writable && !ui->groupExternalConfig->isChecked());
  if (m_layoutEditor) {
    const bool editable =
        writable && !ui->groupExternalConfig->isChecked() && m_serverConfig.desktopLayoutError().isEmpty();
    m_layoutEditor->setEditingEnabled(editable && (!m_monitorMode || m_monitorDraft.error.isEmpty()));
    m_backToArrangement->setEnabled(
        !ui->groupExternalConfig->isChecked() && m_serverConfig.desktopLayoutError().isEmpty()
    );
    m_addLayoutComputer->setEnabled(editable && !m_monitorMode && !m_computerSetupModel.isFull());
  }
  m_restoreGrid->setVisible(custom);
  m_restoreGrid->setEnabled(writable);
  m_deviceControls->setVisible(custom);
  m_deviceControls->setEnabled(writable && !m_monitorMode && !ui->groupExternalConfig->isChecked());
  const auto selected = m_devicePicker->currentText();
  const QSignalBlocker pickerBlock(m_devicePicker);
  m_devicePicker->clear();
  for (const auto &computer : std::as_const(m_serverConfig).computers())
    if (!computer.isNull() && !computer.isServer())
      m_devicePicker->addItem(computer.name());
  const auto previous = m_devicePicker->findText(selected);
  if (previous >= 0)
    m_devicePicker->setCurrentIndex(previous);
  ui->computerSetupView->setEnabled(writable && !custom);
  ui->computerSetupView->setVisible(!custom);
  ui->lblNewComputer->setEnabled(writable && !custom && !m_computerSetupModel.isFull());
  ui->lblRemoveComputer->setEnabled(writable && !custom);
  const auto error = m_serverConfig.desktopLayoutError();
  m_layoutSummary->setText(
      !error.isEmpty() ? tr("Layout needs attention: %1").arg(error)
      : custom         ? tr("Custom alignment · %1 computers. Edit devices below or arrange their shared edges.")
                     .arg(m_serverConfig.numComputers())
               : tr("The grid is preserved. Arrange computers to set sizes, offsets and shared edges.")
  );
}

void ServerConfigDialog::resetFromSettings()
{
  m_serverConfig = m_originalServerConfig;
  loadFromConfig();
  reloadLayoutDraft();
  if (ui->tabWidget->currentWidget() == ui->tabComputers) {
    ui->computerSetupView->reset();
    ui->computerSetupView->update();
  }
}

void ServerConfigDialog::refreshControls()
{
  ui->rbProtocolSynergy->setChecked(m_protocol == NetworkProtocol::Synergy);
  ui->rbProtocolBarrier->setChecked(m_protocol == NetworkProtocol::Barrier);
  ui->cbHeartbeat->setChecked(m_enableHeartbeat);
  ui->sbHeartbeat->setEnabled(ui->cbHeartbeat->isChecked());
  ui->sbHeartbeat->setValue(m_heartbeatRate);
  ui->cbRelativeMouseMoves->setChecked(m_relativeMouseMoves);
  ui->cbWin32KeepForeground->setChecked(m_win32keepForeground);
  ui->cbSwitchDelay->setChecked(m_enableSwitchDelay);
  ui->sbSwitchDelay->setEnabled(ui->cbSwitchDelay->isChecked());
  ui->sbSwitchDelay->setValue(m_switchDelay);
  ui->cbSwitchDoubleTap->setChecked(m_enableSwitchDoubleTap);
  ui->sbSwitchDoubleTap->setEnabled(ui->cbSwitchDoubleTap->isChecked());
  ui->sbSwitchDoubleTap->setValue(m_switchDoubleTap);
  ui->widgetExternalConfigControls->setEnabled(ui->groupExternalConfig->isChecked());
  toggleExternalConfig(ui->groupExternalConfig->isChecked());
  ui->cbDefaultLockToComputerState->setChecked(m_defaultLockToComputerState);
  ui->cbDisableLockToComputer->setChecked(m_disableLockToComputer);
  ui->cbEnableClipboard->setChecked(m_enableClipboard);
  ui->sbClipboardSizeLimit->setEnabled(m_enableClipboard);
  ui->sbClipboardSizeLimit->setValue(m_clipboardSize);
}

void ServerConfigDialog::initConnections() const
{
  connect(m_editLayout, &QPushButton::clicked, this, &ServerConfigDialog::editDesktopLayout);
  connect(m_restoreGrid, &QPushButton::clicked, this, &ServerConfigDialog::restoreGridLayout);
  connect(m_buttonBox, &SettingsDialogButtonBox::accepted, this, &ServerConfigDialog::save);
  connect(m_buttonBox, &SettingsDialogButtonBox::rejected, this, &ServerConfigDialog::cancel);
  connect(m_buttonBox, &SettingsDialogButtonBox::reset, this, &ServerConfigDialog::resetFromSettings);
  connect(m_buttonBox, &SettingsDialogButtonBox::restoreDefault, this, &ServerConfigDialog::restoreFromDefaults);
  connect(ui->tabWidget, &QTabWidget::currentChanged, this, &ServerConfigDialog::setButtonBoxEnabledButtons);
  connect(ui->lblRemoveComputer, &RemoveComputerWidget::computerRemoved, this, &ServerConfigDialog::onComputerRemoved);
  connect(ui->btnNewHotkey, &QPushButton::clicked, this, &ServerConfigDialog::addHotkey);
  connect(ui->btnEditHotkey, &QPushButton::clicked, this, &ServerConfigDialog::editHotkey);
  connect(ui->btnRemoveHotkey, &QPushButton::clicked, this, &ServerConfigDialog::removeHotkey);
  connect(ui->listHotkeys, &QListView::doubleClicked, this, &ServerConfigDialog::editHotkey);
  connect(
      ui->listHotkeys->selectionModel(), &QItemSelectionModel::selectionChanged, this,
      &ServerConfigDialog::listHotkeysSelectionChanged
  );

  connect(ui->btnNewAction, &QPushButton::clicked, this, &ServerConfigDialog::addAction);
  connect(ui->btnEditAction, &QPushButton::clicked, this, &ServerConfigDialog::editAction);
  connect(ui->btnRemoveAction, &QPushButton::clicked, this, &ServerConfigDialog::removeAction);
  connect(ui->listActions, &QListView::doubleClicked, this, &ServerConfigDialog::editAction);
  connect(
      ui->listActions->selectionModel(), &QItemSelectionModel::selectionChanged, this,
      &ServerConfigDialog::listActionsSelectionChanged
  );

  connect(ui->rbProtocolBarrier, &QRadioButton::toggled, this, &ServerConfigDialog::toggleProtocol);
  connect(ui->cbHeartbeat, &QCheckBox::toggled, this, &ServerConfigDialog::toggleHeartbeat);
  connect(ui->sbHeartbeat, QOverload<int>::of(&QSpinBox::valueChanged), this, &ServerConfigDialog::setHeartbeat);
  connect(ui->cbWin32KeepForeground, &QCheckBox::toggled, this, &ServerConfigDialog::toggleWin32Foreground);
  connect(ui->cbSwitchDelay, &QCheckBox::toggled, this, &ServerConfigDialog::toggleSwitchDelay);
  connect(ui->sbSwitchDelay, QOverload<int>::of(&QSpinBox::valueChanged), this, &ServerConfigDialog::setSwitchDelay);
  connect(ui->cbSwitchDoubleTap, &QCheckBox::toggled, this, &ServerConfigDialog::toggleSwitchDoubleTap);
  connect(
      ui->sbSwitchDoubleTap, QOverload<int>::of(&QSpinBox::valueChanged), this, &ServerConfigDialog::setSwitchDoubleTap
  );

  connect(ui->cbRelativeMouseMoves, &QCheckBox::toggled, this, &ServerConfigDialog::toggleRelativeMouseMoves);
  connect(ui->cbEnableClipboard, &QCheckBox::toggled, this, &ServerConfigDialog::toggleClipboard);
  connect(ui->btnBrowseConfigFile, &QPushButton::clicked, this, &ServerConfigDialog::browseConfigFile);
  connect(ui->groupExternalConfig, &QGroupBox::toggled, this, &ServerConfigDialog::toggleExternalConfig);
  connect(ui->lineConfigFile, &QLineEdit::textChanged, this, &ServerConfigDialog::setServerConfig);

  connect(
      ui->sbClipboardSizeLimit, QOverload<int>::of(&QSpinBox::valueChanged), this,
      &ServerConfigDialog::setClipboardLimit
  );
  connect(
      ui->cbDefaultLockToComputerState, &QCheckBox::toggled, this, &ServerConfigDialog::toggleDefaultLockToComputerState
  );
  connect(ui->cbDisableLockToComputer, &QCheckBox::toggled, this, &ServerConfigDialog::toggleLockToComputer);
  connect(
      &m_computerSetupModel, &ComputerSetupModel::computersChanged, this,
      &ServerConfigDialog::setButtonBoxEnabledButtons
  );
  connect(Settings::instance(), &Settings::settingsWritableChanged, this, &ServerConfigDialog::updateControls);
}

void ServerConfigDialog::updateControls() const
{
  const bool writable = Settings::isWritable();
  ui->cbDefaultLockToComputerState->setEnabled(writable);
  ui->cbDisableLockToComputer->setEnabled(writable);
  ui->cbEnableClipboard->setEnabled(writable);
  ui->sbClipboardSizeLimit->setEnabled(writable);
  ui->rbProtocolBarrier->setEnabled(writable);
  ui->rbProtocolSynergy->setEnabled(writable);
  ui->cbHeartbeat->setEnabled(writable);
  ui->cbRelativeMouseMoves->setEnabled(writable);
  ui->cbSwitchDelay->setEnabled(writable);
  ui->cbWin32KeepForeground->setEnabled(writable);
  ui->cbSwitchDoubleTap->setEnabled(writable);
  ui->sbSwitchDoubleTap->setEnabled(writable && ui->cbSwitchDoubleTap->isChecked());
  ui->sbSwitchDelay->setEnabled(writable && ui->cbSwitchDelay->isChecked());
  ui->groupExternalConfig->setEnabled(writable);
  updateLayoutControls();
  setButtonBoxEnabledButtons();
}

void ServerConfigDialog::restoreFromDefaults()
{
  m_protocol = networkProtocolFromString(Settings::defaultValue(Settings::Server::Protocol).toString());
  m_enableHeartbeat = Settings::defaultValue(Settings::Server::EnableHeartbeat).toBool();
  m_heartbeatRate = Settings::defaultValue(Settings::Server::Heartbeat).toInt();
  m_relativeMouseMoves = Settings::defaultValue(Settings::Server::RelativeMouseMoves).toBool();
  m_win32keepForeground = Settings::defaultValue(Settings::Server::Win32KeepForeground).toBool();
  m_enableSwitchDelay = Settings::defaultValue(Settings::Server::EnableSwitchDelay).toBool();
  m_switchDelay = Settings::defaultValue(Settings::Server::SwitchDelay).toInt();
  m_enableSwitchDoubleTap = Settings::defaultValue(Settings::Server::EnableSwitchDoubleTap).toBool();
  m_switchDoubleTap = Settings::defaultValue(Settings::Server::SwitchDoubleTap).toInt();
  m_defaultLockToComputerState = Settings::defaultValue(Settings::Server::DefaultLockToComputerState).toBool();
  m_disableLockToComputer = Settings::defaultValue(Settings::Server::DisableLockToComputer).toBool();
  m_enableClipboard = Settings::defaultValue(Settings::Server::EnableClipboard).toBool();
  m_clipboardSize = Settings::defaultValue(Settings::Server::ClipboardSize).toUInt();

  ui->groupExternalConfig->setChecked(Settings::defaultValue(Settings::Server::ExternalConfig).toBool());
  ui->lineConfigFile->setText(Settings::defaultValue(Settings::Server::ExternalConfigFile).toString());

  refreshControls();
  updateControls();
}

void ServerConfigDialog::setServerConfig()
{
  const auto configFile = ui->lineConfigFile->text();
  if (!QFile::exists(configFile)) {
    m_buttonBox->enableSave(false);
    return;
  }
  setButtonBoxEnabledButtons();
}

bool ServerConfigDialog::addComputer(const QString &clientName, bool doSilent)
{
  bool isAccepted = false;
  Computer newComputer(clientName);

  if (ComputerSettingsDialog dlg(this, &newComputer, &model().m_computers);
      doSilent || dlg.exec() == QDialog::Accepted) {
    if (m_serverConfig.hasDesktopLayout()) {
      m_serverConfig.addClient(newComputer.name());
      for (auto &computer : m_serverConfig.computers())
        if (computer.name() == newComputer.name())
          computer = newComputer;
    } else {
      model().addComputer(newComputer);
    }
    isAccepted = true;
  }

  updateLayoutControls();
  if (isAccepted)
    reloadLayoutDraft();
  setButtonBoxEnabledButtons();
  return isAccepted;
}

bool ServerConfigDialog::isGeneralConfigModified() const
{
  return m_originalServerConfigIsExternal != ui->groupExternalConfig->isChecked() ||
         m_originalServerConfigUsesExternalFile != ui->lineConfigFile->text() ||
         m_protocol != Settings::networkProtocol() ||
         m_enableClipboard != Settings::value(Settings::Server::EnableClipboard).toBool() ||
         m_clipboardSize != Settings::value(Settings::Server::ClipboardSize).toUInt() ||
         m_enableHeartbeat != Settings::value(Settings::Server::EnableHeartbeat).toBool() ||
         m_heartbeatRate != Settings::value(Settings::Server::Heartbeat).toInt() ||
         m_enableSwitchDelay != Settings::value(Settings::Server::EnableSwitchDelay).toBool() ||
         m_switchDelay != Settings::value(Settings::Server::SwitchDelay).toInt() ||
         m_enableSwitchDoubleTap != Settings::value(Settings::Server::EnableSwitchDoubleTap).toBool() ||
         m_switchDoubleTap != Settings::value(Settings::Server::SwitchDoubleTap).toInt() ||
         m_relativeMouseMoves != Settings::value(Settings::Server::RelativeMouseMoves).toBool() ||
         m_win32keepForeground != Settings::value(Settings::Server::Win32KeepForeground).toBool() ||
         m_disableLockToComputer != Settings::value(Settings::Server::DisableLockToComputer).toBool() ||
         m_defaultLockToComputerState != Settings::value(Settings::Server::DefaultLockToComputerState).toBool();
}

bool ServerConfigDialog::isGeneralConfigDefault() const
{
  return ui->groupExternalConfig->isChecked() == Settings::defaultValue(Settings::Server::ExternalConfig).toBool() &&
         ui->lineConfigFile->text() == Settings::defaultValue(Settings::Server::ExternalConfigFile).toString() &&
         m_protocol == networkProtocolFromString(Settings::defaultValue(Settings::Server::Protocol).toString()) &&
         m_enableClipboard == Settings::defaultValue(Settings::Server::EnableClipboard).toBool() &&
         m_clipboardSize == Settings::defaultValue(Settings::Server::ClipboardSize).toUInt() &&
         m_enableHeartbeat == Settings::defaultValue(Settings::Server::EnableHeartbeat).toBool() &&
         m_heartbeatRate == Settings::defaultValue(Settings::Server::Heartbeat).toInt() &&
         m_enableSwitchDelay == Settings::defaultValue(Settings::Server::EnableSwitchDelay).toBool() &&
         m_switchDelay == Settings::defaultValue(Settings::Server::SwitchDelay).toInt() &&
         m_enableSwitchDoubleTap == Settings::defaultValue(Settings::Server::EnableSwitchDoubleTap).toBool() &&
         m_switchDoubleTap == Settings::defaultValue(Settings::Server::SwitchDoubleTap).toInt() &&
         m_relativeMouseMoves == Settings::defaultValue(Settings::Server::RelativeMouseMoves).toBool() &&
         m_win32keepForeground == Settings::defaultValue(Settings::Server::Win32KeepForeground).toBool() &&
         m_disableLockToComputer == Settings::defaultValue(Settings::Server::DisableLockToComputer).toBool() &&
         m_defaultLockToComputerState == Settings::defaultValue(Settings::Server::DefaultLockToComputerState).toBool();
}

void ServerConfigDialog::setButtonBoxEnabledButtons() const
{
  const bool writable = Settings::isWritable();
  auto monitorDraft = m_monitorDraft;
  const bool validDraft =
      !m_layoutEditor || !m_layoutEditor->editingEnabled() ||
      (m_monitorMode ? monitorDraft.apply(m_layoutEditor->desktops())
                     : deskflow::gui::DesktopLayout::draftError(m_layoutEditor->desktops()).isEmpty());
  const bool editorChanged = m_layoutEditor && m_layoutEditor->desktops() != m_loadedLayout;
  m_buttonBox->enableSave(
      writable && validDraft &&
      (ui->groupExternalConfig->isChecked() || m_serverConfig.desktopLayoutError().isEmpty()) &&
      (isGeneralConfigModified() || !(m_originalServerConfig == m_serverConfig))
  );
  m_buttonBox->enableReset(
      writable && (editorChanged || isGeneralConfigModified() || !(m_originalServerConfig == m_serverConfig))
  );
  m_buttonBox->enableRestoreDefaults(writable && !isGeneralConfigDefault() && ui->tabWidget->currentIndex() == 2);
}
