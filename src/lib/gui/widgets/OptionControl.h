// SPDX-FileCopyrightText: (C) 2026 Deskflow Developers
// SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
#pragma once
#include <QCheckBox>
#include <QPainter>
#include <QPainterPath>
#include <QRadioButton>
#include <QStyleOptionButton>
#include <QTextLayout>
#include <QtMath>
#include <cmath>
#include <type_traits>

namespace deskflow::gui {
// Native check/radio semantics, with a label that wraps beside the indicator.
// The indicator follows the first text line, not the full multi-line widget.
template <class Native> class OptionControl : public Native
{
public:
  explicit OptionControl(QWidget *parent = nullptr) : Native(parent)
  {
    initialise();
  }
  explicit OptionControl(const QString &text, QWidget *parent = nullptr) : Native(text, parent)
  {
    initialise();
  }
  QSize sizeHint() const override
  {
    if (this->text().isEmpty())
      return Native::sizeHint();
    const QFontMetrics metrics(this->font());
    return QSize(
        textLeading() + metrics.horizontalAdvance(displayText()) + 4,
        qMax(indicatorSize().height(), metrics.height()) + 8
    );
  }
  QSize minimumSizeHint() const override
  {
    return this->text().isEmpty() ? Native::minimumSizeHint() : QSize(textLeading() + 48, sizeHint().height());
  }
  int heightForWidth(int width) const override
  {
    if (this->text().isEmpty())
      return Native::sizeHint().height();
    QTextLayout text(displayText(), this->font());
    return qCeil(textHeight(text, qMax(1, width - textLeading() - 4))) + 8;
  }

protected:
  bool hitButton(const QPoint &point) const override
  {
    return this->rect().contains(point);
  }
  void paintEvent(QPaintEvent *event) override
  {
    if (this->text().isEmpty()) {
      Native::paintEvent(event);
      return;
    }
    QStyleOptionButton option;
    this->initStyleOption(&option);
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);
    QTextLayout text(displayText(), this->font());
    const auto labelHeight = textHeight(text, qMax(1, this->width() - textLeading() - 4));
    const auto first = text.lineCount() ? text.lineAt(0) : QTextLine();
    const auto indicator = indicatorSize();
    const qreal lineHeight = first.isValid() ? first.height() : this->fontMetrics().height();
    const int top = 4 + qRound((lineHeight - indicator.height()) / 2);
    QRect logicalIndicator(2, top, indicator.width(), indicator.height());
    option.rect = QStyle::visualRect(this->layoutDirection(), this->rect(), logicalIndicator);
    const auto group = this->isEnabled() ? QPalette::Active : QPalette::Disabled;
    // Draw the mark with the same coordinates as the first-line indicator.
    // Some native styles animate against the full widget rect, which loses the
    // check mark when a multi-line label has a different indicator position.
    const bool selected = option.state & (QStyle::State_On | QStyle::State_NoChange);
    const auto accent = this->palette().color(group, QPalette::Highlight);
    auto outline = this->palette().color(group, QPalette::WindowText);
    outline.setAlpha(160);
    const QRectF mark = QRectF(option.rect).adjusted(1, 1, -1, -1);
    painter.setPen(QPen(selected || (option.state & QStyle::State_MouseOver) ? accent : outline, 1.5));
    painter.setBrush(selected && !isRadio ? accent : this->palette().color(group, QPalette::Base));
    if constexpr (isRadio) {
      painter.drawEllipse(mark);
      if (selected) {
        painter.setPen(Qt::NoPen);
        painter.setBrush(accent);
        painter.drawEllipse(mark.adjusted(4, 4, -4, -4));
      }
    } else {
      painter.drawRoundedRect(mark, 3, 3);
      if (selected) {
        painter.setPen(
            QPen(this->palette().color(group, QPalette::HighlightedText), 2, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin)
        );
        if (option.state & QStyle::State_NoChange)
          painter.drawLine(mark.left() + 4, mark.center().y(), mark.right() - 4, mark.center().y());
        else {
          QPainterPath tick;
          tick.moveTo(mark.left() + mark.width() * .23, mark.top() + mark.height() * .52);
          tick.lineTo(mark.left() + mark.width() * .43, mark.top() + mark.height() * .73);
          tick.lineTo(mark.left() + mark.width() * .79, mark.top() + mark.height() * .28);
          painter.setBrush(Qt::NoBrush);
          painter.drawPath(tick);
        }
      }
    }
    painter.setPen(this->palette().color(group, QPalette::WindowText));
    const auto labelRect = QStyle::visualRect(
        this->layoutDirection(), this->rect(),
        QRect(textLeading(), 4, this->width() - textLeading() - 4, this->height() - 8)
    );
    text.draw(&painter, labelRect.topLeft());
    if (!this->icon().isNull())
      this->icon().paint(
          &painter, QStyle::visualRect(
                        this->layoutDirection(), this->rect(),
                        QRect(leading(), 4, this->iconSize().width(), this->iconSize().height())
                    )
      );
    if (this->hasFocus()) {
      const int focusHeight = qMin(this->height(), qCeil(qMax(labelHeight, qreal(indicator.height()))) + 8);
      painter.setPen(QPen(accent, 1.5));
      painter.setBrush(Qt::NoBrush);
      painter.drawRoundedRect(QRectF(0.75, 0.75, this->width() - 1.5, focusHeight - 1.5), 3, 3);
    }
  }
  void changeEvent(QEvent *event) override
  {
    Native::changeEvent(event);
    this->updateGeometry();
  }

private:
  static constexpr bool isRadio = std::is_base_of_v<QRadioButton, Native>;
  void initialise()
  {
    auto policy = this->sizePolicy();
    policy.setHorizontalPolicy(QSizePolicy::Preferred);
    policy.setVerticalPolicy(QSizePolicy::Minimum);
    policy.setHeightForWidth(true);
    this->setSizePolicy(policy);
  }
  QSize indicatorSize() const
  {
    return QSize(
        this->style()->pixelMetric(
            isRadio ? QStyle::PM_ExclusiveIndicatorWidth : QStyle::PM_IndicatorWidth, nullptr, this
        ),
        this->style()->pixelMetric(
            isRadio ? QStyle::PM_ExclusiveIndicatorHeight : QStyle::PM_IndicatorHeight, nullptr, this
        )
    );
  }
  int leading() const
  {
    return indicatorSize().width() + 10;
  }
  int textLeading() const
  {
    return leading() + (this->icon().isNull() ? 0 : this->iconSize().width() + 6);
  }
  QString displayText() const
  {
    auto text = this->text();
    text.replace("&&", QString(QChar(0x001f)));
    text.remove('&');
    text.replace(QChar(0x001f), '&');
    return text;
  }
  qreal textHeight(QTextLayout &text, int width) const
  {
    QTextOption option;
    option.setWrapMode(QTextOption::WrapAtWordBoundaryOrAnywhere);
    option.setTextDirection(this->layoutDirection());
    text.setTextOption(option);
    text.beginLayout();
    qreal y = 0;
    while (true) {
      auto line = text.createLine();
      if (!line.isValid())
        break;
      line.setLineWidth(width);
      line.setPosition(QPointF(0, y));
      y += line.height();
    }
    text.endLayout();
    return qMax(y, qreal(this->fontMetrics().height()));
  }
};
using OptionCheckBox = OptionControl<QCheckBox>;
using OptionRadioButton = OptionControl<QRadioButton>;
} // namespace deskflow::gui
