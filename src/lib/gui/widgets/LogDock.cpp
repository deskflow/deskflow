/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2025 Chris Rizzitello <sithlord48@gmail.com>
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#include "LogDock.h"
#include "LogWidget.h"
#include "gui/Styles.h"

#include <QEvent>
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QVBoxLayout>

LogDock::LogDock(QWidget *parent)
    : QDockWidget(tr("Log"), parent),
      m_textLog{new LogWidget(this)},
      m_btnClose{new QPushButton(this)},
      m_btnFloat{new QPushButton(this)},
      m_lblTitle{new QLabel(tr("Log"), this)}
{
  const auto iconSize = QSize(fontMetrics().height() - 2, fontMetrics().height() - 2);
  const auto maxBtnSize = QSize(fontMetrics().height() + 2, fontMetrics().height() + 2);

  m_btnFloat->setFixedSize(maxBtnSize);
  m_btnFloat->setCheckable(true);
  m_btnFloat->setFlat(true);
  m_btnFloat->setIcon(QIcon::fromTheme(QStringLiteral("window-minimize-pip")));
  m_btnFloat->setIconSize(iconSize);
  m_btnFloat->setToolTip(tr("Detach from window"));
  connect(m_btnFloat, &QPushButton::toggled, this, &LogDock::setFloating);

  m_btnClose->setFixedSize(maxBtnSize);
  m_btnClose->setFlat(true);
  m_btnClose->setIcon(QIcon::fromTheme(QStringLiteral("view-close")));
  m_btnClose->setIconSize(iconSize);
  m_btnClose->setToolTip(tr("Close Log"));
  connect(m_btnClose, &QPushButton::clicked, this, &QDockWidget::hide);

  auto titleWidget = new QWidget(this);
  titleWidget->installEventFilter(this);

  auto titleLayout = new QHBoxLayout(titleWidget);
  titleLayout->addWidget(m_lblTitle, Qt::AlignLeft | Qt::AlignVCenter);
  titleLayout->addSpacerItem(new QSpacerItem(0, 0, QSizePolicy::Expanding, QSizePolicy::Fixed));
  titleLayout->addWidget(m_btnFloat, Qt::AlignRight | Qt::AlignVCenter);
  titleLayout->addWidget(m_btnClose, Qt::AlignRight | Qt::AlignVCenter);
  setTitleBarWidget(titleWidget);

  auto bodyWidget = new QWidget(this);
  auto bodyLayout = new QVBoxLayout(bodyWidget);
  bodyLayout->addWidget(m_textLog);
  setWidget(bodyWidget);

  setFeatures(QDockWidget::DockWidgetClosable | QDockWidget::DockWidgetFloatable);
  setAllowedAreas(Qt::BottomDockWidgetArea);
}

void LogDock::appendLine(const QString &msg)
{
  m_textLog->appendLine(msg);
}

void LogDock::setFloating(bool floating)
{
  if (floating) {
    m_btnFloat->setToolTip(tr("Attach to window"));
    m_btnFloat->setIcon(QIcon::fromTheme(QStringLiteral("window-restore-pip")));
    setWindowFlags(Qt::Dialog);
  } else {
    m_btnFloat->setToolTip(tr("Detach from window"));
    m_btnFloat->setIcon(QIcon::fromTheme(QStringLiteral("window-minimize-pip")));
    setWindowFlags(Qt::Widget);
  }
  m_lblTitle->setVisible(!floating);
  m_btnClose->setVisible(!floating);
  show();
}

bool LogDock::eventFilter(QObject *watched, QEvent *event)
{
  // Filter out doubleclick on the titlebar, we only want the dock to float if the user users the button on the dock
  if (watched == titleBarWidget() && event->type() == QEvent::MouseButtonDblClick)
    return true;
  return false;
}
