// SPDX-FileCopyrightText: (C) 2026 Deskflow Developers
// SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
#include "DesktopWorkspaceView.h"
#include "ActionButton.h"
#include "gui/ExperienceStyle.h"
#include "gui/config/MonitorLayoutDraft.h"
#include "gui/dialogs/DesktopLayoutEditor.h"
#include <QComboBox>
#include <QDoubleSpinBox>
#include <QEvent>
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QScrollArea>
#include <QSignalBlocker>
#include <QSplitter>
#include <QVBoxLayout>
#include <QtMath>

namespace deskflow::gui {
using namespace display;
namespace {
QLabel *textLabel(QWidget *parent, const QString &name)
{
  auto *label = new QLabel(parent);
  label->setObjectName(name);
  label->setWordWrap(true);
  label->setTextFormat(Qt::PlainText);
  label->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Maximum);
  label->setAlignment(Qt::AlignLeft | Qt::AlignTop);
  label->setTextInteractionFlags(Qt::TextSelectableByMouse);
  return label;
}
QPushButton *action(QWidget *parent, const QString &name, const QString &text)
{
  auto *button = new ActionButton(text, parent);
  button->setObjectName(name);
  button->setAutoDefault(false);
  button->ensurePolished();
  button->setMinimumSize(button->sizeHint());
  return button;
}
QString displayScaleText(const WorkspaceDisplay &screen)
{
  // Native rectangles are rounded to whole logical units. Only collapse the
  // two ratios when one integral percentage explains both within that rounding.
  const int percent = qRound((screen.displayScale.x() + screen.displayScale.y()) * 50);
  const double scale = percent / 100.;
  if (scale > 0 && qAbs(screen.pixelResolution.width() / scale - screen.logicalRect.width()) <= 1 &&
      qAbs(screen.pixelResolution.height() / scale - screen.logicalRect.height()) <= 1)
    return QString::number(percent) + '%';
  return QString("%1% × %2%")
      .arg(screen.displayScale.x() * 100, 0, 'f', 1)
      .arg(screen.displayScale.y() * 100, 0, 'f', 1);
}
} // namespace
DesktopWorkspaceView::DesktopWorkspaceView(QWidget *parent)
    : QWidget(parent),
      m_workspace(std::make_unique<DesktopWorkspace>(DeviceSnapshots{})),
      m_geometry({}, true),
      m_canvas(new LayoutCanvas(m_geometry, this))
{
  setObjectName("desktopWorkspace");
  applyExperienceStyle(this);
  auto baseFont = font();
  baseFont.setPointSizeF(qMax(12., baseFont.pointSizeF()));
  setFont(baseFont);
  auto *root = new QVBoxLayout(this);
  root->setContentsMargins(16, 4, 16, 8);
  root->setSpacing(6);
  auto *toolbar = new QHBoxLayout;
  toolbar->setObjectName("workspaceToolbarLayout");
  m_add = action(this, "workspaceAdd", tr("Add computer…"));
  m_identify = action(this, "workspaceIdentify", tr("Identify screens"));
  m_edit = action(this, "workspaceEdit", tr("Edit arrangement"));
  m_activeRouting = textLabel(this, "workspaceActiveRouting");
  m_routingDescription = tr("Routing state has not been reported.");
  m_activeRouting->setText(m_routingDescription);
  m_activeRouting->setMinimumWidth(160);
  toolbar->addWidget(m_activeRouting, 1);
  toolbar->addWidget(m_add);
  toolbar->addWidget(m_identify);
  toolbar->addWidget(m_edit);
  root->addLayout(toolbar);
  m_receiverSummary = textLabel(this, "workspaceReceiverSummary");
  root->addWidget(m_receiverSummary);
  m_details = action(this, "workspaceDetails", tr("View applied arrangement"));
  m_details->setCheckable(true);
  root->addWidget(m_details, 0, Qt::AlignLeft);
  auto *splitter = new QSplitter(Qt::Horizontal, this);
  m_arrangement = splitter;
  splitter->setObjectName("workspacePanes");
  splitter->setChildrenCollapsible(false);
  splitter->setHandleWidth(10);
  root->addWidget(splitter, 1);
  auto *canvasPane = new QWidget(splitter);
  auto *canvasLayout = new QVBoxLayout(canvasPane);
  canvasLayout->setContentsMargins(0, 0, 0, 0);
  canvasLayout->setSpacing(6);
  auto *inspectorPane = new QWidget(splitter);
  inspectorPane->setObjectName("workspaceInspectorPane");
  inspectorPane->setMinimumWidth(240);
  inspectorPane->setMaximumWidth(360);
  auto *inspectorLayout = new QVBoxLayout(inspectorPane);
  inspectorLayout->setContentsMargins(4, 0, 0, 0);
  inspectorLayout->setSpacing(8);
  m_selection = new QComboBox(inspectorPane);
  m_selection->setObjectName("workspaceSelection");
  m_selection->setAccessibleName(tr("Inspect a screen or computer"));
  m_selection->setSizeAdjustPolicy(QComboBox::AdjustToMinimumContentsLengthWithIcon);
  m_selection->setMinimumContentsLength(10);
  m_selection->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
  inspectorLayout->addWidget(m_selection);
  auto *scroll = m_scroll = new QScrollArea(inspectorPane);
  scroll->setObjectName("workspaceScroll");
  scroll->setWidgetResizable(true);
  scroll->setFrameShape(QFrame::NoFrame);
  scroll->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
  auto *body = new QWidget(scroll);
  auto *content = new QVBoxLayout(body);
  content->setContentsMargins(0, 0, 8, 0);
  content->setSpacing(12);
  scroll->setWidget(body);
  inspectorLayout->addWidget(scroll, 1);
  m_help = textLabel(body, "workspaceHelp");
  m_mode = new QComboBox(canvasPane);
  m_mode->setObjectName("workspaceMode");
  m_mode->setAccessibleName(tr("Arrangement mode"));
  m_mode->setSizeAdjustPolicy(QComboBox::AdjustToMinimumContentsLengthWithIcon);
  m_mode->setMinimumContentsLength(10);
  m_mode->addItem(tr("Move computers · keep the system screen arrangement"));
  m_mode->addItem(tr("Custom screen routes · change Deskflow mapping only"));
  canvasLayout->addWidget(m_mode);
  m_canvas->setMonitorMode(true);
  m_canvas->setMinimumWidth(240);
  m_canvas->setAccessibleName(tr("All computers and their screens"));
  canvasLayout->addWidget(m_canvas, 1);
  splitter->addWidget(canvasPane);
  splitter->addWidget(inspectorPane);
  splitter->setStretchFactor(0, 1);
  splitter->setStretchFactor(1, 0);
  splitter->setSizes({620, 260});
  m_inspector = textLabel(body, "workspaceInspector");
  content->addWidget(m_inspector);
  m_reviewOs = action(body, "workspaceReviewOs", tr("Review system arrangement"));
  content->addWidget(m_reviewOs, 0, Qt::AlignLeft);
  m_physicalNote = textLabel(body, "workspacePhysicalNote");
  content->addWidget(m_physicalNote);
  m_calibrationPanel = new QWidget(body);
  auto *calibrationPanelLayout = new QVBoxLayout(m_calibrationPanel);
  calibrationPanelLayout->setContentsMargins(0, 0, 0, 0);
  content->addWidget(m_calibrationPanel);
  auto *physical = new QHBoxLayout;
  m_widthMm = new QDoubleSpinBox(body);
  m_heightMm = new QDoubleSpinBox(body);
  m_widthMm->setObjectName("workspaceWidthMm");
  m_heightMm->setObjectName("workspaceHeightMm");
  for (auto *spin : {m_widthMm, m_heightMm}) {
    spin->setRange(0, 3000);
    spin->setDecimals(1);
    spin->setSuffix(tr(" mm"));
    spin->setSpecialValueText(tr("Unknown"));
  }
  m_widthMm->setAccessibleName(tr("Measured screen width in millimetres"));
  m_heightMm->setAccessibleName(tr("Measured screen height in millimetres"));
  physical->addWidget(new QLabel(tr("Width"), body));
  physical->addWidget(m_widthMm, 1);
  physical->addWidget(new QLabel(tr("Height"), body));
  physical->addWidget(m_heightMm, 1);
  calibrationPanelLayout->addLayout(physical);
  auto *calibration = new QHBoxLayout;
  m_calibrate = action(body, "workspaceCalibrate", tr("Save measured size"));
  m_clearCalibration = action(body, "workspaceClearCalibration", tr("Clear measured size"));
  calibration->addWidget(m_calibrate);
  calibration->addWidget(m_clearCalibration);
  calibration->addStretch();
  calibrationPanelLayout->addLayout(calibration);
  // Optional dimensions remain reachable in the inspector; they never force
  // the canvas or its selected screen outside the viewport.
  physical->setDirection(QBoxLayout::TopToBottom);
  calibration->setDirection(QBoxLayout::TopToBottom);
  content->addWidget(m_help);
  content->addStretch();
  m_status = textLabel(this, "workspaceDraftStatus");
  root->addWidget(m_status);
  auto *footer = new QHBoxLayout;
  footer->setObjectName("workspaceFooterLayout");
  m_undo = action(this, "workspaceUndo", tr("Undo"));
  m_cancel = action(this, "workspaceCancel", tr("Cancel"));
  m_apply = action(this, "workspaceApply", tr("Apply arrangement"));
  m_apply->setProperty("primaryAction", true);
  footer->addWidget(m_undo);
  footer->addStretch();
  footer->addWidget(m_cancel);
  footer->addWidget(m_apply);
  root->addLayout(footer);
  m_receiverSpace = new QSpacerItem(0, 0, QSizePolicy::Minimum, QSizePolicy::Minimum);
  root->addItem(m_receiverSpace);
  connect(m_add, &QPushButton::clicked, this, &DesktopWorkspaceView::addComputerRequested);
  connect(m_identify, &QPushButton::clicked, this, [this] { Q_EMIT identifyRequested(m_selectedDevice); });
  connect(m_edit, &QPushButton::clicked, this, &DesktopWorkspaceView::beginEdit);
  connect(m_details, &QPushButton::toggled, this, [this] { rebuild(); });
  connect(m_cancel, &QPushButton::clicked, this, &DesktopWorkspaceView::cancelEdit);
  connect(m_undo, &QPushButton::clicked, this, [this] {
    if (m_workspace->undo()) {
      rebuild();
      Q_EMIT draftChanged(m_workspace->state().encode());
    }
  });
  connect(m_selection, &QComboBox::currentIndexChanged, this, [this](int index) {
    const auto key = m_selection->itemData(index).toString();
    select(key.section('/', 0, 0), key.section('/', 1));
  });
  connect(m_canvas, &LayoutCanvas::selected, this, [this](int index) {
    const auto displays = m_workspace->displays();
    if (index >= 0 && index < displays.size())
      select(displays[index].deviceId, displays[index].stableId);
  });
  connect(m_canvas, &LayoutCanvas::unknownDeviceSelected, this, [this](const QString &id) { select(id); });
  connect(m_canvas, &LayoutCanvas::moved, this, [this](const QString &key, double x, double y, double threshold) {
    if (!m_editing || !m_readOnlyReason.isEmpty())
      return;
    for (const auto &screen : m_workspace->displays())
      if (MonitorLayoutDraft::token({screen.deviceId, screen.stableId, {}}) == key) {
        if (m_workspace->moveDisplay(screen.deviceId, screen.stableId, {x, y}, threshold)) {
          rebuild();
          Q_EMIT draftChanged(m_workspace->state().encode());
        }
        break;
      }
  });
  connect(m_mode, &QComboBox::currentIndexChanged, this, [this](int index) {
    if (!m_editing)
      return;
    if (!m_workspace->setMode(index == 0 ? ArrangementMode::PreserveOsGroups : ArrangementMode::CustomScreenRoutes)) {
      rebuild();
      m_status->setText(
          tr("This custom arrangement differs from the system layout. Review the system arrangement before regrouping.")
      );
      return;
    }
    rebuild();
    Q_EMIT draftChanged(m_workspace->state().encode());
  });
  connect(m_calibrate, &QPushButton::clicked, this, [this] {
    if (!m_editing || !m_readOnlyReason.isEmpty())
      return;
    if (!m_workspace->calibratePhysicalSize(
            m_selectedDevice, m_selectedDisplay, QSizeF(m_widthMm->value(), m_heightMm->value())
        )) {
      m_status->setText(tr("Enter the measured width and height, each between 10 and 3000 mm."));
      return;
    }
    rebuild();
    Q_EMIT draftChanged(m_workspace->state().encode());
  });
  connect(m_clearCalibration, &QPushButton::clicked, this, [this] {
    if (m_editing && m_readOnlyReason.isEmpty() &&
        m_workspace->calibratePhysicalSize(m_selectedDevice, m_selectedDisplay, {})) {
      rebuild();
      Q_EMIT draftChanged(m_workspace->state().encode());
    }
  });
  connect(m_reviewOs, &QPushButton::clicked, this, [this] {
    if (m_editing && m_readOnlyReason.isEmpty() && m_workspace->reviewOsLayout(m_selectedDevice)) {
      rebuild();
      Q_EMIT draftChanged(m_workspace->state().encode());
    }
  });
  connect(m_apply, &QPushButton::clicked, this, [this] {
    if (m_role != Role::Controller || m_savePending || !m_editing || !m_submitAvailable ||
        !m_readOnlyReason.isEmpty() || !m_workspace->validationError().isEmpty())
      return;
    const auto candidate = m_workspace->candidateForApply();
    const auto state = m_workspace->state().encode();
    // The host must acknowledge a successful atomic write. Merely emitting a
    // request does not clear the draft or claim that live routing changed.
    m_savePending = true;
    m_saveError.clear();
    rebuild();
    if (candidate)
      Q_EMIT applyRequested(state, candidate->encode());
    else
      Q_EMIT saveDraftRequested(state);
  });
  rebuild();
}
void DesktopWorkspaceView::setRole(Role role, const QString &controllerName, bool appliedLayoutAvailable)
{
  if (role != m_role) {
    const QSignalBlocker block(m_details);
    m_details->setChecked(false);
    m_editing = false;
    m_workspace = std::make_unique<DesktopWorkspace>(m_snapshots, m_clean, m_applied);
  }
  m_role = role;
  m_controllerName = controllerName;
  m_appliedViewAvailable = appliedLayoutAvailable;
  rebuild();
  layout()->setAlignment(role == Role::Receiver ? Qt::AlignTop : Qt::Alignment{});
}
void DesktopWorkspaceView::setSnapshot(DeviceSnapshots snapshots)
{
  const auto draft = m_editing ? m_workspace->state().encode() : m_clean;
  m_snapshots = std::move(snapshots);
  m_workspace = std::make_unique<DesktopWorkspace>(m_snapshots, draft, m_applied);
  if (!m_editing && m_workspace->error().isEmpty())
    m_clean = m_workspace->state().encode();
  rebuild();
}
void DesktopWorkspaceView::setDraft(const QByteArray &state, const QByteArray &applied)
{
  m_clean = state;
  m_applied = applied;
  m_editing = false;
  m_savePending = false;
  m_saveError.clear();
  m_workspace = std::make_unique<DesktopWorkspace>(m_snapshots, state, applied);
  rebuild();
}
void DesktopWorkspaceView::acknowledgeSaved(const QByteArray &state, const QByteArray &applied)
{
  setDraft(state, applied);
}
void DesktopWorkspaceView::setAppliedLayout(const QByteArray &layout)
{
  if (m_applied == layout)
    return;
  const auto draft = m_workspace->state().encode();
  m_applied = layout;
  m_workspace = std::make_unique<DesktopWorkspace>(m_snapshots, draft, m_applied);
  rebuild();
}
void DesktopWorkspaceView::rejectSave(const QString &reason)
{
  m_savePending = false;
  m_saveError = reason;
  rebuild();
}
void DesktopWorkspaceView::setReadOnlyReason(const QString &reason)
{
  m_readOnlyReason = reason;
  rebuild();
}
void DesktopWorkspaceView::setSubmitAvailable(bool available)
{
  m_submitAvailable = available;
  rebuild();
}
void DesktopWorkspaceView::setActionsAvailable(bool add, bool identify)
{
  m_addAvailable = add;
  m_identifyAvailable = identify;
  rebuild();
}
void DesktopWorkspaceView::setActiveRouting(const QString &description)
{
  m_routingDescription = description;
  refreshRoutingSummary();
}
void DesktopWorkspaceView::refreshRoutingSummary()
{
  m_activeRouting->setText(
      m_editing ? tr("Current draft has not been applied. %1").arg(m_routingDescription) : m_routingDescription
  );
}
void DesktopWorkspaceView::beginEdit()
{
  if (m_role != Role::Controller || m_savePending || !m_readOnlyReason.isEmpty() || !m_workspace->error().isEmpty())
    return;
  m_editing = true;
  rebuild();
}
void DesktopWorkspaceView::cancelEdit()
{
  if (m_savePending)
    return;
  m_saveError.clear();
  m_editing = false;
  m_workspace = std::make_unique<DesktopWorkspace>(m_snapshots, m_clean, m_applied);
  rebuild();
  Q_EMIT cancelRequested();
}
QString DesktopWorkspaceView::unknownDescription(const DeviceSnapshot &d) const
{
  const auto name = d.displayName.isEmpty() ? d.id : d.displayName;
  if (d.observation == Observation::Unsupported || d.topologyCapability == Capability::Unsupported)
    return tr("%1 · update this computer to report its individual screens").arg(name);
  if (d.observation == Observation::Unavailable)
    return tr("%1 · screen detection unavailable").arg(name);
  return tr("%1 · screen information not received").arg(name);
}
void DesktopWorkspaceView::rebuild()
{
  const bool controller = m_role == Role::Controller;
  m_receiverSpace->changeSize(0, 0, QSizePolicy::Minimum, controller ? QSizePolicy::Minimum : QSizePolicy::Expanding);
  layout()->invalidate();
  std::vector<experience::Desktop> geometry;
  QMap<QString, LayoutCanvas::DisplayPresentation> presentation;
  const auto screens = m_workspace->displays();
  const QSignalBlocker blockSelection(m_selection), blockMode(m_mode);
  m_selection->clear();
  for (const auto &screen : screens) {
    const auto k = DesktopWorkspace::key(screen.deviceId, screen.stableId);
    const auto token = MonitorLayoutDraft::token({screen.deviceId, screen.stableId, {}});
    const auto name = tr("%1 · Screen %2").arg(screen.deviceName).arg(screen.number);
    const auto r = screen.canvasRect;
    geometry.push_back({token.toStdString(), r.x(), r.y(), r.width(), r.height(), name.toStdString()});
    presentation[token] = {
        screen.deviceId, screen.deviceName, QString::number(screen.number),
        screen.cached
            ? tr("Last detected · %1 × %2 px").arg(screen.pixelResolution.width()).arg(screen.pixelResolution.height())
            : tr("%1 × %2 px").arg(screen.pixelResolution.width()).arg(screen.pixelResolution.height()),
        screen.cached
    };
    m_selection->addItem(name, k);
  }
  QMap<QString, QString> unknown;
  for (const auto &id : m_workspace->unknownDevices())
    for (const auto &d : m_snapshots)
      if (d.id == id) {
        const auto name = d.displayName.isEmpty() ? id : d.displayName;
        unknown[id] = name + "\n" +
                      (d.topologyCapability == Capability::Unsupported || d.observation == Observation::Unsupported
                           ? tr("Update required")
                           : tr("Screen information unknown"));
        m_selection->addItem(unknownDescription(d), id);
        break;
      }
  m_geometry = experience::LayoutModel(std::move(geometry), true);
  m_canvas->setDisplayPresentation(std::move(presentation));
  m_canvas->setUnknownDevices(std::move(unknown));
  const bool group = m_workspace->state().mode == ArrangementMode::PreserveOsGroups;
  m_canvas->setDragGroups(group);
  m_canvas->setEditable(m_editing && !m_savePending && m_readOnlyReason.isEmpty());
  QVector<QLineF> edges;
  for (const auto &route : m_workspace->previewRoutes())
    edges.append(route.segment);
  m_canvas->setMonitorEdges(edges);
  m_mode->setCurrentIndex(group ? 0 : 1);
  m_mode->setVisible(m_editing);
  m_mode->setEnabled(!m_savePending && m_readOnlyReason.isEmpty());
  m_help->setText(
      !controller  ? tr("Read-only. The controller manages this arrangement. Select a screen to inspect it.")
      : !m_editing ? tr("Select a screen for its details. Choose Edit arrangement to change Deskflow crossings.")
      : group ? tr("Each rectangle is one detected screen. Move a computer as a group to keep its system arrangement. "
                   "Green edges preview Deskflow crossings.")
              : tr("Custom routes change Deskflow mapping only. They do not change system display settings. Edges "
                   "between this computer's own screens remain controlled by the system.")
  );
  int index = m_selection->findData(
      m_selectedDisplay.isEmpty() ? m_selectedDevice : DesktopWorkspace::key(m_selectedDevice, m_selectedDisplay)
  );
  if (index < 0 && m_selection->count())
    index = 0;
  m_selection->setCurrentIndex(index);
  const auto selected = m_selection->itemData(index).toString();
  m_selectedDevice = selected.section('/', 0, 0);
  m_selectedDisplay = selected.section('/', 1);
  m_canvas->setSelected(index < screens.size() ? index : -1);
  const auto error = m_workspace->validationError();
  if (m_savePending)
    m_status->setText(tr("Saving arrangement…"));
  else if (!m_saveError.isEmpty())
    m_status->setText(m_saveError);
  else if (!m_readOnlyReason.isEmpty())
    m_status->setText(m_readOnlyReason);
  else if (!m_workspace->conflicts().isEmpty())
    m_status->setText(
        tr("Screen configuration changed on %1. The saved arrangement is preserved; review it before applying.")
            .arg(m_workspace->conflicts().keys().join(", "))
    );
  else if (!m_submitAvailable)
    m_status->setText(tr("Preview only · saving and live routing are not connected in this view."));
  else if (!error.isEmpty())
    m_status->setText(
        error == "monitor-layout-overlap" ? tr("Screens overlap. Move the computer or undo this change.")
        : error == "no-reachable-display-edge"
            ? tr("Join exposed screen edges so the pointer can reach the other computer.")
            : tr("The arrangement cannot be applied safely. Review the current screens or cancel to preserve the saved "
                 "arrangement.")
    );
  else if (!m_workspace->candidateForApply())
    m_status->setText(tr("Draft only · reconnect and verify the computers before applying input routes."));
  else
    m_status->setText(
        m_editing ? tr("Draft · not applied. Apply to save and request confirmation from the current core.")
                  : tr("Preview · %1 detected screens. Applying saves routes; live status is shown separately above.")
                        .arg(screens.size())
    );
  if (m_editing && !error.isEmpty() && !m_savePending && m_saveError.isEmpty() && m_readOnlyReason.isEmpty())
    m_status->setText(tr("Draft · not applied. %1").arg(m_status->text()));
  refreshRoutingSummary();
  m_edit->setVisible(controller && !m_editing && m_readOnlyReason.isEmpty());
  m_edit->setEnabled(m_workspace->error().isEmpty() && !screens.isEmpty());
  m_cancel->setVisible(controller && m_editing);
  m_undo->setVisible(controller && m_editing);
  m_apply->setVisible(controller && m_editing);
  m_cancel->setEnabled(!m_savePending);
  m_undo->setEnabled(!m_savePending && m_workspace->canUndo());
  m_apply->setText(m_workspace->candidateForApply() || !error.isEmpty() ? tr("Apply arrangement") : tr("Save draft"));
  m_apply->ensurePolished();
  m_apply->setMinimumSize(m_apply->sizeHint());
  m_apply->setEnabled(!m_savePending && m_submitAvailable && m_readOnlyReason.isEmpty() && error.isEmpty());
  m_add->setVisible(controller && m_addAvailable && !property("externalAddAction").toBool());
  m_add->setEnabled(!m_savePending);
  m_receiverSummary->setVisible(!controller);
  m_receiverSummary->setText(
      m_controllerName.isEmpty()
          ? tr("This computer receives keyboard and mouse input. Arrange screens on the computer that provides them; "
               "no second setup is needed here.")
          : tr("Screen arrangement is managed on %1. This computer receives input; no second setup is needed here.")
                .arg(m_controllerName)
  );
  m_details->setVisible(!controller && m_appliedViewAvailable);
  const bool showArrangement = controller || (m_appliedViewAvailable && m_details->isChecked());
  m_arrangement->setVisible(showArrangement);
  m_scroll->setVisible(showArrangement);
  m_status->setVisible(
      controller &&
      (m_editing || !m_saveError.isEmpty() || !m_readOnlyReason.isEmpty() || !m_workspace->conflicts().isEmpty())
  );
  updateInspector();
  m_canvas->update();
  reflowActions();
}
void DesktopWorkspaceView::reflowActions()
{
  for (auto *button : findChildren<QPushButton *>()) {
    button->ensurePolished();
    button->setMinimumSize(0, 0);
    button->setMinimumSize(button->sizeHint());
  }
  for (const auto &name : {"workspaceToolbarLayout", "workspaceFooterLayout"}) {
    auto *row = findChild<QBoxLayout *>(name);
    if (!row)
      continue;
    int needed = 0, count = 0;
    for (int i = 0; i < row->count(); ++i)
      if (auto *widget = row->itemAt(i)->widget(); widget && !widget->isHidden()) {
        needed += widget->minimumWidth();
        ++count;
      }
    needed += qMax(0, count - 1) * row->spacing();
    row->setDirection(needed > qMax(0, width() - 24) ? QBoxLayout::TopToBottom : QBoxLayout::LeftToRight);
  }
}
void DesktopWorkspaceView::resizeEvent(QResizeEvent *event)
{
  QWidget::resizeEvent(event);
  reflowActions();
  auto *splitter = qobject_cast<QSplitter *>(m_arrangement);
  auto *inspector = findChild<QWidget *>("workspaceInspectorPane");
  if (splitter && inspector) {
    const int detailWidth = qBound(240, width() / 3, 360);
    inspector->setMaximumWidth(detailWidth);
    splitter->setSizes({qMax(240, splitter->width() - splitter->handleWidth() - detailWidth), detailWidth});
  }
}
void DesktopWorkspaceView::changeEvent(QEvent *event)
{
  QWidget::changeEvent(event);
  if (event->type() == QEvent::FontChange || event->type() == QEvent::ApplicationFontChange)
    reflowActions();
}
void DesktopWorkspaceView::select(const QString &device, const QString &monitor)
{
  m_selectedDevice = device;
  m_selectedDisplay = monitor;
  const QSignalBlocker block(m_selection);
  const int index = m_selection->findData(monitor.isEmpty() ? device : DesktopWorkspace::key(device, monitor));
  m_selection->setCurrentIndex(index);
  m_canvas->setSelected(index < m_workspace->displays().size() ? index : -1);
  updateInspector();
  Q_EMIT selectionChanged(device, monitor);
}
void DesktopWorkspaceView::updateInspector()
{
  const DeviceSnapshot *device = nullptr;
  for (const auto &d : m_snapshots)
    if (d.id == m_selectedDevice) {
      device = &d;
      break;
    }
  m_identify->setEnabled(m_identifyAvailable && device && device->identifyCapability == Capability::Supported);
  m_identify->setVisible(m_role == Role::Controller && m_identify->isEnabled());
  m_identify->setToolTip(
      m_identify->isEnabled() ? tr("Show the screen numbers on the selected computer.")
                              : tr("Screen identification is not available for this computer.")
  );
  std::optional<WorkspaceDisplay> selected;
  for (const auto &screen : m_workspace->displays())
    if (screen.deviceId == m_selectedDevice && screen.stableId == m_selectedDisplay) {
      selected = screen;
      break;
    }
  if (!selected) {
    m_inspector->setText(
        device ? unknownDescription(*device) : tr("Select a screen to inspect its resolution and size.")
    );
    if (device && !device->reason.isEmpty())
      m_inspector->setText(m_inspector->text() + "\n" + device->reason);
    m_physicalNote->setText(tr("Screen count and dimensions are unknown. No placeholder resolution is used."));
  } else {
    const auto &s = *selected;
    m_inspector->setText(tr("%1 · Screen %2\nResolution: %3 × %4 pixels\nLogical size: %5 × %6 units\nDisplay scale: "
                            "%7\nSystem position: %8, %9 input units")
                             .arg(s.deviceName)
                             .arg(s.number)
                             .arg(s.pixelResolution.width())
                             .arg(s.pixelResolution.height())
                             .arg(s.logicalRect.width())
                             .arg(s.logicalRect.height())
                             .arg(displayScaleText(s))
                             .arg(s.inputRect.x())
                             .arg(s.inputRect.y()));
    m_physicalNote->setText(
        (s.physicalMm ? tr("Physical size: %1 × %2 mm · %3")
                            .arg(s.physicalMm->width())
                            .arg(s.physicalMm->height())
                            .arg(s.calibrated ? tr("user measured") : tr("reported by the display"))
                      : tr("Physical size is unknown; resolution is not a measurement in inches.")) +
        "\n" +
        tr("Measured size is stored separately. It does not silently resize this arrangement or change input routes.")
    );
  }
  if (!m_identify->isEnabled())
    m_inspector->setText(m_inspector->text() + "\n" + tr("Screen identification is not available for this computer."));
  const QSignalBlocker bw(m_widthMm), bh(m_heightMm);
  m_widthMm->setValue(selected && selected->physicalMm ? selected->physicalMm->width() : 0);
  m_heightMm->setValue(selected && selected->physicalMm ? selected->physicalMm->height() : 0);
  const bool edit = m_editing && !m_savePending && m_readOnlyReason.isEmpty() && selected.has_value();
  m_reviewOs->setVisible(
      edit && (m_workspace->conflicts().contains(m_selectedDevice) ||
               m_workspace->state().mode == ArrangementMode::CustomScreenRoutes)
  );
  m_calibrationPanel->setVisible(edit);
  for (auto *widget :
       {static_cast<QWidget *>(m_widthMm), static_cast<QWidget *>(m_heightMm), static_cast<QWidget *>(m_calibrate),
        static_cast<QWidget *>(m_clearCalibration)})
    widget->setEnabled(edit);
  m_clearCalibration->setEnabled(edit && selected->calibrated);
}
} // namespace deskflow::gui
