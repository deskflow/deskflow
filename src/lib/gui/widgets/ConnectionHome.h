// SPDX-FileCopyrightText: (C) 2026 Deskflow Developers
// SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
#pragma once
#include <QWidget>

class QLabel;
class QLineEdit;
class QPushButton;
class QProgressBar;
class QFrame;

namespace deskflow::gui {
// Presentation only. This widget has no settings, transport, permission or
// credential dependencies. User intent is handled by the existing controller.
class ConnectionHome : public QWidget
{
  Q_OBJECT
public:
  enum class Role
  {
    None,
    Share,
    Receive
  };
  Q_ENUM(Role)
  enum class Stage
  {
    Idle,
    Starting,
    Connecting,
    Verifying,
    Connected,
    Listening,
    Retrying,
    Recovering,
    Stopping,
    NeedsAction,
    Paused,
    Discovering,
    Failed
  };
  Q_ENUM(Stage)
  struct State
  {
    Role role = Role::None;
    Stage stage = Stage::Idle;
    QString localName, peerName, explanation, recoverySummary;
    int retrySeconds = -1;
    bool canRetry = false;
    bool hasAddress = false;
    bool identityReady = false;
    bool canStart = false;
    bool hasSavedDevices = false;
    bool peerKnown = false;
  };
  explicit ConnectionHome(QWidget *parent = nullptr);
  void setState(const State &state);
  void setDesktopWidget(QWidget *widget);
  State state() const
  {
    return m_state;
  }
  void showManualEntry();
  void showRoleSelection();
Q_SIGNALS:
  void roleChosen(deskflow::gui::ConnectionHome::Role role);
  void addressChosen(const QString &address);
  void findDeviceRequested();
  void addComputerRequested();
  void retryRequested();
  void startStopRequested();
  void settingsRequested();
  void identitySetupRequested();
  void savedDevicesRequested();
  void connectionOptionsRequested();
  void diagnosticsRequested();
  void layoutRequested();

protected:
  void changeEvent(QEvent *event) override;
  void resizeEvent(QResizeEvent *event) override;
  bool eventFilter(QObject *watched, QEvent *event) override;

private:
  void render();
  void applyLocalStyle();
  void activatePrimary();
  void acceptAddress();
  void back();
  State m_state;
  bool m_editingAddress = false, m_selectingRole = false, m_actionPending = false;
  QWidget *m_sidebar, *m_contentHost, *m_choices, *m_relationship, *m_addressForm, *m_localGlyph, *m_peerGlyph;
  QWidget *m_actionCard, *m_footerNavigation, *m_sidebarDot, *m_statusDot;
  QLabel *m_eyebrow, *m_title, *m_intro, *m_localName, *m_peerName, *m_localCaption, *m_peerCaption;
  QLabel *m_arrow, *m_status, *m_reason, *m_error, *m_addressLabel, *m_recovery, *m_linkMeta;
  QLabel *m_sidebarDevice, *m_sidebarState, *m_sidebarPeer, *m_sectionLink, *m_sectionControls;
  QLabel *m_directionCaption, *m_directionSummary, *m_footerNote;
  QProgressBar *m_progress;
  QPushButton *m_find;
  QWidget *m_session;
  QPushButton *m_share, *m_receive, *m_back, *m_primary, *m_manual, *m_direction;
  QPushButton *m_saved, *m_options, *m_diagnostics, *m_settings, *m_layout;
  QPushButton *m_sidebarOverview, *m_sidebarComputers, *m_sidebarConnection;
  QPushButton *m_sidebarSettings, *m_sidebarDiagnostics;
  QLabel *m_shareTitle, *m_shareDetail, *m_receiveTitle, *m_receiveDetail;
  QLineEdit *m_address;
  QWidget *m_canvas = nullptr, *m_column = nullptr, *m_viewport = nullptr;
  QWidget *m_desktop = nullptr;
  bool m_desktopShown = false;
};
} // namespace deskflow::gui
Q_DECLARE_METATYPE(deskflow::gui::ConnectionHome::Role)
