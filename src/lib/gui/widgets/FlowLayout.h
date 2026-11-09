// SPDX-FileCopyrightText: (C) 2026 Deskflow Developers
// SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
#pragma once
#include <QLayout>
#include <QWidget>
#include <algorithm>

namespace deskflow::gui {
// Commands keep their measured width and wrap in keyboard/reading order.
class FlowLayout final : public QLayout
{
public:
  explicit FlowLayout(QWidget *parent = nullptr, int gap = 8) : QLayout(parent), m_gap(gap)
  {
    setContentsMargins(0, 0, 0, 0);
  }
  ~FlowLayout() override
  {
    while (auto *item = takeAt(0))
      delete item;
  }
  void addItem(QLayoutItem *item) override
  {
    m_items.append(item);
  }
  int count() const override
  {
    return m_items.size();
  }
  QLayoutItem *itemAt(int index) const override
  {
    return m_items.value(index);
  }
  QLayoutItem *takeAt(int index) override
  {
    return index >= 0 && index < m_items.size() ? m_items.takeAt(index) : nullptr;
  }
  Qt::Orientations expandingDirections() const override
  {
    return {};
  }
  bool hasHeightForWidth() const override
  {
    return true;
  }
  int heightForWidth(int width) const override
  {
    return arrange(QRect(0, 0, width, 0), false);
  }
  QSize sizeHint() const override
  {
    return minimumSize();
  }
  QSize minimumSize() const override
  {
    QSize size;
    for (auto *item : m_items)
      if (!item->isEmpty())
        size = size.expandedTo(item->minimumSize());
    const auto m = contentsMargins();
    return size + QSize(m.left() + m.right(), m.top() + m.bottom());
  }
  void setGeometry(const QRect &rect) override
  {
    QLayout::setGeometry(rect);
    arrange(rect, true);
  }

private:
  int arrange(const QRect &rect, bool apply) const
  {
    const auto m = contentsMargins();
    const auto available = rect.adjusted(m.left(), m.top(), -m.right(), -m.bottom());
    int x = available.x(), y = available.y(), lineHeight = 0;
    for (auto *item : m_items) {
      if (item->isEmpty())
        continue;
      const auto size = item->sizeHint().expandedTo(item->minimumSize());
      if (lineHeight && x + size.width() > available.right() + 1) {
        x = available.x();
        y += lineHeight + m_gap;
        lineHeight = 0;
      }
      if (apply)
        item->setGeometry(QRect(QPoint(x, y), size));
      x += size.width() + m_gap;
      lineHeight = std::max(lineHeight, size.height());
    }
    return y + lineHeight - rect.y() + m.bottom();
  }
  QList<QLayoutItem *> m_items;
  int m_gap;
};
} // namespace deskflow::gui
