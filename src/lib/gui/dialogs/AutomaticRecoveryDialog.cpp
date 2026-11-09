#include "gui/widgets/OptionControl.h"
// SPDX-FileCopyrightText: (C) 2026 Deskflow Developers
// SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
#include "AutomaticRecoveryDialog.h"
#include "common/LabExecutionPolicy.h"
#include "gui/ExperienceStyle.h"
#include <QCheckBox>
#include <QComboBox>
#include <QDialogButtonBox>
#include <QLabel>
#include <QPushButton>
#include <QVBoxLayout>
AutomaticRecoveryDialog::AutomaticRecoveryDialog(
    const QString &fp, const deskflow::gui::AutomaticRecoveryPolicy &saved, deskflow::gui::DiscoveryScopeSource source,
    QWidget *parent
)
    : QDialog(parent),
      m_fingerprint(fp),
      m_scopes(source())
{
  deskflow::gui::applyExperienceStyle(this);
  setWindowTitle(tr("Automatic recovery"));
  resize(600, 340);
  setMinimumWidth(500);
  auto *layout = new QVBoxLayout(this);
  layout->setContentsMargins(24, 24, 24, 20);
  layout->setSpacing(14);
  auto *intro = new QLabel(
      tr("After an unexpected disconnect or wake, search only this network for the same verified device. Pause always "
         "cancels recovery."),
      this
  );
  intro->setWordWrap(true);
  layout->addWidget(intro);
  m_enabled = new deskflow::gui::OptionCheckBox(tr("Automatically find this saved device"), this);
  m_enabled->setObjectName("automaticRecoveryEnabled");
  layout->addWidget(m_enabled);
  m_network = new QComboBox(this);
  m_network->setObjectName("automaticRecoveryNetwork");
  m_network->setAccessibleName(tr("Allowed recovery network"));
  m_network->addItem(tr("Choose a private Wi-Fi or Ethernet network"));
  for (const auto &s : m_scopes)
    m_network->addItem(s.name + " · " + s.address.toString());
  layout->addWidget(m_network);
  m_status = new QLabel(this);
  m_status->setWordWrap(true);
  m_status->setObjectName("automaticRecoveryStatus");
  layout->addWidget(m_status);
  auto *credentials = new QPushButton(tr("Choose test credentials…"), this);
  credentials->setEnabled(deskflow::lab::kProbeNetworkExecutionEnabled);
  layout->addWidget(credentials);
  connect(credentials, &QPushButton::clicked, this, &AutomaticRecoveryDialog::preparedSessionRequested);
#ifdef DESKFLOW_CANDIDATE_BUILD
  credentials->hide();
#endif
  auto *buttons = new QDialogButtonBox(QDialogButtonBox::Save | QDialogButtonBox::Cancel, this);
  layout->addWidget(buttons);
  auto refresh = [this, buttons] {
    m_status->setStyleSheet({});
    m_status->setAccessibleDescription({});
    m_network->setEnabled(m_enabled->isChecked());
    buttons->button(QDialogButtonBox::Save)->setEnabled(!m_enabled->isChecked() || m_network->currentIndex() > 0);
    m_status->setText(
        deskflow::lab::kProbeNetworkExecutionEnabled
            ? tr("An address is saved only after the original TLS identity is verified. A different network or "
                 "identity needs your action.")
            : tr("Settings can be reviewed here. This build cannot use real networks, credentials or input control.")
    );
  };
  connect(m_enabled, &QCheckBox::toggled, this, refresh);
  connect(m_network, &QComboBox::currentIndexChanged, this, refresh);
  connect(buttons, &QDialogButtonBox::accepted, this, &AutomaticRecoveryDialog::accept);
  connect(buttons, &QDialogButtonBox::rejected, this, &QDialog::reject);
  if (saved.fingerprint == fp) {
    m_enabled->setChecked(saved.enabled);
    const auto scope = saved.current(m_scopes);
    if (scope)
      m_network->setCurrentIndex(m_scopes.indexOf(*scope) + 1);
  }
  refresh();
}
deskflow::gui::AutomaticRecoveryPolicy AutomaticRecoveryDialog::policy() const
{
  const int i = m_network->currentIndex() - 1;
  return {
      m_enabled->isChecked(), m_fingerprint,
      i >= 0 && i < m_scopes.size() ? m_scopes[i] : deskflow::gui::DiscoveryScope{}
  };
}
void AutomaticRecoveryDialog::showCredentialResult(bool accepted)
{
  m_status->setText(
      accepted ? tr("Prepared test credentials accepted for this session.")
               : tr("Credentials rejected. Saved identity and trust are unchanged.")
  );
}

void AutomaticRecoveryDialog::setReceivingRole(bool receiving)
{
  if (!m_roleHint) {
    m_roleHint = new QLabel(this);
    m_roleHint->setObjectName("recoveryRoleHint");
    m_roleHint->setWordWrap(true);
    qobject_cast<QVBoxLayout *>(layout())->insertWidget(1, m_roleHint);
  }
  m_roleHint->setText(
      receiving ? tr("This receiving computer reconnects to the saved controlling computer. It retries the current "
                     "address automatically; this option also searches when the address changes.")
                : tr("This computer currently supplies input. Address recovery runs on the receiving computer. You may "
                     "retain this preference for a future receiving role; it is inactive in the current role.")
  );
}
void AutomaticRecoveryDialog::accept()
{
  const auto requested = policy();
  const auto error = !requested.valid() ? tr("Choose an available network before enabling address recovery.")
                     : m_apply          ? m_apply(requested)
                                        : QString();
  if (!error.isEmpty()) {
    m_status->setText(error);
    m_status->setAccessibleDescription(error);
    m_status->setStyleSheet(palette().color(QPalette::Window).lightness() < 128 ? "color:#ffb4a9;" : "color:#9b2c24;");
    return;
  }
  QDialog::accept();
}
