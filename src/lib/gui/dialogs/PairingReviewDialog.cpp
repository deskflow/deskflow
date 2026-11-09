#include "gui/widgets/OptionControl.h"
// SPDX-FileCopyrightText: (C) 2026 Deskflow Developers
// SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
#include "PairingReviewDialog.h"
#include "gui/ExperienceStyle.h"
#include <QCheckBox>
#include <QDialogButtonBox>
#include <QFontDatabase>
#include <QLabel>
#include <QLineEdit>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QTimer>
#include <QVBoxLayout>

using deskflow::gui::IdentityBindings;
PairingReviewDialog::PairingReviewDialog(IdentityBindings &bindings, const QString &localFingerprint, QWidget *parent)
    : QDialog(parent),
      m_bindings(bindings),
      m_generation(bindings.attempt().generation)
{
  deskflow::gui::applyExperienceStyle(this);
  setWindowTitle(tr("Review device identity"));
  resize(640, 480);
  setMinimumSize(540, 440);
  auto *layout = new QVBoxLayout(this);
  layout->setContentsMargins(24, 24, 24, 20);
  layout->setSpacing(12);
  const bool changed = bindings.attempt().review == IdentityBindings::Review::ChangedIdentity;
  auto label = [&](const QString &text) {
    auto *l = new QLabel(text, this);
    l->setTextFormat(Qt::PlainText);
    l->setWordWrap(true);
    layout->addWidget(l);
    return l;
  };
  auto *title = label(changed ? tr("This device's identity changed") : tr("Compare both computers"));
  auto font = title->font();
  font.setPointSizeF(font.pointSizeF() + 5);
  font.setBold(true);
  title->setFont(font);
  title->setMinimumHeight(title->fontMetrics().height() + 4);
  label(
      changed ? tr("The address now presents a different certificate. The saved device and address will remain "
                   "unchanged. Recheck the other computer before starting a separate pairing.")
              : tr("Compare the complete SHA-256 fingerprints on both computers once. No serial number needs to be "
                   "entered. Future connections check the saved identity automatically.")
  );
  auto *hashes = new QPlainTextEdit(this);
  hashes->setObjectName("pairingFingerprints");
  hashes->setReadOnly(true);
  hashes->setFont(QFontDatabase::systemFont(QFontDatabase::FixedFont));
  hashes->setLineWrapMode(QPlainTextEdit::NoWrap);
  const auto format = [](QString fp) {
    QString out;
    for (int i = 0; i < fp.size(); i += 2) {
      if (i)
        out += (i % 16 == 0 ? '\n' : ':');
      out += fp.mid(i, 2);
    }
    return out;
  };
  hashes->setPlainText(
      (changed ? tr("Saved identity") : tr("This computer")) + "\n" +
      format(changed ? bindings.attempt().expected : IdentityBindings::normalizedFingerprint(localFingerprint)) +
      "\n\n" + (changed ? tr("Received identity") : tr("Other computer")) + "\n" + format(bindings.attempt().observed)
  );
  hashes->setAccessibleName(tr("Complete SHA-256 fingerprints"));
  hashes->setFixedHeight(hashes->fontMetrics().lineSpacing() * 11 + 18);
  layout->addWidget(hashes);
  auto *nameLabel = label(tr("Nickname (optional to change)"));
  m_name = new QLineEdit(this);
  m_name->setObjectName("pairingDisplayName");
  m_name->setPlaceholderText(tr("Device display name"));
  // This is an editable nickname, not identity evidence. The selected endpoint
  // is already available; don't make users type it again to complete a review.
  const auto suggestedName = bindings.attempt().endpoint.host.left(80);
  m_name->setText(IdentityBindings::validLabel(suggestedName) ? suggestedName : tr("Other computer"));
  m_name->setAccessibleName(tr("Device display name"));
  layout->addWidget(m_name);
  m_compared = new deskflow::gui::OptionCheckBox(tr("I checked all four lines for each device on both screens"), this);
  m_compared->setObjectName("pairingCompared");
  layout->addWidget(m_compared);
  m_error = label({});
  m_error->setObjectName("pairingError");
  m_error->hide();
  auto *buttons = new QDialogButtonBox(QDialogButtonBox::Cancel, this);
  m_confirm = buttons->addButton(tr("Confirm comparison"), QDialogButtonBox::AcceptRole);
  m_confirm->setObjectName("pairingConfirm");
  m_confirm->setAutoDefault(false);
  m_confirm->setEnabled(false);
  buttons->button(QDialogButtonBox::Cancel)->setDefault(true);
  layout->addWidget(buttons);
  const bool eligible = bindings.attempt().review == IdentityBindings::Review::VerifyNew &&
                        !IdentityBindings::normalizedFingerprint(localFingerprint).isEmpty();
  m_name->setEnabled(eligible);
  m_compared->setEnabled(eligible);
  if (changed) {
    nameLabel->hide();
    m_name->hide();
    m_compared->hide();
    m_confirm->hide();
  }
  auto update = [this, eligible] {
    const bool valid = IdentityBindings::validLabel(m_name->text());
    m_confirm->setEnabled(eligible && !m_deadline.hasExpired() && m_compared->isChecked() && valid);
    m_error->setText(
        !m_name->text().isEmpty() && !valid
            ? tr("Use 1–80 visible characters without line breaks or hidden direction marks.")
            : QString()
    );
    m_error->setVisible(!m_error->text().isEmpty());
  };
  connect(m_compared, &QCheckBox::toggled, this, update);
  connect(m_name, &QLineEdit::textChanged, this, update);
  connect(buttons, &QDialogButtonBox::accepted, this, &PairingReviewDialog::accept);
  connect(buttons, &QDialogButtonBox::rejected, this, &PairingReviewDialog::reject);
  QTimer::singleShot(120000, Qt::PreciseTimer, this, &PairingReviewDialog::expireReview);
  label(
      changed ? tr("The saved identity is retained. Closing this window does not trust the new certificate.")
              : tr("Confirmation alone does not establish a connection. TLS must verify this same identity before its "
                   "address can be saved.")
  );
  // SetMinimumSize replaces an explicit width with the layout's narrow size
  // hint. Keep a readable comparison width and size wrapping labels at it.
  layout->activate();
  setMinimumSize(540, qMax(440, layout->totalHeightForWidth(540)));
  resize(640, qMax(minimumHeight(), layout->totalHeightForWidth(640)));
}
void PairingReviewDialog::accept()
{
  if (m_deadline.hasExpired() || !m_confirm->isEnabled() ||
      !m_bindings.confirm(m_generation, m_name->text(), m_compared->isChecked())) {
    m_error->setText(tr("The identity review expired or the name is invalid. Start a new comparison."));
    m_error->show();
    return;
  }
  QDialog::accept();
}
void PairingReviewDialog::reject()
{
  m_bindings.cancel();
  QDialog::reject();
}
QString PairingReviewDialog::displayName() const
{
  return m_name->text();
}
void PairingReviewDialog::expireReview()
{
  m_deadline = QDeadlineTimer();
  m_bindings.cancel();
  m_confirm->setEnabled(false);
  m_compared->setEnabled(false);
  m_name->setEnabled(false);
  m_error->setText(tr("The comparison expired. Close this window and start again."));
  m_error->show();
}
