// SPDX-FileCopyrightText: (C) 2026 Deskflow Developers
// SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
#include "NearbyDevicesDialog.h"
#include "common/LabExecutionPolicy.h"
#include "gui/ExperienceStyle.h"
#include "gui/core/DeviceRecovery.h"
#include "gui/core/DiscoveryCatalog.h"
#include "gui/core/NearbyDiscovery.h"
#include <QComboBox>
#include <QDialogButtonBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QListWidget>
#include <QPushButton>
#include <QSignalBlocker>
#include <QTimer>
#include <QVBoxLayout>
namespace deskflow::gui {
NearbyDevicesDialog::NearbyDevicesDialog(
    QWidget *parent, DiscoveryScopeSource source, IdentityBindings *bindings, const QString &expected
)
    : QDialog(parent),
      m_discovery(new NearbyDiscovery(this)),
      m_devices(new QListWidget(this))
{
  if (bindings && bindings->devices().contains(IdentityBindings::normalizedFingerprint(expected)))
    m_recovery = new DeviceRecovery(*bindings, *m_discovery, this);
  deskflow::gui::applyExperienceStyle(this);
  setWindowTitle(tr("Nearby computers"));
  resize(620, 460);
  setMinimumWidth(540);
  auto *layout = new QVBoxLayout(this);
  layout->setContentsMargins(24, 24, 24, 20);
  layout->setSpacing(14);
  auto *title = new QLabel(tr("Nearby computers"), this);
  title->setObjectName("nearbyHeading");
  auto titleFont = title->font();
  titleFont.setPointSizeF(titleFont.pointSizeF() + 6);
  titleFont.setBold(true);
  title->setFont(titleFont);
  layout->addWidget(title);
  auto *help = new QLabel(
      tr("Find the computer whose keyboard and mouse you want to use. Its identity is checked before a connection is "
         "allowed."),
      this
  );
  if (m_recovery)
    help->setText(
        tr("Find the saved device on this network. Its address will change only after TLS verifies the same complete "
           "identity.")
    );
  help->setWordWrap(true);
  help->setTextFormat(Qt::PlainText);
  layout->addWidget(help);
  auto *network = new QComboBox(this);
  network->setObjectName("nearbyNetwork");
  network->setAccessibleName(tr("Local network for discovery"));
  auto *networkLabel = new QLabel(tr("Local network for discovery"), this);
  networkLabel->setObjectName("nearbyNetworkLabel");
  networkLabel->setBuddy(network);
  layout->addWidget(networkLabel);
  auto scopes = std::make_shared<QVector<DiscoveryScope>>();
  auto reload = [this, network, scopes, source] {
    m_discovery->stop();
    m_devices->clear();
    const QSignalBlocker blocker(network);
    *scopes = source();
    network->clear();
    network->addItem(tr("Choose a private Wi-Fi or Ethernet network"));
    for (const auto &scope : *scopes)
      network->addItem(tr("%1 · %2").arg(scope.name, scope.address.toString()));
  };
  reload();
  auto *networkRow = new QHBoxLayout;
  auto *reloadButton = new QPushButton(tr("Refresh networks"), this);
  reloadButton->setObjectName("refreshNetworks");
  networkRow->addWidget(network, 1);
  networkRow->addWidget(reloadButton);
  layout->addLayout(networkRow);
  m_devices->setObjectName("nearbyDevices");
  m_devices->setAccessibleName(tr("Discovered computers"));
  layout->addWidget(m_devices, 1);
  auto *empty = new QLabel(
      tr("No computers listed yet. Choose a network and search, or enter the other computer’s address."), this
  );
  empty->setObjectName("nearbyEmpty");
  empty->setWordWrap(true);
  empty->setAlignment(Qt::AlignCenter);
  empty->setContentsMargins(24, 24, 24, 24);
  layout->addWidget(empty, 1);
  const auto updateEmpty = [this, empty] {
    const bool hasDevices = m_devices->count() > 0;
    m_devices->setVisible(hasDevices);
    empty->setVisible(!hasDevices);
  };
  connect(m_devices->model(), &QAbstractItemModel::rowsInserted, this, updateEmpty);
  connect(m_devices->model(), &QAbstractItemModel::rowsRemoved, this, updateEmpty);
  connect(m_devices->model(), &QAbstractItemModel::modelReset, this, updateEmpty);
  updateEmpty();
  auto *statusLabel = new QLabel(this);
  statusLabel->setObjectName("nearbyStatus");
  statusLabel->setTextFormat(Qt::PlainText);
  statusLabel->setWordWrap(true);
  layout->addWidget(statusLabel);
  auto *buttons = new QDialogButtonBox(QDialogButtonBox::Cancel, this);
  auto *select = buttons->addButton(tr("Use address"), QDialogButtonBox::AcceptRole);
  select->setEnabled(false);
  select->setObjectName("selectNearbyAddress");
  if (m_recovery)
    select->hide();
  auto *refresh = buttons->addButton(
      m_recovery ? tr("Search for 2 minutes") : tr("Search for 5 minutes"), QDialogButtonBox::ActionRole
  );
  refresh->setObjectName("refreshNearby");
  layout->addWidget(buttons);
  if (!m_recovery) {
    auto *manual = buttons->addButton(tr("Enter address…"), QDialogButtonBox::ActionRole);
    manual->setObjectName("nearbyManualEntry");
    connect(manual, &QPushButton::clicked, this, [this] {
      reject();
      Q_EMIT manualEntryRequested();
    });
  }
  if (m_recovery) {
    auto *credentials = buttons->addButton(tr("Choose test credentials…"), QDialogButtonBox::ActionRole);
    credentials->setObjectName("chooseProbeCredentials");
    credentials->setEnabled(deskflow::lab::kProbeNetworkExecutionEnabled);
    credentials->setToolTip(tr(
        "Only explicitly prepared isolated test credentials are accepted. Production identity files are never imported."
    ));
    connect(credentials, &QPushButton::clicked, this, &NearbyDevicesDialog::preparedSessionRequested);
#ifdef DESKFLOW_CANDIDATE_BUILD
    credentials->hide();
#endif
  }
  refresh->setEnabled(false);
  connect(network, &QComboBox::currentIndexChanged, this, [this, refresh](int index) {
    if (m_recovery)
      m_recovery->cancel();
    m_discovery->stop();
    m_devices->clear();
    refresh->setEnabled(index > 0);
  });
  connect(refresh, &QPushButton::clicked, this, [this, network, scopes, expected] {
    const auto index = network->currentIndex() - 1;
    if (index >= 0 && index < scopes->size()) {
      if (m_recovery && !m_recovery->begin(expected, (*scopes)[index]))
        return;
      m_discovery->start((*scopes)[index]);
    }
  });
  connect(reloadButton, &QPushButton::clicked, this, [reload, refresh] {
    reload();
    refresh->setEnabled(false);
  });
  connect(this, &QDialog::finished, m_discovery, &NearbyDiscovery::stop);
  if (m_recovery) {
    connect(this, &QDialog::finished, m_recovery, &DeviceRecovery::cancel);
    connect(m_recovery, &DeviceRecovery::recovered, this, [this](const QString &fp, const QString &host, int port) {
      accept();
      Q_EMIT verifiedEndpoint(fp, host, port);
    });
    connect(m_recovery, &DeviceRecovery::stateChanged, this, [this, statusLabel](DeviceRecovery::State state) {
      using S = DeviceRecovery::State;
      if (state == S::Verifying)
        statusLabel->setText(tr("Checking the saved identity. The address has not been saved."));
      else if (state == S::Backoff)
        statusLabel->setText(tr("Candidate rejected. Waiting before checking another address."));
      else if (state == S::NeedsAction) {
        m_discovery->stop();
        statusLabel->setText(tr("Verification needs attention. The saved address and trust are unchanged."));
      }
    });
  }
  connect(buttons, &QDialogButtonBox::rejected, this, &QDialog::reject);
  connect(buttons, &QDialogButtonBox::accepted, this, [this] {
    if (NearbyDiscovery::validEndpoint(selectedHost(), selectedPort()))
      accept();
  });
  connect(m_devices, &QListWidget::currentRowChanged, this, [this, select] {
    select->setEnabled(NearbyDiscovery::validEndpoint(selectedHost(), selectedPort()));
  });
  connect(m_discovery, &NearbyDiscovery::status, statusLabel, &QLabel::setText);
  connect(m_discovery, &NearbyDiscovery::searchingChanged, this, &NearbyDevicesDialog::searchingChanged);
  connect(m_discovery, &NearbyDiscovery::removed, this, [this](const QString &key) {
    const bool lostSelection =
        m_devices->currentItem() && m_devices->currentItem()->data(Qt::UserRole).toString() == key;
    for (int i = m_devices->count() - 1; i >= 0; --i)
      if (m_devices->item(i)->data(Qt::UserRole).toString() == key)
        delete m_devices->takeItem(i);
    if (lostSelection)
      m_devices->setCurrentRow(-1);
  });
  connect(
      m_discovery, &NearbyDiscovery::found, this,
      [this](const QString &key, const QString &name, const QString &host, int port) {
        if (!NearbyDiscovery::validEndpoint(host, port) || !DiscoveryCatalog::validName(name))
          return;
        QListWidgetItem *item = nullptr;
        for (int i = 0; i < m_devices->count(); ++i)
          if (m_devices->item(i)->data(Qt::UserRole).toString() == key)
            item = m_devices->item(i);
        if (!item) {
          if (m_devices->count() >= 64)
            return;
          item = new QListWidgetItem(m_devices);
        }
        if (item == m_devices->currentItem() &&
            (item->data(Qt::UserRole + 1).toString() != host || item->data(Qt::UserRole + 2).toInt() != port))
          m_devices->setCurrentRow(-1); // Address replacement needs a fresh explicit selection.
        item->setText(tr("%1\n%2 · port %3 · identity unverified").arg(name.left(128), host).arg(port));
        item->setData(Qt::UserRole, key);
        item->setData(Qt::UserRole + 1, host);
        item->setData(Qt::UserRole + 2, port);
      }
  );
  statusLabel->setText(tr("Choose a network, then search. VPN and public interfaces are excluded."));
}
QString NearbyDevicesDialog::selectedHost() const
{
  auto *item = m_devices->currentItem();
  return item ? item->data(Qt::UserRole + 1).toString() : QString();
}
int NearbyDevicesDialog::selectedPort() const
{
  auto *item = m_devices->currentItem();
  return item ? item->data(Qt::UserRole + 2).toInt() : 0;
}
} // namespace deskflow::gui
