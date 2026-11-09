// SPDX-FileCopyrightText: (C) 2026 Deskflow Developers
// SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception

#include "CandidatePermissionDialog.h"
#include "gui/ExperienceStyle.h"

#include <QGuiApplication>
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QVBoxLayout>

namespace deskflow::gui {
namespace {
#if (defined(Q_OS_MACOS) || defined(DESKFLOW_PERMISSION_GUIDE_TEST)) &&                                                \
    !defined(DESKFLOW_PERMISSION_GUIDE_UNSUPPORTED_TEST)
constexpr bool permissionGuideAvailable = true;
#else
constexpr bool permissionGuideAvailable = false;
#endif
} // namespace

CandidatePermissionDialog::CandidatePermissionDialog(
    AccessibilityPermissionProvider &provider, int macOSMajorVersion, QWidget *parent
)
    : QDialog(parent),
      m_provider(provider)
{
  applyExperienceStyle(this);
  setWindowTitle(tr("Keyboard and mouse permission"));
  setMinimumWidth(480);
  auto *layout = new QVBoxLayout(this);
  layout->setContentsMargins(24, 24, 24, 24);
  layout->setSpacing(16);

  auto *overview = new QLabel(
      tr("First setup: allow keyboard and mouse access here, then choose which computer supplies the keyboard and "
         "mouse, prepare this computer's identity and pair with your other computer."),
      this
  );
  overview->setObjectName("permissionSetupOverview");
  overview->setWordWrap(true);
  layout->addWidget(overview);

  const auto permissionName = macOSMajorVersion >= 27 ? tr("Device Control and Data Access") : tr("Accessibility");
  auto *instructions =
      new QLabel(tr("Check DeskflowCandidate in System Settings > Privacy & Security > %1.").arg(permissionName), this);
  instructions->setObjectName("permissionInstructions");
  instructions->setWordWrap(true);
  layout->addWidget(instructions);

  m_status = new QLabel(this);
  m_status->setObjectName("permissionStatus");
  m_status->setWordWrap(true);
  layout->addWidget(m_status);

  auto *actions = new QHBoxLayout;
  m_requestButton = new QPushButton(tr("Request system permission"), this);
  m_requestButton->setObjectName("requestPermission");
  auto *checkButton = new QPushButton(tr("Check again"), this);
  checkButton->setObjectName("checkPermission");
  actions->addWidget(m_requestButton);
  actions->addWidget(checkButton);
  layout->addLayout(actions);

  m_continueButton = new QPushButton(tr("Continue with sharing paused"), this);
  m_continueButton->setObjectName("continuePaused");
  auto *quitButton = new QPushButton(tr("Quit"), this);
  quitButton->setObjectName("quitPermissionGuide");
  auto *navigation = new QHBoxLayout;
  navigation->addWidget(quitButton);
  navigation->addStretch();
  navigation->addWidget(m_continueButton);
  layout->addLayout(navigation);
  if (!permissionGuideAvailable) {
    if (auto *setup = findChild<QLabel *>("permissionSetupOverview"))
      setup->hide();
    for (QWidget *widget : QList<QWidget *>{instructions, m_requestButton, checkButton, m_continueButton})
      widget->hide();
  }

  // Enter must not accidentally request persistent access or continue startup.
  for (auto *button : {m_requestButton, checkButton, m_continueButton, quitButton}) {
    button->setAutoDefault(false);
    button->setDefault(false);
    button->ensurePolished();
    // Keep the text plus padding and the thicker focused border visible even
    // when the user resizes the guide to its minimum height.
    button->setMinimumSize(button->sizeHint().expandedTo(
        QSize(button->fontMetrics().horizontalAdvance(button->text()) + 32, button->fontMetrics().height() + 20)
    ));
  }
  connect(m_requestButton, &QPushButton::clicked, this, &CandidatePermissionDialog::requestPermission);
  connect(checkButton, &QPushButton::clicked, this, [this] { refreshPermission(true); });
  connect(m_continueButton, &QPushButton::clicked, this, &QDialog::accept);
  connect(quitButton, &QPushButton::clicked, this, &QDialog::reject);
  connect(qGuiApp, &QGuiApplication::applicationStateChanged, this, [this](Qt::ApplicationState state) {
    if (state == Qt::ApplicationActive)
      refreshPermission(true);
  });
  refreshPermission(false);
}

void CandidatePermissionDialog::refreshPermission(bool releasePendingRequest)
{
  if (m_finished)
    return;
  if (!permissionGuideAvailable) {
    m_status->setText(tr("This macOS permission guide is not available on this platform."));
    m_requestButton->setEnabled(false);
    m_continueButton->setEnabled(false);
    return;
  }
  if (releasePendingRequest)
    m_requestPending = false;
  const bool granted = m_provider.isGranted();
  m_requestButton->setEnabled(!granted && !m_requestPending);
  m_continueButton->setEnabled(granted);
  if (granted)
    m_status->setText(tr("Permission granted. You can continue with sharing paused."));
  else if (m_requestPending)
    m_status->setText(tr("If a system prompt appears, complete it, then return here or click Check again."));
  else
    m_status->setText(
        tr("macOS does not currently trust this running instance. If the setting is already on, an app identity "
           "mismatch or "
           "a system check issue may need investigation; this result does not mean you have not allowed the app. "
           "You can check again without requesting permission.")
    );
  // A successful passive check never accepts the dialog or starts the app.
  if (auto *contents = layout()) {
    contents->activate();
    setMinimumHeight(qMax(contents->minimumSize().height(), contents->totalHeightForWidth(minimumWidth())));
  }
}

void CandidatePermissionDialog::requestPermission()
{
  if (!permissionGuideAvailable || m_finished || m_requestPending)
    return;
  if (m_provider.isGranted()) {
    refreshPermission(false);
    return;
  }
  m_requestPending = true;
  m_requestButton->setEnabled(false);
  m_provider.request(); // The only native request path, reached by an explicit button activation.
  refreshPermission(false);
}

void CandidatePermissionDialog::done(int result)
{
  if (m_finished)
    return;
  // Permission can be revoked between displaying the button and clicking it.
  if (result == QDialog::Accepted && (!permissionGuideAvailable || !m_provider.isGranted())) {
    refreshPermission(true);
    return;
  }
  m_finished = true;
  QDialog::done(result);
}

} // namespace deskflow::gui
