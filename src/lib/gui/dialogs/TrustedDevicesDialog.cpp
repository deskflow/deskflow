// SPDX-FileCopyrightText: (C) 2026 Deskflow Developers
// SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
#include "TrustedDevicesDialog.h"
#include "gui/ExperienceStyle.h"
#include <QDialogButtonBox>
#include <QLabel>
#include <QLineEdit>
#include <QListWidget>
#include <QPushButton>
#include <QVBoxLayout>

TrustedDevicesDialog::TrustedDevicesDialog(
    deskflow::gui::IdentityBindings &bindings, bool allowed, QWidget *parent, const QString &unavailableReason
)
    : QDialog(parent)
{
  deskflow::gui::applyExperienceStyle(this);
  setWindowTitle(tr("Saved devices"));
  resize(640, 520);
  setMinimumSize(540, 360);
  auto *layout = new QVBoxLayout(this);
  layout->setContentsMargins(24, 24, 24, 20);
  layout->setSpacing(12);
  auto *title = new QLabel(tr("Saved devices"), this);
  auto titleFont = title->font();
  titleFont.setPointSizeF(titleFont.pointSizeF() + 6);
  titleFont.setBold(true);
  title->setFont(titleFont);
  layout->addWidget(title);
  auto *intro = new QLabel(
      tr("Connections must pass TLS and match the saved full fingerprint, even when an address changes."), this
  );
  intro->setWordWrap(true);
  intro->setMinimumHeight(intro->heightForWidth(minimumWidth() - 48));
  layout->addWidget(intro);
  auto *list = new QListWidget(this);
  list->setObjectName("trustedDeviceList");
  layout->addWidget(list);
  auto *nameLabel = new QLabel(tr("Device display name"), this);
  layout->addWidget(nameLabel);
  auto *name = new QLineEdit(this);
  name->setObjectName("trustedDeviceName");
  name->setAccessibleName(tr("Device display name"));
  layout->addWidget(name);
  nameLabel->setBuddy(name);
  auto *detail = new QLabel(this);
  detail->setWordWrap(true);
  detail->setTextFormat(Qt::PlainText);
  detail->setTextInteractionFlags(Qt::TextSelectableByMouse | Qt::TextSelectableByKeyboard);
  layout->addWidget(detail);
  auto *hint = new QLabel(
      allowed ? tr("Rename and remove affect saved shortcuts only. Removing a shortcut does not remove its TLS trust.")
              : (unavailableReason.isEmpty() ? tr("Reconnect is unavailable while sharing or connected. Saved names "
                                                  "and recovery preferences remain available.")
                                             : unavailableReason),
      this
  );
  hint->setObjectName("trustedConnectionHint");
  hint->setWordWrap(true);
  hint->setMinimumHeight(hint->heightForWidth(minimumWidth() - 48));
  layout->addWidget(hint);
  auto *buttons = new QDialogButtonBox(QDialogButtonBox::Close, this);
  auto *locate = new QPushButton(tr("Find a new address for this device…"), this);
  locate->setObjectName("locateTrustedDevice");
  layout->addWidget(locate);
  auto *automatic = new QPushButton(tr("Automatic recovery on this network…"), this);
  automatic->setObjectName("configureAutomaticRecovery");
  layout->addWidget(automatic);
  auto *rename = buttons->addButton(tr("Rename"), QDialogButtonBox::ActionRole);
  rename->setObjectName("renameTrustedDevice");
  auto *forget = buttons->addButton(tr("Remove shortcut"), QDialogButtonBox::ActionRole);
  forget->setObjectName("removeTrustedDevice");
  auto *connectButton = buttons->addButton(tr("Connect"), QDialogButtonBox::ActionRole);
  connectButton->setObjectName("connectTrustedDevice");
  layout->addWidget(buttons);
  auto refresh = [list, &bindings] {
    list->clear();
    list->setFixedHeight(qBound(80, 64 * int(bindings.devices().size()) + 14, 206));
    for (const auto &device : bindings.devices()) {
      auto *item = new QListWidgetItem(
          device.label + "\n" + device.endpoint.host + ":" + QString::number(device.endpoint.port), list
      );
      item->setData(Qt::UserRole, device.sha256);
      item->setToolTip(item->text());
    }
  };
  auto selected = [list] {
    return list->currentItem() ? list->currentItem()->data(Qt::UserRole).toString() : QString();
  };
  auto update = [&, this, name, detail, rename, forget, connectButton, locate, automatic, selected, allowed] {
    const auto fp = selected();
    const bool exists = bindings.devices().contains(fp);
    name->setEnabled(exists);
    forget->setEnabled(exists);
    rename->setEnabled(exists && deskflow::gui::IdentityBindings::validLabel(name->text()));
    connectButton->setEnabled(allowed && exists);
    locate->setEnabled(allowed && exists);
    automatic->setEnabled(exists);
    QStringList groups;
    for (int i = 0; i < fp.size(); i += 16)
      groups << fp.mid(i, 16);
    detail->setText(
        exists
            ? tr("Full SHA-256 identity: %1").arg(groups.join(' '))
            : tr("No saved device selected. A successful verified connection is required before a shortcut is saved.")
    );
    detail->setMinimumHeight(detail->heightForWidth(minimumWidth() - 48));
  };
  connect(list, &QListWidget::currentRowChanged, this, [&bindings, name, selected, update] {
    name->setText(bindings.devices().value(selected()).label);
    update();
  });
  connect(name, &QLineEdit::textChanged, this, [update] { update(); });
  connect(rename, &QPushButton::clicked, this, [this, &bindings, name, selected, refresh] {
    if (bindings.rename(selected(), name->text())) {
      refresh();
      Q_EMIT metadataChanged();
    }
  });
  connect(forget, &QPushButton::clicked, this, [this, &bindings, selected, refresh] {
    if (bindings.forget(selected())) {
      refresh();
      Q_EMIT metadataChanged();
    }
  });
  connect(connectButton, &QPushButton::clicked, this, [this, &bindings, selected, allowed] {
    if (!allowed || !bindings.devices().contains(selected()))
      return;
    const auto device = bindings.devices()[selected()];
    Q_EMIT connectRequested(device.sha256, device.endpoint.host, device.endpoint.port);
    accept();
  });
  connect(locate, &QPushButton::clicked, this, [this, selected, allowed] {
    const auto fp = selected();
    if (allowed && !fp.isEmpty()) {
      accept();
      Q_EMIT locateRequested(fp);
    }
  });
  connect(automatic, &QPushButton::clicked, this, [this, selected] {
    const auto fp = selected();
    if (!fp.isEmpty())
      Q_EMIT automaticRecoveryRequested(fp);
  });
  connect(buttons, &QDialogButtonBox::rejected, this, &QDialog::reject);
  refresh();
  // Preselect an unambiguous shortcut without connecting or confirming trust.
  if (list->count() == 1)
    list->setCurrentRow(0);
  update();
  if (bindings.devices().isEmpty()) {
    intro->setText(tr("Your computers will appear here after the first verified connection."));
    list->hide();
    nameLabel->hide();
    name->hide();
    detail->hide();
    hint->hide();
    locate->hide();
    automatic->hide();
    rename->hide();
    forget->hide();
    connectButton->hide();
    setMinimumSize(480, 220);
    resize(520, 240);
  }
}
