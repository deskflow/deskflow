// SPDX-FileCopyrightText: (C) 2026 Deskflow Developers
// SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
#pragma once
#include "ExperienceStyle.h"
#include <QBoxLayout>
#include <QDialog>
#include <QEvent>
#include <QGroupBox>
#include <QPushButton>
#include <QScrollArea>
#include <QStyleOptionButton>
#include <QTimer>
#include <QtMath>

namespace deskflow::gui {
class FormButtonMetrics final : public QObject
{
  QPushButton *m_button;
  void refresh()
  {
    m_button->ensurePolished();
    QStyleOptionButton option;
    option.initFrom(m_button);
    option.text = m_button->text();
    if (m_button->autoDefault())
      option.features |= QStyleOptionButton::AutoDefaultButton;
    if (m_button->isDefault())
      option.features |= QStyleOptionButton::DefaultButton;
    auto needed = QFontMetricsF(m_button->font(), m_button).size(Qt::TextShowMnemonic, m_button->text()).toSize();
    if (!m_button->icon().isNull()) {
      needed.rwidth() += m_button->iconSize().width() + 4;
      needed.setHeight(qMax(needed.height(), m_button->iconSize().height()));
    }
    const auto measured =
        m_button->style()->sizeFromContents(QStyle::CT_PushButton, &option, needed, m_button) + QSize(4, 4);
    m_button->setMinimumSize(measured);
  }

public:
  explicit FormButtonMetrics(QPushButton *button) : QObject(button), m_button(button)
  {
    button->setProperty("formButtonMetrics", true);
    button->installEventFilter(this);
    refresh();
  }
  bool eventFilter(QObject *, QEvent *event) override
  {
    if (event->type() == QEvent::Show || event->type() == QEvent::FontChange || event->type() == QEvent::StyleChange ||
        event->type() == QEvent::LanguageChange)
      QTimer::singleShot(0, this, [this] { refresh(); });
    return false;
  }
};
// Existing fields retain their validation/save paths. Only the content scrolls;
// the dialog's original action row stays reachable at every window height.
inline void prepareFormDialog(QDialog *dialog, QWidget *footer, const QSize &initial = {560, 480})
{
  applyExperienceStyle(dialog);
  auto *root = qobject_cast<QVBoxLayout *>(dialog->layout());
  if (!root)
    return;
  auto *body = new QWidget(dialog);
  body->setObjectName("formContents");
  auto *contents = new QVBoxLayout(body);
  contents->setContentsMargins(0, 0, 10, 0);
  contents->setSpacing(16);
  while (auto *item = root->takeAt(0)) {
    if (item->widget() == footer) {
      delete item;
      continue;
    }
    if (auto *nested = item->layout()) {
      nested->setParent(nullptr);
      contents->addLayout(nested);
    } else if (auto *widget = item->widget()) {
      contents->addWidget(widget);
      delete item;
    } else
      contents->addItem(item);
  }
  contents->addStretch();
  for (auto *group : dialog->findChildren<QGroupBox *>()) {
    if (auto *layout = group->layout()) {
      layout->setContentsMargins(14, 20, 14, 14);
      layout->setSpacing(12);
    }
    group->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Maximum);
  }
  auto *scroll = new QScrollArea(dialog);
  scroll->setObjectName("formScroll");
  scroll->setFrameShape(QFrame::NoFrame);
  scroll->setWidgetResizable(true);
  scroll->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
  scroll->setWidget(body);
  root->setContentsMargins(20, 20, 20, 16);
  root->setSpacing(16);
  root->addWidget(scroll, 1);
  root->addWidget(footer);
  const bool dark = dialog->palette().color(QPalette::Window).lightness() < 128;
  dialog->setStyleSheet(
      dialog->styleSheet() +
      QString(
          "QGroupBox{border:1px solid %1;border-radius:7px;margin-top:12px;}"
          "QGroupBox::title{subcontrol-origin:margin;subcontrol-position:top left;left:12px;padding:0 4px;color:%2;}"
          "QListWidget{border:1px solid %1;border-radius:5px;background:%3;color:%2;}"
          "QListWidget::item{padding:5px;}"
      )
          .arg(dark ? "#45464b" : "#d9dadd", dark ? "#f5f5f7" : "#1d1d1f", dark ? "#2c2d31" : "#ffffff")
  );
  for (auto *button : footer->findChildren<QPushButton *>())
    if (!button->property("formButtonMetrics").toBool())
      new FormButtonMetrics(button);
  dialog->setMinimumSize(520, 320);
  dialog->resize(initial);
}
class ResponsiveFormColumns final : public QObject
{
  QBoxLayout *m_layout;

public:
  ResponsiveFormColumns(QWidget *window, QBoxLayout *layout) : QObject(window), m_layout(layout)
  {
    window->installEventFilter(this);
    m_layout->setDirection(QBoxLayout::TopToBottom);
  }
  bool eventFilter(QObject *object, QEvent *event) override
  {
    if (event->type() == QEvent::Resize)
      m_layout->setDirection(
          static_cast<QWidget *>(object)->width() < 700 ? QBoxLayout::TopToBottom : QBoxLayout::LeftToRight
      );
    return false;
  }
};
} // namespace deskflow::gui
