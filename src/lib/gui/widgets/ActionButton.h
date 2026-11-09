// SPDX-FileCopyrightText: (C) 2026 Deskflow Developers
// SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
#pragma once

#include <QLayout>
#include <QPushButton>
#include <QStyle>
#include <QStyleOptionButton>

namespace deskflow::gui {
// Measure the current paint style, not a size cached during a controller/style
// transition. Native QPushButton painting, focus and activation stay intact.
class ActionButton : public QPushButton
{
public:
  using QPushButton::QPushButton;

  QSize sizeHint() const override
  {
    ensurePolished();
    QStyleOptionButton option;
    initStyleOption(&option);
    if (option.text.isEmpty() && option.icon.isNull()) {
      auto hint = QPushButton::sizeHint();
      if (layout())
        hint = hint.expandedTo(layout()->sizeHint());
      return hint;
    }

    QSize content = fontMetrics().size(Qt::TextShowMnemonic, option.text);
    if (!option.icon.isNull()) {
      content.setWidth(content.width() + option.iconSize.width() + (option.text.isEmpty() ? 0 : 4));
      content.setHeight(qMax(content.height(), option.iconSize.height()));
    }
    option.rect.setSize(content);
    if (option.features.testFlag(QStyleOptionButton::HasMenu))
      content.rwidth() += style()->pixelMetric(QStyle::PM_MenuButtonIndicator, &option, this);
    return style()->sizeFromContents(QStyle::CT_PushButton, &option, content, this).expandedTo(minimumSize());
  }

  QSize minimumSizeHint() const override
  {
    return sizeHint();
  }
};
} // namespace deskflow::gui
