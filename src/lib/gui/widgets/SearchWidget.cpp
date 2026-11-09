/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2026 Chris Rizzitello <sithlord48@gmail.com>
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#include "SearchWidget.h"

#include <QEvent>
#include <QHBoxLayout>
#include <QLineEdit>
#include <QPushButton>

SearchWidget::SearchWidget(QWidget *parent)
    : QWidget{parent},
      m_btnToggle{new QPushButton(this)},
      m_btnNext{new QPushButton(this)},
      m_btnPrev{new QPushButton(this)},
      m_searchLine{new QLineEdit(this)}
{
  setSizePolicy(QSizePolicy::Maximum, QSizePolicy::Maximum);

  setObjectName("logSearch");
  const auto iconSize = QSize(16, 16);
  const auto maxBtnSize = QSize(30, 30);
  setStyleSheet(
      "QPushButton{min-height:0;padding:4px;border:1px solid "
      "transparent;border-radius:4px;}QPushButton:focus{border:2px solid #5687d5;}"
  );
  m_btnToggle->setObjectName("logSearchToggle");
  m_btnNext->setObjectName("logSearchNext");
  m_btnPrev->setObjectName("logSearchPrevious");
  m_searchLine->setObjectName("logSearchText");

  m_btnToggle->setFixedSize(maxBtnSize);
  m_btnToggle->setCheckable(true);
  m_btnToggle->setChecked(false);
  m_btnToggle->setFlat(true);
  m_btnToggle->setIcon(
      QIcon::fromTheme(QIcon::ThemeIcon::SystemSearch, QIcon(":/icons/deskflow-light/actions/16/system-search.svg"))
  );
  m_btnToggle->setIconSize(iconSize);

  m_btnNext->setFixedSize(maxBtnSize);
  m_btnNext->setFlat(true);
  m_btnNext->setIcon(
      QIcon::fromTheme(QIcon::ThemeIcon::GoDown, QIcon(":/icons/deskflow-light/actions/16/go-down.svg"))
  );
  m_btnNext->setIconSize(iconSize);

  m_btnPrev->setFixedSize(maxBtnSize);
  m_btnPrev->setFlat(true);
  m_btnPrev->setIcon(QIcon::fromTheme(QIcon::ThemeIcon::GoUp, QIcon(":/icons/deskflow-light/actions/16/go-up.svg")));
  m_btnPrev->setIconSize(iconSize);

  m_searchLine->setMinimumWidth(140);

  connect(m_btnToggle, &QPushButton::toggled, this, &SearchWidget::toggleVisible);
  connect(m_btnNext, &QPushButton::clicked, this, &SearchWidget::next);
  connect(m_btnPrev, &QPushButton::clicked, this, &SearchWidget::previous);
  connect(m_searchLine, &QLineEdit::editingFinished, this, &SearchWidget::next);

  setText();

  auto mainLayout = new QHBoxLayout;
  mainLayout->setContentsMargins(0, 0, 0, 0);
  mainLayout->setSpacing(4);
  mainLayout->addWidget(m_btnToggle);
  mainLayout->addWidget(m_searchLine);
  mainLayout->addWidget(m_btnNext);
  mainLayout->addWidget(m_btnPrev);

  setLayout(mainLayout);
  toggleVisible();
  adjustSize();
}

void SearchWidget::changeEvent(QEvent *e)
{
  QWidget::changeEvent(e);
  if (e->type() == QEvent::LanguageChange || e->type() == QEvent::PaletteChange)
    setText();
}

void SearchWidget::toggleVisible(bool visible)
{
  m_searchLine->setVisible(visible);
  m_btnNext->setVisible(visible);
  m_btnPrev->setVisible(visible);
}

void SearchWidget::setText()
{
  m_btnToggle->setToolTip(tr("Search"));
  m_btnNext->setToolTip(tr("Find next"));
  m_btnPrev->setToolTip(tr("Find previous"));
  m_searchLine->setPlaceholderText(tr("Find..."));
  m_btnToggle->setAccessibleName(m_btnToggle->toolTip());
  m_btnNext->setAccessibleName(m_btnNext->toolTip());
  m_btnPrev->setAccessibleName(m_btnPrev->toolTip());
  const auto theme = palette().color(QPalette::Window).lightness() < 128 ? "dark" : "light";
  m_btnToggle->setIcon(QIcon(QString(":/icons/deskflow-%1/actions/16/system-search.svg").arg(theme)));
  m_btnNext->setIcon(QIcon(QString(":/icons/deskflow-%1/actions/16/go-down.svg").arg(theme)));
  m_btnPrev->setIcon(QIcon(QString(":/icons/deskflow-%1/actions/16/go-up.svg").arg(theme)));
}

void SearchWidget::next()
{
  Q_EMIT findNext(m_searchLine->text());
}

void SearchWidget::previous()
{
  Q_EMIT findPrevious(m_searchLine->text());
}
