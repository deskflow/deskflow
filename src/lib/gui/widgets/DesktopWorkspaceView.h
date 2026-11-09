// SPDX-FileCopyrightText: (C) 2026 Deskflow Developers
// SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
#pragma once
#include "common/DesktopWorkspace.h"
#include "experience/LayoutModel.h"
#include <QWidget>
#include <memory>
class QLabel;
class QPushButton;
class QComboBox;
class QDoubleSpinBox;
class QScrollArea;
class LayoutCanvas;
class QSpacerItem;

namespace deskflow::gui {
// Shared, embeddable main-desktop surface. It does not own Settings, sockets,
// discovery, authentication, native permissions or service mutation handlers.
class DesktopWorkspaceView final : public QWidget
{
  Q_OBJECT
public:
  enum class Role
  {
    Controller,
    Receiver
  };
  explicit DesktopWorkspaceView(QWidget *parent = nullptr);
  // A receiver never owns an editable arrangement. Its optional details are
  // shown only when the host has an authoritative controller snapshot.
  void setRole(Role, const QString &controllerName = {}, bool appliedLayoutAvailable = false);
  void setSnapshot(display::DeviceSnapshots);
  void setDraft(const QByteArray &state, const QByteArray &appliedLayout = {});
  void setReadOnlyReason(const QString &reason);
  void setSubmitAvailable(bool available);
  void setActionsAvailable(bool addComputer, bool identify);
  void setActiveRouting(const QString &description);
  void acknowledgeSaved(const QByteArray &state, const QByteArray &appliedLayout);
  void setAppliedLayout(const QByteArray &layout);
  void rejectSave(const QString &reason);
  const display::DesktopWorkspace &workspace() const
  {
    return *m_workspace;
  }
  LayoutCanvas *canvas() const
  {
    return m_canvas;
  }
  void beginEdit();

protected:
  void resizeEvent(QResizeEvent *) override;
  void changeEvent(QEvent *) override;
Q_SIGNALS:
  void selectionChanged(const QString &deviceId, const QString &displayId);
  void draftChanged(const QByteArray &workspace);
  void applyRequested(const QByteArray &workspace, const QByteArray &routingLayout);
  void saveDraftRequested(const QByteArray &workspace);
  void cancelRequested();
  void addComputerRequested();
  void identifyRequested(const QString &deviceId);

private:
  void rebuild();
  void updateInspector();
  void select(const QString &device, const QString &monitor = {});
  void cancelEdit();
  void reflowActions();
  void refreshRoutingSummary();
  QString unknownDescription(const display::DeviceSnapshot &) const;
  display::DeviceSnapshots m_snapshots;
  QByteArray m_clean, m_applied;
  std::unique_ptr<display::DesktopWorkspace> m_workspace;
  experience::LayoutModel m_geometry;
  LayoutCanvas *m_canvas;
  QComboBox *m_selection, *m_mode;
  QLabel *m_status, *m_help, *m_inspector, *m_physicalNote, *m_activeRouting, *m_receiverSummary;
  QPushButton *m_details;
  QScrollArea *m_scroll;
  QPushButton *m_add, *m_identify, *m_edit, *m_apply, *m_cancel, *m_undo, *m_calibrate, *m_clearCalibration,
      *m_reviewOs;
  QDoubleSpinBox *m_widthMm, *m_heightMm;
  QWidget *m_calibrationPanel;
  QWidget *m_arrangement;
  QSpacerItem *m_receiverSpace;
  QString m_selectedDevice, m_selectedDisplay, m_readOnlyReason, m_saveError, m_routingDescription;
  bool m_savePending = false;
  Role m_role = Role::Controller;
  QString m_controllerName;
  bool m_appliedViewAvailable = false;
  bool m_editing = false, m_submitAvailable = false, m_addAvailable = false, m_identifyAvailable = false;
};
} // namespace deskflow::gui
